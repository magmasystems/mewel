/*===========================================================================*/
/*                                                                           */
/* File    : WGUIPOLY.C                                                      */
/*                                                                           */
/* Purpose : Implements the Polygon function                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

static VOID PASCAL EnginePoly(HDC, INT, POINT *, BOOL, BOOL);

/****************************************************************************/
/*                                                                          */
/* Function : Polygon(hDC, lpPoints, nCount)                                */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : TRUE if drawn, FALSE if not.                                  */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL Polygon(hDC, lpPoints, nCount)
  HDC     hDC;
  const POINT FAR *lpPoints;
  INT     nCount;
{
  INT     rc = TRUE;
  BOOL    bBrushSet;
  BOOL    bPenSet;
  POINT  *lppt;
  LPHDC   lphDC;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  /*
    Realize the brush and the pen to drawn with
  */
  bPenSet = RealizePen(hDC);
  bBrushSet = RealizeBrush(hDC);

  /*
    Convert the array of logical points to device points without distrurbing
    the original array.
  */
  if ((lppt = (POINT *) emalloc(sizeof(POINT) * nCount)) == NULL)
    return FALSE;
  lmemcpy((LPSTR) lppt, (LPSTR) lpPoints, nCount * sizeof(POINT));
  GrLPtoSP(hDC, (LPPOINT) lppt, nCount);
  _XNLogToPhys(lphDC, nCount, lppt);

  MOUSE_HideCursor();

  /*
    Graphics-engine-specific rendering
  */
#if defined(XWINDOWS)
  XFillPolygon(XSysParams.display, lphDC->drawable, lphDC->gc,
               lppt, nCount, Complex, CoordModeOrigin);

#elif defined(META) && !70993
  {
  polyHead polyHdr;
  polyHdr.polyBgn = 0;
  polyHdr.polyEnd = nCount-1;
  mwScreenRect(&polyHdr.polyRect);
  if (bBrushSet)
    mwErasePoly(1, &polyHdr, (point *) lpPoints);
  mwFramePoly(1, &polyHdr, (point *) lpPoints);
  }



#elif defined(MSC) || defined(BGI) || defined(GURU) || defined(GX) || defined(META)

#ifdef USE_REGIONS
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT r2, r3, rTmp;
    INT  i, j;
    PREGION pRegion;
    POINT  *lppt2 = NULL;

    /*
      Build up the bounding rectangle in screen coordinates
    */
    INT x1 = 0x7FFF, y1 = 0x7FFF, x2 = 0, y2 = 0;
    for (j = 0;  j < nCount;  j++)
    {
      POINT pt = lppt[j];
      x1 = min(pt.x, x1);
      y1 = min(pt.y, y1);
      x2 = max(pt.x, x2);
      y2 = max(pt.y, y2);
    }
    SetRect(&r2, x1, y1, x2, y2);
    OffsetRect(&r2, lphDC->rClipping.left, lphDC->rClipping.top);
    if (!IntersectRect(&r2, &r2, &SysGDIInfo.rectLastClipping))
      goto bye;


    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    /*
      Draw part of the polygon for each visible region.
    */
    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      if (IntersectRect(&rTmp, &pRegion->rects[i], &r2))
      {
        INT cx, cy;

        /*
          rTmp is expressed in screen coordinates
        */
        SetRect(&r3, rTmp.left, rTmp.top, SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
        if (!IntersectRect(&r3, &r3, &rTmp))
          continue;
        _setviewport(r3.left, r3.top,
                     r3.right-EXTENT_OFFSET, r3.bottom-EXTENT_OFFSET);

        /*
          Transform a copy of the points into viewport-relative
          coordinates.
        */
        if (!lppt2)
          lppt2 = (POINT *) emalloc(sizeof(POINT) * nCount);
        memcpy(lppt2, lppt, nCount * sizeof(POINT));
        cx = lphDC->rClipping.left - rTmp.left;
        cy = lphDC->rClipping.top  - rTmp.top;
        for (j = 0;  j < nCount;  j++)
        {
          lppt2[j].x += cx;
          lppt2[j].y += cy;
        }

        /*
          Call the graphics engine to do the rendering
        */
        EnginePoly(hDC, nCount, lppt2, bBrushSet, bPenSet);
      }
    }

    /*
      Free up everything.
    */
    MyFree(lppt2);
    UNLOCKREGION(lphDC->hRgnVis);
    _setviewport(SysGDIInfo.rectLastClipping.left, 
                 SysGDIInfo.rectLastClipping.top, 
                 SysGDIInfo.rectLastClipping.right, 
                 SysGDIInfo.rectLastClipping.bottom);
  }
  else
