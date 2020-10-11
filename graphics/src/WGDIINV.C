/*===========================================================================*/
/*                                                                           */
/* File    : WGDIINV.C                                                       */
/*                                                                           */
/* Purpose : Implements the InvertRect() function for MEWEL/GUI.             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

HANDLE PASCAL _GraphicsSaveRect(LPRECT lpRect);
VOID   PASCAL _GraphicsRestoreRect(LPRECT lpRect, HANDLE hBuf);
static VOID PASCAL EngineInvertRect(LPHDC lphDC, LPRECT lpRect);


VOID FAR PASCAL InvertRect(HDC hDC, CONST RECT FAR *lpRect)
{
  RECT   r;
  LPHDC  lphDC;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return;

  /*
    Transform the logical coords to screen coords
  */
  r = *lpRect;
  GrLPtoSP(hDC, (LPPOINT) &r, 2);

  /*
    Sort the points (in case the mapping mode gave us an "upside-down"
    rectangle).
  */
  MEWELSortPoints(&r.left, &r.top, &r.right, &r.bottom);
  _XLogToPhys(lphDC, &r.left, &r.top);
  _XLogToPhys(lphDC, &r.right, &r.bottom);


#if defined(GX)
  /*
    GX's gxGetImage and gxPutImage need absolute screen coords. They do
    not pay attention to the viewport. So, first undo the subtraction
    of the clipping rectangle which GrLPtoSP performed. Then, intersect
    the rectangle with the clipping rectangle to narrow down the area.
  */
  OffsetRect((LPRECT) &r, lphDC->rClipping.left, lphDC->rClipping.top);
  if (!IntersectRect(&r, &r, &lphDC->rClipping))
    return;
#endif

#if !defined(GX) && !defined(MOTIF)
  /*
    We need to see if the left or top of the rectangle is a negative
    number. If it is, then it means that we are trying to invert an
    area which is outside of the clipping region. This can happen, for
    example, when only a part of a listbox has to be drawn, and
    ListBoxRefresh() tries to invert the entire width of the listbox.
  */
  OffsetRect((LPRECT) &r, lphDC->rClipping.left, lphDC->rClipping.top);
  IntersectRect(&r, &r, &lphDC->rClipping);
  IntersectRect(&r, &r, &SysGDIInfo.rectScreen);
  OffsetRect((LPRECT) &r, -lphDC->rClipping.left, -lphDC->rClipping.top);
  if (lphDC->rClipping.top < 0)
  {
    r.top    += lphDC->rClipping.top;
    r.bottom += lphDC->rClipping.top;
  }
  if (lphDC->rClipping.left < 0)
  {
    r.left   += lphDC->rClipping.left;
    r.right  += lphDC->rClipping.left;
  }
#endif

  /*
    Hide the mouse
  */
  MOUSE_HideCursor();


#if defined(USE_REGIONS)
  /*
    Go through all of the visible rectangles in the HDC and draw
    part of the rectangle.
  */
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT r2, r3, rTmp;
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
#if defined(META)
        _setviewport(r3.left, r3.top,
                     /* compensate for +1 in _setviewport() */
                     r3.right-1, r3.bottom-1);
#else
        _setviewport(r3.left, r3.top,
                     r3.right-EXTENT_OFFSET, r3.bottom-EXTENT_OFFSET);

#endif

        /*
          Get the original drawing area into r3, and offset it from the
          intersection of the drawing area and the visible region. r3
          is in viewport-relative coordinates.
        */
        r3 = r2;
        OffsetRect(&r3, -rTmp.left, -rTmp.top);

        /*
          Call the graphics-engine to draw the rectangle
        */
        EngineInvertRect(lphDC, &r3);
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
  EngineInvertRect(lphDC, &r);


  /*
    Restore the mouse
  */
bye:
  MOUSE_ShowCursor();
}


static VOID PASCAL EngineInvertRect(LPHDC lphDC, LPRECT lpRect)
{
  HANDLE hImg;

  (void) lphDC;
  (void) hImg;

#if defined(XWINDOWS)
  XSetFunction(XSysParams.display, lphDC->gc, GXinvert);
  XFillRectangle(XSysParams.display, lphDC->drawable, lphDC->gc,
                 lpRect->left, lpRect->top,
                 lpRect->right - lpRect->left,
                 lpRect->bottom - lpRect->top);

#elif defined(META)
  mwInvertRect((rect *) lpRect);

#else
  /*
    Invert the rectangle by getting the bits in the rectangle and
    restoring them with the XOR mode.
  */
  SysGDIInfo.fFlags |= GDISTATE_INVERTINGRECT;
  hImg = _GraphicsSaveRect(lpRect);
  _GraphicsRestoreRect(lpRect, hImg);
  GlobalFree(hImg);
  SysGDIInfo.fFlags &= ~GDISTATE_INVERTINGRECT;
#endif
}

