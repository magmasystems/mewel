/*===========================================================================*/
/*                                                                           */
/* File    : WLPTODP.C                                                       */
/*                                                                           */
/* Purpose : Impelements the LPtoDP and DPtoLP functions                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


BOOL FAR PASCAL LPtoDP(hDC, lpPt, nCount)
  HDC     hDC;
  LPPOINT lpPt;
  int     nCount;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (lphDC->wMappingMode == MM_TEXT)
  {
    while (nCount-- > 0)
    {
      lpPt->x = (lpPt->x - lphDC->ptWindowOrg.x + lphDC->ptViewOrg.x);
      lpPt->y = (lpPt->y - lphDC->ptWindowOrg.y + lphDC->ptViewOrg.y);
      lpPt++;
    }
  }
  else
  {
    while (nCount-- > 0)
    {
      lpPt->x = (int)
                (((long) (lpPt->x - lphDC->ptWindowOrg.x)) * (long) lphDC->extView.cx /
                 (long) lphDC->extWindow.cx + lphDC->ptViewOrg.x);
      lpPt->y = (int) 
                (((long) (lpPt->y - lphDC->ptWindowOrg.y)) * (long) lphDC->extView.cy /
                 (long) lphDC->extWindow.cy + lphDC->ptViewOrg.y);
      lpPt++;
    }
  }

  return TRUE;
}

BOOL FAR PASCAL LPtoSP(hDC, lpPt, nCount)
  HDC     hDC;
  LPPOINT lpPt;
  int     nCount;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (lphDC->wMappingMode == MM_TEXT)
  {
    while (nCount-- > 0)
    {
      lpPt->x = (lpPt->x - lphDC->ptWindowOrg.x + lphDC->ptViewOrg.x)
                 + lphDC->ptOrg.x;
      lpPt->y = (lpPt->y - lphDC->ptWindowOrg.y + lphDC->ptViewOrg.y)
                 + lphDC->ptOrg.y;
      lpPt++;
    }
  }
  else
  {
    while (nCount-- > 0)
    {
      lpPt->x = (int)
                (((long) (lpPt->x - lphDC->ptWindowOrg.x)) * (long) lphDC->extView.cx /
                 (long) lphDC->extWindow.cx + lphDC->ptViewOrg.x) +
                 lphDC->ptOrg.x;
      lpPt->y = (int) 
                (((long) (lpPt->y - lphDC->ptWindowOrg.y)) * (long) lphDC->extView.cy /
                 (long) lphDC->extWindow.cy + lphDC->ptViewOrg.y) +
                 lphDC->ptOrg.y;
      lpPt++;
    }
  }

  return TRUE;
}



BOOL FAR PASCAL DPtoLP(hDC, lpPt, nCount)
  HDC     hDC;
  LPPOINT lpPt;
  int     nCount;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (lphDC->wMappingMode == MM_TEXT)
  {
    while (nCount-- > 0)
    {
      lpPt->x = (lpPt->x - lphDC->ptViewOrg.x) + lphDC->ptWindowOrg.x;
      lpPt->y = (lpPt->y - lphDC->ptViewOrg.y) + lphDC->ptWindowOrg.y;
      lpPt++;
    }
  }
  else
  {
    while (nCount-- > 0)
    {
      lpPt->x = (int)
                (((long)(lpPt->x - lphDC->ptViewOrg.x)) * (long) lphDC->extWindow.cx /
                 (long) lphDC->extView.cx + lphDC->ptWindowOrg.x);
      lpPt->y = (int)
                (((long)(lpPt->y - lphDC->ptViewOrg.y)) * (long) lphDC->extWindow.cy / 
                 (long) lphDC->extView.cy + lphDC->ptWindowOrg.y);
      lpPt++;
    }
  }
  return TRUE;
}


BOOL FAR PASCAL SPtoLP(hDC, lpPt, nCount)
  HDC     hDC;
  LPPOINT lpPt;
  int     nCount;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (lphDC->wMappingMode == MM_TEXT)
  {
    while (nCount-- > 0)
    {
      lpPt->x = (lpPt->x - lphDC->ptOrg.x) - lphDC->ptViewOrg.x
                 + lphDC->ptWindowOrg.x;
      lpPt->y = (lpPt->y - lphDC->ptOrg.y) - lphDC->ptViewOrg.y
                 + lphDC->ptWindowOrg.y;
      lpPt++;
    }
  }
  else
  {
    while (nCount-- > 0)
    {
      lpPt->x = (int)
                (((long)(lpPt->x - lphDC->ptViewOrg.x)) * (long) lphDC->extWindow.cx /
                 (long) lphDC->extView.cx + lphDC->ptWindowOrg.x) -
                 lphDC->ptOrg.x;
      lpPt->y = (int)
                (((long)(lpPt->y - lphDC->ptViewOrg.y)) * (long) lphDC->extWindow.cy / 
                 (long) lphDC->extView.cy + lphDC->ptWindowOrg.y) -
                 lphDC->ptOrg.y;
      lpPt++;
    }
  }
  return TRUE;
}


