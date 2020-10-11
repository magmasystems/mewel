/*===========================================================================*/
/*                                                                           */
/* File    : WGDISDIB.C                                                      */
/*                                                                           */
/* Purpose : Windows Device-Independent Bitmap (DIB) functions               */
/*           SetDIBits()                                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if defined(GX)
#define GX_USE_VS
#endif


INT FAR PASCAL SetDIBits(hDC, hBitmap, nStartScan, nScanLines, lpvBits, 
                         lpBMI, fuColorUse)
  HDC     hDC;
  HBITMAP hBitmap;
  UINT    nStartScan;
  UINT    nScanLines;
  CONST VOID FAR *lpvBits;
  BITMAPINFO FAR *lpBMI;
  UINT    fuColorUse;
{
  INT         rc, i;
  INT         nWidth, nPlaneWidth;
  INT         nBytesPerLine, nSrcBytesPerLine;
  INT         iSrcBitsPixel;
  UINT        nScan;
  HANDLE      hBitmapMem;
  LPOBJECT    lpObj;
  LPBITMAP    lpbm;
  char HUGE   *pImg;
  char HUGE   *lpScanLine;
  char HUGE   *lpSrcBits;
  RGBQUAD FAR *lpRGB;

  BOOL        bIsMonoBitmap = FALSE;
  BOOL        b4to8 = FALSE;
  BOOL        bInvertedPalette;

#if defined(GX)
  GXHEADER *pGX;
#if defined(GX_USE_VS)
  LPSTR    pBuf;
#endif

#elif defined(GNUGRX)
  GrContext *pGC;

#elif defined(META)
  grafMap  *pGM;
#endif


  (void) hDC;
  (void) lpBMI;
  (void) fuColorUse;

  /*
    Get a pointer to the underlying BITMAP structure
  */
  lpObj = _ObjectDeref(hBitmap);
  (void) lpObj;

  /*
    Get a pointer to the bitmap bits and BITMAP structure
  */
  if ((pImg = HObjectToImageBits(hBitmap, &hBitmapMem, &lpbm, NULL)) == NULL)
    return 0;

#if defined(GX)
  if ((pGX = (GXHEADER *) lpObj->lpExtra) == NULL)
  {
    rc = 0;
    goto bye;
  }
  pImg = pGX->vptr;
#if defined(GX_USE_VS)
  if ((pBuf = emalloc(pGX->bplin * 4)) == NULL)
  {
    rc = 0;
    goto bye;
  }
  (void) pImg;
#endif


#elif defined(GNUGRX)
  if ((pGC = (GrContext *) lpObj->lpExtra) == NULL)
  {
    rc = 0;
    goto bye;
  }
  pImg = (char HUGE *) pGC->gc_baseaddr;


#elif defined(META)
  pGM = (grafMap *) lpObj->lpExtra;
  (void) pImg;
