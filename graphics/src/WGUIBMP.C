/*===========================================================================*/
/*                                                                           */
/* File    : WGUIBMP.C                                                       */
/*                                                                           */
/* Purpose : Bitmap functions for MEWEL/GUI                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
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


/*
  Define USEPALETTE if you want the bitmap palettes to be used.
*/
#if 0
#define USEPALETTE
#endif

static RGBQUAD DefaultMonoPalette[2] =
{
  { 0x00, 0x00, 0x00, 0 },
  { 0xFF, 0xFF, 0xFF, 0 }
};

/*
  Define XLATE_IMAGE if you want mono bitmaps to be translated to
  16-color bitmaps.
  The pitfalls of this are
    1) if USE_PALETTE is defined, all blues on the screen get changed
       to white. If USE_PALETTE is not defined, then the mono image
       comes out with all white pixels replaced by blue pixels.
       (palette entry 1).
    2) It takes time for the XlateImage() routine to work.
*/
/* #define XLATE_IMAGE */


/*
  If the BITMAPS_CONVERTED define is set, then we attempt to turn the bitmap 
  into a device-dependent bitmap which can be used by the fast putimage() 
  functions in the graphics libs.

  A problem is that the putimage() functions usually do not apply any clipping
  so that if we are blasting a bitmap to a window which is smaller than the
  bitmap, the bitmap will overwrite the area outside of the window. The only
  way to get around this is to use the much slower method of drawing a pixel
  at a time. The SetPixel functions of the various graphics libs seem to
  obey the viewport.
*/
#define BITMAPS_CONVERTED

/*
  This table maps Windows' ROP codes to specific graphics engine modes.
*/
static struct
{
  DWORD  dwRop;
  int    iMode;
} RopToModeInfo[] =
{
  { SRCCOPY,   _GPSET },
  { PATINVERT, _GXOR  },
  { SRCINVERT, _GXOR  },
#if defined(META) && 101394
  { NOTSRCCOPY, zINVERTz },
#else
  { NOTSRCCOPY,_GXOR  },
#endif
  { MERGECOPY, _GAND  },
  { SRCAND,    _GAND  },
  { MERGEPAINT,_GOR   },
  { SRCPAINT,  _GOR   }
};


static char HUGE *PASCAL SetUpHeader(char HUGE *, LPBITMAP, RGBQUAD FAR*, INT);
static HBITMAP PASCAL HBitmapToObject(HDC, HBITMAP);

#if defined(META) || defined(GX) || defined(GNUGRX) || defined(XWINDOWS)
#define STORE_IMAGE_ONCE
#define GX_USE_VS
/*
   Pointer to image bits which will be set in the call to HBitmapToObject
*/
static char HUGE *DDBtoSet = NULL;
#endif

#if defined(META)
static grafMap *PASCAL MetaCreateBitmap(HDC, LPBITMAP, char HUGE *);
static image FAR *PASCAL MWXlateImg(image FAR *);
static VOID PASCAL XlateToMono(grafPort *colorPort,   /* source */
                               grafPort *monoPort,    /* dest   */
                               int      transColor);  /* transp color */
#endif
#if defined(GX)
static GXHEADER *PASCAL GxCreateBitmap(HDC, LPBITMAP, char HUGE *);
#endif
#if defined(GNUGRX)
static GrContext *PASCAL GrxCreateBitmap(HDC, LPBITMAP, char HUGE *);
#endif
#if defined(XWINDOWS)
extern Pixmap PASCAL XMEWELCreateBitmap(HDC, LPBITMAP, char HUGE *);
#endif


/*
  We have a problem with some colors being reversed. The pixels
  in the 4-color bitmap are really indices into the RGBQUAD array.
  If instead we interpret the pixels as BIOS color values, then
  the following transformations must be made :
    1 <-> 4  (blue <-> red)
    3 <-> 6  (cyan <-> yellow)
    9 <-> C  (bright blue <-> bright red)
    B <-> E  (bright cyan <-> bright yellow)
  For example, a pixel value of 1 translates into an RGBQUAD value
  of red, not blue as in a BIOS color value. So, all pixel values of
  1 must be translated into 4's so they can be displayed as blue
  pixels.

  This is why we use the iDIBtoDDBPlaneOrder[] array in the loop below.
  We first process plane 0 (the intensity bit) as normal. Then
  we process the RGB values in reverse order.

  Note : This whole color mess does not take into account the instance
  where we have custom colors in the RGBQUAD array!!! This is a whole
  other mess!
*/
int iDIBtoDDBPlaneOrder[] =
#if defined(META) || defined(METABGI)
{ 1, 2, 3, 0 };
#elif defined(GNUGRX)
{ 1, 2, 3, 0 };
#elif defined(BGI)
{ 0, 3, 2, 1 };
#elif defined(WC386)
{ 0, 3, 2, 1 };
#else
{ 1, 2, 3, 0 };
#endif

int iDDBtoDIBPlaneOrder[] =
#if defined(META) || defined(METABGI)
{ 0, 3, 2, 1 };
#elif defined(BGI)
{ 1, 2, 3, 0 };
#elif defined(WC386)
{ 1, 2, 3, 0 };
#else
{ 0, 3, 2, 1 };
#endif

#if defined(METABGI)
static VOID PASCAL METABGI_putimage(int x, int y, IMGHEADER *pImgSrc, int mode);
#endif


