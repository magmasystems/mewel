/*===========================================================================*/
/*                                                                           */
/* File    : WVIEWEXT.C                                                      */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible mapping functions              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

DWORD FAR PASCAL GetViewportExt(hDC)
  HDC hDC;
{
  SIZE sz;
  GetViewportExtEx(hDC, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

DWORD FAR PASCAL SetViewportExt(hDC, cx, cy)
  HDC hDC;
  INT cx, cy;
{
  SIZE sz;
  SetViewportExtEx(hDC, cx, cy, &sz);
  return MAKELONG(sz.cx, sz.cy);
}

/*-------------------------------------------------------------------------*/

BOOL WINAPI GetViewportExtEx(hDC, lpSize)
  HDC   hDC;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extView;
  return TRUE;
}

BOOL WINAPI SetViewportExtEx(hDC, cx, cy, lpSize)
  HDC hDC;
  INT cx, cy;
  SIZE FAR *lpSize;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lpSize)
    *lpSize = lphDC->extView;
  lphDC->extView.cx = cx;
  lphDC->extView.cy = cy;
  return TRUE;
}

