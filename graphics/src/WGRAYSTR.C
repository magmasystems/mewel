/*===========================================================================*/
/*                                                                           */
/* File    : WGRAYSTR.C                                                      */
/*                                                                           */
/* Purpose : Implements the GrayString() function...                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

typedef BOOL FAR PASCAL GRAYPROC(HDC, DWORD, int);


BOOL FAR PASCAL GrayString(hDC, hBrush, lpOutputFunc, lpData, nCount,
                           X, Y, nWidth, nHeight)
  HDC     hDC;
  HBRUSH  hBrush;
  FARPROC lpOutputFunc;
  DWORD   lpData;
  int     nCount;
  int     X, Y;
  int     nWidth, nHeight;
{
  COLORREF oldClr;
  LOGBRUSH lb;

  (void) nWidth;  (void) nHeight;

  GetObject(hBrush, sizeof(lb), (LPSTR) &lb);
  oldClr = SetTextColor(hDC, lb.lbColor);
  if (nCount <= 0) 
    nCount = lstrlen((LPSTR) lpData);

  if (lpOutputFunc)
    (* (GRAYPROC *) lpOutputFunc)(hDC, lpData, nCount);
  else
    TextOut(hDC, X, Y, (LPSTR) lpData, nCount);

  SetTextColor(hDC, oldClr);
  return TRUE;
}

