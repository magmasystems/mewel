/*===========================================================================*/
/*                                                                           */
/* File    : WFILLWIN.C                                                      */
/*                                                                           */
/* Purpose : The undocumented FillWindow() function.                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL FillWindow(hParent, hWnd, hDC, hBrush)
  HWND hParent;
  HWND hWnd;
  HDC  hDC;
  HBRUSH hBrush;
{
  RECT r;

  (void) hParent;

  GetClientRect(hWnd, (LPRECT) &r);
  if (hDC == 0)
  {
    hDC = GetDC(hWnd);
    FillRect(hDC, (LPRECT) &r, hBrush);
    ReleaseDC(hWnd, hDC);
  }
  else
    FillRect(hDC, (LPRECT) &r, hBrush);
}

