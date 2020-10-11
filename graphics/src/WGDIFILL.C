/*===========================================================================*/
/*                                                                           */
/* File    : WGDIFILL.C                                                      */
/*                                                                           */
/* Purpose : Implements the Rectangle, FillRect, and RoundRect functions.    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#define INCLUDE_MOUSE  

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#if defined(XWINDOWS)
#define paintFRAME(x1, y1, x2, y2) \
      XDrawRectangle(XSysParams.display, \
                     lphDCFill->drawable, \
                     lphDCFill->gc,                \
                     x1, y1, (x2) - (x1), (y2) - (y1))
#define paintFILL(x1, y1, x2, y2)  \
      XFillRectangle(XSysParams.display, \
                     lphDCFill->drawable, \
                     lphDCFill->gc,                \
                     x1, y1, (x2) - (x1), (y2) - (y1))
#elif defined(GX)
#define paintFRAME(x1, y1, x2, y2)  grDrawRect(x1, y1, x2, y2, grOUTLINE)
#define paintFILL(x1, y1, x2, y2)   grDrawRect(x1, y1, x2, y2, grFILL)
#elif defined(GURU)
#define paintFRAME(x1, y1, x2, y2)  RFrame(x1, y1, x2, y2)
#define paintFILL(x1, y1, x2, y2)   RBox(x1, y1, x2, y2)
#elif defined(MSC)
#define paintFRAME(x1, y1, x2, y2)  _rectangle(_GBORDER,       x1, y1, x2, y2)
#define paintFILL(x1, y1, x2, y2)   _rectangle(_GFILLINTERIOR, x1, y1, x2, y2)
#elif defined(BGI)
#define paintFRAME(x1, y1, x2, y2)  rectangle(x1, y1, x2, y2)
#define paintFILL(x1, y1, x2, y2)   bar(x1, y1, x2, y2)
#endif


/*
  Structure used by the RoundRect function.
*/
static struct
{
  BOOL bRoundRect;  /* Are we in the middle of a call to RoundRect()? */
  INT  cx, cy;
} RoundRectInfo =
{
  FALSE,
  0, 0
};

#if defined(XWINDOWS)
static LPHDC lphDCFill;
#endif

static BOOL PASCAL CommonRectangle(HDC hDC, RECT r, 
                                   HBRUSH hBrush, BOOL bFromRectangle);
static VOID PASCAL EngineRectErase(LPRECT);
static VOID PASCAL EngineRectangle(HDC, RECT, BOOL, BOOL);

/****************************************************************************/
/*                                                                          */
/* Function : FillRect()                                                    */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL FillRect(HDC hDC, CONST RECT FAR *lpRect, HBRUSH hBrush)
{
  RECT r;
  r = *lpRect; /* make a copy of the rectangle */
  return CommonRectangle(hDC, r, hBrush, FALSE);
}

/****************************************************************************/
/*                                                                          */
/* Function : Rectangle()                                                   */
/*                                                                          */
/* Purpose  : Draws a rectangle with the frame controlled by the pen which  */
/*            is in the DC and the interior filled wih the bursh which is   */
/*            in the DC.                                                    */
/*                                                                          */
/* Returns  : TRUE if drawn, FALSE if not.                                  */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL Rectangle(HDC hDC, int x1, int y1, int x2, int y2)
{
  RECT r;

  /*
    We need to generate a sorted rectangle
  */
  r.left   = (MWCOORD) min(x1, x2);
  r.top    = (MWCOORD) min(y1, y2);
  r.right  = (MWCOORD) max(x1, x2);
  r.bottom = (MWCOORD) max(y1, y2);
  return CommonRectangle(hDC, r, NULL, TRUE);
}


