/*===========================================================================*/
/*                                                                           */
/* File    : WGDIELLI.C                                                      */
/*                                                                           */
/* Purpose : Implements the Ellipse() function                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if defined(MOTIF)
#undef HUGE
#include <math.h>
#endif

static VOID PASCAL EngineEllipse(HDC, INT, INT, INT, INT, BOOL, BOOL);

/****************************************************************************/
/*                                                                          */
/* Function : Ellipse()                                                     */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL Ellipse(hDC, x1, y1, x2, y2)
  HDC hDC;
  INT x1, y1, x2, y2;
{
  POINT pt[2];
  LPHDC lphDC;
  BOOL  bBrushSet;
  BOOL  bPenSet;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  /*
    Realize the ROP2 code, the brush, and the pen
  */
  RealizeROP2(hDC);
  bPenSet   = RealizePen(hDC);
  bBrushSet = RealizeBrush(hDC);

  /*
    Convert logical to device coordinates
  */
  pt[0].x = (MWCOORD) x1;  pt[0].y = (MWCOORD) y1;
  pt[1].x = (MWCOORD) x2;  pt[1].y = (MWCOORD) y2;
  GrLPtoSP(hDC, (LPPOINT) pt, 2);
  _XLogToPhys(lphDC, (MWCOORD *) &pt[0].x, (MWCOORD *) &pt[0].y);
  _XLogToPhys(lphDC, (MWCOORD *) &pt[1].x, (MWCOORD *) &pt[1].y);

  /*
    Sort the points
  */
  MEWELSortPoints(&pt[0].x, &pt[0].y, &pt[1].x, &pt[1].y);
  x1 = pt[0].x;  y1 = pt[0].y;
  x2 = pt[1].x;  y2 = pt[1].y;


  /*
    Hide the mouse
  */
  MOUSE_ConditionalOffDC(lphDC, x1, y1, x2, y2);

  /*
    Draw the ellipse
  */
#if defined(USE_REGIONS) && defined(GX)
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT r2, r3, rTmp;
    INT  i;
    INT  vx1, vy1, vx2, vy2;
    INT  cx1, cy1, cx2, cy2;
    PREGION pRegion;

    /*
      Convert the viewport-based rectangle to a screen-based rectangle
      because all of the rectangles in the region are in screen coords.
    */
    SetRect(&r2, x1, y1, x2, y2);
    OffsetRect(&r2, lphDC->rClipping.left, lphDC->rClipping.top);
    if (!IntersectRect(&r2, &r2, &SysGDIInfo.rectLastClipping))
      goto bye;

    grGetViewPort(&vx1, &vy1, &vx2, &vy2);
    grGetClipRegion(&cx1, &cy1, &cx2, &cy2);

    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      if (IntersectRect(&rTmp, &pRegion->rects[i], &r2))
      {
        /*
          rTmp is expressed in screen coordinates
        */
        SetRect(&r3, rTmp.left, rTmp.top, SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
        if (!IntersectRect(&r3, &r3, &rTmp))
          continue;

        grSetClipRegion(r3.left, r3.top, r3.right, r3.bottom);
        grSetClipping(grCLIP);
        EngineEllipse(hDC, x1, y1, x2, y2, bPenSet, bBrushSet);
      }
    }

    UNLOCKREGION(lphDC->hRgnVis);
    grSetViewPort(vx1, vy1, vx2, vy2);
    grSetClipRegion(cx1, cy1, cx2, cy2);
  }
  else
#endif
  EngineEllipse(hDC, x1, y1, x2, y2, bPenSet, bBrushSet);

  /*
    Restore the mouse
  */
bye:
  MOUSE_ShowCursorDC();
  return TRUE;
}


static VOID PASCAL EngineEllipse(HDC hDC, INT x1, INT y1, INT x2, INT y2,
                                 BOOL bPenSet, BOOL bBrushSet)
{
#if defined(META)
  RECT r;
  SetRect(&r, x1, y1, x2-1, y2-1);
#endif

#if defined(MOTIF) || defined(XWINDOWS)
  LPHDC  lphDC = _GetDC(hDC);
#endif


  /*
    Draw the interior of the ellipse
  */
  if (bBrushSet)
  {
    RealizeBrush(hDC);

#if defined(XWINDOWS)
    XFillArc(XSysParams.display, lphDC->drawable, lphDC->gc,
             x1, y1, abs(x2-x1), abs(y2-y1), 0, 64*360);
#elif defined(META)
    mwEraseOval((rect *) &r);
#elif defined(GX)
    grDrawEllipse((x1+x2) / 2, (y1+y2) / 2, (x2-x1)/2, (y2-y1)/2, grFILL);
#elif defined(GURU)
    FREllipse((x1+x2) / 2, (y1+y2) / 2, (x2-x1)/2, (y2-y1)/2);
#elif defined(MSC)
    _ellipse(_GFILLINTERIOR, (short) x1, (short) y1, (short) x2, (short) y2);
#elif defined(BGI)
    fillellipse((x1+x2) / 2, (y1+y2) / 2, (x2-x1)/2, (y2-y1)/2);
#endif
  }


  /*
    Draw the outline of the ellipse
  */
  if (bPenSet)
  {
    RealizePen(hDC);

#if defined(XWINDOWS)
    XDrawArc(XSysParams.display, lphDC->drawable, lphDC->gc,
             x1, y1, abs(x2-x1), abs(y2-y1), 0, 64*360);
#elif defined(META)
    mwFrameOval((rect *) &r);
#elif defined(GX)
    grDrawEllipse((x1+x2) / 2, (y1+y2) / 2, (x2-x1)/2, (y2-y1)/2, grOUTLINE);
#elif defined(GURU)
    REllipse((x1+x2) / 2, (y1+y2) / 2, (x2-x1)/2, (y2-y1)/2);
#elif defined(MSC)
    _ellipse(_GBORDER, (short) x1, (short) y1, (short) x2, (short) y2);
#elif defined(BGI)
    ellipse((x1+x2) / 2, (y1+y2) / 2, 0, 360, (x2-x1)/2, (y2-y1)/2);
#endif
  }
}

