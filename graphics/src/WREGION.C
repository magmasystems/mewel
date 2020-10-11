/*===========================================================================*/
/*                                                                           */
/* File    : WREGION.C                                                       */
/*                                                                           */
/* Purpose : Calculates the visibility region of a window.                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/* #define DEBUG */
#define INCLUDE_REGIONS
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

/* static HRGN FAR PASCAL CreateRectRegion(INT, INT, INT, INT); */
static BOOL PASCAL WinUpdateVisibilityRegion(WINDOW *, HRGN *, LPRECT);
static HRGN FAR PASCAL CreateRectRegion(LPRECT);
static BOOL FAR PASCAL AddRectToRegion(PREGION, LPRECT, HRGN *);

#if defined(__DPMI32__)
#define LOCALLOCK     GlobalLock
#define LOCALUNLOCK   GlobalUnlock
#define LOCALFREE     GlobalFree
#define LOCALALLOC    GlobalAlloc
#define LOCALREALLOC  GlobalReAlloc
#define MEMALLOC_FLAG LMEM_FIXED
#else
#define LOCALLOCK     LocalLock
#define LOCALUNLOCK   LocalUnlock
#define LOCALFREE     LocalFree
#define LOCALALLOC    LocalAlloc
#define LOCALREALLOC  LocalReAlloc
#define MEMALLOC_FLAG LMEM_MOVEABLE
#endif

#if defined(__DPMI32__)
#define DONT_USE_REALLOC
#endif


/****************************************************************************/
/*                                                                          */
/* Function : IsVisRegionEmpty(hDC, lprcBounding)                           */
/*                                                                          */
/* Purpose  : Tests the visibilty region to see if a window is visible.     */
/*            Also, returns the bounding rect of the region.                */
/*                                                                          */
/* Returns  : TRUE if the visibility region is empty (the window is totally */
/*            obscured). FALSE if at least part of the window is visible.   */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsVisRegionEmpty(hDC, lprcBounding)
  HDC      hDC;
  LPRECT   lprcBounding;
{
  LPHDC    lphDC = _GetDC(hDC);
  PREGION  pRegion;
  BOOL     bEmpty;

  /*
    If we did not calculate a vis region, then assume that the window
    is visible.
  */
  if (lphDC == NULL || lphDC->hRgnVis == NULL)
    return FALSE;

  /*
    Get a pointer to the clipping region and examine the number of
    regions in it. If it's zero, then the window is totally obscured.
  */
  if ((pRegion = (PREGION) LOCALLOCK(lphDC->hRgnVis)) == NULL)
    return FALSE;
  bEmpty = (BOOL) (pRegion->nRects == 0);
  if (lprcBounding)
    *lprcBounding = pRegion->rectBounding;
  LOCALUNLOCK(lphDC->hRgnVis);

  return bEmpty;
}


