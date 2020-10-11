/*===========================================================================*/
/*                                                                           */
/* File    : WWINEXT.C                                                       */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible mapping functions              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

DWORD FAR PASCAL GetWindowExt(hDC)
  HDC hDC;
{
  SIZE sz;
  GetWindowExtEx(hDC, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

DWORD FAR PASCAL SetWindowExt(hDC, cx, cy)
  HDC hDC;
  INT cx, cy;
{
  SIZE sz;
  SetWindowExtEx(hDC, cx, cy, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

/*-------------------------------------------------------------------------*/

BOOL WINAPI GetWindowExtEx(hDC, lpSize)
  HDC   hDC;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extWindow;
  return TRUE;
}

BOOL WINAPI SetWindowExtEx(hDC, cx, cy, lpSize)
  HDC hDC;
  INT cx, cy;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extWindow;
  lphDC->extWindow.cx = cx;
  lphDC->extWindow.cy = cy;
  return TRUE;
}

