/*===========================================================================*/
/*                                                                           */
/* File    : WGDIGPIX.C                                                      */
/*                                                                           */
/* Purpose : Implements the GetPixel() function                              */
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
/* Function : GetPixel()                                                    */
/*                                                                          */
/* Purpose  : Retrieves the color value of the pixel at the point <x. y>    */
/*                                                                          */
/* Returns  : The RGB color of the pixel at point <x,y>                     */
/*                                                                          */
/****************************************************************************/
DWORD PASCAL GetPixel(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  LPHDC lphDC;
  POINT pt;
  COLOR attr;

  if ((lphDC = GDISetup(hDC)) == NULL)
    return 0L;

  /*
    Translate logical to device coordinates
  */
  pt.x = x;  pt.y = y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  x = pt.x;  y = pt.y;

  /*
    Hide the mouse
  */
  MOUSE_ConditionalOffDC(lphDC, x, y, x+1, y+1);


  /*
    Get the pixel and translate the BIOS color to an RGB color.
  */
#if defined(XWINDOWS)
  {
  XImage *image = XGetImage(XSysParams.display, lphDC->drawable, x, y, 1, 1,
                            AllPlanes, ZPixmap);
  attr = XGetPixel(image, 0, 0);
  XDestroyImage(image);
  }
#else
  attr = getpixel(x, y);
#endif

#if defined(META)
  if ((int) attr == -1)
    attr = 15;
#endif

  /*
    Re-enable the mouse
  */
  MOUSE_ShowCursorDC();

  return AttrToRGB(attr);
}

