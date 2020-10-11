/*===========================================================================*/
/*                                                                           */
/* File    : WGDIBRUSH.C                                                     */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible brush functions                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


DWORD WINAPI GetBrushOrg(HDC hDC)
{
  POINT pt;
  GetBrushOrgEx(hDC, &pt);
  return MAKELONG(pt.x, pt.y);
}

BOOL WINAPI GetBrushOrgEx(HDC hDC, POINT FAR *lppt)
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lppt)
    *lppt = lphDC->ptBrushOrg;
  return TRUE;
}

DWORD WINAPI SetBrushOrg(HDC hDC, int x, int y)
{
  LPHDC lphDC;
  POINT oldpt;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  oldpt = lphDC->ptBrushOrg;
  lphDC->ptBrushOrg.x = x;
  lphDC->ptBrushOrg.y = y;
  return MAKELONG(oldpt.x, oldpt.y);
}

