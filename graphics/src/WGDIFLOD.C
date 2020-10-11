/*===========================================================================*/
/*                                                                           */
/* File    : WGUIFLOD.C                                                      */
/*                                                                           */
/* Purpose : Implements the (Ext)FloodFill function.                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


BOOL FAR PASCAL ExtFloodFill(hDC, X, Y, crColor, wFillType)
  HDC  hDC;           /* handle to device context               */
  INT  X, Y;          /* logical point where filling begins     */
  COLORREF crColor;   /* color of boundary or area to be filled */
  UINT wFillType;     /* FLOODFILLBORDER or FLOODFILLSURFACE    */
{
#ifdef REALLY_SHOULD_BE_THIS_WAY
  if (wFillType == FLOODFILLBORDER)
    return FloodFill(hDC, X, Y, crColor);
  else
    return FALSE;
#else
  (void) wFillType;
  return FloodFill(hDC, X, Y, crColor);
#endif
}


BOOL FAR PASCAL FloodFill(hDC, X, Y, crColor)
  HDC  hDC;           /* handle to device context               */
  INT  X, Y;          /* logical point where filling begins     */
  COLORREF crColor;   /* color of boundary or area to be filled */
{
  POINT pt;
  COLOR attr;

  if (GDISetup(hDC) == NULL)
    return FALSE;

  /*
    Set the filling brush
  */
  RealizeROP2(hDC);
  RealizeBrush(hDC);

  /*
    Transform the logical point to a screen point
  */
  pt.x = X;  pt.y = Y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);

  /*
    Map the RGB color to a BIOS color
  */
  attr = RGBtoAttr(hDC, crColor);

  MOUSE_HideCursor();

#if defined(META)
  if (attr == 15)
    attr = -1;
  mwBoundaryFill(pt.x, pt.y, attr, (rect *) NULL);

#elif defined(GX) || defined(BGI) || defined(MSC)
  _floodfill(pt.x, pt.y, attr);

#endif

  MOUSE_ShowCursor();
  return TRUE;
}

