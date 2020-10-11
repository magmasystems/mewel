/*===========================================================================*/
/*                                                                           */
/* File    : WGDILINE.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#define INCLUDE_MOUSE  

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

/****************************************************************************/
/*                                                                          */
/* Function : MoveTo(HDC hDC, int x, y)                                     */
/*                                                                          */
/* Purpose  : Moves the 'current position' to coordinates <x, y>            */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL MoveTo(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;

  if (MoveToEx(hDC, x, y, (POINT FAR *) &pt))
    return MAKELONG(pt.x, pt.y);
  else
    return 0L;
}

BOOL FAR PASCAL MoveToEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  POINT pt;
  LPHDC lphDC;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  /*
    Save the old pen position for the returned POINT
  */
  if (lppt)
  {
    lppt->x = lphDC->ptPen.x;
    lppt->y = lphDC->ptPen.y;
  }

  /*
    Convert the logical points to screen points
  */
  pt.x = (MWCOORD) x;  pt.y = (MWCOORD) y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  _XLogToPhys(lphDC, &pt.x, &pt.y);

  /*
    Move to the specified coordinates
  */
  _moveto((short) pt.x, (short) pt.y);

  lphDC->ptPen.x = (MWCOORD) x;
  lphDC->ptPen.y = (MWCOORD) y;
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : LineTo(HDC hDC, int x, y)                                     */
/*                                                                          */
/* Purpose  : Draws a line from the current position to <x, y> using the    */
/*            current pen style.                                            */
/*                                                                          */
/* Returns  : TRUE if the line was drawn, FALSE if not.                     */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL LineTo(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;
  POINT ptOrg;
  LPHDC lphDC;
  BOOL  bPenSet;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;
    
  /*
    Set the pen style and the writing mode
  */
  bPenSet = RealizePen(hDC);
  RealizeROP2(hDC);


  /*
    In TC, we need to do a moveto() because the setviewport() call
    resets the pen position to 0,0.
  */
  ptOrg.x = lphDC->ptPen.x;    ptOrg.y = lphDC->ptPen.y;
  GrLPtoSP(hDC, (LPPOINT) &ptOrg, 1);
  _XLogToPhys(lphDC, &ptOrg.x, &ptOrg.y);
  _moveto(ptOrg.x, ptOrg.y);

  /*
    Convert the logical coordinates into screen coordinates
  */
  pt.x = (MWCOORD) x;  pt.y = (MWCOORD) y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  _XLogToPhys(lphDC, &pt.x, &pt.y);


  /*
    Hide the mouse if it overlaps the line
  */
  if (bPenSet)
    MOUSE_ConditionalOffDC(lphDC, ptOrg.x, ptOrg.y, pt.x, pt.y);


  /*
    Draw the line if the pen is not PS_NULL
  */
#ifdef USE_REGIONS
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis && bPenSet)
  {
    RECT rLine, rTmp;
    RECT rClipping;
    INT  i;
    PREGION pRegion;

    /*
      Get the screen-based coordinates of the endpoints.
    */
    SetRect(&rLine, ptOrg.x, ptOrg.y, pt.x, pt.y);
    OffsetRect(&rLine, SysGDIInfo.rectLastClipping.left,
                       SysGDIInfo.rectLastClipping.top);

    ptOrg.x = rLine.left;    /* ptOrg and pt are now in screen coords */
    ptOrg.y = rLine.top;
    pt.x    = rLine.right;
    pt.y    = rLine.bottom;
    MEWELSortPoints(&rLine.left, &rLine.top, &rLine.right, &rLine.bottom);

    /*
      Get the DC's clipping rect into a local variable for speed
    */
    rClipping = lphDC->rClipping;

    /*
      Some graphics libs freak out if you use a negative coordinate
      for the lineto function. So make sure that the coordinates are
      positive or 0.
    */
    if (rClipping.left < 0)
    {
      ptOrg.x += rClipping.left;
      pt.x    += rClipping.left;
      OffsetRect(&rLine, rClipping.left, 0);
    }
    if (rClipping.top < 0)
    {
      ptOrg.y += rClipping.top;
      pt.y    += rClipping.top;
      OffsetRect(&rLine, 0, rClipping.top);
    }

    /*
      Make the line obey the DC's clipping region
      (Don't use IntersectRect)
    */
    rLine.left   = max(rLine.left,    SysGDIInfo.rectLastClipping.left);
    rLine.top    = max(rLine.top,     SysGDIInfo.rectLastClipping.top);
    rLine.right  = max(rLine.right,   SysGDIInfo.rectLastClipping.right);
    rLine.bottom = max(rLine.bottom,  SysGDIInfo.rectLastClipping.bottom);

    /*
      Make the enclosing rectangle non-empty. The rect can be empty
      if we are drawing a strictly vertical or horizontal line.
    */
    if (rLine.left == rLine.right)
      rLine.right++;
    if (rLine.top == rLine.bottom)
      rLine.bottom++;

    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      /*
        Does the line cross through one of the clipping regions?
      */
      if (IntersectRect(&rTmp, &pRegion->rects[i], &rLine))
      {
        /*
          Convert the rectangle to fill back to viewport-relative
          coords and call the graphics engine to erase the region.
        */
        rTmp.right  = min(rTmp.right,  SysGDIInfo.cxScreen-1);
        rTmp.bottom = min(rTmp.bottom, SysGDIInfo.cyScreen-1);
        if (!IsRectEmpty(&rTmp))
        {
          _setviewport(rTmp.left, rTmp.top,
                       rTmp.right-EXTENT_OFFSET, rTmp.bottom-EXTENT_OFFSET);
          _moveto(ptOrg.x - rTmp.left, ptOrg.y - rTmp.top);
          _lineto(pt.x    - rTmp.left, pt.y    - rTmp.top);
        }
      }
    }

    /*
      Unlock the region data, reset the viewport the the original one,
      and show the mouse again.
    */
    UNLOCKREGION(lphDC->hRgnVis);
    _setviewport(SysGDIInfo.rectLastClipping.left, 
                 SysGDIInfo.rectLastClipping.top, 
                 SysGDIInfo.rectLastClipping.right, 
                 SysGDIInfo.rectLastClipping.bottom);
  }
  else
#endif
  if (bPenSet)
  {
#if defined(XWINDOWS)
    XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc, 
              ptOrg.x,ptOrg.y, pt.x,pt.y);
#else
    _lineto(pt.x, pt.y);
#endif
  }


  /*
    Show the mouse again
  */
  if (bPenSet)
    MOUSE_ShowCursorDC();

  /*
    Set the current position to the endpoint of the line.
  */
  lphDC->ptPen.x = (MWCOORD) x;
  lphDC->ptPen.y = (MWCOORD) y;
  return TRUE;
}

