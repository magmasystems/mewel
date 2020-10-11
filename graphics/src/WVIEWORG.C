/*===========================================================================*/
/*                                                                           */
/* File    : WVIEWORG.C                                                      */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible mapping functions              */
/*           SetViewportOrg, GetViewportOrg, OffsetViewportOrg               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


DWORD FAR PASCAL GetViewportOrg(hDC)
  HDC hDC;
{
  POINT pt;
  GetViewportOrgEx(hDC, &pt);
  return MAKELONG(pt.x, pt.y);
}

DWORD FAR PASCAL SetViewportOrg(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;
  SetViewportOrgEx(hDC, x, y, &pt);
  return MAKELONG(pt.x, pt.y);
}

DWORD FAR PASCAL OffsetViewportOrg(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;
  OffsetViewportOrgEx(hDC, x, y, &pt);
  return MAKELONG(pt.x, pt.y);
}

/*-------------------------------------------------------------------------*/

BOOL WINAPI GetViewportOrgEx(hDC, lppt)
  HDC   hDC;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if (lppt)
    *lppt = lphDC->ptViewOrg;
  return TRUE;
}

BOOL WINAPI SetViewportOrgEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  if (lppt)
    *lppt = lphDC->ptViewOrg;

  lphDC->ptViewOrg.x = x;
  lphDC->ptViewOrg.y = y;

  return TRUE;
}

BOOL WINAPI OffsetViewportOrgEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  if (lppt)
    *lppt = lphDC->ptViewOrg;

  lphDC->ptViewOrg.x += x;
  lphDC->ptViewOrg.y += y;

  return TRUE;
}