/****************************************************************************/
/*                                                                          */
/* Function : PaintWindowWithBitmap()                                       */
/*                                                                          */
/* Purpose  : Draws the named bitmap in the window at coordinates x,y.      */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL DrawBitmapToDC(hDC, x, y, hBitmap, dwRop)
  HDC     hDC;
  int     x, y;
  HBITMAP hBitmap;
  DWORD   dwRop;
{
  BITMAP  bmp;
  HBITMAP hOldBitmap;
  HDC     hMemDC;

  if ((hMemDC = CreateCompatibleDC((HDC) NULL)) != NULL)
  {
    GetObject(hBitmap, sizeof(bmp), (LPSTR) &bmp);
    hOldBitmap = SelectObject(hMemDC, hBitmap);
    BitBlt(hDC, x, y, bmp.bmWidth, bmp.bmHeight, hMemDC, 0, 0, dwRop);
    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : DrawDIBitmap(hDC, hBitmap, x, y, dwRop)                       */
/*                                                                          */
/* Purpose  : This is a MEWEL-specific function which draws a bitmap onto   */
/*            the display (or a memory bitmap) starting at position (x,y).  */
/*            An attempt to use the ROP code is made.                       */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: BitBlt() and DrawIcon()                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MEWELDrawDIBitmap(hDestDC,x,y,nWidth,nHeight,hSrcDC,XSrc,YSrc,nSrcWidth,nSrcHeight,dwRop)
  HDC     hDestDC;
  HDC     hSrcDC;
  INT     x, y;
  INT     nWidth, nHeight;
  INT     XSrc, YSrc;
  INT     nSrcWidth, nSrcHeight;
  DWORD   dwRop;
{
  LPHDC   lphDestDC, lphSrcDC;
  BITMAP  bm;
  LPBITMAP lpbm;
  POINT   ptOrg;
  HBITMAP hBitmap;
  HBITMAP hOldBitmap;
  HANDLE  hBitmapMem;
  INT     row, col, i, iBit, attr, nRGB;
  INT     nBytesPerLine;
  INT     iMode;
  IMGHEADER *pImg;
  char HUGE *lpBits;
  BYTE    chBit;

  (void) nWidth;
  (void) nHeight;
  (void) nSrcWidth;
  (void) nSrcHeight;
  (void) XSrc;
  (void) YSrc;

  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return;

  /*
    Get pointers to the DC info.
  */
  if ((lphDestDC = _GetDC(hDestDC)) == NULL)
    return;
  if ((lphSrcDC = _GetDC(hSrcDC)) == NULL)
    return;

  /*
    Get a handle to the source DC's bitmap.
  */
  if ((hBitmap = lphSrcDC->hBitmap) == NULL)
    return;

  /*
    Make sure we are not trying to blit the default 1x1 mono bitmap.
    If the source DC is not a memory DC, then assume that we are
    trying to copy part of the screen to another DC.
  */
  if (hBitmap == SysGDIInfo.hDefMonoBitmap && IS_MEMDC(lphSrcDC))
    return;

  /*
    Set the clipping area
  */
  if (!_GraphicsSetViewport(hDestDC))
    return;

  /*
    Make the bitmap the display surface of the memory DC
  */
#if !defined(META) && !defined(GX) && !defined(XWINDOWS)
#ifndef ZAPP
  if (IS_MEMDC(lphDestDC))
#endif
    hOldBitmap = SelectObject(hDestDC, hBitmap);
#endif

  /*
    Get the dimensions of the bitmap and get to the image header and image data
  */
  if ((pImg = (IMGHEADER *) HObjectToImageBits(hBitmap, &hBitmapMem, &lpbm, &nRGB)) == NULL)
    goto bye;
  pImg = (IMGHEADER *) (((LPSTR) pImg) - sizeof(IMGHEADER));
  memcpy((char *) &bm, (char *) lpbm, sizeof(bm));

  /*
    If we are using the physical screen for the source, then use the
    destination dimensions.
  */
  if (hBitmap == SysGDIInfo.hDefMonoBitmap && !IS_MEMDC(lphSrcDC))
  {
    bm.bmHeight    = nHeight;
    bm.bmWidth     = nWidth;
    bm.bmBitsPixel = SysGDIInfo.nBitsPerPixel;
  }

  /*
    Translate device coords to logical coords
  */
  ptOrg.x = 0 - (lphDestDC->ptOrg.x - lphDestDC->rClipping.left);
  ptOrg.y = 0 - (lphDestDC->ptOrg.y - lphDestDC->rClipping.top);
#if defined(XWINDOWS)
  /*
    There is no real viewport in X Windows.
  */
  ptOrg.x = x;
  ptOrg.y = y;
  GrLPtoSP(hDestDC, &ptOrg, 1);
  x = ptOrg.x;
  y = ptOrg.y;
#else
  DPtoLP(hDestDC, &ptOrg, 1);
  x -= ptOrg.x;
  y -= ptOrg.y;
#endif

  /*
    Map the rop2 code to a graphics engine specific rop code.
  */
  iMode = MapROPCode(dwRop);


#if defined(BITMAPS_CONVERTED)
  /*
    If we have the bits in putimage() format, and if the bitmap's
    color format matches the display's color format, do a fast
    putimage(). If the putimage fails, then drop down and do the
    bitmap pixel by pixel.
  */
#if defined(ZAPP) || defined(META) || defined(XWINDOWS)
  if (1)
#else
  if (GetDeviceCaps(hDestDC, BITSPIXEL) == (int) bm.bmBitsPixel)
#endif
  {
#if defined(META)
     RECT r;
     char *pLine;
     int  nLineWidth;
     int  iOrigHeight;

     /*
       Set the ROP2 code
     */
     mwRasterOp(iMode);

     /*
       Use the palette which is stored in the bitmap description
     */
#ifdef USEPALETTE
     if (nRGB)
     {
       RGBQUAD FAR *lpRGB = (RGBQUAD FAR *) (bm.bmBits + sizeof(BMPHEADER));
       palData FAR *pPalData;

       if ((pPalData = (palData FAR*)EMALLOC_FAR(nRGB*sizeof(palData))) == NULL)
         goto freeRGB;
       mwReadPalette(0, 0, nRGB-1, pPalData);
       for (i = 0;  i < nRGB;  i++)
       {
         pPalData[i].palRed   = ((int) lpRGB[i].rgbRed)    * 256;
         pPalData[i].palGreen = ((int) lpRGB[i].rgbGreen)  * 256; 
         pPalData[i].palBlue  = ((int) lpRGB[i].rgbBlue)   * 256; 
       }
       mwWritePalette(0, 0, nRGB-1, pPalData);

freeRGB:
       if (pPalData)
         MYFREE_FAR(pPalData);
     }
#endif /* USEPALETTE */


     {
     grafPort *gpSrc = ((LPMETADCINFO) lphSrcDC->lpDCExtra)->pGrafPort;
     grafPort *gpDest = ((LPMETADCINFO) lphDestDC->lpDCExtra)->pGrafPort;
     RECT     rSrc, rDest;
     extern BOOL bMetaCalledFromStretchBlt;

     /*
       For a mono bitmap, we have to map pixel value 0 to black
       (the Meta back color) and pixel value 1 to white (the
       Meta pen color).
     */
     if (bm.bmBitsPixel == 1)
     {
       mwBackColor(0);
       mwPenColor(-1);
     }

     if (bMetaCalledFromStretchBlt)
       SetRect(&rSrc, XSrc, YSrc, XSrc+nSrcWidth, YSrc+nSrcHeight);
     else
       SetRect(&rSrc, XSrc, YSrc, XSrc+bm.bmWidth, YSrc+bm.bmHeight);
     SetRect(&rDest, x, y, x + nWidth, y + nHeight);
     if (!IS_MEMDC(lphDestDC))
       MOUSE_HideCursor();

     /*
       Are we transferring a color bitmap to a mono bitmap
       (probably for masking purposes)?
     */
     if (gpDest->portMap->pixBits   == 1 &&
         gpDest->portMap->pixPlanes == 1 &&
         (gpSrc->portMap->pixPlanes != 1 || gpDest->portMap->pixBits != 1))
     {
       COLOR attr = RGBtoAttr(hSrcDC, lphSrcDC->clrBackground);
       XlateToMono(gpSrc, gpDest, attr);
     }
     else
     if (bMetaCalledFromStretchBlt &&
             (nWidth != nSrcWidth || nHeight != nSrcHeight))
       mwZoomBlit(gpSrc, gpDest, (rect *) &rSrc, (rect *) &rDest);
     else
       mwCopyBlit(gpSrc, gpDest, (rect *) &rSrc, (rect *) &rDest);

     if (!IS_MEMDC(lphDestDC))
       MOUSE_ShowCursor();
     }


#elif defined(GX)
    if (hBitmap)
    {
      GXHEADER *pGXHeaderDest = ((LPGXDCINFO) lphDestDC->lpDCExtra)->gxHeader;
      GXHEADER *pGXHeaderSrc  = ((LPGXDCINFO) lphSrcDC->lpDCExtra)->gxHeader;
      INT      xEnd, yEnd;
      INT      rc;

      x   += lphDestDC->rClipping.left;
      y   += lphDestDC->rClipping.top;
      xEnd = XSrc + nWidth  - 1;
      yEnd = YSrc + nHeight - 1;

      MOUSE_HideCursor();
      if (pGXHeaderSrc == NULL)  /* the source is the screen */
      {
        if (pGXHeaderDest == NULL)    /* screen to screen */
          rc = gxDisplayDisplay(XSrc,YSrc,xEnd,yEnd, 0, x, y, 0);
        else                          /* screen to virtual */
          rc = gxDisplayVirtual(XSrc,YSrc,xEnd,yEnd, 0, pGXHeaderDest, x, y);
      }
      else
      {
        if (pGXHeaderDest == NULL)   /* virtual to screen */
        {
          /*
            GX deficiency note :
              gxPutImage can handle ROP codes, but it cannot take a
            custom origin and dimensions of the image to put.
            gxVirtualDisplay cannot handle ROP codes, but can take
            custom source coords.

            5/8/93 (maa)
              Force gxVirtualDisplay to be used if we are dealing with
            a subset of the entire bitmap. Forget about the ROP2 code
            in this case.

            6/9/93 (maa)
              gxPutImage can only deal with bitmaps which are < 64K, so
            use gxVirtualDisplay for large bitmaps.
          */
          if (XSrc || YSrc || nWidth != bm.bmWidth || nHeight != bm.bmHeight ||
              (DWORD)nHeight*(DWORD)nWidth*(DWORD)bm.bmBitsPixel/8L > 64000L)
            rc = gxVirtualDisplay(pGXHeaderSrc, XSrc, YSrc,
                                  x, y, x+nWidth-1, y+nHeight-1, 0);
          else
            rc = gxPutImage(pGXHeaderSrc, iMode, x, y, 0);
        }
        else                         /* virtual to virtual */
           rc = gxVirtualVirtual(pGXHeaderSrc, XSrc, YSrc, xEnd, yEnd,
                                 pGXHeaderDest, x, y, iMode);
       }
       (void) rc;
       MOUSE_ShowCursor();
    }


#elif defined(GNUGRX)
    {
    GrContext *pGC = ((LPGRXDCINFO) lphSrcDC->lpDCExtra)->grContext;
    GrSetContext(NULL);
    GrResetClipBox();
    MOUSE_HideCursor();
    GrBitBlt(NULL, x+lphDestDC->ptOrg.x, y+lphDestDC->ptOrg.y, pGC, 0, 0, pGC->gc_xmax, pGC->gc_ymax, iMode);
    MOUSE_ShowCursor();
    }

#elif defined(XWINDOWS)
    {
    XSetFunction(XSysParams.display, lphDestDC->gc, iMode);
    if (bm.bmBitsPixel == XSysParams.depth)
    {
      XCopyArea(XSysParams.display,
                lphSrcDC->drawable,
                lphDestDC->drawable,
                lphDestDC->gc,
                XSrc, YSrc,
                nWidth, nHeight,
                x, y);
    }
    else if (bm.bmBitsPixel == 1) /* src bitmap moust be mono */
    {
      XCopyPlane(XSysParams.display,
                lphSrcDC->drawable,
                lphDestDC->drawable,
                lphDestDC->gc,
                XSrc, YSrc,
                nWidth, nHeight,
                x, y, 1);
    }
    }

#else /* !defined(META) && !defined(GX) */
    MOUSE_HideCursor();
#if defined(METABGI) && 0
    METABGI_putimage(x, y, pImg, iMode);
#else
    _putimage(x, y, pImg, iMode);
#endif
    MOUSE_ShowCursor();
#endif /* defined(META) */

    goto bye;
  }
#endif /* BITMAPS_CONVERTED */


#if !defined(GX) && !defined(META) && !defined(XWINDOWS)
  /*
    Figure out how many pixels one byte translates into
  */
  if ((nBytesPerLine=BitmapBitsToLineWidth(bm.bmBitsPixel,bm.bmWidth,NULL)) < 0)
    goto bye;

  /*
    Transfer the bitmap from the memory DC to the window
    For this function, let's assume that the user defined a bitmap
    in the "correct order", unlike the representation of the bitmap
    image in Windows' BMP files.
  */
  lpBits = ((char *) pImg) + sizeof(IMGHEADER);
  MOUSE_HideCursor();
  for (row = 0;  row < (int) bm.bmHeight;  row++, y++)
  {
    /*
      TODO :
        1) Apply rop2 code
    */
    for (col = i = 0;  i < nBytesPerLine;  i++)
    {
      chBit = *lpBits++;
      switch (bm.bmBitsPixel)
      {
        case 1 :
          for (iBit = 0;  iBit < 8;  iBit++)
            if (col < bm.bmWidth)
            {
              attr = ((chBit & (0x80 >> iBit)) != 0) ? 0x0F : 0x00;
              SETPIXEL(hDC, (col++)+x, y, attr);
            }
          break;
  
        case 4 :
          if (col < bm.bmWidth)
          {
            attr = (chBit >> 4) & 0x0F;
            SETPIXEL(hDC, (col++)+x, y, attr);
          }
          if (col < bm.bmWidth)
          {
            attr = chBit & 0x0F;
            SETPIXEL(hDC, (col++)+x, y, attr);
          }
          break;
  
        case 8 :
          if (col < bm.bmWidth)
            SETPIXEL(hDC, (col++)+x, y, chBit);
          break;
      }
    } /* for i */
  }
  MOUSE_ShowCursor();
#endif


bye:
  GlobalUnlock(hBitmapMem);
#if !defined(META) && !defined(GX) && !defined(XWINDOWS)
  if (IS_MEMDC(lphDestDC))
    SelectObject(hDestDC, hOldBitmap);
#endif
  return;
}


int FAR PASCAL MapROPCode(DWORD dwRop)
{
  /*
    Map the rop2 code to a graphics engine specific rop code.
  */
#if defined(XWINDOWS) || (defined(META) && !101394)
  if (dwRop <= 16L)
    return ROPtoEngineCode[(int)dwRop-1];
  else
  {
    /*
      We have something with the same digit in byte 4
    */
    dwRop >>= 16L;
    dwRop = (dwRop & 0x03) | ((dwRop >> 4) & 0x0C);
    return ROPtoEngineCode[(int) dwRop];
  }

#else
  int i;
  for (i = 0;  i < sizeof(RopToModeInfo) / sizeof(RopToModeInfo[0]);  i++)
    if (RopToModeInfo[i].dwRop == dwRop)
      return RopToModeInfo[i].iMode;
  return _GPSET;
#endif
}



/****************************************************************************/
/*                                                                          */
/* Function : CreateDIBitmap()                                              */
/*                                                                          */
/* Purpose  : The main function for creating a GDI Bitmap object given a    */
/*            sequence of bits and a BITMAPHEADER.                          */
/*                                                                          */
/* Returns  : A handle to the bitmap object if successful, NULL if not.     */
/*                                                                          */
/****************************************************************************/
/*
  A note about the INTERNAL_FORMAT constant....
  The GetObject(hBitmapObj, &bm, sizeof(BITMAP)) call must return
    a BITMAP structure, and not a BITMAPINFOHEADER structure. Therefore,
    if INTERNAL_FORMAT is defined, we store the bitmap info as a
    BITMAPINFOHEADER structure, plus the RGBQUAD array. If not, then
    we store the bitmap info as a BITMAP structure, and ignore the
    RGBQUAD array.
*/
HBITMAP FAR PASCAL CreateDIBitmap(hDC, lpInfoHeader, dwUsage, lpInitBits,
                                       lpInitInfo, wUsage)
  HDC hDC;
  LPBITMAPINFOHEADER lpInfoHeader;
  DWORD dwUsage;
  CONST VOID FAR *lpInitBits;
  LPBITMAPINFO lpInitInfo;
  UINT wUsage;
{
  HDC      hOrigDC;
  LPHDC    lphDC;
  INT      iOrigBitCount;
  INT      nColors, nBytesPerLine;
  DWORD    dwBitmapSize;
  HANDLE   hMem;
  HANDLE   hBitmap = NULL;
  LPBITMAP lpbm;
  RGBQUAD FAR *lpPalette;
  BOOL     bIsMonoBitmap = FALSE;
  BOOL     bLowerColorBitmap = FALSE;

  /*
    Get a pointer to the DC info
  */
  if ((hOrigDC = hDC) == NULL)
    hDC = GetDC(_HwndDesktop);
  lphDC = _GetDC(hDC);

  /*
    Translate mono bitmap to color?
    (MetaWindows does this automatically when the bitmap is being drawn)
  */
  iOrigBitCount = (INT) lpInfoHeader->biBitCount;
  if ((INT) lpInfoHeader->biBitCount != lphDC->wBitsPerPixel)
  {
    /*
      Deficiency:
      We cannot display a bitmap which has more colors than the
      current video mode permits. (Of course, we can dither....)
    */
    if ((INT) lpInfoHeader->biBitCount > lphDC->wBitsPerPixel)
      goto bye;
    if (lpInfoHeader->biBitCount == 1)       /* 1->4 or 1->8 */
      bIsMonoBitmap = TRUE;
    else if (lpInfoHeader->biBitCount >= 4)  /* 4->8 */
      bLowerColorBitmap = TRUE;
    /*
      Bump up the bitmap to the higher number of colors.
      (Except for MetaWindows, which can display mono bitmaps in 16 or
       256 color mode)/
    */
#if defined(META) || defined(XWINDOWS)
    if (!bIsMonoBitmap)
#endif
    lpInfoHeader->biBitCount = lphDC->wBitsPerPixel;
  }

  /*
    Determine the number of colors and the number of bytes per scanline.
    The bits must be padded to a LONG boundary.
  */
  if ((nBytesPerLine = BitmapBitsToLineWidth(lpInfoHeader->biBitCount,
                                             (int) lpInfoHeader->biWidth,
                                             &nColors)) < 0)
    goto bye;


  /*
    If we have a 24-bit color bitmap, then there is no palette.
  */
  if (lpInfoHeader->biBitCount >= 24)
    nColors = 0;

  /*
    If we have a mono bitmap with a valid size, then recalc for the padding
  */
  if (bIsMonoBitmap && lpInfoHeader->biSizeImage != 0)
  {
    nBytesPerLine = (int) (lpInfoHeader->biSizeImage / lpInfoHeader->biHeight);
    nBytesPerLine *= lphDC->wBitsPerPixel;
  }


  /*
    Figure out the size of the image. It's the height of the image times
    the number of bytes per scanline. Also, add in sizeof(IMGHEADER) to
    accomodate the various graphics engine's putimage headers, then add
    in the RGBQUAD array and the BMPHEADER.

    We add a fudge factor of 2 cause BGI's imagesize() gives a header
    size of 6 bytes, not 4 bytes.
  */
#if defined(STORE_IMAGE_ONCE)
  dwBitmapSize = 
      sizeof(BMPHEADER) + nColors*sizeof(RGBQUAD) + sizeof(IMGHEADER) + 0;
#else
  dwBitmapSize = (lpInfoHeader->biHeight * nBytesPerLine) + 
      sizeof(BMPHEADER) + nColors*sizeof(RGBQUAD) + sizeof(IMGHEADER) + 2;
#endif

#if defined(STORE_IMAGE_ONCE)
  DDBtoSet = NULL;
#endif

  /*
    Allocate space for the BITMAP structure, the RGBQUAD array, and
    the image bits.
  */
  if ((hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE | GMEM_ZEROINIT,
                          sizeof(BITMAP) + dwBitmapSize)) == NULL)
    return (HBITMAP) NULL;
  lpbm = (LPBITMAP) GlobalLock(hMem);


  /*
    Copy over the bitmap header, the RGB array, and possibly, the bits.
  */
  lpbm->bmWidth      = (int) lpInfoHeader->biWidth;
  lpbm->bmHeight     = (int) lpInfoHeader->biHeight;
  lpbm->bmWidthBytes = nBytesPerLine;
  lpbm->bmPlanes     = 1;
  lpbm->bmBitsPixel  = (BYTE) lpInfoHeader->biBitCount;

  /*
    Point to the place in the BITMAP structure where the image
    bits will be stored.
  */
  lpbm->bmBits = ((LPSTR) &lpbm->bmBits) + 4;
  if (bIsMonoBitmap)
  {
    lpPalette = DefaultMonoPalette;
#if 51495
    lpPalette = (RGBQUAD FAR*) (((LPSTR)lpInitInfo) + sizeof(BITMAPINFOHEADER));
#endif
  }
  else if (bLowerColorBitmap)
    lpPalette = NULL;
  else
    lpPalette = (RGBQUAD FAR*) (((LPSTR)lpInitInfo) + sizeof(BITMAPINFOHEADER));
  SetUpHeader((char HUGE *) lpbm->bmBits, lpbm, lpPalette, nColors);

  /*
    Restore the old bit count from the original bitmap.
  */
  lpInfoHeader->biBitCount = iOrigBitCount;

  /*
    Convert the bitmap to a MEWEL GDI object
  */
  GlobalUnlock(hMem);
  hBitmap = HBitmapToObject(hDC, hMem);

  /*
    Set the initial bits of the DDB
  */
  if (dwUsage == CBM_INIT && lpInitBits != NULL)
    SetDIBits(hDC, hBitmap, 0, lpInfoHeader->biHeight, lpInitBits, lpInitInfo, wUsage);

  /*
    Delete the temp DC and return the handle to the bitmap
  */
bye:
  if (hOrigDC == NULL)
    ReleaseDC(_HwndDesktop, hDC);
  return hBitmap;
}


/****************************************************************************/
/*                                                                          */
/*                    BITMAP CREATION ROUTINES                              */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* Function : CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBits)     */
/*                                                                          */
/* Purpose  : Creates a device-dependent bitmap using the specified args.   */
/*            lpBits contains an array of pixels which is used to init the  */
/*            bitmap. If lpBits is NULL, the bitmap remains uninitializes.  */
/*                                                                          */
/* Returns  : A handle to the bitmap if successful, NULL if not.            */
/*                                                                          */
/****************************************************************************/
HBITMAP FAR PASCAL CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBits)
  int  nWidth;
  int  nHeight;
  UINT nPlanes;
  UINT nBitCount;
  CONST VOID FAR *lpBits;
{
  BITMAP bm;

  bm.bmWidth      = nWidth;
  bm.bmHeight     = nHeight;
  bm.bmPlanes     = (BYTE) nPlanes;
  bm.bmBitsPixel  = (BYTE) nBitCount;
  bm.bmBits       = (LPSTR) lpBits;
  return CreateBitmapIndirect(&bm);
}