#endif


  /*
    Get some important values into local variables
  */
  nWidth        = lpbm->bmWidth;
  nBytesPerLine = lpbm->bmWidthBytes;
  rc            = nScanLines;


  /*
    Resolve any mismatch between the device bitmap format and the
    eventual DBB format
  */
  iSrcBitsPixel = (INT) lpBMI->bmiHeader.biBitCount;
  if (iSrcBitsPixel != lpbm->bmBitsPixel)
  {
    /*
      Deficiency:
      We cannot display a bitmap which has more colors than the
      current video mode permits. (Of course, we can dither....)
    */
    if (iSrcBitsPixel > lpbm->bmBitsPixel)
    {
      rc = 0;
      goto bye;
    }
    if (iSrcBitsPixel == 1)       /* 1->4 or 1->8 */
      bIsMonoBitmap = TRUE;
    else if (iSrcBitsPixel == 4)  /* 4->8 */
      b4to8 = TRUE;
  }


  /*
    Get a pointer to the palette
  */
  lpRGB = lpBMI->bmiColors;
  if (bIsMonoBitmap)
  {
    bInvertedPalette = (BOOL)
      (RGB(lpRGB[0].rgbRed, lpRGB[0].rgbGreen, lpRGB[0].rgbBlue) != 0L);
  }


  /*
    Processing for 16-color (4 plane) bitmaps....
  */
  if (lpbm->bmBitsPixel == 4)
  {
    nPlaneWidth = nBytesPerLine >> 2;

    /*
      If we are converting a mono bitmap to a 4-color bitmap, the number
      of bytes in the source mono bitmap is the pixel width divided by 8,
      and rounded to the nearest long.

      4/19/94 (maa)   Sometimes, the number of bytes per line is the
      total size of the image divided by the total height.

      For a 16-color DIB, the number of bytes per line in the DIB is
      the rounded width divided by 2 and then rounded to the nearest long.
    */
    if (bIsMonoBitmap)
    {
      if (lpBMI->bmiHeader.biSizeImage != 0)
        nSrcBytesPerLine = (int) (lpBMI->bmiHeader.biSizeImage / lpBMI->bmiHeader.biHeight);
      else
        nSrcBytesPerLine = (int) ((((nWidth >> 3) + 3) >> 2) << 2);
    }
    else
    {
      nSrcBytesPerLine = (int) ((((nWidth+1) >> 1) + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1));
    }

    /*
      Point to the last line (but the first scan-line) of the DIB bits.
    */
    lpSrcBits = (char HUGE *) lpvBits;
    lpSrcBits += ((DWORD) lpbm->bmHeight-1) * (DWORD) nSrcBytesPerLine;


#if defined(GNUGRX) || defined(METABGI)
    /*
      Process each plane
    */
    for (i = 0;  i <= 3;  i++)
    {
      int iPlane, iByte, iBit;
      UINT nScanLines2 = nScanLines;
      INT  iShift1;  /* shift for left nibble  */
      INT  iShift2;  /* shift for right nibble */
      LONG iBytesInPlane;
      char HUGE *lpSrcBits2;

      iPlane = iDIBtoDDBPlaneOrder[i];

      iBytesInPlane = nScanLines * nPlaneWidth;

      lpSrcBits = (char HUGE *) lpvBits;
      lpSrcBits += ((DWORD) lpbm->bmHeight-1) * (DWORD) nSrcBytesPerLine;

      iShift1 = 7 - iPlane;
      iShift2 = 3 - iPlane;

      /*
        Go through all of the requested scanlines
      */
      for (nScan = nStartScan;  nScanLines2-- > 0;  nScan++)
      {
        lpScanLine = pImg + (nScan*nPlaneWidth + iBytesInPlane*i);
        lpSrcBits2 = lpSrcBits;

        for (iByte = 0;  iByte < nBytesPerLine;  )
        {
          BYTE chScan = 0;
          for (iBit = 7;  iBit >= 0;  iByte++)
          {
            BYTE ch = *lpSrcBits2++;
            chScan |= ((ch >> iShift1) & 0x01) << (iBit--);
            chScan |= ((ch >> iShift2) & 0x01) << (iBit--);
          }
          *lpScanLine++ = chScan;
        } /* for iByte */
        lpSrcBits -= nSrcBytesPerLine;
      } /* for nScan */
    } /* for iPlane */

#else
    /*
      Go through all of the requested scanlines
    */
    for (nScan = nStartScan;  nScanLines-- > 0;  nScan++)
    {
      int iPlane, iByte, iBit;

#if defined(GX)
#if defined(GX_USE_VS)
      lpScanLine = pBuf;
#else
      lpScanLine = pImg + ((LONG) nScan * (pGX->bplin * 4));
#endif

#elif !defined(META)
      lpScanLine = pImg + ((LONG) nScan * lpbm->bmWidthBytes);
#endif

      /*
        Process each plane
      */
      for (i = 0;  i <= 3;  i++)
      {
#if defined(META)
#if defined(__DPMI32__)
        unsigned char **pRowTbl = pGM->mapTable[i];
        ((char *) pRowTbl) -= ((DWORD) pGM->mapAltMgr);
        lpScanLine = pRowTbl[nScan];
        ((char *) lpScanLine) -= ((DWORD) pGM->mapAltMgr);
#else
        lpScanLine = (pGM->mapTable[i])[nScan];
#endif
#endif

        /*
          For a mono bitmap, we simply copy the bits of the scanline
          4 times, one copy for each plane.
        */
        if (bIsMonoBitmap)
        {
          lmemcpy(lpScanLine, lpSrcBits, nPlaneWidth);
#if 51495
          if (bInvertedPalette)
          {
            int i2 = nPlaneWidth;
            while (i2--)
              *lpScanLine++ ^= 0xFF;
          }
          else
#endif
          lpScanLine += nPlaneWidth;
        }
        else
        {
          INT iShift1;  /* shift factor for left nibble  */
          INT iShift2;  /* shift factor for right nibble */
          char HUGE *lpSrcBits2;

          iPlane = iDIBtoDDBPlaneOrder[i];
          iShift1 = 7 - iPlane;
          iShift2 = 3 - iPlane;
          lpSrcBits2 = lpSrcBits;

          for (iByte = 0;  iByte < nBytesPerLine;  )
          {
            BYTE chScan  = 0;
            /*
              Go through 8 pixels (4 bytes) of the image at a time, and 
              build up one byte's worth of scanline for a single plane.
            */
            for (iBit = 7;  iBit >= 0;  iByte++)
            {
              BYTE ch = *lpSrcBits2++;
              chScan |= ((ch >> iShift1) & 0x01) << (iBit--);
              chScan |= ((ch >> iShift2) & 0x01) << (iBit--);
            }
            *lpScanLine++ = chScan;
          } /* for iByte */
        }
      } /* for iPlane */

      lpSrcBits -= nSrcBytesPerLine;
#if defined(GX_USE_VS)
      gxSetVirtualScanline(pGX, nScan, pBuf);
#endif

    } /* for row */
#endif /* GNUGRX */

  } /* end 4 plane processing */


  else  /* not 4 planes */
  /*
    See if we are converting a mono or a 4-color bitmap to a 256-color
    bitmap.
  */
  if (lpbm->bmBitsPixel == 8 && (bIsMonoBitmap || b4to8))
  {
    nSrcBytesPerLine = (nWidth+1) >> (bIsMonoBitmap ? 3 : 1);

    /*
      Point to the last line (but the first scan-line) of the DIB bits.
    */
    lpSrcBits = (char HUGE *) lpvBits;
    lpSrcBits += ((DWORD) lpbm->bmHeight-1) * (DWORD) nSrcBytesPerLine;

    for (nScan = nStartScan;  nScanLines-- > 0;  nScan++)
    {
      int  col, i, iBit;

#if defined(GX)
#if defined(GX_USE_VS)
      lpScanLine = pBuf;
#else
      lpScanLine = pImg + (nScan * pGX->bplin);
#endif

#elif defined(META)
#if defined(__DPMI32__)
      unsigned char **pRowTbl = pGM->mapTable[0];
      ((char *) pRowTbl) -= ((DWORD) pGM->mapAltMgr);
      lpScanLine = pRowTbl[nScan];
      ((char *) lpScanLine) -= ((DWORD) pGM->mapAltMgr);
#else
      lpScanLine = (pGM->mapTable[0])[nScan];
#endif

#else
      lpScanLine = pImg + (nScan * lpbm->bmWidthBytes);
#endif

      for (col = i = 0;  i < nSrcBytesPerLine;  i++)
      {
        /*
          Grab a byte from the DIB
        */
        BYTE chBit = *lpSrcBits++;

        /*
          If the source bitmap is mono, set the byte of the 256-color DIB
          to either white (if the corresponding bit of the mono bitmap is
          1) or black (if the corresponding bit of the mono bitmap is 0).
        */
        if (bIsMonoBitmap)
        {
          for (iBit = 0;  iBit < 8;  iBit++)
            if (col < nWidth)
            {
              *lpScanLine++ = (BYTE)(((chBit&(0x80>>iBit)) != 0) ? 0x0F : 0x00);
              col++;
            }
        }

        /*
          If we are converting a 4-color DIB to a 256-color bitmap, then
          just copy each nibble from the source to an individual byte in
          the destination.
        */
        else /* b4to8 */
        {
          if (col < nWidth)
          {
            *lpScanLine++ = (BYTE) ((chBit >> 4) & 0x0F);
            col++;
          }
          if (col < nWidth)
          {
            *lpScanLine++ = (BYTE) (chBit & 0x0F);
            col++;
          }
        } /* end b4to8 */
      } /* end for col */

#if defined(GX_USE_VS)
      gxSetVirtualScanline(pGX, nScan, pBuf);
#endif
      lpSrcBits -= (nSrcBytesPerLine << 1);
    } /* end for row */
  }

  /*
    The final case is that we are transferring a mono bitmap to a mono
    bitmap, or a 256-color bitmap to a 256-color bitmap. In this case,
    the DIB is in the same format as the DDB. So, just copy the bits directly.
  */
  else
  {
#if defined(GX)
#if !defined(GX_USE_VS)
    lpScanLine = pImg + (nStartScan * pGX->bplin);
#endif

#elif !defined(META)
    lpScanLine = pImg + (nStartScan * nBytesPerLine);
#endif

    lpSrcBits = (char HUGE *) lpvBits;
    lpSrcBits += ((DWORD) lpbm->bmHeight-1) * (DWORD) nBytesPerLine;

    for (nScan = nStartScan;  nScanLines-- > 0;  nScan++)
    {
#if defined(GX) && defined(GX_USE_VS)
      gxSetVirtualScanline(pGX, nScan, (LPSTR) lpSrcBits);
#endif
#if defined(META)
#if defined(__DPMI32__)
      unsigned char **pRowTbl = pGM->mapTable[0];
      ((char *) pRowTbl) -= ((DWORD) pGM->mapAltMgr);
      lpScanLine = pRowTbl[nScan];
      ((char *) lpScanLine) -= ((DWORD) pGM->mapAltMgr);
#else
      lpScanLine = (pGM->mapTable[0])[nScan];
#endif
#endif
      lmemcpy(lpScanLine, lpSrcBits, nBytesPerLine);

#if defined(GX)
#if !defined(GX_USE_VS)
      lpScanLine += pGX->bplin;
#endif

#elif !defined(META)
      lpScanLine += nBytesPerLine;
#endif
      lpSrcBits -= nBytesPerLine;
    }
  }

bye:
  GlobalUnlock(hBitmapMem);
#if defined(GX_USE_VS)
  MyFree(pBuf);
#endif
  return rc;
}

