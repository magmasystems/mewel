/*===========================================================================*/
/*                                                                           */
/* File    : WGDISTRD.C                                                      */
/*                                                                           */
/* Purpose : Implements the StretchDIBits() function                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif


#define sign(x) ((x) > 0 ? 1:-1)


INT FAR PASCAL StretchDIBits(hDC, X, Y, nWidth, nHeight,
                                  XSrc, YSrc, nSrcWidth, nSrcHeight,
                                  lpvBits, lpBMI, fuColorUse, dwRop)
  HDC   hDC;
  INT   X, Y;
  INT   nWidth, nHeight;
  INT   XSrc, YSrc;
  INT   nSrcWidth, nSrcHeight;
  CONST VOID FAR *lpvBits;
  LPBITMAPINFO lpBMI;
  UINT  fuColorUse;
  DWORD dwRop;
{
  BITMAPINFOHEADER FAR *lpbih;
  LPHDC       lphDC;
  INT         bmWidthBytes, nColors;
  char HUGE   *hpBits;

  INT  xMirror = 1,
       yMirror = FALSE;

  INT  cyDest, cySrc, cyDest2, cyDiff;
  INT  y;
  INT  cxDest, cxSrc, cxDest2, cxDiff;
  INT  x;

#if defined(GX)
  GXHEADER *pGXDest;
#endif


  /*
    These two args aren't used right now...
  */
  (void) dwRop;
  (void) fuColorUse;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  (void) lphDC;

  /*
    Set the clipping area
  */
  if (!_GraphicsSetViewport(hDC))
    return FALSE;

  /*
    Get the image data
  */
  hpBits = (char HUGE *) lpvBits;
  lpbih  = &lpBMI->bmiHeader;

  /*
    MEWEL cannot deal with RLE4 and RLE8 compressed BMPs right now.
  */
  if (lpbih->biCompression)
    return FALSE;

#if defined(GX)
  pGXDest = NULL;
  if (IS_MEMDC(lphDC))
  {
    LPOBJECT lpObj;
    if ((lpObj = _ObjectDeref(lphDC->hBitmap)) != NULL)
      pGXDest = (GXHEADER *) lpObj->lpExtra;
  }
#endif

  /*
    If nWidth is negative, then the bitmap should be rotated around the
    y axis. If nHeight is negative, then the bitmap should be rotated 
    around the x axis.
  */
  if (nWidth  < 0)
  {
    nWidth = -nWidth;
    xMirror = -1;
  }
  if (nHeight < 0)
  {
    nHeight = -nHeight;
    yMirror = -1;
  }
  if (nSrcWidth < 0)
  {
    nSrcWidth = -nSrcWidth;
    xMirror = -1;
  }
  if (nSrcHeight < 0)
  {
    nSrcHeight = -nSrcHeight;
    yMirror = -1;
  }


  /*
    Figure out how many bytes the DIB bits take up.
  */
  if ((bmWidthBytes = BitmapBitsToLineWidth((UINT) lpbih->biBitCount,
                                            (UINT) lpbih->biWidth,
                                            &nColors)) < 0)
    return FALSE;

  /*
    Since we are dealing with a DIB, round to the nearest long when dealing
    with 4-plane bitmaps. (BitmapBitsToLineWidth gives a valid value
    for the dest DDB, not the source DIB).
  */
  if (lpbih->biBitCount == 4)
    bmWidthBytes = (int) ((((lpbih->biWidth+1) >> 1) + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1));
  else if (lpbih->biBitCount == 1 && lpbih->biSizeImage)
    bmWidthBytes = (int) (lpbih->biSizeImage / lpbih->biHeight);

  

  if (yMirror != -1)  /* cause we want to travserve the DIB backwards */
    bmWidthBytes = -bmWidthBytes;


  /*
    Adjust the origin of the output
  */
#if 0
#if defined(GX)
  /*
    Since the GX gxPutImage and gxVirtualDisplay do not obey the viewport,
    we need to get the absolute screen coordinates to write to.
  */
  X += lphDC->rClipping.left;
  Y += lphDC->rClipping.top;
#endif
#endif
  X -= (lphDC->rClipping.left - lphDC->ptOrg.x);
  Y -= (lphDC->rClipping.top  - lphDC->ptOrg.y);


  /*
    If the DC has a clipping rectangle which is a subrectangle of
    the bitmap, then we must adjust the origin of the source bitmap.
  */
#if 0
  {
  int xOffset = lphDC->rClipping.left - lphDC->ptOrg.x;
  int yOffset = lphDC->rClipping.top  - lphDC->ptOrg.y;
  XSrc += xOffset;
  YSrc += yOffset;
  }
#endif


  /*
    Calculate stretching parameters for the y direction
  */
  cyDest  = nHeight;
  cySrc   = nSrcHeight << 1;
  cyDest2 = cyDest << 1;
  cyDiff  = cySrc - cyDest;


  /*
    Calculate stretching parameters for the x direction
  */
  cxDest  = nWidth;
  cxSrc   = nSrcWidth << 1;
  cxDest2 = cxDest << 1;
  cxDiff  = cxSrc - cxDest;

  /*
    Advance the pointer to the image bits to the proper source row.
    Remember that the bits in hpBits are stored upside-down
  */
  if (yMirror == -1)
    hpBits += (YSrc * bmWidthBytes);
  else
    hpBits += (((lpbih->biHeight-1) - YSrc) * (-bmWidthBytes) );

  /*
    Hide the mouse if we are blitting to the screen
  */
  if (!IS_MEMDC(lphDC))
    MOUSE_HideCursor();

  /*
    Go through all of the rows of the bitmap
  */
  for (y = 0;  y < cyDest;  y++)
  {
    int xSrc  = XSrc;
    int xDest = 0;

    if (xMirror == -1)
      xSrc = nSrcWidth - xSrc - 1;

    /*
      Stretch one scanline 
    */
    for (x = 0;  x < cxDest;  x++)
    {
      /*
        Transfer the pixel from the source to the destination
      */
      BYTE color;

      switch (nColors)
      {
        case 2 :
          color = ((hpBits[xSrc>>3] & (0x80 >> (xSrc&7))) != 0) ? 0x0F : 0x00;
          break;

        case 16:
          color = hpBits[xSrc >> 1];
          if (!(xSrc & 0x01))
            color >>= 4;
          color &= 0x0F;
          break;

        case 256:
          color = hpBits[xSrc];
          break;
      }
      SETPIXEL(hDC, X+xDest, Y, color);

      /*
        Possibly move to the next source pixel, but definitely move to the
        next destination pixel.
      */
      while (cxDiff >= 0)
      {
        xSrc += xMirror;
        cxDiff -= cxDest2;
      }
      xDest++;
      cxDiff += cxSrc;
    }

    /*
      Possibly move to the next source scanline, but definitely move to the
      next destination scanline.
    */
    while (cyDiff >= 0)
    {
      YSrc++;
      if (--nSrcHeight <= 0)
        goto bye;
      cyDiff -= cyDest2;
      hpBits += bmWidthBytes;
    }
    Y++;
    cyDiff += cySrc;
  }


  /*
    Clean up....
  */
bye:
  if (!IS_MEMDC(lphDC))
    MOUSE_ShowCursor();
  return TRUE;
}

