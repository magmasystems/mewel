/*===========================================================================*/
/*                                                                           */
/* File    : GRAPHPCX.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989,1990 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

static int  PASCAL _PCXReadCache(int fd);

#define USE_PUTIMAGE

/****************************************************************************/
/*                                                                          */
/* Function :                                                               */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL DrawPCXFile(hDC, szFileName, x, y)
  HDC  hDC;
  char *szFileName;
  INT  x, y;
{
  PCXHEADER PCXHeader;

#if defined(META)
  grafPort  *currPort;
  grafMap   *currBitmap;
  RECT      r;
#endif

  int  fd;
  int  ch;
  char pFilename[65];
  BYTE *pLine = NULL, *pCurr;

  int  nTotalBytesPerLine;
  int  nBytesRead;
  int  nLinesRead = 0;
  BYTE ch12;
  UINT wPixel;
  BYTE *achPixels = NULL, *pPixel;
  IMGHEADER *pImg;
  POINT pt;

#ifdef USE_PUTIMAGE
  BOOL bCompatible = TRUE;  /* if bits and planes of PCX image is compatible
                               with _putimage() */
#else
  BOOL bCompatible = FALSE; /* if bits and planes of PCX image is compatible
                               with _putimage() */
#endif


  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return FALSE;

  /*
    Make sure that the file has a PCX extension
  */
  strcpy(pFilename, szFileName);
  if (strchr(pFilename, '.') == NULL)
    strcat(pFilename, ".pcx");

  /*
    Open the image file
  */
  if ((fd = open(pFilename, O_RDONLY | O_BINARY)) < 0)
    return FALSE;

  /*
    Read the PCX header
  */
  read(fd, (char *) &PCXHeader, sizeof(PCXHeader));

#if defined(META)
  {
  palData *RGBColors;
  long    ulSeekPos;

  /*
    Save a pointer to the current port. Also, check to see that the
    PCX image matches 
  */
  mwGetPort(&currPort);
  currBitmap = currPort->portMap;
  if (PCXHeader.bBitsPerPixel != currBitmap->pixBits ||
      PCXHeader.bNPlanes != currBitmap->pixPlanes)
    bCompatible = FALSE;

  if ((RGBColors = (palData FAR *) emalloc_far(256 * sizeof(palData))) == NULL)
    goto freeRGB;

  /*
    convert PCX digital palette data into MetaWINDOW compatible format
  */
  for (ch = 0;  ch < 16;  ch++)
  {
    int j = ch * 3;
    RGBColors[ch].palRed   = ((int) PCXHeader.rgbPalette[j])   * 256;
    RGBColors[ch].palGreen = ((int) PCXHeader.rgbPalette[j+1]) * 256;
    RGBColors[ch].palBlue  = ((int) PCXHeader.rgbPalette[j+2]) * 256;
  }

  /*
    Check for optional 256 color palette
  */
  ulSeekPos = tell(fd);
  lseek(fd, 0L, SEEK_END);   /* seek to 769 bytes from end of file */
  lseek(fd, tell(fd) - 769L, SEEK_SET); 
  /* should be a 12 there */
  read(fd, &ch12, sizeof(ch12));
  if (ch12 == 12)
  {
    BYTE rgb[3];
    for (ch = 0;  ch < 256;  ch++)
    {
      read(fd, rgb, 3);
      RGBColors[ch].palRed   *= rgb[0] * 256;
      RGBColors[ch].palGreen *= rgb[1] * 256;
      RGBColors[ch].palBlue  *= rgb[2] * 256;
    }
  }

#ifdef NOTYET
  /*
    Should we allow the system color palette to be messed up?
  */
  mwWritePalette(0, 0, mwQueryColors(), RGBColors);
#endif

  lseek(fd, ulSeekPos, 0);

freeRGB:
  if (RGBColors)
    MyFree_far(RGBColors);
  }

#else
  /*
    For a 16-color image, a PCX file usually has planes=4 and bitsperpix=1.
    So, test the bits-per-pixel against the device's PLANES cap (which is
    usually 1) and test the planes against the device's BITSPIXEL caps.
  */
  if (PCXHeader.bBitsPerPixel != GetDeviceCaps(hDC, PLANES) ||
      PCXHeader.bNPlanes != GetDeviceCaps(hDC, BITSPIXEL))
    bCompatible = FALSE;
#endif

  /*
    Determine the the number of bytes needed to represent one scan line.
  */
  nTotalBytesPerLine = PCXHeader.bNPlanes * PCXHeader.wBytesPerLine;

  /*
    Allocate a buffer for working with a single scan line.
  */
  if ((pLine = (BYTE*) emalloc(nTotalBytesPerLine*2 + sizeof(IMGHEADER))) == NULL)
    return FALSE;
  pCurr = pLine + sizeof(IMGHEADER);

  if (bCompatible)
  {
    pImg = (IMGHEADER *) pLine;
    /*
      We put a small header at the start of the bitmap bits. This header
      is needed by the BGI and MSC bitmap writing routines. The header
      tells the putimage() call about the dimensions of the image.
    */
#if defined(META)
     pImg->imWidth    = PCXHeader.rDimensions.right - PCXHeader.rDimensions.left;
     pImg->imHeight   = 1;
     pImg->imAlign    = 0;
     pImg->imRowBytes = PCXHeader.wBytesPerLine;
     pImg->imBits     = PCXHeader.bBitsPerPixel;
     pImg->imPlanes   = PCXHeader.bNPlanes;

#elif defined(GX)
    (void) pImg;

#elif defined(GNUGRX)
    (void) pImg;

#else
    pImg->wWidth  = PCXHeader.wBytesPerLine * 8 - PUTIMAGE_OFFSET;
    pImg->wHeight = 1;  /* height is 1 since we're only writing one scanline */
#endif
  }


  /*
    Set the viewport and transform the logical coordinates to device coords
  */
  if (!_GraphicsSetViewport(hDC))
    goto bye;
  pt.x = x;  pt.y = y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  x = pt.x;  y = pt.y;

  MOUSE_HideCursor();
  nBytesRead = 0;

  while ((ch = _PCXReadCache(fd)) >= 0)
  {
    /*
      If the high two bits are on, then the count is in the lower 6 bits.
      If not, then data is this one byte.
    */
    if ((ch & 0xC0) == 0xC0)
    {
      int n = (ch & 0x3F);
      nBytesRead += n;
      ch = (unsigned char) _PCXReadCache(fd);
      while (n-- > 0)
        *pCurr++ = (BYTE) ch;
    }
    else
    {
      *pCurr++ = (char) ch;
      nBytesRead++;
    }

    if (nBytesRead >= nTotalBytesPerLine)
    {
      UINT iLine;
      int  iPlane;
      int  iBit;

#ifdef USE_PUTIMAGE
      if (bCompatible)
      {
#if defined(META)
        SetRect(&r, x, nLinesRead+y, x+pImg->imWidth, nLinesRead+y+1);
        mwWriteImage((rect *) &r, pLine);
#else
        _putimage(x, nLinesRead + y, pLine, _GPSET);
#endif
        goto reset_ptr;
      }
#endif

      /*
        Dump the line
      */
      if (PCXHeader.bBitsPerPixel == 8)
      {
        int  y2 = y + nLinesRead;
        int  x2 = x;

        pCurr = pLine + sizeof(IMGHEADER);
        for (iLine = 0;  iLine < PCXHeader.wBytesPerLine;  iLine++, x2++)
        {
          BYTE ch = *pCurr++;
          SETPIXEL(hDC, x2, y2, ch);
        }
      }
      else if (PCXHeader.bBitsPerPixel == 1 && PCXHeader.bNPlanes == 1)
      {
        int  y2 = y + nLinesRead;
        int  x2 = x + iLine;

        pCurr = pLine + sizeof(IMGHEADER);
        for (iLine = 0;  iLine < PCXHeader.wBytesPerLine;  iLine++)
        {
          BYTE ch = *pCurr++;
          for (iBit = 7;  iBit >= 0;  iBit--, x2++)
          {
            INT  attr = ((ch & (1 << iBit)) != 0) ? 15 : 0;
            SETPIXEL(hDC, x2, y2, attr);
          }
        }
      }
      else if (PCXHeader.bNPlanes == 4 || PCXHeader.bBitsPerPixel == 4)
      {
        if (!achPixels)
          achPixels = emalloc(nTotalBytesPerLine*8);
        memset((char *) achPixels, 0, nTotalBytesPerLine*8);
        pPixel = achPixels;
        pCurr = pLine + sizeof(IMGHEADER);

        /*
          Combine the color scan for each pixel into a n-bit attribute.
        */
        for (iLine = 0;  iLine < PCXHeader.wBytesPerLine;  iLine++)
        {
          for (iBit = 7;  iBit >= 0;  iBit--)
          {
            for (wPixel = iPlane = 0;  iPlane < 4;  iPlane++)
              wPixel |= ((pCurr[iPlane*PCXHeader.wBytesPerLine + iLine] &
                         (1<<iBit)) >> iBit) << iPlane;
            *pPixel++ = (BYTE) wPixel;
          }
        }

        /*
          Output the scan line. For each pixel in the scan line, render
          the pixel in the appropriate color.
        */
        for (iLine = 0;  iLine < PCXHeader.wBytesPerLine*8;  iLine++)
        {
          BYTE ch = achPixels[iLine];
  #if 1
          SETPIXEL(hDC, iLine + x, nLinesRead + y, ch);
  #else
          COLORREF rgb;
          RGBTRIPLE *pTriple = &PCXHeader.rgbPalette[ch];
  #if 1
          rgb = RGB(pTriple->rgbtRed, pTriple->rgbtGreen, pTriple->rgbtBlue);
  #else
          rgb = AttrToRGB(ch);
  #endif
          SetPixel(hDC, iLine + x, nLinesRead + y, rgb);
  #endif
        }
      }



      /*
        Reset the work-line's pointers
      */
reset_ptr:
      pCurr = pLine + sizeof(IMGHEADER);
      nBytesRead = 0;

      /*
        Done reading?
      */
      if (++nLinesRead >= RECT_HEIGHT(PCXHeader.rDimensions))
        break;
    }
  }


  /*
    Close the image file and free the cache.
  */
bye:
  MOUSE_ShowCursor();
  if (achPixels)
    MyFree(achPixels);
  if (pLine)
    MyFree(pLine);
  close(fd);
  _PCXReadCache(-1);
#if defined(META)
  mwSetPort(currPort);   /* restore original port */
#endif
  return TRUE;
}


#define CACHESIZE  4096

static int PASCAL _PCXReadCache(int fd)
{
  static unsigned char *pCache = NULL;
  static unsigned char *pCurr;
  static BOOL bFirstTime = 1;
  static int  nBytesInCache = 0;

  if (bFirstTime)
  {
    pCache = emalloc(CACHESIZE);
    bFirstTime = 0;
    nBytesInCache = 0;
  }

  if (fd == -1)
  {
    if (pCache)
      MyFree(pCache);
    bFirstTime = 1;
    return -1;
  }

  if (nBytesInCache == 0)
    if ((nBytesInCache = read(fd, pCurr = pCache, CACHESIZE)) <= 0)
      return -1;

  nBytesInCache--;
  return *pCurr++;
}