/****************************************************************************/
/*                                                                          */
/* Function : CommonRectangle()                                             */
/*                                                                          */
/* Purpose  : Common function for the FillRect and Rectangle functions.     */
/*                                                                          */
/* Returns  : TRUE if drawn, FALSE if an error occurred.                    */
/*                                                                          */
/****************************************************************************/
static BOOL PASCAL CommonRectangle(HDC hDC, RECT r, 
                                   HBRUSH hBrush, BOOL bFromRectangle)
{
  LPHDC  lphDC;
  HBRUSH hOldBrush;
  BOOL   bPenSet, bBrushSet;

  /*
    Set up the viewport
  */
  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

#if defined(XWINDOWS)
  lphDCFill = lphDC;
#endif


  /*
    Set the brush and the pen
  */
  if (bFromRectangle)
  {
    bBrushSet = RealizeBrush(hDC);
    bPenSet   = RealizePen(hDC);
  }
  else
  {
    /*
      For the FillRect function, see if we have a NULL brush. If so, exit.
    */
    hOldBrush = lphDC->hBrush;
    lphDC->hBrush = hBrush;
    if (!RealizeBrush(hDC))
      goto restore_brush;
  }

  /*
    Use the proper ROP2 code
  */
  RealizeROP2(hDC);

  /*
    Translate logical coordinates to physical coordinates.
    After the call to GrLPtoSP, 'r' will still be in coordinates
    relative to the graphics engine viewport and *not* in
    absolute screen coordinates.
  */
  GrLPtoSP(hDC, (LPPOINT) &r, 2);

  /*
    Sort the points (in case the mapping mode gave us an "upside-down"
    rectangle).
  */
  MEWELSortPoints(&r.left, &r.top, &r.right, &r.bottom);
  _XLogToPhys(lphDC, &r.left, &r.top);
  _XLogToPhys(lphDC, &r.right, &r.bottom);

  /*
    Turn off the mouse if it falls within the area to be drawn.
  */
  MOUSE_ConditionalOffDC(lphDC, r.left, r.top, r.right, r.bottom);


#ifdef USE_REGIONS
  /*
    Go through all of the visible rectangles in the HDC and draw
    part of the rectangle.
  */
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT r2, r3, rTmp, rPen;
    INT  i;
    PREGION pRegion;

    /*
      r2 is screen-based coordinates
    */
    r2 = r;
    OffsetRect(&r2, lphDC->rClipping.left, lphDC->rClipping.top);

    /*
      If the clipping region is smaller than the rectangular area we
      want to draw, then there is a bug.
      DO NOT intersect with the clipping region, or else the outline
      of the rectangle will be drawn around the clipping region.
      We will use another RECT called rPen to save the actual coordinates
      of the rectangle outline.
    */
    if (bFromRectangle)
      rPen = r2;
    if (!IntersectRect(&r2, &r2, &SysGDIInfo.rectLastClipping))
      goto bye;

    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      /*
        Clip the drawing area to the visible region, and put it into 'rTmp'.
        rTmp is expressed in screen coordinates
      */
      if (IntersectRect(&rTmp, &pRegion->rects[i], &r2))
      {
        /*
          Set the viewport (r3) to the drawing area clipped to the
          screen (BGI doesn't like when you set the viewport past the
          edge of the screen). r3 is in screen coordinates.
        */
        SetRect(&r3, rTmp.left, rTmp.top, SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
        if (!IntersectRect(&r3, &r3, &rTmp))
          continue;

        /*
          Set the clipping viewport.
        */
        _setviewport(r3.left, r3.top,
                     r3.right-EXTENT_OFFSET, r3.bottom-EXTENT_OFFSET);

        /*
          Get the original drawing area into r3, and offset it from the
          intersection of the drawing area and the visible region. r3
          is in viewport-relative coordinates.
        */
        if (bFromRectangle)
          r3 = rPen;  /* see comment above about rPen */
        else
          r3 = r2;
        OffsetRect(&r3, -rTmp.left, -rTmp.top);

        /*
          Call the graphics-engine to draw the rectangle
        */
        if (bFromRectangle)
          EngineRectangle(hDC, r3, bPenSet, bBrushSet);
        else
          EngineRectErase(&r3);
      }
    }

    /*
      Restore the original viewport
    */
    UNLOCKREGION(lphDC->hRgnVis);
    _setviewport(SysGDIInfo.rectLastClipping.left, 
                 SysGDIInfo.rectLastClipping.top, 
                 SysGDIInfo.rectLastClipping.right, 
                 SysGDIInfo.rectLastClipping.bottom);
  }
  else
#endif
    if (bFromRectangle)
      EngineRectangle(hDC, r, bPenSet, bBrushSet);
    else
      EngineRectErase(&r);

bye:
  /*
    Restore the possibly-hidden mouse
  */
  MOUSE_ShowCursorDC();

  /*
    Restore the original DC brush
  */
restore_brush:
  if (!bFromRectangle)
  {
    lphDC->hBrush = hOldBrush;
    RealizeBrush(hDC);
  }
  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : EngineRectangle, EngineRectErase                              */
