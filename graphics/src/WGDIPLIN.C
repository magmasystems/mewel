/*===========================================================================*/
/*                                                                           */
/* File    : WGUIPLIN.C                                                      */
/*                                                                           */
/* Purpose : Implements the PolyLine() function.                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


/****************************************************************************/
/*                                                                          */
/* Function : PolyLine()                                                    */
/*                                                                          */
/* Purpose  : Draws multiple line segments.                                 */
/*                                                                          */
/* Returns  : TRUE if the drawing was done, FALSE if not.                   */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL Polyline(hDC, lpPoint, nPoints)
  HDC     hDC;
  CONST POINT FAR *lpPoint;
  INT     nPoints;
{
  LPHDC  lphDC;
  HPEN   hPen;
  POINT  *lppt;
  int    i;

  if (nPoints <= 0 || (lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  (void) lphDC;

  /*
    Set the viewport, the pen, and the writing mode.
  */
  RealizePen(hDC);
  RealizeROP2(hDC);

  /*
    Because we need to translate logical to device points, we must
    make a copy of the passed lpPoint[] array and work on this
    instead of on the original.
  */
  if ((lppt = (POINT *) emalloc(sizeof(POINT) * nPoints)) == NULL)
    return FALSE;

  lmemcpy((LPSTR) lppt, (LPSTR) lpPoint, nPoints * sizeof(POINT));
  GrLPtoSP(hDC, (LPPOINT) lppt, nPoints);
  _XNLogToPhys(lphDC, nPoints, lppt);

  /*
    Do a series of LineTo's
  */
#if defined(XWINDOWS)
  XDrawLines(XSysParams.display,lphDC->drawable,lphDC->gc,lppt,nPoints,CoordModeOrigin);

#else
  MOUSE_HideCursor();
  _moveto((short) lppt[0].x, (short) lppt[0].y);
  for (i = 1;  i < nPoints;  i++)
    _lineto((short) lppt[i].x, (short) lppt[i].y);
  MOUSE_ShowCursor();
#endif

  /*
    Get rid of the temp array and return
  */
  MyFree(lppt);
  return TRUE;
}

