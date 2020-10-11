/*===========================================================================*/
/*                                                                           */
/* File    : WGDIGDIB.C                                                      */
/*                                                                           */
/* Purpose : Windows Device-Independent Bitmap (DIB) functions               */
/*           GetDIBits()                                                     */
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

static VOID PASCAL CopyPaletteFromBitmapToBitmapinfo(LPBITMAP lpbm, 
                                                     BITMAPINFO FAR *lpBMI);


INT FAR PASCAL GetDIBits(hDC, hBitmap, nStartScan, nScanLines, lpvBits, 
                         lpBMI, fuColorUse)
  HDC     hDC;
  HBITMAP hBitmap;
  UINT    nStartScan;
  UINT    nScanLines;
  VOID FAR *lpvBits;
  BITMAPINFO FAR *lpBMI;
  UINT    fuColorUse;
{
  INT         rc;
  INT         nWidth, nPlaneWidth;
  INT         nPlanes;
  INT         nScan;
  INT         nSrcWidthBytes, nDestWidthBytes;
  HANDLE      hBitmapMem;
  LPOBJECT    lpObj;
  LPBITMAP    lpbm;
  BITMAPINFOHEADER FAR *lpbih;
  char HUGE   *pImg;
  char HUGE   *lpScanLine;
  char HUGE   *lpDestBits;

#if defined(GX)
  GXHEADER *pGX;
#if defined(GX_USE_VS)
  LPSTR    pBuf = NULL;
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
    Get a pointer to the bitmapinfoheader
  */
  lpbih = &lpBMI->bmiHeader;

  /*
    Get a pointer to the bitmap bits and BITMAP structure
  */
  if ((pImg = HObjectToImageBits(hBitmap, &hBitmapMem, &lpbm, NULL)) == NULL)
    return 0;

  /*
    If lpvBits is NULL, then we should only fill in the lpBMI
    structure with info about the bitmap.
  */
  if (lpvBits == NULL)
  {
    INT biBits =  lpbm->bmPlanes * lpbm->bmBitsPixel;
    #define WIDTHBYTES(i)   (((i)+31) / 32 * 4)

    lpbih->biSizeImage = 
       WIDTHBYTES((DWORD)lpbm->bmWidth * biBits) * lpbm->bmHeight;
    if (lpbih->biCompression != BI_RGB)
      lpbih->biSizeImage = (lpbih->biSizeImage * 3) / 2;
    CopyPaletteFromBitmapToBitmapinfo(lpbm, lpBMI);
    rc = nScanLines;
    goto bye;
  }



#if defined(GX)
  if ((pGX = (GXHEADER *) lpObj->lpExtra) == NULL)
  {
    rc = 0;
    goto bye;
  }
  pImg = pGX->vptr;
  nPlanes = pGX->nplanes;
#if defined(GX_USE_VS)
  if ((pBuf = emalloc(pGX->bplin * nPlanes)) == NULL)
  {
    rc = 0;
    goto bye;
  }
  (void) pImg;
#endif

#elif defined(META)
  pGM = (grafMap *) lpObj->lpExtra;
  nPlanes = pGM->pixPlanes;
  (void) pImg;

#elif defined(GNUGRX)
  pGC = (GrContext *) lpObj->lpExtra;
  pImg = (char HUGE *) pGC->gc_baseaddr;
  nPlanes = lpbm->bmPlanes;

#else
  nPlanes = lpbm->bmPlanes;
#endif


  nWidth      = lpbm->bmWidth;
  nPlaneWidth = lpbm->bmWidthBytes >> 2;
  rc          = nScanLines;

  nSrcWidthBytes  = lpbm->bmWidthBytes;
  /*
    Since we are dealing with a DIB, round to the nearest long when dealing
    with 4-plane bitmaps. (BitmapBitsToLineWidth gives a valid value
    for the dest DDB, not the source DIB).
  */
  if (lpbih->biBitCount == 4)
    nDestWidthBytes = (int) ((((lpbih->biWidth+1) >> 1) + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1));
  else if (lpbih->biBitCount == 1 && lpbih->biSizeImage)
    nDestWidthBytes = (int) (lpbih->biSizeImage / lpbih->biHeight);
  else
    nDestWidthBytes = lpbm->bmWidthBytes;


  /*
    Point to the last line (but the first scan-line) of the DIB bits.
  */
  lpDestBits = (char HUGE *) lpvBits;
  lpDestBits += ((DWORD) lpbm->bmHeight-1) * (DWORD) nDestWidthBytes;


  /*
    Go through all of the scanlines requested
  */
  for (nScan = nStartScan;  nScanLines-- > 0;  nScan++)
  {
    INT   xSrc;
    char HUGE *lpDestBits2;

    /*
      Point to the start of the scanline for the row we are about to process
    */
#if defined(GX)
#if defined(GX_USE_VS)
    gxGetVirtualScanline(pGX, nScan, pBuf);
    lpScanLine = pBuf;
#else
    lpScanLine = pImg + (nScan * (pGX->bplin * nPlanes));
#endif

#elif !defined(META)
    lpScanLine = pImg + (nScan * nSrcWidthBytes);
#endif


    /*
      Test for mono or 256-color mode. In this case, just do a simple
      memcpy from the source to the destination buffer.
    */
    if (nPlanes == 1)
    {
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
      lmemcpy(lpDestBits, lpScanLine, nDestWidthBytes);
      lpDestBits -= nDestWidthBytes;
      continue;
    }

    /*
      Go through each pixel in the scanline
    */
    lpDestBits2 = lpDestBits;
    for (xSrc = 0;  xSrc < nWidth;  xSrc++)
    {
      char HUGE *lpPlane;
      UINT  uMask;
      INT   iPlane;
      BYTE  chAttr;

      /*
        Create a mask which is used to isolate a particular bit 
        in the scanline
      */
      uMask = 0x80 >> (xSrc & 0x07);

      /*
        Go through each of the 4 planes and build up the pixel color.
      */
      lpPlane = lpScanLine;
      for (chAttr = iPlane = 0;  iPlane < 4;  iPlane++)
      {
#if defined(META)
#if defined(__DPMI32__)
        unsigned char **pRowTbl = pGM->mapTable[iPlane];
        ((char *) pRowTbl) -= ((DWORD) pGM->mapAltMgr);
        lpPlane = pRowTbl[nScan];
        ((char *) lpPlane) -= ((DWORD) pGM->mapAltMgr);
#else
        lpPlane = (pGM->mapTable[iPlane])[nScan];
#endif
#endif

        chAttr |= ((*lpPlane & uMask) != 0) << (3-iDIBtoDDBPlaneOrder[iPlane]);

        /*
          Move to the next plane in the scanline
        */
#if !defined(META)
        lpPlane += nPlaneWidth;
#endif
      }

      /*
        Put the DIB pixel into the destination buffer
      */
      if (xSrc & 0x01)
        *lpDestBits2++ |= chAttr;   /* put in the second half of byte */
      else
        *lpDestBits2 = chAttr << 4; /* put in the first half of byte  */

      /*
        If we have finished with the 8 bits of this byte, move on to the
        next byte of the scanline.
      */
      if ((xSrc & 0x07) == 7)
        lpScanLine++;

    } /* end for (xSrc) */

    /*
      Go backwards to the next scanline in the destination buffer
    */
    lpDestBits -= nDestWidthBytes;
  } /* end for (nScanLines) */


bye:
  GlobalUnlock(hBitmapMem);
#if defined(GX_USE_VS)
  MyFree(pBuf);
#endif
  return rc;
}


static VOID PASCAL CopyPaletteFromBitmapToBitmapinfo(LPBITMAP lpbm, 
                                                     BITMAPINFO FAR *lpBMI)
{
  /*
    Get a pointer to the RGBQUAD array in the BITMAPINFO structure
  */
  RGBQUAD FAR *lpRGBDest = lpBMI->bmiColors;

  /*
    Get a pointer to the palette info which is stored as part of the
    internal MEWEL bitmap structure. It follows the BMPHEADER structure.
  */
  RGBQUAD FAR *lpRGBSrc  = (RGBQUAD FAR *) (lpbm->bmBits + sizeof(BMPHEADER));

  /*
    Copy the palette from the internal MEWEL bitmap to the BITMAPINFO struct
  */
  lmemcpy((LPSTR) lpRGBDest, (LPSTR) lpRGBSrc, 
          sizeof(RGBQUAD) * ((LPBMPHEADER) lpbm->bmBits)->nPaletteEntries);
}

