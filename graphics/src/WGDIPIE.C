/*===========================================================================*/
/*                                                                           */
/* File    : WGDIPIE.C                                                       */
/*                                                                           */
/* Purpose : Implements the Pie() function                                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#undef HUGE
#include <math.h>

static BOOL bDoingChord = FALSE;

/****************************************************************************/
/*                                                                          */
/* Function : Pie()                                                         */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL Pie(hDC, x1, y1, x2, y2, x3, y3, x4, y4)
  HDC hDC;
  int  x1, y1, /* top-left corner of rectangle */
       x2, y2, /* bottom-right corner of rectangle */
       x3, y3, /* coord of arc start */
       x4, y4; /* coord of arc end */
{
#if defined(XWINDOWS)
  return _Arc(hDC,
              (MWCOORD) x1, (MWCOORD) y1, (MWCOORD) x2, (MWCOORD) y2, 
              (MWCOORD) x3, (MWCOORD) y3, (MWCOORD) x4, (MWCOORD) y4,
              MODE_PIE);

#else


  POINT pt[4];
  LPHDC lphDC;
  BOOL  bBrushSet, bPenSet;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  /*
    Realize the brush, pen, and rop2 code
  */
  bBrushSet = RealizeBrush(hDC);
  bPenSet   = RealizePen(hDC);
  RealizeROP2(hDC);

  /*
    Convert logical coords to device coordinates
  */
  pt[0].x = x1;  pt[0].y = y1;
  pt[1].x = x2;  pt[1].y = y2;
  pt[2].x = x3;  pt[2].y = y3;
  pt[3].x = x4;  pt[3].y = y4;
  GrLPtoSP(hDC, (LPPOINT) pt, 4);
  x1 = pt[0].x;  y1 = pt[0].y;
  x2 = pt[1].x;  y2 = pt[1].y;
  x3 = pt[2].x;  y3 = pt[2].y;
  x4 = pt[3].x;  y4 = pt[3].y;

  MOUSE_ConditionalOffDC(lphDC, x1, y1, x2, y2);

#if defined(META) || defined(BGI)
  {
  int    xMid, yMid;
  double fwidth, fheight;
  double fangle1, fangle2;
  int    iangle1, iangle2;
  int    iDist;
  RECT   r;

  /*
    Calculate the center of the bounding box and put it in (xMid, yMid)
  */
  xMid = (x1+x2) / 2;
  yMid = (y1+y2) / 2;

  /*
    Calculate the angle of the first point by getting the arctan of the
    right triangle. The width of the triangle is the x coordinate of the
    first point minus the midpoint of the bounding box.
  */
  fwidth  = (double) abs(x3 - xMid);
  fheight = (double) abs(y3 - yMid);
  if (fwidth == 0.0)
    iangle1 = 90;
  else if (fheight == 0.0)
    iangle1 = 0;
  else
  {
    /*
      atan returns radians. To get degrees, divide by (PI/180).
    */
    fangle1 = (atan(fheight / fwidth) * 180.0) / 3.1415926;
    iangle1 = ((int) (fangle1 + 0.5));
  }

  fwidth  = (double) abs(x4 - xMid);
  fheight = (double) abs(y4 - yMid);
  if (fwidth == 0.0)
    iangle2 = 90;
  else if (fheight == 0.0)
    iangle2 = 0;
  else
  {
    fangle2 = (atan(fheight / fwidth) * 180.0) / 3.1415926;
    iangle2 = ((int) (fangle2 + 0.5));
  }

  /*
    Adjust the angle for the quadrants which the points appear in.
    Right now, the angle is based on the first quadrant (0-90).
  */
  if (x3 < xMid)
    if (y3 < yMid)
      iangle1 = 180 - iangle1;
    else
      iangle1 = 180 + iangle1;
  else
    if (y3 > yMid)
      iangle1 = 360 - iangle1;
  				
  if (x4 < xMid)
    if (y4 < yMid)
      iangle2 = 180 - iangle2;
    else
      iangle2 = 180 + iangle2;
  else
    if (y4 > yMid)
      iangle2 = 360 - iangle2;

  SetRect(&r, x1, y1, x2, y2);

#if defined(META)
  iDist = (iangle2 - iangle1);
  if (iDist <= 0)
    iDist += 360;
#endif

  if (bBrushSet)
  {
#if defined(META)
    mwEraseArc((rect *) &r, iangle1*10, iDist*10);
#else
    sector((x1+x2) / 2, (y1+y2) / 2, iangle1, iangle2, (x2-x1)/2, (y2-y1)/2);
#endif
  }

  if (bPenSet)
  {
#if defined(META)
    RealizePen(hDC);
    mwFrameArc((rect *) &r, iangle1*10, iDist*10);
    if (bDoingChord)
    {
      mwMoveTo(x3, y3);
      mwLineTo(x4, y4);
    }
    else
    {
      mwMoveTo(xMid, yMid);
      mwLineTo(x3, y3);
      mwMoveTo(xMid, yMid);
      mwLineTo(x4, y4);
    }
#else
    RealizePen(hDC);
    sector((x1+x2) / 2, (y1+y2) / 2, iangle1, iangle2, (x2-x1)/2, (y2-y1)/2);
    if (bDoingChord)
    {
      moveto(x3, y3);
      lineto(x4, y4);
    }
    else
    {
      moveto(xMid, yMid);
      lineto(x3, y3);
      moveto(xMid, yMid);
      lineto(x4, y4);
    }
#endif
  }
  }

#elif defined(GX)
  (void) bBrushSet;

#elif defined(MSC)
  _pie(bBrushSet ? _GFILLINTERIOR : _GBORDER,
       (short) x1, (short) y1, (short) x2, (short) y2,
       (short) x3, (short) y3, (short) x4, (short) y4);

#endif

  MOUSE_ShowCursorDC();
  return TRUE;
#endif /* XWINDOWS */
}


BOOL FAR PASCAL Chord(hDC, X1, Y1, X2, Y2, X3, Y3, X4, Y4)
  HDC hDC;
  INT X1, Y1, X2, Y2, X3, Y3, X4, Y4;
{
  BOOL  rc;

  bDoingChord = TRUE;
  rc = Pie(hDC, X1, Y1, X2, Y2, X3, Y3, X4, Y4);
  bDoingChord = FALSE;
  return rc;
}

