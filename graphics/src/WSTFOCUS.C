/*===========================================================================*/
/*                                                                           */
/* File    : WSTFOCUS.C                                                      */
/*                                                                           */
/* Purpose : SetFocus(hWnd) function                                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"

/*
  Saves the current focus window while we are in a menu. This is so
  that if an app processes the WM_INITMENU message and calls GetFocus(),
  it will find a valid window instead of NULLHWND.
*/
static HWND hWndMenuFocus = 0;


HWND FAR PASCAL GetFocus(void)
{
  /*
    Windows does not consider menus to be actual windows. So, if the
    current focus window is a menu, then return what ever it saved
    as the old focus window.
  */
  if (IsMenu(InternalSysParams.hWndFocus))
#if 12595
    return hWndMenuFocus;
#else
    return _MenuHwndToStruct(InternalSysParams.hWndFocus)->hOldFocus;
#endif

  return InternalSysParams.hWndFocus;
}


HWND FAR PASCAL SetFocus(hWnd)
  HWND hWnd;
{
  HWND hOld = InternalSysParams.hWndFocus;

  /*
    Setting the focus to the window which currently has the focus? A no-op.
  */
  if (hOld == hWnd)
    return hOld;
  /*
    Make sure the window exists. We are allowed to have NULLHWND as
    the hWnd value.
  */
  if (hWnd != NULLHWND && !IsWindow(hWnd))
    return 0;

  if (hOld)
  {
    /*
      We set CurrFocusWnd = NULLHWND so that windows which process the 
      WM_KILLFOCUS message don't think that they still have the focus.
    */
    InternalSysParams.hWndFocus = NULLHWND;
    SendMessage(hOld, WM_KILLFOCUS, hWnd, 0L);
  }

  /*
    Record the non-menu window which is now getting the focus.
  */
  if (!IsMenu(hWnd))
    hWndMenuFocus = hWnd;

  InternalSysParams.hWndFocus = hWnd;

#if defined(MOTIF) || defined(DECWINDOWS)
  _XSetFocus(hWnd);
#else
  WinRefreshActiveTitleBar(hWnd);
#endif /* MOTIF */

  /*
    Notify the window which is receiving the focus by sending WM_SETFOCUS.
  */
  if (hWnd)
    SendMessage(hWnd, WM_SETFOCUS, hOld, 0L);
  return hOld;
}


VOID FAR PASCAL WinRefreshActiveTitleBar(hFocus)
  HWND hFocus;
{
  HWND   hRoot;
  WINDOW *wFocus, *w = NULL;

  if ((wFocus = WID_TO_WIN(hFocus)) == NULL)
    return;

  /*
    Don't do any refreshin' if we invoked a menu.
  */
  if (IS_MENU(wFocus))
    return;

  /*
    Redraw the caption of the closest parent who has a caption (or the
    window itself). We set 'w' to this window. 
  */
  if ((hRoot = _WinGetRootWindow(hFocus)) == NULL)
    return;

  if (hRoot != GetActiveWindow() && (w = WID_TO_WIN(hRoot)) != InternalSysParams.wDesktop)
  {
    /*
      It does not look pleasing when a child overlapped window is
      active and the app's main window's caption is greyed. So, redraw
      the main window (if it has a caption) in the active color.
    */
    if (w->parent != InternalSysParams.wDesktop)
    {
      for (w = w->parent;  w && w->parent != InternalSysParams.wDesktop;  w = w->parent)
        ;
      if (w && HAS_CAPTION(w->flags))
        SendMessage(w->win_id, WM_NCACTIVATE, TRUE, 0L);
    }
  }
}

