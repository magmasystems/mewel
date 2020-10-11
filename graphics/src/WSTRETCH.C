/*===========================================================================*/
/*                                                                           */
/* File    : WSTRETCH.C                                                      */
/*                                                                           */
/* Purpose : Stretches a rectangular bitmap area                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if defined(GX)
#define GX_USE_VS
#endif

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif


#define sign(x) ((x) > 0 ? 1:-1)

/* #define PUT_ENTIRE_BMP */

#if defined(META)

BOOL bMetaCalledFromStretchBlt = FALSE;

BOOL FAR PASCAL StretchBlt(hDestDC, X, Y, nWidth, nHeight,
                           hSrcDC, XSrc, YSrc, nSrcWidth, nSrcHeight, dwRop)
  HDC hDestDC;
  int X, Y;
  int nWidth, nHeight;
  HDC hSrcDC;
  int XSrc, YSrc;
  int nSrcWidth, nSrcHeight;
  DWORD dwRop;
{
  bMetaCalledFromStretchBlt = TRUE;
  MEWELDrawDIBitmap(hDestDC, X,Y, nWidth,nHeight, 
                    hSrcDC, XSrc,YSrc, nSrcWidth, nSrcHeight, dwRop);
  bMetaCalledFromStretchBlt = FALSE;
  return TRUE;
}

#else