/****************************************************************************/
/*                                                                          */
/* Function :                                                               */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
HRGN FAR PASCAL WinCalcVisRgn(hWnd, hDC)
  HWND     hWnd;
  HDC      hDC;
{
  RECT     rTarget;
  WINDOW   *wTarget, *wSibling, *w;
  DWORD    dwTargetStyle;
  HRGN     hRgn;
  PREGION  pRegion;
  INT      rc;
  LPHDC    lphDC;

  if ((wTarget = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return NULL;

  lphDC = _GetDC(hDC);

  /*
    No clipping regions for memory DCs.
  */
  if (IS_MEMDC(lphDC))
    return NULL;

  /*
    Figure out the clipping rectangle for the window's client area or
    the entire window area (if hDC is a window DC).
    If a bitmap is the canvas, then bypass the clipiing operations
  */
#if defined(GX)
  if (((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader)
  {
    BITMAP bm;
    GetObject(lphDC->hBitmap, sizeof(bm), &bm);
    SetRect(&rTarget, 0, 0, bm.bmWidth, bm.bmHeight);
  }
  else
#endif
    rc = WinGenAncestorClippingRect(hWnd, hDC, &rTarget);

  /*
    Initialize the clipping region with the clipping rectangle.
  */
  if ((hRgn = CreateRectRegion((LPRECT) &rTarget)) == NULL)
    return NULL;

  /*
    If a bitmap is the canvas, then bypass the clipiing operations
  */
#if defined(GX)
  if (((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader)
    goto do_bounding;
#endif

  /*
    Do not clip popup nor system menus nor combo-listboxes nor
    scrollbars in combo-listboxes.
  */
  if (IS_MENU(wTarget))
  {
    MENU *m = _MenuHwndToStruct(hWnd);
    if (m->flags & (M_POPUP | M_SYSMENU))
      goto do_bounding;
  }
  if ((wTarget->ulStyle & LBS_IN_COMBOBOX) ||
        (wTarget->idClass == SCROLLBAR_CLASS && 
        (wTarget->parent->ulStyle & LBS_IN_COMBOBOX)))
    goto do_bounding;


  /*
    If the clipping rectangle is empty, then set the nRects member to 0
    and return.
  */
  pRegion = (PREGION) LOCALLOCK(hRgn);
  if (!rc)
  {
    pRegion->nRects = 0;
    goto bye;
  }
  LOCALUNLOCK(hRgn);


  /*
    Clip to the 1st-level children, but only if the window is not an icon.
  */
  dwTargetStyle = wTarget->flags;
  if ((dwTargetStyle & (WS_CLIPCHILDREN | WS_MINIMIZE)) == WS_CLIPCHILDREN)
    for (w = wTarget->children;  w;  w = w->sibling)
      if (!WinUpdateVisibilityRegion(w, &hRgn, &rTarget))
        goto err;

  /*
    Compute the overlap of higher siblings.
    If the sibling is visible and it intersects the point, then the point
    is not visible.
    Then try the windows which are above or to the left of the parent.
  */
  for (w = wTarget;  w && w->parent;  w = w->parent)
  {
    if (w == wTarget && !(wTarget->flags & WS_CLIPSIBLINGS))
      continue;
    for (wSibling=w->prevSibling;  wSibling;  wSibling=wSibling->prevSibling)
    {
      /*
        Do not clip if the higher sibling owns window 'w'. For example,
        if we have a modeless dialog box owned by the main window.
      */
      if (w->hWndOwner == wSibling->win_id)
        continue;

      if (!IsFrameClass(wSibling))
        if (!WinUpdateVisibilityRegion(wSibling, &hRgn, &rTarget))
          goto err;
    }
  }


  /*
    If there is a popup menu active (and the popup is not the window
    which we are testing), then clip all drawing to the popup.
  */
  if (InternalSysParams.hWndFocus && InternalSysParams.hWndFocus != hWnd)
  {
    wSibling = WID_TO_WIN(InternalSysParams.hWndFocus);
    if (IS_MENU(wSibling))
      if (!WinUpdateVisibilityRegion(wSibling, &hRgn, &rTarget))
        goto err;
  }


  /*
    Figure out the bounding rectangle
  */
do_bounding:
  if ((pRegion = (PREGION) LOCALLOCK(hRgn)) == NULL)
    goto err;
  if (pRegion->nRects > 0)
  {
    INT i;

    rTarget = pRegion->rects[0];
    for (i = pRegion->nRects - 1;  i >= 1;  i--)
      UnionRect(&rTarget, &pRegion->rects[i], &rTarget);
    pRegion->rectBounding = rTarget;
  }

#if defined(META)
  if (TEST_PROGRAM_STATE(STATE_META_CLIP))
  {
    ((LPMETADCINFO) lphDC->lpDCExtra)->pClipRegion =
           mwRectListToRegion(pRegion->nRects, (rect *) pRegion->rects);
    mwClipRegion( ((LPMETADCINFO) lphDC->lpDCExtra)->pClipRegion );
  }
#endif

bye:
  LOCALUNLOCK(hRgn);
  return hRgn;

err:
  return NULL;
}


/*
  WinUpdateVisibilityRegion(w, phRgn, lprTarget)

  Returns TRUE if the hRgn is not NULL, FALSE if it is NULL. The
  hRgn is NULL if either the LocalLock failed or if the LocalReAlloc
  in AddRectToRegion failed.
*/
static BOOL PASCAL WinUpdateVisibilityRegion(w, phRgn, lprTarget)
  WINDOW   *w;
  HRGN     *phRgn;
  LPRECT   lprTarget;
{
  RECT     rTile[4];
  RECT     rTmp;
  PREGION  pRegion;
  LPRECT   lprIntersecting;
  INT      nRects;
  INT      n, i;
  HRGN     hRgn;

  /*
    Don't do anything if the window is hidden.
  */
  if (TEST_WS_HIDDEN(w))
    return TRUE;

  /*
    See if the window we are testing overlaps the target rectangle.
    If not, return.
  */
  lprIntersecting = &w->rect;
  if (!IntersectRect(&rTmp, lprTarget, lprIntersecting))
    return TRUE;

  /*
    Go through all of the visibility rectangles in the region
  */
  hRgn = *phRgn;
  if ((pRegion = (PREGION) LOCALLOCK(hRgn)) == NULL)
  {
    return FALSE;
  }

  nRects = pRegion->nRects;
  for (n = nRects-1;  n >= 0;  n--)
  {
    /*
      See if the window we are considering overlaps this visibility rectangle.
      If not, then we don't even need to consider it.
    */
    if (!IntersectRect(&rTmp, &pRegion->rects[n], lprIntersecting))
      continue;

    /*
      We further divide this visibility rectangle into a new group of
      rectangles by tiling it with the window we are considering.
    */
    RectTile(pRegion->rects[n], *lprIntersecting, rTile);

    /*
      Delete the just-tiled visibility rect from the visibility list
    */
    if (n < pRegion->nRects-1)
      memcpy(&pRegion->rects[n], &pRegion->rects[n+1], 
              sizeof(RECT) * (pRegion->nRects - n - 1));
    pRegion->nRects--;

    /*
      Go through the list of visibility rectangles. For each rect in the
      tile array, see if that rectangle is already in the visibility list.
      If not, add it to the end.
    */
    for (i = 0;  i < 4;  i++)
    {
      HRGN hNewRgn;
      if (AddRectToRegion(pRegion, &rTile[i], &hNewRgn))
      {
        /*
          If AddRectToRegion returns TRUE, then we reallocated hRgn, and
          thus, we must dereference hRgn again.
        */
        if ((hRgn = hNewRgn) == NULL || 
            (pRegion = (PREGION) LOCALLOCK(hRgn)) == NULL)
          return FALSE;
      }
    }
  }

  *phRgn = hRgn;
  LOCALUNLOCK(hRgn);

  return TRUE;
}


static HRGN FAR PASCAL CreateRectRegion(lpRect)
  LPRECT lpRect;
  /* INT  x1, y1, x2, y2; */
{
  RECT    r;
  HRGN    hRgn;
  PREGION pRgn;
  
  /*
    Allocate and lock a region structure
  */
  if ((hRgn = LOCALALLOC(MEMALLOC_FLAG, sizeof(REGION))) == NULL)
    return NULL;
  pRgn = (PREGION) LOCALLOCK(hRgn);

  /*
    Fill the fields of the region structure
  */
  r = *lpRect;
  /* SetRect((LPRECT) &r, x1, y1, x2, y2); */
  pRgn->nRects = pRgn->maxRects = 1;
  pRgn->rects[0]     = r;
  pRgn->hRgn         = hRgn;

  /*
    Close up and return the region handle.
  */
  LOCALUNLOCK(hRgn);
  return hRgn;
}


static BOOL FAR PASCAL AddRectToRegion(pRgn, lpRect, pNewHRGN)
  PREGION pRgn;
  LPRECT  lpRect;
  HRGN    *pNewHRGN;
{
  RECT   r;
  INT    rc = FALSE;  /* FALSE if we didn't reallocate */
  INT    nRects, maxRects, i;

  /*
    We don't want to add an empty rectangle to the list.
  */
  if (IsRectEmpty(lpRect))
    return FALSE;

  /*
    Get things into local vars for speed.
  */
  r = *lpRect;
  nRects = pRgn->nRects;
  maxRects = pRgn->maxRects;

  /*
    Go through the rectangle list and see if we have this rectangle
    in the list already.
  */
  for (i = 0;  i < nRects;  i++)
    if (!memcmp(&pRgn->rects[i], &r, sizeof(RECT)))
      goto bye;

  /*
    See if we need to grow the list
  */
  if (nRects >= maxRects)
  {
    HRGN hRgn = pRgn->hRgn;
#ifdef DONT_USE_REALLOC
    HRGN    hNewRgn;
    PREGION pNew;
    int     oldMax = maxRects;
#endif

    maxRects = (maxRects == 1) ? 4 : (maxRects << 1);

    /*
      Reallocate the region list and lock it
    */
    LOCALUNLOCK(hRgn);

#ifdef DONT_USE_REALLOC
    if ((hNewRgn = LOCALALLOC(MEMALLOC_FLAG, 
                      sizeof(REGION) + (maxRects * sizeof(RECT)))) != NULL)
    {
      pRgn = (PREGION) LOCALLOCK(hRgn);
      pNew = (PREGION) LOCALLOCK(hNewRgn);

      lmemcpy((LPSTR) pNew, (LPSTR) pRgn,
                sizeof(REGION) + (oldMax * sizeof(RECT)));

      LOCALUNLOCK(hNewRgn);
      LOCALUNLOCK(hRgn);

      LOCALFREE(hRgn);
      hRgn = hNewRgn;
    }
    else
    {
      hRgn = NULL;
    }
#else
    hRgn = LOCALREALLOC(hRgn, sizeof(REGION) + (maxRects * sizeof(RECT)),
                        MEMALLOC_FLAG);
#endif

    if (hRgn == NULL || (pRgn = (PREGION) LOCALLOCK(hRgn)) == NULL)
    {
      *pNewHRGN = NULL;
      rc = TRUE;
      goto bye;
    }

    /*
      Set the new number of max-rects and assign the new region handle
    */
    pRgn->maxRects = maxRects;
    *pNewHRGN = pRgn->hRgn = hRgn;
    rc = TRUE;
  }

  /*
    Add the rectangle to the list and modify the bounding rectangle.
  */
  pRgn->rects[nRects++] = r;
  pRgn->nRects = nRects;

  /*
    Close up and return
  */
bye:
  return rc;
}



#ifdef DEBUG

static int iLevel = 0;
static FILE *f;

static VOID PASCAL _DumpWindowTreeRegions(WINDOW *);
static VOID PASCAL DumpRegionToFile(WINDOW *, HRGN);

VOID PASCAL DumpWindowTreeRegions(void)
{
  if ((f = fopen("region.dmp", "w")) == NULL)
    return;

  iLevel = 0;
  _DumpWindowTreeRegions(InternalSysParams.wDesktop);

  fclose(f);
}

static VOID PASCAL _DumpWindowTreeRegions(w)
  WINDOW *w;
{
  HRGN hRgn;

  if (w == NULL)
    return;

  hRgn = WinCalcVisRgn(w->win_id, (HDC) 0);
  DumpRegionToFile(w, hRgn);
  LOCALFREE(hRgn);

  iLevel++;
  _DumpWindowTreeRegions(w->children);
  iLevel--;

  _DumpWindowTreeRegions(w->sibling);
}

static VOID PASCAL DumpRegionToFile(w, hRgn)
  WINDOW *w;
  HRGN   hRgn;
{
  char szBlanks[64];
  int  n;
  PREGION pRgn;

  memset(szBlanks, ' ', iLevel*2);
  szBlanks[iLevel*2] = '\0';

  pRgn = (PREGION) LOCALLOCK(hRgn);

  fprintf(f, szBlanks);
  fprintf(f, "Dumping regions for window %d ([%s]).  There are %d rects.\n",
             w->win_id, (w->title) ? w->title : NULL, pRgn->nRects);

  if (!pRgn->nRects)
  {
    fprintf(f, szBlanks);
    fprintf(f, "[empty]\n");
  }
  else
  {
    for (n = 0;  n < pRgn->nRects;  n++)
    {
      RECT r;
      r = pRgn->rects[n];
      fprintf(f, szBlanks);
      fprintf(f, "[%d,%d %d,%d]\n", r.left, r.top, r.right, r.bottom);
    }
  }

  fprintf(f, "\n");
  LOCALUNLOCK(hRgn);
}

#endif


