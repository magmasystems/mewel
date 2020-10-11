/*===========================================================================*/
/*                                                                           */
/* File    : WCREICON.C                                                      */
/*                                                                           */
/* Purpose : Implements the CreateIcon() function.                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#define INCLUDE_SYSICONS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

HICON WINAPI CreateIcon(HINSTANCE hInst, int nWidth, int nHeight,
                        BYTE bPlanes, BYTE bBitsPixel, 
                        CONST VOID FAR *lpvANDbits, CONST VOID FAR *lpvXORbits)
{
  HICON      hIcon;
  HBITMAP    hXorBitmap, hAndBitmap;
  LPICONINFO lpII;

  (void) hInst;


  /*
    Create the XOR bitmap
  */
  if ((hXorBitmap=CreateBitmap(nWidth,nHeight,bPlanes,bBitsPixel,lpvXORbits))==NULL)
    return NULL;

  /*
    Create the monochrome AND mask
  */
  hAndBitmap = CreateBitmap(nWidth, nHeight, 1, 1, lpvANDbits);


  /*
    Create an icon-info structure and store the handles to the image
    bitmap and the AND bitmap.
  */
  if ((hIcon = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(ICONINFO))) != NULL)
  {
    lpII = (LPICONINFO) GlobalLock(hIcon);
    lpII->hBitmap    = hXorBitmap;
    lpII->hAndBitmap = hAndBitmap;
    GlobalUnlock(hIcon);
  }
  else
  {
    DeleteObject(hXorBitmap);
    DeleteObject(hAndBitmap);
  }

  return hIcon;
}

