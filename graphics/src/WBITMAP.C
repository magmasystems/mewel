/*===========================================================================*/
/*                                                                           */
/* File    : WBITMAP.C                                                       */
/*                                                                           */
/* Purpose : Stubs for the Windows bitmap functions                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI)
#include "wgraphic.h"
#endif

/*
  For Borland C++, get rid of 'parameter never used' warnings
*/
#if defined(__TURBOC__)
#pragma warn -par
#endif


INT FAR PASCAL SetStretchBltMode(hDC, iMode)
  HDC hDC;
  int iMode;
{
  LPHDC lphDC;
  INT   oldMode;

  if ((lphDC = _GetDC(hDC)) != NULL)
  {
    oldMode = lphDC->wStretchMode;
    lphDC->wStretchMode = iMode;
    return oldMode;
  }
  else
    return 0;
}

INT FAR PASCAL GetStretchBltMode(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) != NULL)
    return lphDC->wStretchMode;
  else
    return 0;
}


#if !defined(MEWEL_GUI) && !defined(XWINDOWS)

INT FAR PASCAL SetDIBitsToDevice(hDC, x, y, nWidth, nHeight, 
                                 xSrc, ySrc, nStartScan, nNumScans,
                                 lpBits, pbmi, wMode)
  HDC  hDC;
  INT  x, y, nWidth, nHeight;
  INT  xSrc, ySrc;
  UINT nStartScan, nNumScans;
  VOID FAR *lpBits;
  BITMAPINFO FAR* pbmi;
  UINT wMode;
{
  return 0;
}


INT FAR PASCAL StretchDIBits(hDC, X, Y, nWidth, nHeight,
                                  XSrc, YSrc, nSrcWidth, nSrcHeight,
                                  lpvBits, lpBMI, fuColorUse, dwRop)
  HDC hDC;
  INT  X, Y;
  INT  nWidth, nHeight;
  INT  XSrc, YSrc;
  INT  nSrcWidth, nSrcHeight;
  CONST VOID FAR *lpvBits;
  LPBITMAPINFO lpBMI;
  UINT  fuColorUse;
  DWORD dwRop;
{
  return 0;
}


INT FAR PASCAL GetDIBits(hDC, hBitmap, nStartScan, nScanLines, lpvBits, 
                         lpBMI, fuColorUse)
  HDC  hDC;
  HBITMAP hBitmap;
  UINT nStartScan;
  UINT nScanLines;
  VOID FAR *lpvBits;
  BITMAPINFO FAR *lpBMI;
  UINT fuColorUse;
{
  (void) hDC;
  (void) hBitmap;
  (void) nStartScan;
  (void) nScanLines;
  (void) lpvBits;
  (void) lpBMI;
  (void) fuColorUse;

  return 0;
}


LONG FAR PASCAL GetBitmapBits(hBitmap, dwCount, lpBits)
  HBITMAP hBitmap;
  LONG    dwCount;
  VOID FAR *lpBits;
{
  (void) hBitmap;
  (void) dwCount;
  (void) lpBits;

  return 0L;
}

LONG FAR PASCAL SetBitmapBits(hBitmap, dwCount, lpBits)
  HBITMAP hBitmap;
  DWORD   dwCount;
  CONST VOID FAR *lpBits;
{
  (void) hBitmap;
  (void) dwCount;
  (void) lpBits;

  return 0L;
}


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
  (void) nWidth;  (void) nHeight;
  return BitBlt(hDestDC, X,Y, nSrcWidth,nSrcHeight, hSrcDC, XSrc,YSrc, dwRop);
}


/****************************************************************************/
/*                                                                          */
/*                    BITMAP CREATION ROUTINES                              */
/*                                                                          */
/****************************************************************************/
HBITMAP FAR PASCAL CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBits)
  int  nWidth;
  int  nHeight;
  UINT nPlanes;
  UINT nBitCount;
  CONST VOID FAR *lpBits;
{
  return (HBITMAP) 0;
}

HBITMAP FAR PASCAL CreateBitmapIndirect(lpBitmap)
  BITMAP FAR *lpBitmap;
{
  return (HBITMAP) 0;
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
  return (HBITMAP) 0;
}

HBITMAP FAR PASCAL CreateDIBitmap(hDC, lpInfoHeader, dwUsage, lpInitBits,
                                  lpInitInfo, wUsage)
  HDC hDC;
  LPBITMAPINFOHEADER lpInfoHeader;
  DWORD dwUsage;
  CONST VOID FAR *lpInitBits;
  LPBITMAPINFO lpInitInfo;
  UINT wUsage;
{
  return (HBITMAP) 0;
}

#endif /* MEWEL_GUI */

