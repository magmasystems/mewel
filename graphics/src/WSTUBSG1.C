/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSG1.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           These are for GDI font functions.                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


BOOL WINAPI CreateScalableFontResource(UINT fHidden, LPCSTR lpszRes, 
/* GDI.310 */                          LPCSTR lpszFont, LPCSTR lpszPath)
{
  (void) fHidden;  (void) lpszRes;  (void) lpszFont;  (void) lpszPath;
  return FALSE;
}

int WINAPI EnumFontFamilies(HDC hDC, LPCSTR lpszFamily, FONTENUMPROC lpfn, LPSTR lParam)
/* GDI.330 */
{
  (void) hDC;  (void) lpszFamily;  (void) lpfn;  (void) lParam;
  return 0;
}

DWORD WINAPI GetAspectRatioFilter(HDC hDC)  /* GDI.353 */
{
  SIZE sz;
  GetAspectRatioFilterEx(hDC, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

BOOL WINAPI GetAspectRatioFilterEx(HDC hDC, SIZE FAR *lpSZ)  /* GDI.486 */
{
  (void) hDC;
  lpSZ->cx = 1;
  lpSZ->cy = 1;
  return TRUE;
}

BOOL WINAPI GetCharABCWidths(HDC hDC, UINT chFirst, UINT chLast, ABC FAR* lpabc)
/* GDI.307 */
{
  (void) hDC;  (void) chFirst;  (void) chLast;  (void) lpabc;
  return 0;
}

DWORD WINAPI GetFontData(HDC hDC, DWORD dwTable, DWORD dwOffset, 
                         VOID FAR* lpvBuf, DWORD cbData)  /* GDI.311 */
{
  (void) hDC;  (void) dwTable;  (void) dwOffset;  (void) lpvBuf;  (void) cbData;
  return 0L;
}

DWORD WINAPI GetGlyphOutline(HDC hDC, UINT uChar, UINT fuFormat, 
                             GLYPHMETRICS FAR* lpgm, DWORD cbBuffer,
                             VOID FAR *lpBuffer, CONST MAT2 FAR* lpMat2)
/* GDI.309 */
{
  (void) hDC;  (void) uChar;  (void) fuFormat;  (void) lpgm;  (void) cbBuffer;
  (void) lpBuffer;  (void) lpMat2;
  return 0L;
}

int WINAPI GetKerningPairs(HDC hDC, int nPairs, KERNINGPAIR FAR* lpkp)
/* GDI.332 */
{
  (void) hDC;  (void) nPairs;  (void) lpkp;
  return 0;
}

UINT WINAPI GetOutlineTextMetrics(HDC hDC, UINT cbData, 
/* GDI.308 */                     OUTLINETEXTMETRIC FAR* lpotm)
{
  (void) hDC;  (void) cbData;  (void) lpotm;
  return FALSE;
}

BOOL WINAPI GetRasterizerCaps(RASTERIZER_STATUS FAR* lprs, int cb)
/* GDI.313 */
{
  (void) lprs;  (void) cb;
  return FALSE;
}