#endif
    EnginePoly(hDC, nCount, lppt, bBrushSet, bPenSet);

#endif

  /*
    Get rid of the temp array and return
  */
bye:
  MyFree(lppt);
  MOUSE_ShowCursor();
  return rc;
}


BOOL FAR PASCAL PolyPolygon(hDC, lppt, lpnPolyCounts, cPolygons)
  HDC              hDC;
  CONST POINT FAR *lppt;
  int         FAR *lpnPolyCounts;
  int              cPolygons;
{
  int i;

  for (i = 0;  i < cPolygons;  i++)
  {
    Polygon(hDC, lppt, lpnPolyCounts[i]);
    lppt += lpnPolyCounts[i];
  }

  return TRUE;
}


static VOID PASCAL EnginePoly(HDC hDC, INT nPoints, POINT *lppt, 
                              BOOL bBrushSet, BOOL bPenSet)
{
  (void) hDC;

#if defined(GURU)
  RPolygon(nPoints, (int far *) lppt);

#elif defined(META)
  {
  polyHead polyHdr;
  polyHdr.polyBgn = 0;
  polyHdr.polyEnd = nPoints-1;

  mwScreenRect(&polyHdr.polyRect);
#if !defined(__DPMI32__)
  if (bBrushSet)
  {
    RealizeBrush(hDC);
    mwErasePoly(1, &polyHdr, (point *) lppt);
  }
  mwFramePoly(1, &polyHdr, (point *) lppt);
#endif
  }

#elif defined(GX)
  if (bBrushSet)
    RealizeBrush(hDC);
  grDrawPoly((int *) lppt, nPoints, (bBrushSet ? grFILL : 0) +
                                    (bPenSet ? grOUTLINE : 0));

#elif defined(BGI)
#if defined(USE_BCC2GRX) || defined(__DPMI32__)
  {
  /*
    GRX expects each point to be an int, not a short MWCOORD. So we need
    to make a copy of the original lppt array and make an integer-point
    version of it.
  */
  int i;
  struct pointtype *lppt2 = (struct pointtype *)
                               emalloc(sizeof(struct pointtype) * nPoints);
  for (i = 0;  i < nPoints;  i++)
  {
    lppt2[i].x = (int) lppt[i].x;
    lppt2[i].y = (int) lppt[i].y;
  }
  if (bPenSet)
    drawpoly(nPoints, (const int far *) lppt2);
  if (bBrushSet)
    fillpoly(nPoints, (const int far *) lppt2);
  MyFree(lppt2);
  }
#else
  if (bPenSet)
    drawpoly(nPoints, (int far *) lppt);
  if (bBrushSet)
    fillpoly(nPoints, (int far *) lppt);
#endif

#elif defined(MSC)
  /*
    Bug note :
      Since the MSC graphics lib uses the current foregound color (as
     set by _setcolor()) for both the pen and the brush, it seems
     impossible to get a polygon which has a border which is colored
     differently than the interior. Why not subtract the pen width
     from each coordinate pair in 'lppt'? Think of what it would do
     to shapes like triangles!
  */
  if (bBrushSet)
    RealizeBrush(hDC);

#if defined(WC386) && !defined(__HIGHC__)
  _polygon(bBrushSet ? _GFILLINTERIOR : _GBORDER, nPoints,
             (struct xycoord FAR *) lppt);
#else
  _polygon(bBrushSet ? _GFILLINTERIOR : _GBORDER,
             (struct xycoord FAR *) lppt, nPoints);
#endif

#endif
}

