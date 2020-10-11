/*===========================================================================*/
/*                                                                           */
/* File    : WGDIARC.C                                                       */
/*                                                                           */
/* Purpose : Implements the Windows' Arc() function                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#undef HUGE
#include <math.h>


/****************************************************************************/
/*                                                                          */
/* Function : Arc()                                                         */
/*                                                                          */
/* Purpose  : Draws an arc in the bounding box specified by <x1,y1 x2,y2>.  */
/*            The starting point is specified by <x3,y3> and the endpoint   */
/*            is <x4,y4>. The Arc is drawn counter-clockwise.               */
/*                                                                          */
/* Returns  : TRUE if the arc was drawn, FALSE if not.                      */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL Arc(hDC, x1, y1, x2, y2, x3, y3, x4, y4)
  HDC  hDC;
  int  x1, y1, x2, y2, x3, y3, x4, y4;
{
#if defined(XWINDOWS)
  return _Arc(hDC,
              (MWCOORD) x1, (MWCOORD) y1, (MWCOORD) x2, (MWCOORD) y2, 
              (MWCOORD) x3, (MWCOORD) y3, (MWCOORD) x4, (MWCOORD) y4,
              MODE_ARC);

#else
  LPHDC  lphDC;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;

  /*
    Make sure that (x1,y1) is the upper-left corner and (x2,y2) is the
    lower-right corner of the bounding box.
  */
#if defined(MEWEL_32BITS)
  {
  MWCOORD x11 = x1; MWCOORD y11 = y1; MWCOORD x22 = x2; MWCOORD y22 = y2;
  MEWELSortPoints(&x11, &y11, &x22, &y22);
  x1 = x11; y1 = y11; x2 = x22; y2 = y22;
  }
#else
  MEWELSortPoints(&x1, &y1, &x2, &y2);
#endif
 
  /*
    Realize the drawing pen and the ROP2 code
  */
  RealizePen(hDC);
  RealizeROP2(hDC);

  /*
    Transform logical to screen coordinates
  */
  if (lphDC->wMappingMode != MM_TEXT)
  {
    POINT pt[4];
    pt[0].x = x1;  pt[0].y = y1;
    pt[1].x = x2;  pt[1].y = y2;
    pt[2].x = x3;  pt[2].y = y3;
    pt[3].x = x4;  pt[3].y = y4;
    GrLPtoSP(hDC, (LPPOINT) pt, 4);
    x1 = pt[0].x;  y1 = pt[0].y;
    x2 = pt[1].x;  y2 = pt[1].y;
    x3 = pt[2].x;  y3 = pt[2].y;
    x4 = pt[3].x;  y4 = pt[3].y;
  }

  /*
    Make sure that the two arc points are inside the bounding box
  */
#if 0
  x3 = min(max(x3, x1), x2);
  x4 = min(max(x4, x1), x2);
  y3 = min(max(y3, y1), y2);
  y4 = min(max(y4, y1), y2);
#endif


#if defined(META)
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
    iangle1 += (y3 <= yMid) ? 90 : 180;
  else
    iangle1 += (y3 <= yMid) ? 0  : 270;
  if (x4 < xMid)
    iangle2 += (y4 <= yMid) ? 90 : 180;
  else
    iangle2 += (y4 <= yMid) ? 0  : 270;

  SetRect(&r, x1, y1, x2, y2);
  iDist = abs(iangle2-iangle1);
  if (iangle1 >= iangle2)
    iDist += 360;

  MOUSE_ConditionalOffDC(lphDC, r.left, r.top, r.right, r.bottom);
  mwFrameArc((rect *) &r, iangle1*10, iDist*10);
  MOUSE_ShowCursorDC();
  }

#elif defined(GX)

#elif defined(MSC)
  MOUSE_ConditionalOffDC(lphDC, x1, y1, x2, y2);
  _arc((short) x1, (short) y1, (short) x2, (short) y2,
       (short) x3, (short) y3, (short) x4, (short) y4);
  MOUSE_ShowCursorDC();

#elif defined(BGI)
  {
  int    xRadius, yRadius;
  int    xMid, yMid;
  double fwidth, fheight;
  double fangle1, fangle2;
  int    iangle1, iangle2;


  /*
    Calculate the center of the bounding box and put it in (xMid, yMid)
  */
  xMid = (x1+x2) / 2;
  yMid = (y1+y2) / 2;

  /*
    Calculate the radii
  */
  xRadius = abs(x2 - xMid);
  yRadius = abs(y2 - yMid);

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
    iangle1 += (y3 <= yMid) ? 90 : 180;
  else
    iangle1 += (y3 <= yMid) ? 0  : 270;
  if (x4 < xMid)
    iangle2 += (y4 <= yMid) ? 90 : 180;
  else
    iangle2 += (y4 <= yMid) ? 0  : 270;

  MOUSE_ConditionalOffDC(lphDC, xMid-xRadius, yMid-yRadius, 
                                xMid+xRadius, yMid+yRadius);
  ellipse(xMid, yMid, iangle1, iangle2, xRadius, yRadius);
  MOUSE_ShowCursorDC();
  }
