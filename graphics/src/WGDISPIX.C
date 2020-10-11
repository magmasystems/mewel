/*===========================================================================*/
/*                                                                           */
/* File    : WGDISPIX.C                                                      */
/*                                                                           */
/* Purpose : Implements the SetPixel() function                              */
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
/* Function : SetPixel()                                                    */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : The actual RGB color drawn.                                   */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL SetPixel(hDC, x, y, crColor)
  HDC hDC;
  int x, y;
  COLORREF crColor;
{
  LPHDC lphDC;
  POINT pt;
  INT   clrBios;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return 0L;

  /*
    Translate logical to device coordinates
  */
  pt.x = (MWCOORD) x;  pt.y = (MWCOORD) y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  _XLogToPhys(lphDC, &pt.x, &pt.y);
  x = pt.x;  y = pt.y;

  /*
    Map RGB color to a BIOS color.
  */
  clrBios = RGBtoAttr(hDC, crColor);

  /*
    Hide the mouse
  */
  MOUSE_ConditionalOffDC(lphDC, x, y, x+1, y+1);

  /*
    Draw the pixel.
  */
#if defined(XWINDOWS)
  XSetForeground(XSysParams.display, lphDC->gc, clrBios);
  XDrawPoint(XSysParams.display, lphDC->drawable, lphDC->gc, x, y);

#elif defined(META)
  mwRasterOp(zREPz);
  _setcolor(clrBios);
  mwSetPixel(x, y);

#elif defined(GX)
  grPutPixel(x, y, clrBios);

#elif defined(GURU)
  _setcolor(clrBios);
  RPlot(x, y);

#elif defined(MSC)
  _setcolor(clrBios);
  _setpixel(x, y);

#elif defined(BGI)
  putpixel(x, y, clrBios);

#endif

  /*
    Re-enable the mouse
  */
  MOUSE_ShowCursorDC();

  return clrBios;

  /*
    Note :
    We should really return AttrToRGB(clrBios), but this would take 
    too much time for 256-color mode.
  */
}

