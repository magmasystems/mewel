/*===========================================================================*/
/*                                                                           */
/* File    : WINENABLE.C                                                     */
/*                                                                           */
/* Purpose : Enables and disables a window                                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/****************************************************************************/
/*                                                                          */
/* Function : IsWindowEnabled(hWnd)                                         */
/*                                                                          */
/* Purpose  : Determines if a window is enabled for input. It is disabled   */
/*            if its WS_DISABLED style is set or if one of its ancestors    */
/*            is disabled.                                                  */
/*                                                                          */
/* Returns  : TRUE if enabled, FALSE if not.                                */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsWindowEnabled(HWND hWnd)
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  /*
    Walk up the ancestor tree and see if any are disabled.
  */
  while (w && w != InternalSysParams.wDesktop)
  {
    if (w->flags & WS_DISABLED)
      return FALSE;
    w = w->parent;
  }
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : EnableWindow(hWnd, bEnable)                                   */
/*                                                                          */
/* Purpose  : Enables or disables a window.                                 */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL EnableWindow(HWND hWnd, BOOL bEnable)
{
  WINDOW *w;
  BOOL   bDisabledPrev;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  /*
    If the window is disabled and we want to disable it, or if the
    window is enabled and we want to enable it, then don't do anything.
    This especially important in MFC2, where MFC spends its idle time
    checking on the enable/disable state of things.
  */
  bDisabledPrev = (BOOL) ((w->flags & WS_DISABLED) != 0);
  if (bEnable && !bDisabledPrev || !bEnable && bDisabledPrev)
    return TRUE;

  /*
    Enable the window
  */
  if (bEnable)
  {
    w->flags &= ~WS_DISABLED;
#if defined(MOTIF)
    XMEWELEnableWindow(hWnd, bEnable);
#endif
  }
  
  /*
    Disable the window
  */
  else
  {
    w->flags |= WS_DISABLED;
#if defined(MOTIF)
    XMEWELEnableWindow(hWnd, bEnable);
#endif
  
    /*
      Make sure that the disabled window doesn't end up with the focus
    */
    if (InternalSysParams.hWndFocus == hWnd)
      SetFocus(NULLHWND);
  
  }
  
  /*
    Inform the window that it's being enabled/disabled
  */
  SendMessage(hWnd, WM_ENABLE, bEnable, 0L);
  return TRUE;
}