HBITMAP FAR PASCAL CreateBitmapIndirect(lpBitmap)
  BITMAP FAR *lpBitmap;
{
  INT      nBytesPerLine;
  INT      nColors;
  DWORD    dwBitmapSize;
  HANDLE   hMem;
  LPSTR    lpMem;
  char HUGE *lpInitBits;
  char HUGE *lpDest;


  /*
    Save a pointer to the bitmap bits
  */
  lpInitBits = (char HUGE *) lpBitmap->bmBits;

  /*
    Determine the number of colors and the number of bytes per scanline.
    The bits must be padded to a LONG boundary.
  */
  if ((nBytesPerLine = BitmapBitsToLineWidth(lpBitmap->bmBitsPixel,
                                             lpBitmap->bmWidth,
                                             &nColors)) < 0)
    return (HBITMAP) NULL;

  lpBitmap->bmWidthBytes = nBytesPerLine;

  /*
    Figure out the size of the image. It's the height of the image times
    the number of bytes per scanline. Also, add in sizeof(IMGHEADER) to
    accomodate the various graphics engine's putimage headers.
  */
  dwBitmapSize = ((DWORD) lpBitmap->bmHeight) * nBytesPerLine + sizeof(IMGHEADER) +
                      + (nColors * sizeof(RGBQUAD)) + sizeof(BMPHEADER);

#if defined(BGI)
  dwBitmapSize += 2;  /* imagesize() sometimes gives a header of 6 bytes */
#endif
#if defined(STORE_IMAGE_ONCE)
  dwBitmapSize -= ((DWORD) lpBitmap->bmHeight) * nBytesPerLine;
  DDBtoSet = NULL;
#endif

  /*
    Allocate space for the BITMAPINFOHEADER, the RGBQUAD array, and
    the image bits.
  */
  if ((hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE | GMEM_ZEROINIT,
                           sizeof(BITMAP) + dwBitmapSize)) == NULL)
    return (HBITMAP) NULL;
  lpMem = GlobalLock(hMem);

  lmemcpy(lpMem, (LPSTR) lpBitmap, sizeof(BITMAP));
  lpBitmap = (BITMAP FAR *) lpMem;

  /*
    Point to the place in the BITMAP structure where the image
    bits will be stored.
  */
  lpBitmap->bmBits = ((LPSTR) &lpBitmap->bmBits) + sizeof(lpBitmap->bmBits);

  /*
    Set up the graphics engine header
  */
  lpDest = SetUpHeader((char HUGE *) lpBitmap->bmBits, lpBitmap, NULL, nColors);

  /*
    Now copy the bits. We need to do a huge copy.
  */
  if (lpInitBits != NULL)
  {
#if defined(STORE_IMAGE_ONCE)
    (void) lpDest;
    DDBtoSet = lpInitBits;
#else
    hmemcpy(lpDest, lpInitBits,
            dwBitmapSize - 
            (sizeof(IMGHEADER)+sizeof(BMPHEADER)+(nColors * sizeof(RGBQUAD))));
#endif
  }


