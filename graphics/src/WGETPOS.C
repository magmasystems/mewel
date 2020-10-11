/*===========================================================================*/
/*                                                                           */
/* File    : WGETPOS.C                                                       */
/*                                                                           */
/* Purpose : Implements the GetCurrentPosition(Ex) functions.                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* $Log:   E:/vcs/mewel/wgetpos.c_v  $                                       */
/*	                                                                         */
/*	   Rev 1.0   18 Nov 1993  9:44:38   Adler                                */
/*	Initial revision.                                                        */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


DWORD FAR PASCAL GetCurrentPosition(hDC)
  HDC hDC;
{
  LPHDC lphDC = _GetDC(hDC);
  if (lphDC)
    return MAKELONG(lphDC->ptPen.x, lphDC->ptPen.y);
  else
    return 0L;
}

BOOL FAR PASCAL GetCurrentPositionEx(hDC, lppt)
  HDC hDC;
  POINT FAR *lppt;
{
  LPHDC lphDC = _GetDC(hDC);
  if (lphDC && lppt)
  {
    lppt->x = lphDC->ptPen.x;
    lppt->y = lphDC->ptPen.y;
    return TRUE;
  }
  else
    return FALSE;
}

