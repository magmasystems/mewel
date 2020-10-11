/*===========================================================================*/
/*                                                                           */
/* File    : WACTIVE.C                                                       */
/*                                                                           */
/* Purpose : Implements GetActiveWindow() and SetActiveWindow()              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


HWND FAR PASCAL GetActiveWindow(void)
{
  return InternalSysParams.hWndActive;
}

HWND FAR PASCAL SetActiveWindow(hWnd)
  HWND hWnd;
{
  HWND hOld = GetActiveWindow();

  /*
    Let SetWindowPos do the brunt of the work. In particular, let
    it activate and arrange the z-order of the window.
  */
  if (hOld != hWnd)
    SetWindowPos(hWnd, NULLHWND, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  return hOld;
}


HWND FAR PASCAL _WinActivate(HWND hWnd, BOOL bMouse)
{
  HWND hOldActive;

  if ((hOldActive = InternalSysParams.hWndActive) == hWnd)
    return hOldActive;

  if (hOldActive != NULLHWND)
  {
    SendMessage(hOldActive, WM_NCACTIVATE, FALSE, 0L);
    SendMessage(hOldActive, WM_ACTIVATE,0,MAKELONG(hWnd,IsIconic(hOldActive)));
  }

#if !91792
  {
  WINDOW *w = WID_TO_WIN(hWnd);
  if (IS_MDIDOC(w))
    hWnd = w->parent->parent->win_id;  /* set the active window to the frame */
  }
#endif

  if ((InternalSysParams.hWndActive = hWnd) != NULLHWND)
  {
    SendMessage(hWnd, WM_NCACTIVATE, (bMouse) ? 2 : 1, 0L);
    SendMessage(hWnd, WM_ACTIVATE, (bMouse) ? 2 : 1, MAKELONG(hOldActive, 0));
  }

  return hOldActive;
}



/****************************************************************************/
/*                                                                          */
/* Function : _WinActivateFirstWindow()                                     */
/*                                                                          */
/* Purpose  : If the currently active window is hidden or iconized, then    */
/*            this function searches for the first visible, enabled top     */
/*            level window and activates it.                                */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinActivateFirstWindow(void)
{
  WINDOW *w;

  for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
  {
    HWND hWnd = w->win_id;
    if (IsWindowVisible(hWnd) && IsWindowEnabled(hWnd))
    {
      SetActiveWindow(hWnd);
      break;
    }
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinGetRootWindow(hChild)                                     */
/*                                                                          */
/* Purpose  : Given a window handle, goes up the parent tree until we find  */
/*            a top-level window. This is used to determine what window     */
/*            will be getting the activation status if we click on a child  */
/*            window.                                                       */
/*                                                                          */
/* Returns  : The handle of the window, or NULL if not found.               */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL _WinGetRootWindow(hChild)
  HWND hChild;
{
  WINDOW *w;

  w = WID_TO_WIN(hChild);

  while (w && w != InternalSysParams.wDesktop)
  {
    /*
      If we are dealing with an MDI document window, return that one.
      Return this window if it is a popup window.
    */
#if !91792
    if (                                (w->flags & WS_POPUP))
#elif 121092 && !defined(IMS)
    /*
      In Autosoft's EVENTS program, the child WS_POPUP windows 
      *should* get the activation status (ie - hitting CTRL-TAB
      should toggle between the child dialog windows)
    */
    if (IS_MDIDOC(w) ||
         ((w->flags & WS_POPUP) /* && !(w->flags & WS_CHILD) */ ))
#else
    /*
      10/12/92 (maa)
        It seems that if a popup window is a child window, then the
        popup does not become the active window.
    */
    if (IS_MDIDOC(w) ||
         ((w->flags & WS_POPUP) && !(w->flags & WS_CHILD)))
#endif
      return w->win_id;

    /*
      Look at non-child windows.
    */
    if (!(w->flags & WS_CHILD))
    {
      /*
        Confirm that it is not a control window.
      */
      int iClass = _WinGetLowestClass(w->idClass);
      if (iClass == NORMAL_CLASS || iClass == DIALOG_CLASS)
#if 91792
        /*
          The root of an mdi doc window is the frame window, not the
          MDI client window.
        */
        if (!(w->ulStyle & WIN_IS_MDICLIENT))
#endif
        return w->win_id;
    }

    /*
      Go up the chain to the parent.
    */
    w = w->parent;
  }

  return NULLHWND;
}

