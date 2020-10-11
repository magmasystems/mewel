/*===========================================================================*/
/*                                                                           */
/* File    : WGDIGLUE.C                                                      */
/*                                                                           */
/* Purpose : Some higher-level GDI functions for MEWEL and Windows           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "windows.h"
#if defined(MEWEL) && defined(USE_GDIWRAPPER)
#include "wgraphic.h"
#endif


#ifdef MEWEL

#if defined(USE_GDIWRAPPER)
#if defined(GX)
#define SETPIXEL(x,y,attr)  grPutPixel(x, y, attr)
#elif defined(MSC)
#define SETPIXEL(x,y,attr)  _setcolor(attr), _setpixel(x,y)
#elif defined(__TURBOC__)
#define SETPIXEL(x,y,attr)  putpixel(x, y, attr)
#endif
#else
#define SETPIXEL(x,y,attr)  
#endif


VOID FAR PASCAL DrawDIBitmap(HDC hDC, HBITMAP hBitmap, int x, int y)
{
  BITMAP  bm;
  POINT   ptSize, ptOrg;
  int     row, col, i, iBit;
  int     nTotalBytesPerLine = 0;
  LPSTR   lpBits;
  BYTE    chBit;
  extern VOID  PASCAL _GraphicsSetViewport(HDC hDC);


  /*
    Set the clipping area
  */
#if defined(USE_GDIWRAPPER)
  if (VID_IN_GRAPHICS_MODE())
    _GraphicsSetViewport(hDC);
#endif

  /*
    Make the bitmap the display surface of the memory DC
  */
  SelectObject(hDC, hBitmap);

  /*
    Get the dimensions of the bitmap
  */
  GetObject(hBitmap, sizeof(bm), (LPSTR) &bm);

  /*
    Translate device coords to logical coords
  */
  ptSize.x = bm.bmWidth;
  ptSize.y = bm.bmHeight;
  DPtoLP(hDC, &ptSize, 1);
  ptOrg.x = 0;
  ptOrg.y = 0;
  DPtoLP(hDC, &ptOrg, 1);

  x -= ptOrg.x;
  y -= ptOrg.y;

  /*
    Figure out how many pixels one byte translates into
  */
  switch (bm.bmBitsPixel)
  {
    case 1 : /* 1 byte = 8 pixels */
      nTotalBytesPerLine = (int) (bm.bmWidth >> 3);
      break;
    case 2 : /* 1 byte = 4 pixels */
    case 4 : /* 1 byte = 2 pixels */
      nTotalBytesPerLine = (int) (bm.bmWidth >> 1);
      break;
    case 8 : /* 1 byte = 1 pixels */
      nTotalBytesPerLine = (int) (bm.bmWidth);
      break;
    default :
      return;
  }

  /*
    Transfer the bitmap from the memory DC to the window
    For this function, let's assume that the user defined a bitmap
    in the "correct order", unlike the representation of the bitmap
    image in Windows' BMP files.
  */
  lpBits = bm.bmBits;

  if (!VID_IN_GRAPHICS_MODE())
  {
    RECT rect;
    SetRect((LPRECT) &rect, x/FONT_WIDTH, y/VideoInfo.yFontHeight,
            bm.bmWidth/FONT_WIDTH, bm.bmHeight/VideoInfo.yFontHeight);
    FillRect(hDC, (LPRECT) &rect, GetStockObject(BLACK_BRUSH));
    if (bm.bmWidth / FONT_WIDTH > 6)
      TextOut(hDC, rect.left+1, rect.top+1, "Bitmap", 6);
    return;
  }

  for (row = 0;  row < (int) bm.bmHeight;  row++, y++)
  {
    for (col = i = 0;  i < nTotalBytesPerLine;  i++)
    {
      chBit = *lpBits++;
      switch (bm.bmBitsPixel)
      {
        case 1 :
          for (iBit = 0;  iBit < 8;  iBit++)
            SETPIXEL((col++)+x, y, 
                     ((chBit & (0x80 >> iBit)) != 0) ? 0 : 0x0F);
          break;
  
        case 4 :
          SETPIXEL((col++)+x, y, (chBit >> 4) & 0x0F);
          SETPIXEL((col++)+x, y, (chBit)      & 0x0F);
          break;
  
        case 8 :
          SETPIXEL((col++)+x, y, chBit);
          break;
      }
    } /* for i */
  }
}

#else


VOID FAR PASCAL DrawDIBitmap(HDC hDC, HBITMAP hBitmap, int x, int y)
{
  HDC     hMemDC = (HDC) 0;
  BITMAP  bm;
  POINT   ptSize, ptOrg;


  if ((hMemDC = CreateCompatibleDC(hDC)) == NULL)
    return;

  /*
    Make the bitmap the display surface of the memory DC
  */
  SelectObject(hMemDC, hBitmap);
  SetMapMode(hMemDC, GetMapMode(hDC));

  /*
    Get the dimensions of the bitmap
  */
  GetObject(hBitmap, sizeof(bm), (LPSTR) &bm);

  /*
    Translate device coords to logical coords
  */
  ptSize.x = bm.bmWidth;
  ptSize.y = bm.bmHeight;
  DPtoLP(hDC, &ptSize, 1);
  ptOrg.x = ptOrg.y = 0;
  DPtoLP(hDC, &ptOrg, 1);

  /*
    Transfer the bitmap from the memory DC to the window
  */
  BitBlt(hDC, x, y, ptSize.x, ptSize.y, hMemDC, ptOrg.x, ptOrg.y, SRCCOPY);

  DeleteDC(hMemDC);
}

#endif



#if !defined(USE_GDIWRAPPER)
int PASCAL WinOpenGraphics(iMode)
  int iMode;
{
  return TRUE;
}
int PASCAL WinCloseGraphics()
{
  return TRUE;
}
LONG FAR PASCAL GetClientPixelCursorPos(hWnd, lParam)
  HWND hWnd;
  LONG lParam;
{
  return lParam;
}
VOID PASCAL _GraphicsSetViewport(hDC)
  HDC hDC;
{
}
#endif
