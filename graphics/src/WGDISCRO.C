/*===========================================================================*/
/*                                                                           */
/* File    : WGDISCROL.C                                                     */
/*                                                                           */
/* Purpose : Low-level graphics scrolling                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


VOID FAR PASCAL GDIScrollWindow(HDC hDC, LPRECT lprcDest, LPRECT lprcSrc)
{
#if defined(META)

  RECT   rSrc, rDest;
  LPHDC  lphDC;
  int    cx, cy;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return;

  cx = lprcDest->left - lprcSrc->left;
  cy = lprcDest->top  - lprcSrc->top;

  rSrc = *lprcSrc;
  rDest = *lprcDest;


  /*
    Hide the mouse
  */
  MOUSE_HideCursor();


#ifdef USE_REGIONS
  /*
    Go through all of the visible rectangles in the HDC and draw
    part of the rectangle.
  */
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT rSrc2, rDest2, rSrc3;
    RECT rScroll, rScrollScr;
    RECT rErase;
    INT  i;
    PREGION pRegion;

    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      /*
        Get the parts of the src and dest rects which intersect with
        this region. Then union them. rScroll is in screen coordinates.
      */
      if (!IntersectRect(&rSrc2,  &pRegion->rects[i], &rSrc))
        continue;
      if (!IntersectRect(&rDest2, &pRegion->rects[i], &rDest))
        continue;
      UnionRect(&rScroll, &rSrc2, &rDest2);

      /*
        Set the clipping viewport.
      */
      _setviewport(rScroll.left, rScroll.top, rScroll.right-1, rScroll.bottom-1);

      /*
        Save the screen coordinates of the scrolled area
      */
      rScrollScr = rScroll;

      /*
        Get the visible region into viewport-relative coordinates
      */
      OffsetRect(&rScroll, -rScroll.left, -rScroll.top);
      mwScrollRect((rect *) &rScroll, cx, cy);


      /*
        Convert rErase back into screen coords, and then into client coords.
        Then invalidate the area. (We should really tile this rect)
      */
      if (lphDC->hWnd && !IS_MEMDC(lphDC))
      {
        rErase = rScrollScr;
        if (cy < 0)
        {
          rErase.top = rScrollScr.bottom + cy;
          rErase.bottom = rScrollScr.bottom;
        }
        else if (cy > 0)
        {
          rErase.top = rScrollScr.top;
          rErase.bottom = rScrollScr.top + cy;
        }
/*
        if (cx < 0)
        {
          rErase.left = rScrollScr.right + cx;
          rErase.right = rScrollScr.right;
        }
        else if (cx > 0)
        {
          rErase.left = rScrollScr.left;
          rErase.right = rScrollScr.left + cx;
        }
*/
        WinScreenRectToClient(lphDC->hWnd, &rErase);
        InvalidateRect(lphDC->hWnd, &rErase, TRUE);
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
    mwScrollRect((rect *) lprcSrc, cx, cy);


  /*
    Restore the mouse
  */
bye:
  MOUSE_ShowCursor();

#endif /* META */
}