BOOL FAR PASCAL StretchBlt(hDestDC, X, Y, nWidth, nHeight,
                           hSrcDC, XSrc, YSrc, nSrcWidth, nSrcHeight, dwRop)
  HDC hDestDC;
  int X, Y;
  int nWidth, nHeight;
  HDC hSrcDC;
  int XSrc, YSrc;
  int nSrcWidth, nSrcHeight;
  DWORD dwRop;
{
#if defined(GX)
  GXHEADER    *pGXSrc, *pGXDest;
  GXHEADER    gxTmpHeader;
#if defined(GX_USE_VS)
  LPSTR       pBuf;
  int         nScan;
#endif

#elif defined(GNUGRX)
  GrContext  *pGCSrc, *pGCDest;
  GrContext  *pGCTmpContext;
  int        iBytesInPlane;

#elif defined(METABGI)
  int        iBytesInPlane;

#endif

  LPOBJECT    lpObj;
  LPHDC       lphDestDC, lphSrcDC;
  BITMAP      bm;
  HBITMAP     hBitmap;
  LPBMPHEADER lpBMPHeader;
  IMGHEADER FAR *pScan;
  int         nRGB, iPlane, nDestScanWidth;
  int         bmWidthBytes;
  char HUGE   *hpBits;
  char HUGE   *pScan2;

  int  xMirror = 1,
       yMirror = FALSE;

  POINT pt;
  int  cyDest, cySrc, cyDest2, cyDiff;
  int  y;
  int  cxDest, cxSrc, cxDest2, cxDiff;
  int  x;
  int  iEngineROP;


  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return FALSE;

  if ((lphDestDC = _GetDC(hDestDC)) == NULL)
    return FALSE;
  if ((lphSrcDC = _GetDC(hSrcDC)) == NULL)
    return FALSE;
  (void) lphDestDC;

  if ((hBitmap = lphSrcDC->hBitmap) == NULL)
    return FALSE;

  /*
    Set the clipping area
  */
  if (!_GraphicsSetViewport(hDestDC))
    return FALSE;

  /*
    Get the engine-specific ROP code
  */
  iEngineROP = MapROPCode(dwRop);

  /*
    Get the dimensions of the bitmap
  */
  lpObj = _ObjectDeref(hBitmap);
  GetObject(hBitmap, sizeof(bm), (LPSTR) &bm);
  (void) lpObj;

  /*
    Get to the image header and image data
  */
  lpBMPHeader = (LPBMPHEADER) bm.bmBits;
  hpBits = (char HUGE *)
              (bm.bmBits + sizeof(BMPHEADER) + sizeof(IMGHEADER) +
               (lpBMPHeader->nPaletteEntries * sizeof(RGBQUAD)));
#if defined(GX)
  if ((pGXSrc = (GXHEADER *) lpObj->lpExtra) == NULL)
    return 0;
  hpBits = pGXSrc->vptr;

  pGXDest = NULL;
  if (IS_MEMDC(lphDestDC))
  {
    if ((lpObj = _ObjectDeref(lphDestDC->hBitmap)) != NULL)
      pGXDest = (GXHEADER *) lpObj->lpExtra;
  }

#if defined(GX_USE_VS)
  if ((pBuf = emalloc(pGXSrc->bplin * 4)) == NULL)
    return 0;
#endif

#elif defined(GNUGRX)
  if ((pGCSrc = (GrContext *) lpObj->lpExtra) == NULL)
    return 0;
  hpBits = pGCSrc->gc_baseaddr;

  pGCDest = NULL;
  if (IS_MEMDC(lphDestDC))
  {
    if ((lpObj = _ObjectDeref(lphDestDC->hBitmap)) != NULL)
      pGCDest = (GrContext *) lpObj->lpExtra;
  }

  if (bm.bmBitsPixel == 4)
    iBytesInPlane = bm.bmHeight * (bm.bmWidthBytes >> 2);

#elif defined(METABGI)
  if (bm.bmBitsPixel == 4)
    iBytesInPlane = bm.bmHeight * (bm.bmWidthBytes >> 2);

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

  bmWidthBytes = bm.bmWidthBytes;
  if (yMirror == -1)
    bmWidthBytes = -bmWidthBytes;


  /*
    Adjust the origin of the output
  */
#if defined(GX) || defined(GNUGRX)
  /*
    Since the GX gxPutImage and gxVirtualDisplay do not obey the viewport,
    we need to get the absolute screen coordinates to write to.
  */
  X += lphDestDC->rClipping.left;
  Y += lphDestDC->rClipping.top;
#endif
  X -= (lphDestDC->rClipping.left - lphDestDC->ptOrg.x);
  Y -= (lphDestDC->rClipping.top  - lphDestDC->ptOrg.y);

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
    Allocate one scanline for the stretched bitmap. We will use putimage()
    to output one scanline at a time.
  */
  if ((nDestScanWidth = BitmapBitsToLineWidth(bm.bmBitsPixel,
                                              cxDest,
                                              &nRGB)) < 0)
    return FALSE;

#ifdef PUT_ENTIRE_BMP
  pScan = (IMGHEADER *) emalloc_far(sizeof(IMGHEADER) + 
                                 ((LONG) cyDest * (LONG) nDestScanWidth));
  pScan->wWidth  = cxDest - PUTIMAGE_OFFSET;
  pScan->wHeight = cyDest - PUTIMAGE_OFFSET;
  pScan2 = (char HUGE *) (((LPSTR) pScan) + sizeof(IMGHEADER));
#else

#if defined(GX)
  bmWidthBytes = (bm.bmBitsPixel == 4) ? pGXSrc->bplin*4 : pGXSrc->bplin;
  gxCreateVirtual(gxCMM, &gxTmpHeader, gxGetDisplay(), cxDest, 1);
  nDestScanWidth = gxTmpHeader.bplin * gxTmpHeader.nplanes;
  pScan = (IMGHEADER *) emalloc(nDestScanWidth);
#elif defined(GNUGRX)
  pScan = (IMGHEADER *) emalloc(nDestScanWidth);
  pGCTmpContext = GrCreateContext(cxDest, 1, pScan, NULL);
#elif defined(METABGI)
  pScan = (IMGHEADER *) emalloc(sizeof(IMGHEADER) + nDestScanWidth);
  pScan->wWidth     = cxDest - PUTIMAGE_OFFSET;
  pScan->wHeight    = 1 - PUTIMAGE_OFFSET;
  pScan->imWidth    = cxDest - PUTIMAGE_OFFSET;
  pScan->imHeight   = 1 - PUTIMAGE_OFFSET;
  pScan->imAlign    = 0;
  pScan->imFlags    = 0;
  pScan->imBits     = (bm.bmBitsPixel == 4) ? 1 : bm.bmBitsPixel;
  pScan->imPlanes   = (bm.bmBitsPixel == 4) ? 4 : 1;
  pScan->imRowBytes = nDestScanWidth ;
  pScan->imRowBytes /= pScan->imPlanes;
#else
  pScan = (IMGHEADER *) emalloc(sizeof(IMGHEADER) + nDestScanWidth);
  pScan->wWidth  = cxDest - PUTIMAGE_OFFSET;
  pScan->wHeight = 1 - PUTIMAGE_OFFSET;
#endif
#endif

  /*
    Advance the pointer to the image bits to the proper source row.
  */
  if (yMirror == -1)
    hpBits += (((bm.bmHeight-1) - YSrc) * bm.bmWidthBytes);
  else
    hpBits += (YSrc * bm.bmWidthBytes);

#if defined(GX_USE_VS)
  nScan = (yMirror == -1) ? (bm.bmHeight-1) - YSrc : YSrc;
  gxGetVirtualScanline(pGXSrc, nScan, hpBits = pBuf);
#endif

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
      Get a pointer to the new scanline's image and zero everything out.
    */
#ifndef PUT_ENTIRE_BMP
#if defined(GX) || defined(GNUGRX)
    pScan2 = (char HUGE *) (LPSTR) pScan;
#else
    pScan2 = (char HUGE *) (((LPSTR) pScan) + sizeof(IMGHEADER));
#endif
    memset(pScan2, 0, nDestScanWidth);
#endif

    /*
      Stretch one scanline 
    */
    for (x = 0;  x < cxDest;  x++)
    {
      /*
        Transfer the pixel from the source to the destination
      */
      int  color;
      BYTE ch;

      switch (bm.bmBitsPixel)
      {
        case 1 :
          color = (hpBits[xSrc >> 3] & (0x80 >> (xSrc & 0x07))) != 0;
          *pScan2 |= color << (7 - (xDest & 0x07));

          /*
            Advance to the next destination pixel?
          */
          if ((xDest & 0x07) == 0x07)
            pScan2++;
          break;

        case 4 :
        {
#if defined(GNUGRX) || defined(METABGI)
          int  scanWidth = iBytesInPlane;
#else
          int  scanWidth = bm.bmWidthBytes >> 2;
#endif
          int  mask      = 0x80 >> (xSrc & 0x07);
          char HUGE *hp  = hpBits + (xSrc >> 3);

          LPSTR pScan3   = (LPSTR) pScan2;
          int  shiftDest = (7 - (xDest & 0x07));
          int  destWidth = nDestScanWidth >> 2;

          for (iPlane = 0;  iPlane < 4;  iPlane++)
          {
            *pScan3 |= ((*hp & mask) != 0) << shiftDest;
            if (iPlane < 3)
            {
              hp     += scanWidth;  /* advance to the next src plane  */
              pScan3 += destWidth;  /* advance to the next dest plane */
            }
          }

          /*
            Advance to the next destination pixel?
          */
          if ((xDest & 0x07) == 0x07)
            pScan2++;

          break;
        }

        case 8 :
          *pScan2++ = hpBits[xSrc];
          break;
      }

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

#ifdef PUT_ENTIRE_BMP
    pScan2 += nDestScanWidth;
#else

#if defined(GX)
    gxSetVirtualScanline(&gxTmpHeader, 0, (char *) pScan);
    if (pGXDest == NULL)
      gxVirtualDisplay(&gxTmpHeader, 0, 0, X, Y, X+cxDest-1, Y, 0);
    else
      gxVirtualVirtual(&gxTmpHeader, 0, 0, cxDest-1, 0,
                       pGXDest, X, Y, iEngineROP);

#elif defined(GNUGRX)
  GrBitBlt(pGCDest, X,Y, pGCTmpContext, 0, 0, 
           pGCTmpContext->gc_xmax, pGCTmpContext->gc_ymax, iEngineROP);

#else
    _putimage(X, Y, (char HUGE *) pScan, iEngineROP);
#endif
#endif

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
#if defined(GNUGRX) || defined(METABGI)
      hpBits += bmWidthBytes >> 2;
#else
      hpBits += bmWidthBytes;
#endif
#if defined(GX_USE_VS)
      gxGetVirtualScanline(pGXSrc, (yMirror == -1) ? --nScan : ++nScan, 
                           hpBits = pBuf);
#endif
    }
    Y++;
    cyDiff += cySrc;
  }

bye:
#ifdef PUT_ENTIRE_BMP
  _putimage(X, Y, pScan, iEngineROP);
#endif

#if defined(GX)
  gxDestroyVirtual(&gxTmpHeader);
#if defined(GX_USE_VS)
  MyFree(pBuf);
#endif
#elif defined(GNUGRX)
  GrDestroyContext(pGCTmpContext);
#endif

  MOUSE_ShowCursor();
  if (pScan)
    MyFree(pScan);
  return TRUE;
}

#endif /* META*/