#endif

  return TRUE;

#endif /* XWINDOWS */
}



/****************************************************************************/
/*                                                                          */
/* Function : Arc()                                                         */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
#if defined(XWINDOWS)
BOOL PASCAL _Arc(HDC hDC, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, 
                 MWCOORD x3, MWCOORD y3, MWCOORD x4, MWCOORD y4, int arcMode)
{
  LPHDC  lphDC;
  int    x11, x22, y11, y22;
  int    iWidth, iHeight;
  int    xMid, yMid;
  double fwidth, fheight;
  double fangle1, fangle2;
  int    iangle1, iangle2;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return FALSE;
 
  MEWELSortPoints(&x1, &y1, &x2, &y2);
 
  _XLogToPhys(lphDC, &x1, &y1);
  _XLogToPhys(lphDC, &x2, &y2);
  _XLogToPhys(lphDC, &x3, &y3);
  _XLogToPhys(lphDC, &x4, &y4);

  RealizePen(hDC);
  RealizeROP2(hDC);

  if (lphDC->wMappingMode != MM_TEXT)
  {
    POINT pt[4];
    pt[0].x = x1;  pt[0].y = y1;
    pt[1].x = x2;  pt[1].y = y2;
    pt[2].x = x3;  pt[2].y = y3;
    pt[3].x = x4;  pt[3].y = y4;
    LPtoDP(hDC, (LPPOINT) pt, 4);
    x1 = pt[0].x;  y1 = pt[0].y;
    x2 = pt[1].x;  y2 = pt[1].y;
    x3 = pt[2].x;  y3 = pt[2].y;
    x4 = pt[3].x;  y4 = pt[3].y;
  }

  /*
    Make sure that the two arc points are inside the bounding box
  */
  x3 = min(max(x3, x1), x2);
  x4 = min(max(x4, x1), x2);
  y3 = min(max(y3, y1), y2);
  y4 = min(max(y4, y1), y2);

  /*
    Now, compress the bounding box so that it encompasses the arc points
    exactly
  */
#if 0
  x11 = min(x3, x4);
  x22 = max(x3, x4);
#else
  x11 = x1;
  x22 = x2;
#endif
#if 0
  y11 = min(y3, y4);
  y22 = max(y3, y4);
#else
  y11 = y1;
  y22 = y2;
#endif

  xMid = (x11+x22)/2;
  yMid = (y11+y22)/2;

  iWidth  = abs(x22 - x11);
  iHeight = abs(y22 - y11);

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

#if 0
printf("Arc(1) - x11 %d, y11 %d, x22 %d, y22 %d, xMid %d, yMid %d, fa1 %f, fa2 %f\n",
       x11, y11, x22, y22, xMid, yMid, fangle1, fangle2);
#endif

  /*
    Adjust the angle for the quadrants which the points appear in.
    Right now, the angle is based on the first quadrant (0-90).
  */
  if (x3 < xMid)
    iangle1 += (y3 <= yMid) ? 90 : 180;
  else
    iangle1 += (y3 <= yMid) ? 0  : 270;
  if (x4 < xMid)
    iangle2 += (y4 <= yMid) ? 90 : 180;
  else
    iangle2 += (y4 <= yMid) ? 0  : 270;
#if 0
printf("Arc(2) - iangle1 %d, iangle2 %d\n", iangle1, iangle2);
#endif


  if (arcMode == MODE_CHORD)
    XSetArcMode(XSysParams.display, lphDC->gc, ArcChord);

  XDrawArc(XSysParams.display, lphDC->drawable, lphDC->gc,
           x11, y11, iWidth, iHeight,
           iangle1*64, (iangle2 - iangle1) * 64);

  if (arcMode == MODE_PIE)
  {
    XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc,
              xMid, yMid,
              xMid + (int) (cos(((iangle1*3.1415926)/180.0)) * iWidth  / 2),
              yMid - (int) (sin(((iangle1*3.1415926)/180.0)) * iHeight / 2));
    XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc,
              xMid, yMid,
              xMid + (int) (cos(((iangle2*3.1415926)/180.0)) * iWidth  / 2),
              yMid - (int) (sin(((iangle2*3.1415926)/180.0)) * iHeight / 2));
  }


  if (arcMode == MODE_CHORD)
    XSetArcMode(XSysParams.display, lphDC->gc, ArcPieSlice);

  return TRUE;
}
#endif