#if defined(XWINDOWS)
  /*
    For X, calculate the size of lpInitBits before we unlock the memory
  */
  dwBitmapSize = (LONG) lpBitmap->bmWidth * (LONG) lpBitmap->bmHeight;
  GlobalUnlock(hMem);
  hMem = HBitmapToObject(0, hMem);
  if (lpInitBits)
    SetBitmapBits(hMem, dwBitmapSize, lpInitBits);
  return hMem;
#else
  GlobalUnlock(hMem);
  return HBitmapToObject(0, hMem);
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : CreateCompatibleBitmap(hDC, nWidth, nHeight)                  */
/*                                                                          */
/* Purpose  : Creates an "in-memory" bitmap which has the same format as    */
/*            the bitmap in the specified DC. We usually attach this to     */
/*            a memory DC which was created by CreateCompatibleDC() by      */
/*            using the SelectObject() function. The dimensions of this     */
/*            bitmap are specified by the nWidth and nHeight parameters.    */
/*                                                                          */
/* Returns  : A handle to the bitmap if successful, NULL if not.            */
/*                                                                          */
/****************************************************************************/
HBITMAP FAR PASCAL CreateCompatibleBitmap(hDC, nWidth, nHeight)
  HDC hDC;
  int nWidth, nHeight;
{
  LPHDC  lphDC;
  INT    nBits;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return NULL;
  nBits = (lphDC->hBitmap) ? lphDC->wBitsPerPixel : 1;
  return CreateBitmap(nWidth, nHeight, 1, nBits, NULL);
}