/*                                                                          */
/* Purpose  : Graphics-engine-specific functions for Rectangle/FillRect     */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL EngineRectangle(HDC hDC,RECT r,BOOL bPenSet,BOOL bBrushSet)
{
#if defined(XWINDOWS)
  if (RoundRectInfo.bRoundRect)
  {
    POINT pt;

    /*
      Change the rounded corner radius from logical to device units.
    */
    pt.x = (MWCOORD) RoundRectInfo.cx;
    pt.y = (MWCOORD) RoundRectInfo.cy;
    GrLPtoSP(hDC, (LPPOINT) &pt, 1);
    _XLogToPhys(lphDCFill, &pt.x, &pt.y);

    if (bBrushSet)
    {
      RealizeBrush(hDC);
      XFillArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.left, r.top, pt.x, pt.y, 64*90, 64*90);
      XFillArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.right-pt.x, r.top, pt.x, pt.y, 64*0, 64*90);
      XFillArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.right-pt.x, r.bottom-pt.y, pt.x, pt.y, 64*270,64*90);
      XFillArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.left, r.bottom-pt.y, pt.x, pt.y, 64*180,64*90);

      /*
        We need three bands of rectangles to fill in the rounded area
      */
      pt.x >>= 1;  pt.y >>= 1;
      paintFILL(r.left, r.top+pt.y, r.left+pt.x, r.bottom-pt.y);
      paintFILL(r.left+pt.x, r.top, r.right-pt.x, r.bottom);
      paintFILL(r.right-pt.x, r.top+pt.y, r.right, r.bottom-pt.y);
      pt.x <<= 1;  pt.y <<= 1;
    }
    
    if (bPenSet)
    {
      RealizePen(hDC);

      pt.x >>= 1;  pt.y >>= 1;
      XDrawLine(XSysParams.display, lphDCFill->drawable, lphDCFill->gc, r.left + pt.x, r.top, r.right - pt.x, r.top);
      XDrawLine(XSysParams.display, lphDCFill->drawable, lphDCFill->gc, r.right, r.top + pt.y, r.right, r.bottom - pt.y);
      XDrawLine(XSysParams.display, lphDCFill->drawable, lphDCFill->gc, r.left + pt.x, r.bottom, r.right - pt.x, r.bottom);
      XDrawLine(XSysParams.display, lphDCFill->drawable, lphDCFill->gc, r.left, r.top + pt.y, r.left, r.bottom - pt.y);
      pt.x <<= 1;  pt.y <<= 1;
  
      XDrawArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.left, r.top, pt.x, pt.y, 64*90, 64*90);
      XDrawArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.right-pt.x, r.top, pt.x, pt.y, 64*0, 64*90);
      XDrawArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.right-pt.x, r.bottom-pt.y, pt.x, pt.y, 64*270,64*90);
      XDrawArc(XSysParams.display,  lphDCFill->drawable, lphDCFill->gc, r.left, r.bottom-pt.y, pt.x, pt.y, 64*180,64*90);
    }
  }
  else
  {
    if (bBrushSet)
    {
      RealizeBrush(hDC);
      paintFILL(r.left, r.top, r.right, r.bottom);
    }
    if (bPenSet)
    {
      RealizePen(hDC);
      paintFRAME(r.left, r.top, r.right, r.bottom);
    }
  }

#elif defined(META)
  if (RoundRectInfo.bRoundRect)
  {
    POINT pt;

    /*
      Change the rounded corner radius from logical to device units.
    */
    pt.x = RoundRectInfo.cx;
    pt.y = RoundRectInfo.cy;
    GrLPtoSP(hDC, (LPPOINT) &pt, 1);

    if (bBrushSet)
      mwEraseRoundRect((rect *) &r, pt.x, pt.y);
    if (bPenSet)
      mwFrameRoundRect((rect *) &r, pt.x, pt.y);
  }
  else
  {
    if (bBrushSet)
    {
      RealizeBrush(hDC);
      mwEraseRect((rect *) &r);
    }
#if 22894
    r.right--;  r.bottom--;
#endif
    mwFrameRect((rect *) &r);
  }

#elif defined(MSC) || defined(BGI) || defined(GX) || defined(GURU)
  /*
    Make sure that we get the lower-right corner right. Unlike Windows,
    BGI does not include the lower-right point.
  */
  r.right--;
  r.bottom--;

  if (bBrushSet)
  {
    RealizeBrush(hDC);
    paintFILL(r.left, r.top, r.right, r.bottom);
  }
  if (bPenSet)
  {
    RealizePen(hDC);
    paintFRAME(r.left, r.top, r.right, r.bottom);
  }
#endif
}

static VOID PASCAL EngineRectErase(LPRECT lpRect)
{ 
#if defined(META)
  mwEraseRect((rect *) lpRect);

#elif defined(MSC) || defined(BGI) || defined(GX) || defined(GURU) || defined(XWINDOWS)
  /*
    Make sure that we get the lower-right corner correct. Unlike Windows,
    BGI, MSC, and GX do not include the lower-right point.
    NOTE : we might have a problem here if the width or height of the
    polygon is 2. 
  */
  MWCOORD x2, y2;
  x2 = lpRect->right;  y2 = lpRect->bottom;
#if !defined(XWINDOWS)
  if (x2 - lpRect->left > 0)
    x2--;
  if (y2 - lpRect->top > 1)
    y2--;
#endif
  paintFILL(lpRect->left, lpRect->top, x2, y2);
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : RoundRect()                                                   */
/*                                                                          */
/* Purpose  : Draws a rectangle with the frame controlled by the pen which  */
/*            is in the DC and the interior filled wih the bursh which is   */
/*            in the DC.                                                    */
/*                                                                          */
/* Returns  : TRUE if drawn, FALSE if not.                                  */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL RoundRect(HDC hDC, int x1, int y1, int x2, int y2, int cx, int cy)
{
  BOOL rc;

  /*
    Set up the RoundRect structure for Rectangle to use
  */
  RoundRectInfo.bRoundRect = TRUE;
  RoundRectInfo.cx         = cx;
  RoundRectInfo.cy         = cy;

  rc = Rectangle(hDC, x1, y1, x2, y2);

  /*
    Tell subsequent calls to Rectangle not to use RoundRect anymore
  */
  RoundRectInfo.bRoundRect = FALSE;
  return rc;
}

