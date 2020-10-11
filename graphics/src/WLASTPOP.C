/*===========================================================================*/
/*                                                                           */
/* File    : WLASTPOP.C                                                      */
/*                                                                           */
/* Purpose : Implements the GetLastActivePopup() function.                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


HWND FAR PASCAL GetLastActivePopup(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *wChild;

  /*
    If this window has an owner, then return the window itself.
  */
  if (w->hWndOwner)
    return hWnd;

  /*
    Scan the top-level windows for a popup window who owns hWnd.
  */
  for (wChild = InternalSysParams.wDesktop->children;  wChild;  wChild = wChild->sibling)
    if ((wChild->flags & WS_POPUP) && w->hWndOwner == wChild->win_id)
      return wChild->win_id;

  /*
    No popup found who owns hWnd? Return the window itself
  */
  return hWnd;
}

