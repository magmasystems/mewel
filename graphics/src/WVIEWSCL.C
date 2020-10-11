/*===========================================================================*/
/*                                                                           */
/* File    : WVIEWSCL.C                                                      */
/*                                                                           */
/* Purpose : Implements the ScaleViewportExt(Ex) functions.                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


DWORD WINAPI ScaleViewportExt(hDC, nXNum, nXDenom, nYNum, nYDenom)
  HDC hDC;
  INT nXNum, nXDenom, nYNum, nYDenom;
{
  SIZE sz;
  ScaleViewportExtEx(hDC, nXNum, nXDenom, nYNum, nYDenom, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

BOOL WINAPI ScaleViewportExtEx(hDC, nXNum, nXDenom, nYNum, nYDenom, lpSize)
  HDC hDC;
  INT nXNum, nXDenom, nYNum, nYDenom;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extView;
  lphDC->extView.cx = (int) ((long) lphDC->extView.cx * (long) nXNum / 
                                                 (long) nXDenom);
  lphDC->extView.cy = (int) ((long) lphDC->extView.cy * (long) nYNum /
                                                 (long) nYDenom);
  return TRUE;
}