/****************************************************************************/
/*                                                                          */
/* Function : SetUpHeader()                                                 */
/*                                                                          */
/* Purpose  : Sets up the fields of a bitmap header.                        */
/*                                                                          */
/* Returns  : The address where the actual image data should go.            */
/*                                                                          */
/****************************************************************************/
static char HUGE *PASCAL SetUpHeader(lpBitmapBytes, lpbm, lpPalette, nRGB)
  char HUGE *lpBitmapBytes;
  LPBITMAP lpbm;
  RGBQUAD FAR *lpPalette;
  INT      nRGB;
{
  LPBMPHEADER lpBMPHeader;
  IMGHEADER   *pImg;

  /*
    We put a small header at the start of the bitmap bits. This header
    is needed by the BGI and MSC bitmap writing routines. The header
    tells the putimage() call about the dimensions of the image.
  */
  lpBMPHeader = (LPBMPHEADER) lpBitmapBytes;
  lpBMPHeader->wSignature      = SIG_DDB;
  lpBMPHeader->nPaletteEntries = nRGB;
  if (nRGB && lpPalette)
    lmemcpy(lpBitmapBytes+sizeof(BMPHEADER), lpPalette, sizeof(RGBQUAD)*nRGB);

  pImg = (IMGHEADER*)(lpBitmapBytes + sizeof(BMPHEADER) + nRGB*sizeof(RGBQUAD));

#if defined(META) || defined(METABGI)
  pImg->imWidth    = lpbm->bmWidth  - PUTIMAGE_OFFSET;
  pImg->imHeight   = lpbm->bmHeight - PUTIMAGE_OFFSET;
  pImg->imAlign    = 0;
  pImg->imFlags    = 0;
  pImg->imRowBytes = lpbm->bmWidthBytes;
  pImg->imBits     = lpbm->bmBitsPixel;
  pImg->imPlanes   = lpbm->bmPlanes;

  /*
    MetaWindows expects the imBits field to always contain 1, and the
    imPlanes field to contain the number of bit planes.
    Also, adjust the RowBytes field so that the number of planes is
    taken into consideration.
  */
  if (pImg->imPlanes == 1 && pImg->imBits == 4)
    pImg->imPlanes = 4, pImg->imBits = 1;
  pImg->imRowBytes /= pImg->imPlanes;

#if defined(METABGI)
  pImg->wWidth     = lpbm->bmWidth  - PUTIMAGE_OFFSET;
  pImg->wHeight    = lpbm->bmHeight - PUTIMAGE_OFFSET;
#endif


#elif defined(GX) || defined(GNUGRX) || defined(XWINDOWS)
  (void) pImg;
  (void) lpbm;

#else
  pImg->wWidth     = lpbm->bmWidth  - PUTIMAGE_OFFSET;
  pImg->wHeight    = lpbm->bmHeight - PUTIMAGE_OFFSET;
#endif

  lpBitmapBytes += sizeof(BMPHEADER) + nRGB*sizeof(RGBQUAD) + sizeof(IMGHEADER);
  return lpBitmapBytes;
}


