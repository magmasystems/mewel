/*===========================================================================*/
/*                                                                           */
/* File    : WWINORG.C                                                       */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible mapping functions              */
/*           SetWindowOrg, GetWindowOrg, OffsetWindowOrg and the ext funcs   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


DWORD FAR PASCAL GetWindowOrg(hDC)
  HDC hDC;
{
  POINT pt;
  GetWindowOrgEx(hDC, &pt);
  return MAKELONG(pt.x, pt.y);
}

DWORD FAR PASCAL SetWindowOrg(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;
  SetWindowOrgEx(hDC, x, y, &pt);
  return MAKELONG(pt.x, pt.y);
}

DWORD FAR PASCAL OffsetWindowOrg(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;
  OffsetWindowOrgEx(hDC, x, y, &pt);
  return MAKELONG(pt.x, pt.y);
}

/*-------------------------------------------------------------------------*/

BOOL WINAPI GetWindowOrgEx(hDC, lppt)
  HDC   hDC;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lppt)
    *lppt = lphDC->ptWindowOrg;
  return TRUE;
}

BOOL WINAPI SetWindowOrgEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  if (lppt)
    *lppt = lphDC->ptWindowOrg;

  lphDC->ptWindowOrg.x = x;
  lphDC->ptWindowOrg.y = y;

  return TRUE;
}

BOOL WINAPI OffsetWindowOrgEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  if (lppt)
    *lppt = lphDC->ptWindowOrg;

  lphDC->ptWindowOrg.x += x;
  lphDC->ptWindowOrg.y += y;

  return TRUE;
}

