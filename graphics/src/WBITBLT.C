/*===========================================================================*/
/*                                                                           */
/* File    : WBITBLT.C                                                       */
/*                                                                           */
/* Purpose : Bitmap functions for MEWEL/GUI                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#endif

/****************************************************************************/
/*                                                                          */
/* Function : BitBlt()                                                      */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : TRUE if it succeeds, FALSE if not.                            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL BitBlt(hDestDC, X,Y, nWidth,nHeight, hSrcDC, XSrc,YSrc, dwRop)
  HDC hDestDC;
  int X, Y;
  int nWidth, nHeight;
  HDC hSrcDC;
  int XSrc, YSrc;
  DWORD dwRop;
{
  /*
    A little fast hack.
    We ignore the nWidth, nHeight, XSrc, and YSrc arguments.
  */
  (void) XSrc;
  (void) YSrc;

  /*
    If the ROP code is BLACKNESS or WHITENESS, then fill the DC with a
    black or white brush.
  */
  if (dwRop == BLACKNESS || dwRop == WHITENESS)
  {
    RECT r;
    SetRect((LPRECT) &r, X, Y, X + nWidth, Y + nHeight);
    FillRect(hDestDC, (LPRECT) &r,
           GetStockObject((dwRop == WHITENESS) ? WHITE_BRUSH : BLACK_BRUSH));
  }

  /*
    We assume that we are blasting a bitmap from the source DC to the
    dest DC.
  */
  else
  {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    MEWELDrawDIBitmap(hDestDC, X, Y, nWidth, nHeight, hSrcDC, XSrc, YSrc, -1, -1, dwRop);
#else
    (void) hSrcDC;
    return FALSE;
#endif
  }

  return TRUE;
}

