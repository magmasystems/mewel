/*===========================================================================*/
/*                                                                           */
/* File    : WSHOWPOP.C                                                      */
/*                                                                           */
/* Purpose : Implements the ShowOwnedPopups() function                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

VOID WINAPI ShowOwnedPopups(HWND hWnd, BOOL fShow)
{
  WINDOW *w, *wNext;

  for (w = InternalSysParams.wDesktop->children;  w;  w = wNext)
  {
    wNext = w->sibling;   /* in case something alters the chain */
    if (w->hWndOwner == hWnd)
      ShowWindow(w->win_id, fShow ? SW_SHOWNA : SW_HIDE);
  }
}

