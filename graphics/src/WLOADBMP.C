/*===========================================================================*/
/*                                                                           */
/* File    : WLOADBMP.C                                                      */
/*                                                                           */
/* Purpose : Implements the LoadBitmap() function.                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES

#ifdef NOGDI
#undef NOGDI
#endif

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif


HBITMAP FAR PASCAL LoadBitmap(hModule, lpszBmp)
  HINSTANCE hModule;
  LPCSTR  lpszBmp;
{
  HBITMAP hBitmap = NULL;
  HANDLE  hRes;
  LPSTR   lpMem;
  DWORD   dwBytes;
  LPBITMAPINFOHEADER lpbih;

  /*
    See if the resource exists.
  */
  if ((lpMem = LoadResourcePtr(hModule, lpszBmp, RT_BITMAP, &hRes)) == 0)
    return NULL;

  /*
    Get a pointer to the BITMAPINFOHEADER structure
  */
  swBitmapInfoHeader(lpbih = (LPBITMAPINFOHEADER) lpMem);

  /*
    Figure out the number of colors in the bitmap so that we can move
    [ast the RGBQUAD array and get a pointer directly to the bitmap image.
  */
  dwBytes = sizeof(BITMAPINFOHEADER);
  if (lpbih->biBitCount != 24)
    dwBytes += sizeof(RGBQUAD) << lpbih->biBitCount;

  /*
    Create a device-dependent bitmap
  */
  hBitmap =
  CreateDIBitmap((HDC) 0, 
                 lpbih, 
                 CBM_INIT,
                 (LPSTR) (lpMem + dwBytes),
                 (LPBITMAPINFO) lpMem,
                 DIB_RGB_COLORS);

  /*
    Unlock and free the bitmap data in the resource
  */
  UnloadResourcePtr(hModule, lpMem, hRes);
  return hBitmap;
}


