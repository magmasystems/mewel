/*===========================================================================*/
/*                                                                           */
/* File    : WNOTIFY.C                                                       */
/*                                                                           */
/* Purpose : Issues the WM_PARENTNOTIFY message at the appropriate time.     */
/*                                                                           */
/* History : (maa) 5/6/92 - created                                          */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL WinParentNotify(hWnd, wMsg, lParam)
  HWND hWnd;
  UINT wMsg;
  DWORD lParam;
{
  WINDOW *w, *wParent;

  /*
    Get a pointer to the window and its parent.
  */
  w = WID_TO_WIN(hWnd);

  /*
    Don't send the WM_PARENTNOTIFY message if the window is not a child
    window, or if it has the WS_EX_NOPARENTNOTIFY style.
  */
  if (!(w->flags & WS_CHILD) || (w->dwExtStyle & WS_EX_NOPARENTNOTIFY))
    return;

  /*
    For the mouse message, leave lParam alone.... it's the col,row
    cordinates. For the other messages, create an lParam of the child
    window handle and the child id.
  */
  if (wMsg == WM_CREATE || wMsg == WM_DESTROY)
    lParam = MAKELONG(hWnd, w->idCtrl);

  /*
    We go up the ancestor tree, including the desktop window, and
    notify each ancestor withthe WM_PARENTNOTIFY message.
  */
  wParent = w->parent;
  while (wParent != NULL)
  {
    SendMessage(wParent->win_id, WM_PARENTNOTIFY, wMsg, lParam);
    if (wParent == InternalSysParams.wDesktop)
      break;
    /*
      Get the parent. Dialog boxes are special cases, so get the owner.
    */
    if (IS_DIALOG(wParent))
    {
      HWND hP = wParent->hWndOwner;
      if (hP)
        wParent = WID_TO_WIN(hP);
      else
        break;
    }
    else
      wParent = wParent->parent;
  }
}