/****************************************************************************/
/*                                                                          */
/* Function : HBitmapToObject(hDC, hBitmap)                                 */
/*                                                                          */
/* Purpose  : Creates a MEWEL GDI object from the bitmap.                   */
/*                                                                          */
/* Returns  : The handle of the bitmap object.                              */
/*                                                                          */
/****************************************************************************/
static HBITMAP PASCAL HBitmapToObject(hDC, hBitmap)
  HDC     hDC;
  HBITMAP hBitmap;
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  /*
    A bitmap is a GDI *OBJECT*, so we have to allocate object space
    for it. Return the handle of the GDI object.
  */
  if ((hObj = _ObjectAlloc(OBJ_BMP)) == (HANDLE) NULL ||
      (lpObj = _ObjectDeref(hObj)) == NULL)
    return NULL;
  lpObj->uObject.uhBitmap = hBitmap;


  /*
    For MetaWindows, Genux GX, and GRX, we need to allocate
    a graphics-engine-specific bitmap structure and fill it
    with the initial bits.
  */
#if defined(META) || defined(GX) || defined(GNUGRX) || defined(XWINDOWS)

#if   defined(META)
#define ENGINECREATEBITMAP  MetaCreateBitmap
#elif defined(GX)
#define ENGINECREATEBITMAP  GxCreateBitmap
#elif defined(GNUGRX)
#define ENGINECREATEBITMAP  GrxCreateBitmap
#elif defined(XWINDOWS)
#define ENGINECREATEBITMAP  XMEWELCreateBitmap
#endif

  {
  LPBITMAP   lpbm;
  char HUGE *lpBits;

  lpBits = HObjectToImageBits(hObj, &hBitmap, &lpbm, NULL);
#if defined(STORE_IMAGE_ONCE)
  lpBits = DDBtoSet;
#endif
  lpObj->lpExtra = (LPVOID) ENGINECREATEBITMAP(hDC,lpbm,lpBits);
  GlobalUnlock(hBitmap);
  }
#endif


  return hObj;
}


