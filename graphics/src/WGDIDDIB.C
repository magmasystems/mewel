/*===========================================================================*/
/*                                                                           */
/* File    : WGDIDDIB.C                                                      */
/*                                                                           */
/* Purpose : Windows Device-Independent Bitmap (DIB) functions               */
/*           SetDIBitsToDevice()                                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

INT FAR PASCAL SetDIBitsToDevice(hDC,
                                 x, y, nWidth, nHeight,
                                 xSrc, ySrc,
                                 nStartScan, nNumScans,
                                 lpBits, pbmi, wMode)

  HDC  hDC;                      /* HDC of the device to blit to       */
  INT  x, y;                     /* coordinates on device to blit      */
  INT  nWidth, nHeight;          /* width and height of the image      */
  INT  xSrc, ySrc;               /* starting point within the image    */
  UINT nStartScan, nNumScans;    /* scanline number of 1st scanline in lpBits */
  VOID FAR *lpBits;              /* DIB bits of the image              */
  BITMAPINFO FAR *pbmi;          /* BITMAPINFOHEADER and RGBQUAD array */
  UINT wMode;                    /* DIB_PAL_COLORS or DIB_RGB_COLORS   */
{
  RECT    r, r3, rTmp;
  INT     nRects;
  PREGION pRegion;
  char HUGE *lpSrc;
  BITMAPINFOHEADER FAR *lpbih;

  POINT pt;
  LPHDC lphDC;
  INT   nColors;
  INT   nSrcWidthBytes, nDestWidthBytes;
  INT   row, col;


  /*
    We aren't using the palette here .... (things to do)
  */
  (void) wMode;

  /*
    Get a pointer to the underlying DC structure
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  /*
    Set the clipping area
  */
  if (!_GraphicsSetViewport(hDC))
    return 0;

  /*
    Translate logical coords to device coords
  */
  pt.x = x;  pt.y = y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  x = pt.x;  y = pt.y;

  /*
    If we are banding, then we must start below the part of the banded
    bitmap which was already drawn.
  */
  y += nStartScan;

  /*
    Get a pointer to the bitmapinfoheader
  */
  lpbih = &pbmi->bmiHeader;

  /*
    Figure out how many pixels one byte translates into
  */
  if ((nSrcWidthBytes = BitmapBitsToLineWidth((UINT) lpbih->biBitCount,
                               (UINT) lpbih->biWidth, &nColors)) < 0)
    return 0;
  /*
    Since we are dealing with a DIB, round to the nearest long when dealing
    with 4-plane bitmaps. (BitmapBitsToLineWidth gives a valid value
    for the dest DDB, not the source DIB).
  */
  if (lpbih->biBitCount == 4)
    nSrcWidthBytes = (int) ((((lpbih->biWidth+1) >> 1) + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1));
  else if (lpbih->biBitCount == 1 && lpbih->biSizeImage)
    nSrcWidthBytes = (int) (lpbih->biSizeImage / lpbih->biHeight);


  /*
    Figure out how many bytes are taken up by the width of the bitmap
    which we want to display.
  */
  if ((nDestWidthBytes = BitmapBitsToLineWidth(lpbih->biBitCount,
                                           nWidth, &nColors)) < 0)
    return 0;

  /*
    Hide the mouse while we blit
  */
  MOUSE_HideCursor();


  if ((INT) (nStartScan + nNumScans) > nHeight)
    nNumScans = nHeight - nStartScan;


  if (lphDC->hRgnVis == NULL)
    goto bye;

  /*
    Get 'r' into screen coordinates.
  */
  SetRect((LPRECT) &r, x, y, x+nWidth, y+nNumScans);
  OffsetRect(&r, lphDC->rClipping.left, lphDC->rClipping.top);
  
  /*
    Clip the bits to the screen. 'r' is in screen coordinates.
  */
  pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);
  for (nRects = pRegion->nRects - 1;  nRects >= 0;  nRects--)
  {
    if (IntersectRect(&rTmp, &pRegion->rects[nRects], &r))
    {
      SetRect(&r3, rTmp.left, rTmp.top, SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
      if (!IntersectRect(&r3, &r3, &rTmp))
        continue;
      _setviewport(r3.left, r3.top, r3.right, r3.bottom);

      /*
        x and y are the 0-based offsets within rectangle 'r' which the
        bitmap should be draw at.
      */
      x =  ((RECT_WIDTH(r)  - (int) nWidth)  / 2);
      y =  ((RECT_HEIGHT(r) - (int) nNumScans) / 2);

      /*
        If we are drawing partially off the screen, then the starting
        x and y coordinates should be off the screen.
      */
      if (r.left - r3.left < 0)
        x = r.left - r3.left;
      if (r.top  - r3.top < 0)
        y = r.top  - r3.top;

      /*
        Point to the first scanline of the DIB to render. The DIB is stored
        upside-down, so we start at the last row of the bitmap bits.
      */
      lpSrc = ((char HUGE*)lpBits) + ((ySrc+nNumScans-1) * nSrcWidthBytes) + xSrc;

      /*
        Go through all of the requested scanlines
      */
      for (row = 0;  row < (INT) nNumScans;  row++, y++)
      {
        char HUGE *lpSrc2 = lpSrc;  /* lpSrc2 is something we can change */
        INT   i;

        for (col = i = 0;  i < nDestWidthBytes;  i++)
        {
          BYTE  chBit = *lpSrc2++;
          INT   iBit;
          COLOR attr;

          switch (nColors)
          {
            case 2 :
              for (iBit = 0;  iBit < 8;  iBit++)
                if (col < nWidth)
                {
                  attr = ((chBit & (0x80 >> iBit)) != 0) ? 0x0F : 0x00;
                  SETPIXEL(hDC, (col++)+x, y, attr);
                }
              break;
      
            case 16 :
              if (col < nWidth)
              {
                attr = (chBit >> 4) & 0x0F;
                SETPIXEL(hDC, (col++)+x, y, attr);
              }
              if (col < nWidth)
              {
                attr = chBit & 0x0F;
                SETPIXEL(hDC, (col++)+x, y, attr);
              }
              break;
      
            case 256 :
              if (col < nWidth)
                SETPIXEL(hDC, (col++)+x, y, chBit);
              break;
          }
        } /* for col */

        /*
          Advance the source pointer to the next scanline.
          (Remember the DIB is stored upside-down)
        */
        lpSrc -= nSrcWidthBytes;
      } /* end for row */

    } /* if IntersectRect */
  } /* for nRects */

  UNLOCKREGION(lphDC->hRgnVis);


  /*
    Restore the mouse and the original clipping area
  */
bye:
  MOUSE_ShowCursor();
  _setviewport(SysGDIInfo.rectLastClipping.left, 
               SysGDIInfo.rectLastClipping.top, 
               SysGDIInfo.rectLastClipping.right, 
               SysGDIInfo.rectLastClipping.bottom);
  return nNumScans;
}

