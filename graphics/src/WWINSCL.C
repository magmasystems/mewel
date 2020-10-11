/*===========================================================================*/
/*                                                                           */
/* File    : WWINSCL.C                                                       */
/*                                                                           */
/* Purpose : Implements the ScaleWindowExt(Ex) functions.                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


DWORD WINAPI ScaleWindowExt(hDC, nXNum, nXDenom, nYNum, nYDenom)
  HDC hDC;
  INT nXNum, nXDenom, nYNum, nYDenom;
{
  SIZE sz;
  ScaleWindowExtEx(hDC, nXNum, nXDenom, nYNum, nYDenom, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

BOOL WINAPI ScaleWindowExtEx(hDC, nXNum, nXDenom, nYNum, nYDenom, lpSize)
  HDC hDC;
  INT nXNum, nXDenom, nYNum, nYDenom;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extWindow;
  lphDC->extWindow.cx = (int) ((long) lphDC->extWindow.cx * (long) nXNum / 
                                                            (long) nXDenom);
  lphDC->extWindow.cy = (int) ((long) lphDC->extWindow.cy * (long) nYNum /
                                                            (long) nYDenom);
  return TRUE;
}

