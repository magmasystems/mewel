/*===========================================================================*/
/*                                                                           */
/* File    : WINSCROLL.C                                                     */
/*                                                                           */
/* Purpose : Contains the ScrollWindow() function                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI)
#include "wgraphic.h"
#endif


static BOOL PASCAL _WinScrollChildren(WINDOW *, int, int);


VOID FAR PASCAL ScrollWindow(hWnd, nCols, nRows, lpRect, lpClipRect)
  HWND hWnd;    /* handle of the window to scroll */
  int  nCols;   /* number of columns to scroll */
  int  nRows;   /* number of rows to scroll    */
  CONST RECT FAR *lpRect;  /* ptr to scrolling rect, NULL if it's the whole client */
  CONST RECT FAR *lpClipRect; /* ptr to clipping rectangle */
{
  ScrollWindowOrDC(hWnd, (HDC) 0, nCols, nRows, lpRect, lpClipRect, NULL);
}

BOOL FAR PASCAL ScrollWindowOrDC(hWnd, hDC, nCols, nRows, lpRect,
                                 lpClipRect, lpUpdateRect)
  HWND hWnd;    /* handle of the window to scroll */
  HDC  hDC;     /* DC to scroll (if called by ScrollDC) */
  int  nCols;   /* number of columns to scroll */
  int  nRows;   /* number of rows to scroll    */
  CONST RECT FAR *lpRect;  /* ptr to scrolling rect, NULL if it's the whole client */
  CONST RECT FAR *lpClipRect; /* ptr to clipping rectangle */
  LPRECT         lpUpdateRect;
{
  RECT   r, rTile[5], rDest, rSrc;
  int    i;
  WINDOW *w, *wParent;
  BOOL   bScrolledChildren = FALSE;
  BOOL   bHidCursor;


  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL || (w->flags & WS_MINIMIZE))
    return FALSE;

  /*
    Get the rectangle that the user specified for scrolling. If lpRect is
    NULL, then scroll the entire client area along with all children.
  */
  if (lpRect)
    rSrc = *lpRect;
  else
    GetClientRect(hWnd, &rSrc);   /* use the whole client area */

  /*
    Intersect with the clipping rectangle
  */
  if (lpClipRect)
    if (!IntersectRect((LPRECT) &rSrc, (LPRECT) &rSrc, (LPRECT) lpClipRect))
      return FALSE;

  /*
    Change the scrolling rect into screen coordinates. Intersect the 
    scrolling rect with the client area to bound the operation.
  */
#if defined(MOTIF)
  GetClientRect(hWnd, &rDest);
  if (!IntersectRect((LPRECT) &r, (LPRECT) &rDest, (LPRECT) &rSrc))
    return FALSE;
#else
  WinClientRectToScreen(hWnd, &rSrc);
  if (!IntersectRect((LPRECT) &r, (LPRECT) &w->rClient, (LPRECT) &rSrc))
    return FALSE;
#endif

  /*
    Clip the scrolling rect to the ancestor's client areas
  */
#if !defined(MOTIF)
  for (wParent = w->parent;  wParent;  wParent = wParent->parent)
    IntersectRect((LPRECT) &r, (LPRECT) &r, (LPRECT) &wParent->rClient);
#endif

  /*
    Rectangle 'r' is the source rectangle to scroll, in screen coordinates.
    Make sure we didn't specify a NULL scrolling area
  */
  if (IsRectEmpty(&r))
    return FALSE;

  /*
    Make sure that the scrolling area is inside the window. If it's not,
    the window should be blanked out. Just invalidate the entire window.
  */
  if (abs(nCols) > RECT_WIDTH(r) || abs(nRows) > RECT_HEIGHT(r))
  {
    /*
      Scroll all of the children if lpRect is NULL...
    */
    if (lpRect == (LPRECT) NULL)
      _WinScrollChildren(w, nRows, nCols);
#if defined(MOTIF)
    GetClientRect(hWnd, &r);
#else
    WinScreenRectToClient(hWnd, &r);
#endif
    InvalidateRect(hWnd, (lpRect == NULL) ? NULL : &r, TRUE);
    if (lpUpdateRect)
      *lpUpdateRect = r;
    return TRUE;
  }

  /*
    Calculate the source & destination rectangles for the blitting operation
  */
  rSrc = r;
  if (nRows >= 0 && nCols < 0)
    OffsetRect(&rSrc, -nCols, 0);
  else if (nRows < 0)
    OffsetRect(&rSrc, (nCols < 0) ? -nCols : 0, -nRows);
  rSrc.right -= abs(nCols);   rSrc.bottom -= abs(nRows);
  rDest = rSrc;
  OffsetRect(&rDest, nCols, nRows);

  /*
    Hide the cursor if it's being shown
  */
