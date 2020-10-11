/*===========================================================================*/
/*                                                                           */
/* File    : WGUIFLOD.C                                                      */
/*                                                                           */
/* Purpose : Implements the (Ext)FloodFill function.                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


extern BOOL bGraphicsSystemInitialized;


BOOL FAR PASCAL ExtFloodFill(hDC, X, Y, crColor, wFillType)
  HDC  hDC;           /* handle to device context               */
  INT  X, Y;          /* logical point where filling begins     */
  COLORREF crColor;   /* color of boundary or area to be filled */
  WORD wFillType;     /* FLOODFILLBORDER or FLOODFILLSURFACE    */
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
  LPHDC lphDC;
  POINT pt;
  COLOR attr;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (bGraphicsSystemInitialized)
  {
    /*
      Set the viewport and the filling brush
    */
    _GraphicsSetViewport(hDC);
    _SetROP2(hDC);
    _SetBrush(hDC);

    /*
      Transform the logical point to a screen point
    */
    pt.x = X;  pt.y = Y;
    GrLPtoSP(hDC, (LPPOINT) &pt, 1);

    /*
      Map the RGB color to a BIOS color
    */
    attr = RGBtoAttr(crColor);

#if defined(META)

#elif defined(GX) || defined(MSC) || defined(BGI)
    floodfill(pt.x, pt.y, attr);
#endif

    return TRUE;
  }

  return FALSE;
}