/****************************************************************************/
/*                                                                          */
/* Function : hmemcpy()                                                     */
/*                                                                          */
/* Purpose  : Like _fmemcpy, but works with huge pointers and >64K lengths. */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL hmemcpy(lpDest, lpSrc, dwBytes)
  VOID HUGE *lpDest;
  CONST VOID HUGE *lpSrc;
  LONG      dwBytes;
{
  UINT      wBytes;

  for (  ;  dwBytes > 0L;  dwBytes -= wBytes)
  {
    /*
      Do a memcpy for, at most, chunks of 32K bytes.
    */
    wBytes = (UINT) (min(dwBytes, 0x7FFF));
    lmemcpy(lpDest, lpSrc, wBytes);
    lpDest = (char HUGE *) lpDest + wBytes;
    lpSrc  = (char HUGE *) lpSrc + wBytes;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : HObjectToImageBits()                                          */
/*                                                                          */
/* Purpose  : Given a handle to a GDI object, returns a pointer to the      */
/*            actual image bits of a bitmap. Also returns a handle to       */
/*            the bitmap memory which the caller must eventually unlock.    */
/*                                                                          */
/* Returns  : A pointer to the actual image bits. Note : for GX and         */
/*            MetaWindows, the actual bits are pointed to in some way by    */
/*            the engine-specific structure pointed to by lpObj->lpExtra.   */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL HObjectToImageBits(hObj, phMemToUnlock, plpBitmap, pnColors)
  HANDLE   hObj;
  HANDLE   *phMemToUnlock;
  LPBITMAP *plpBitmap;
  INT      *pnColors;
{
  LPOBJECT lpObj;
  LPBITMAP lpbm;
  LPSTR    pImg;
  INT      nColors;
  HBITMAP  hBitmap;

  /*
    Get a pointer to the underlying BITMAP structure
  */
  lpObj = _ObjectDeref(hObj);
  if ((hBitmap = lpObj->uObject.uhBitmap) == NULL)
    return (LPSTR) (*phMemToUnlock = NULL);
  lpbm = (LPBITMAP) GlobalLock(hBitmap);

  /*
    Re-resolve the conv memory address of the bits in case we
    swapped out the bitmap.
  */
  lpbm->bmBits = ((LPSTR) &lpbm->bmBits) + 4;

  /*
    Get a pointer to the actual image bits.
  */
  nColors = ((LPBMPHEADER) lpbm->bmBits)->nPaletteEntries;
  pImg = (lpbm->bmBits + sizeof(BMPHEADER) + sizeof(IMGHEADER) +
          (nColors * sizeof(RGBQUAD)));

  /*
    Return the handle to the DDB bitmap data.
  */
  *phMemToUnlock = hBitmap;
  *plpBitmap     = lpbm;
  if (pnColors)
    *pnColors = nColors;
  return pImg;
}


/****************************************************************************/
/*                                                                          */
/* Function : RealizeBitmap(hDC, hBitmap)                                   */
/*                                                                          */
/* Purpose  : Internal function called by SelectObject(hDC, hBitmap).       */
/*            Does graphics-engine-specific stuff to attach the bitmap      */
/*            to the DC.                                                    */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL RealizeBitmap(hDC, hBitmap)
  HDC     hDC;
  HBITMAP hBitmap;
{
  LPHDC    lphDC = _GetDC(hDC);
  LPOBJECT lpObj;

#if defined(XWINDOWS)
  if (hBitmap != NULL)
#else
  if (hBitmap != NULL && hBitmap != SysGDIInfo.hDefMonoBitmap)
#endif
  {
    RECT   r;
    BITMAP bm;
     
    if ((lpObj = _ObjectDeref(hBitmap)) == NULL)
      return;

    lphDC->fFlags |= DC_BITMAPSELECTED;

    GetObject(hBitmap, sizeof(bm), &bm);
    SetRect(&r, 0, 0, bm.bmWidth, bm.bmHeight);

#if defined(META)
    /*
      Have the grafMap pointer in the DC point to the MetaWindows
      grafMap structure which is attached to the bitmap.
      Then, associate the bitmap with the port.
    */
    ((LPMETADCINFO) lphDC->lpDCExtra)->pGrafMap = lpObj->lpExtra;
    mwSetPort(((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort);
    mwPortBitmap((grafMap *) lpObj->lpExtra);
    mwPortSize(r.right, r.bottom);
    mwClipRect((rect *) &r);

#elif defined(GX)
    /*
      Associate the bitmap with the DC
    */
    ((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader = lpObj->lpExtra;
    /*
      Reset the clipping rectangle so that it is based at 0,0
    */
    lphDC->rClipping = r;
    lphDC->ptOrg.x = 0;
    lphDC->ptOrg.y = 0;
    if (lphDC->hRgnVis)
      LocalFree(lphDC->hRgnVis);
    lphDC->hRgnVis = WinCalcVisRgn(lphDC->hWnd, hDC);

#elif defined(GNUGRX)
    /*
      Associate the bitmap with the DC
    */
    ((LPGRXDCINFO) lphDC->lpDCExtra)->grContext = lpObj->lpExtra;
    /*
      Reset the clipping rectangle so that it is based at 0,0
    */
    lphDC->rClipping = r;
    lphDC->ptOrg.x = 0;
    lphDC->ptOrg.y = 0;
    if (lphDC->hRgnVis)
      LocalFree(lphDC->hRgnVis);
    lphDC->hRgnVis = WinCalcVisRgn(lphDC->hWnd, hDC);

#elif defined(XWINDOWS)
    /*
      Associate the bitmap with the DC
    */
    ((LPXDCINFO) lphDC->lpDCExtra)->pixmap = (Pixmap) lpObj->lpExtra;
    lphDC->drawable = (Pixmap) lpObj->lpExtra;

    /*
      We need to delete the X GC associated with the DC, and set up
      a new one based on the depth of the bitmap
    */
    if (lphDC->wBitsPerPixel != bm.bmBitsPixel)
    {
      XFreeGC(XSysParams.display, lphDC->gc);
      lphDC->gc = XCreateGC(XSysParams.display, lphDC->drawable, 0, NULL);
      lphDC->wBitsPerPixel = bm.bmBitsPixel;
    }

    /*
      Reset the clipping rectangle so that it is based at 0,0
    */
    lphDC->rClipping = r;
    lphDC->ptOrg.x = 0;
    lphDC->ptOrg.y = 0;

#else

    (void) lpObj;
#endif

  }
  else
  {
#if defined(META)
    mwClipRect(NULL);
#elif defined(GX)
    GXVirtualScreen(FALSE);
    ((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader = NULL;
#endif
    lphDC->fFlags &= ~DC_BITMAPSELECTED;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : EngineDeleteBitmap()                                          */
/*                                                                          */
/* Purpose  : Deletes a graphics-engine-specific bitmap. This function is   */
/*            called whenever DeleteObject(hBitmap) is called.              */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EngineDeleteBitmap(hObj)
  HANDLE hObj;
{
  LPOBJECT lpObj;

  /*
    Can't delete the stock mono bitmap.
  */
  if (hObj == SysGDIInfo.hDefMonoBitmap)
  {
#if defined(GX)
    GXVirtualScreen(FALSE);
#endif
    return;
  }

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return;

#if defined(META)
  /*
    The lpExtra field of the object points to a MetaWindows grafMap.
  */
  if (lpObj->lpExtra)
    mwCloseBitmap((grafMap *) lpObj->lpExtra);

#elif defined(GX)
  /*
    The lpExtra field of the object points to a GX virtual bitmap structure.
  */
  if (lpObj->lpExtra)
  {
    GXHEADER *gx = (GXHEADER *) lpObj->lpExtra;
    GXVirtualScreen(FALSE);

    /*
      Free the bitmap bits which was allocated in GxCreateBitmap()
    */
#if defined(MSC)
    hfree(gx->vptr);
#else
    MyFree_far(gx->vptr);
#endif
#if 50693
    /*
      5/6/93 (maa)
        It seems that gxDestroyVirtual calls gxFarFree to free up
        gx->vptr, and gxFarFree uses INT21/49H to free the memory.
        So, free the memory ourselves, and fool gxDestroyVirtual into
        thinking that there's no attached bitmap.
    */
    gx->vptr = NULL;
#endif

    gxDestroyVirtual(gx);
  }

#elif defined(GNUGRX)
  /*
    Free the bitmap bits which was allocated in GxCreateBitmap()
  */
  MyFree_far(((GrContext *) lpObj->lpExtra)->gc_baseaddr);
  GrDestroyContext((GrContext *) lpObj->lpExtra);

#elif defined(XWINDOWS)
  /*
    Free the pixmap which was allocated in XMEWELCreateBitmap()
  */
  if (lpObj->lpExtra)
  {
    XFreePixmap(XSysParams.display, (Pixmap) lpObj->lpExtra);
    lpObj->lpExtra = NULL;
  }

#else
  /*
    For other graphics engines, do nothing.
  */
  (void) lpObj;

#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : GxCreateBitmap()                                              */
/*                                                                          */
/* Purpose  : GX-specific routine to create a GX virtual bitmap from a      */
/*            MEWEL bitmap.                                                 */
/*                                                                          */
/* Returns  : The GXHEADER corresponding to the bitmap.                     */
/*                                                                          */
/* Called by: HObjectToBitmap()                                             */
/*                                                                          */
/****************************************************************************/
#ifdef GX
static GXHEADER *PASCAL GxCreateBitmap(hDC, lpbm, lpBits)
  HDC      hDC;
  LPBITMAP lpbm;
  char HUGE *lpBits;
{
  GXHEADER *pGXHeader;
  DWORD dwMemNeeded;
  INT   iDisplay;
  INT   iMemType;
  INT   nWidth, nHeight;
  INT   i;
  LPOBJECT lpObj;
  LPSTR lpNewBits;

  static int aiMemTypes[] =
  {
    gxCMM,
#if 0
    gxEMM,
    gxXMM,
#endif
#ifdef USE_GENUS_DMM
    gxDMM,
#endif
  };


  (void) hDC;

  nWidth  = lpbm->bmWidth;
  nHeight = lpbm->bmHeight;

  /*
    Get the display type and the amount of memory needed for the bitmap
  */
  iDisplay = gxGetDisplay();
  dwMemNeeded = gxVirtualSize(iDisplay, nWidth, nHeight);

  /*
    Determine which kind of memory we should use
  */
  for (i = 0;  i < sizeof(aiMemTypes)/sizeof(INT);  i++)
  {
    DWORD dwMemFree;

    iMemType = aiMemTypes[i];

    /*
      Determine if the type of memory is installed
    */
#ifdef USE_GENUS_DMM
    if (iMemType == gxDMM)
    {
      if (gxDMInstalled() != gxSUCCESS)
        gxInstallDM("", gxCMM);
    }
#endif

    /*
      See if enough of this kind of memory is available for the bitmap
    */
    dwMemFree = gxVirtualFree(iMemType);
    if (dwMemFree >= dwMemNeeded)
      break;
  }

  /*
    See if we found memory among the available types. If not, return
    failure.
  */
  if (i >= sizeof(aiMemTypes)/sizeof(INT))
    return NULL;

  /*
    Allocate a GXHEADER structure and stick it in the extra space
    in the OBJECT structure.
  */
  if ((pGXHeader = (GXHEADER *) EMALLOC_FAR(sizeof(GXHEADER))) == NULL)
    return NULL;

  /*
    Call the graphics engine to allocate the off-screen bitmap
  */

  dwMemNeeded = max(dwMemNeeded, ((DWORD)nHeight * (DWORD)lpbm->bmWidthBytes));
#if defined(MSC)
  lpNewBits = (LPSTR) halloc(dwMemNeeded, 1L);
#else
  lpNewBits = (LPSTR) emalloc_far(dwMemNeeded);
#endif
  if (lpNewBits && lpBits)
  {
    hmemcpy((char HUGE *) lpNewBits, lpBits, nHeight * lpbm->bmWidthBytes);
  }

  if (gxSetVirtualHeader(iMemType, pGXHeader, (long) lpNewBits, iDisplay,
                         nWidth, nHeight) != gxSUCCESS)
  {
#if defined(MSC)
    if (lpNewBits)
      hfree(lpNewBits);
#else
    MyFree_far(lpNewBits);
#endif
    MYFREE_FAR(pGXHeader);
    return NULL;
  }

  return pGXHeader;
}
#endif



/****************************************************************************/
/*                                                                          */
/* Function : MetaCreateBitmap()                                            */
/*                                                                          */
/* Purpose  : Meta-specific routine to create a Meta virtual bitmap from a  */
/*            MEWEL bitmap.                                                 */
/*                                                                          */
/* Returns  : The grafMap corresponding to the bitmap.                      */
/*                                                                          */
/* Called by: HObjectToBitmap()                                             */
/*                                                                          */
/****************************************************************************/
#ifdef META
static grafMap *PASCAL MetaCreateBitmap(hDC, lpbm, lpBits)
  HDC      hDC;
  LPBITMAP lpbm;
  char HUGE *lpBits;
{
  grafMap *gm;

  (void) hDC;

  /*
    Allocate a grafMap structure and fill it.
  */
  gm = (grafMap *) EMALLOC_FAR(sizeof(grafMap));
  gm->pixWidth  = lpbm->bmWidth;
  gm->pixHeight = lpbm->bmHeight;
  gm->pixResX   = 100;
  gm->pixResY   = 100;
  if (lpbm->bmBitsPixel == 4)
  {
    /*
      For 16-color bitmaps, MetaWindows needs the pixPlanes value to be 4
    */
    gm->pixBits   = lpbm->bmPlanes;
    gm->pixPlanes = lpbm->bmBitsPixel;
  }
  else
  {
    gm->pixBits   = lpbm->bmBitsPixel;
    gm->pixPlanes = lpbm->bmPlanes;
  }

  if (mwInitBitmap(cMEMORY, gm) != 0)
  {
    /* error... */
  }
  else
  {
    /*
      If there is image data in the bitmap, then copy it over to
      the grafMap.
    */
    int nRowBytes = gm->pixBytes;
    int row, iPlane;

    for (row = 0;  row < lpbm->bmHeight;  row++)
      for (iPlane = 0;  iPlane < gm->pixPlanes;  iPlane++)
      {
        unsigned char **pRowTbl;
        unsigned char *pRow;
        pRowTbl = gm->mapTable[iPlane];
#if defined(__DPMI32__)
        ((char *) pRowTbl) -= ((DWORD) gm->mapAltMgr);
        pRow = pRowTbl[row];
        ((char *) pRow) -= ((DWORD) gm->mapAltMgr);
#else
        pRow = pRowTbl[row];
#endif

        /*
          If we have no initial bits to set yet (or we're doing a DIB),
          then set this row to all black pixels.
        */
        if (lpBits == NULL)
          lmemset(pRow, 0, nRowBytes);
        else
        {
          lmemcpy(pRow, lpBits, nRowBytes);
          lpBits += nRowBytes;
        }
      }
  }

  return gm;
}
#endif /* META */



/****************************************************************************/
/*                                                                          */
/* Function : GrxCreateBitmap()                                             */
/*                                                                          */
/* Purpose  : Grx-specific routine to create a Grx virtual bitmap from a    */
/*            MEWEL bitmap.                                                 */
/*                                                                          */
/* Returns  : The GrContext corresponding to the bitmap.                    */
/*                                                                          */
/* Called by: HObjectToBitmap()                                             */
/*                                                                          */
/****************************************************************************/
#ifdef GNUGRX
static GrContext *PASCAL GrxCreateBitmap(hDC, lpbm, lpBits)
  HDC      hDC;
  LPBITMAP lpbm;
  char HUGE *lpBits;
{
  GrContext *pGrContext;
  DWORD dwMemNeeded;
  LPSTR lpNewBits;
  INT   nWidth, nHeight;


  (void) hDC;

  nWidth  = lpbm->bmWidth;
  nHeight = lpbm->bmHeight;

  /*
    Call the graphics engine to allocate the off-screen bitmap
  */
  dwMemNeeded = (DWORD) nHeight * (DWORD) lpbm->bmWidthBytes;
  lpNewBits = (LPSTR) emalloc_far(dwMemNeeded);
  if (lpNewBits && lpBits)
  {
    hmemcpy((char HUGE *) lpNewBits, lpBits, nHeight * lpbm->bmWidthBytes);
  }

  if ((pGrContext = GrCreateContext(nWidth,nHeight,lpNewBits,NULL)) == NULL)
  {
    MyFree_far(lpNewBits);
    MYFREE_FAR(pGrContext);
    return NULL;
  }

  return pGrContext;
}
#endif


#if defined(METABGI) && 0
/*
  Given an image, outputs it a single scan line at a time
*/
static VOID PASCAL METABGI_putimage(int x, int y, IMGHEADER *pImgSrc, int mode)
{
  IMGHEADER *pImgDest;
  char *pDataDest;
  char *pDataSrc;
  int  nRowBytes = pImgSrc->imRowBytes * 4;
  
  pImgDest = (IMGHEADER *) emalloc(sizeof(IMGHEADER) + nRowBytes);
  memcpy((char *) pImgDest, (char *) pImgSrc, sizeof(IMGHEADER));
  pImgDest->wHeight = pImgDest->imHeight = 1;

  pDataDest = (char *) (((LPSTR) pImgDest) + sizeof(IMGHEADER));
  pDataSrc  = (char *) (((LPSTR) pImgSrc ) + sizeof(IMGHEADER));

  for (y = 0;  y < pImgSrc->wHeight;  y++)
  {
    memcpy(pDataDest, pDataSrc, nRowBytes);
    _putimage(x, y, pImgDest, mode);
    pDataSrc += nRowBytes;
  }

  MyFree(pImgDest);
}
#endif


#if defined(META)
static VOID PASCAL XlateToMono(grafPort *colorPort,   /* source */
                               grafPort *monoPort,    /* dest   */
                               int      transColor)   /* transp color */
{
  image colorBuf[1300];  /* large enough for 1 line of color data */
  image monoBuf[256];    /* large enough for 1 line of mono data  */
  long  xlTable[256];    /* color translation table */
  rect  r;
  int   i;
  int   nHeight;
  grafPort *gpCurr;

  /*
    Save a pointer to the current grafPort
  */
  mwGetPort(&gpCurr);

  /*
    Set up the color translation table
  */
  memset((char *) xlTable, 0, sizeof(xlTable));
  xlTable[transColor] = -1;

  mwSetRect(&r, 0, 0, colorPort->portMap->pixWidth, 1);
  nHeight = colorPort->portMap->pixHeight;

  for (i = 0;  i < nHeight;  i++)
  {
    /*
      Read a scanline from the color bitmap
    */
    mwSetPort(colorPort);
    mwReadImage(&r, colorBuf);

    /*
      Convert the color scanline to a mono scanline
    */
    mwXlateImage(colorBuf, monoBuf, 1, 1, xlTable);

    /*
      Transfer the scanline to the mono bitmap
    */
    mwSetPort(monoPort);
    mwWriteImage(&r, monoBuf);

    /*
      Advance to the next row of the bitmap
    */
    r.Ymin++;
    r.Ymax++;
  }

  /*
    Restore the current port
  */
  mwSetPort(gpCurr);
}


#if defined(XLATE_IMAGE)
static image FAR *PASCAL MWXlateImg(pSrcImg)
  image far *pSrcImg;
{
  DWORD ulNeeded;
  INT   iBits, iPlanes;
  image FAR *pDestImg;

  iBits   = _MetaInfo.pOrigPort->portMap->pixBits;
  iPlanes = _MetaInfo.pOrigPort->portMap->pixPlanes;

  ulNeeded = mwXlateImage(pSrcImg, NULL, iBits, iPlanes, NULL);
  if ((pDestImg = (image FAR *) emalloc_far(ulNeeded)) != NULL)
    mwXlateImage(pSrcImg, pDestImg, iBits, iPlanes, NULL);
  return pDestImg;
} 
#endif

#endif