#if defined(XWINDOWS)
  HideCaret(hWnd);
#else
  bHidCursor = CaretPush(0, VideoInfo.length);
#endif

  /*
    Scroll all of the children if lpRect is NULL...
  */
  if (lpRect == (LPRECT) NULL)
  {
    if ((bScrolledChildren = _WinScrollChildren(w, nRows, nCols)) == TRUE)
      InvalidateRect(hWnd, NULL, TRUE);
  }

  /*
    See if we want to scroll everything off the screen....
  */
  if (IsRectEmpty(&rSrc))
  {
    if (!bScrolledChildren)
    {
      WinScreenRectToClient(hWnd, &r);
      InvalidateRect(hWnd, &r, TRUE);
    }
    goto byebye;
  }
  else
  {
    /* 
      We don't want to scroll an invalidated region. To get a valid
      repaint area which includes all areas scrolled off, we must
      intersect the source rectangle with any existing update rect,
      and subtract the invalid rectangle.
    */
    RECT rUpdate;

    if (GetUpdateRect(hWnd, &rUpdate, FALSE)) /* Already has an update rect? */
    {
#if !defined(MOTIF)
      WinClientRectToScreen (hWnd, &rUpdate);
#endif
      if (IntersectRect(&rUpdate, &rSrc, &rUpdate))
      {
        RectTile(rSrc, rUpdate, rTile);
        rSrc = RectEmpty;
        for (i = 0;  i < 4;  i++)
          if (!IsRectEmpty(&rTile[i]))
            UnionRect(&rSrc, &rSrc, &rTile[i]);
        rDest = rSrc;
        OffsetRect(&rDest, nCols, nRows);
      }
    }

    /*
      Do the blitting
    */
#if defined(XWINDOWS)
    _XScrollWindow(hWnd, rDest.left - rSrc.left, 
                         rDest.top - rSrc.top, 
                         &rSrc, (LPRECT) NULL);
#elif defined(MEWEL_GUI) && defined(META)
    GDIScrollWindow(hDC, &rDest, &rSrc);
#else
    WinBltRect(hWnd, hDC, &rDest, &rSrc);
#endif
    if (lpUpdateRect)
    {
      r = w->rUpdate;
      if (!IsRectEmpty(&r))
        UnionRect(lpUpdateRect, lpUpdateRect, &r);
    }
  }


  /*
    If we scrolled the children, we will redraw the whole window anyway
  */
#if !(defined(MEWEL_GUI) && defined(META))
  if (!bScrolledChildren)
  {
    /*
      Calculate the one or two rectangles to redraw.
    */
    RectTile(rSrc, rDest, rTile);
    for (i = 0;  i < 4;  i++)
    {
      if (!IsRectEmpty(&rTile[i]))
      {
        /* The posted rectangle should be in client coordinates */
#if !defined(MOTIF)
        WinScreenRectToClient(hWnd, &rTile[i]);
#endif
        InvalidateRect(hWnd, &rTile[i], TRUE);
        if (lpUpdateRect)
          UnionRect(lpUpdateRect, lpUpdateRect, &rTile[i]);
      }
    }
  }
#endif

byebye:
  /*
    Restore the hardware cursor if we hid it during the scrolling operation
  */
#if defined(XWINDOWS)
  ShowCaret(hWnd);
#else
  if (bHidCursor)
    CaretPop();
#endif
  return TRUE;
}


static BOOL PASCAL _WinScrollChildren(w, nRows, nCols)
  WINDOW *w;
  int    nRows, nCols;
{
  WINDOW *wChild;
  BOOL   rc = FALSE;

  /*
    We only need move the top children, as WinMove() takes care of the
    other lower-level kids. First, span all of the popup windows
    because they should not get scrolled.
  */
  for (wChild = w->children;  wChild && (wChild->flags & WS_POPUP);  
       wChild = wChild->sibling)
    ;
  SET_PROGRAM_STATE(STATE_DEFER_VISMAP);
  while (wChild)
  {
    WinMove(wChild->win_id,wChild->rect.top+nRows,wChild->rect.left+nCols);
    InternalInvalidateWindow(wChild->win_id, TRUE);
    wChild = wChild->sibling;
    rc = TRUE;
  }
  CLR_PROGRAM_STATE(STATE_DEFER_VISMAP);
  WinUpdateVisMap();

  return rc;
}


int WINAPI ScrollWindowEx(HWND hwnd, int dx, int dy,
            CONST RECT FAR* prcScroll, CONST RECT FAR* prcClip,
            HRGN hrgnUpdate, RECT FAR* prcUpdate, UINT flags) /* USER.319 */
{
  (void) hrgnUpdate;  (void) prcUpdate;  (void) flags;
  ScrollWindow(hwnd,dx,dy,prcScroll,prcClip);
  return TRUE;
}

