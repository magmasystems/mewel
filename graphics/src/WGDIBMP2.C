/*===========================================================================*/
/*                                                                           */
/* File    : WGDIBMP2.C                                                      */
/*                                                                           */
/* Purpose : Bitmap functions for MEWEL/GUI                                  */
/*           Most of these are stubs for little-used bitmap functions.       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

HBRUSH WINAPI CreateDIBPatternBrush(HGLOBAL hbm, UINT iClr)
{
  (void) iClr;
  return CreatePatternBrush(hbm);
}

HBITMAP WINAPI CreateDiscardableBitmap(HDC hDC, int cx, int cy)
{
  return CreateCompatibleBitmap(hDC, cx, cy);
}

DWORD WINAPI GetBitmapDimension(HBITMAP hBMP)
{
  SIZE sz;
  GetBitmapDimensionEx(hBMP, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

BOOL WINAPI GetBitmapDimensionEx(HBITMAP hBMP, SIZE FAR *lpSize)
{
  (void) hBMP;
  if (lpSize)
    lpSize->cx = lpSize->cy = 0;
  return TRUE;
}

DWORD WINAPI SetBitmapDimension(HBITMAP hBMP, int cx, int cy)
{
  SIZE sz;
  SetBitmapDimensionEx(hBMP, cx, cy, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

BOOL WINAPI SetBitmapDimensionEx(HBITMAP hBMP, int cx, int cy, SIZE FAR *lpSize)
{
  (void) hBMP;
  (void) cx;
  (void) cy;

  if (lpSize)
    lpSize->cx = lpSize->cy = 0;
  return TRUE;
}

