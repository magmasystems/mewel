/*===========================================================================*/
/*                                                                           */
/* File    : WINSHOW.C                                                       */
/*                                                                           */
/* Purpose : Implements the ShowWindow() function.                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-94 Marc Adler/Magma Systems     All Rights Reserved    */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif
static VOID PASCAL SetVisibilityFlags(WINDOW *, BOOL);
#ifdef __cplusplus
}
#endif

#if !defined(XWINDOWS)
#define XMEWELShowWindow(w, wFlags)
#endif


BOOL FAR PASCAL ShowWindow(hWnd, nCmdShow)
  HWND   hWnd;
  int    nCmdShow;
{
  int    rc = TRUE;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  ulFlags;
  UINT   ulSetPosFlags;
  RECT   r;

  /*
    Don't do anything if we are hiding an already hidden window.
  */
  if (!w || (nCmdShow == SW_HIDE && TEST_WS_HIDDEN(w)))
    return FALSE;

  ulFlags = w->flags;

  switch (nCmdShow)
  {
    case SW_HIDE :
      SetWindowPos(hWnd, 0, 0, 0, 0, 0, 
                   SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOMOVE | 
                   SWP_NOSIZE | SWP_NOACTIVATE);
      if (InternalSysParams.hWndActive == hWnd)
        _WinActivateFirstWindow();
      break;

    /*
      SetWindowPos flags
        SW_SHOW        SHOWWINDOW NOZORDER NOMOVE NOSIZE
        SW_SHOWNA      SHOWWINDOW NOZORDER NOMOVE NOSIZE NOACTIVATE
        SW_SHOWNOACT   SHOWWINDOW NOZORDER               NOACTIVATE
        SW_SHOWNORMAL  SHOWWINDOW NOZORDER
    */
    case SW_SHOW           :
      ulSetPosFlags = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE;
      goto call_setpos;
    case SW_SHOWNA         :
      ulSetPosFlags = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE |
                      SWP_NOACTIVATE;
      goto call_setpos;
    case SW_SHOWNOACTIVATE :
      ulSetPosFlags = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE;
call_setpos:
      r = w->rect;
      /*
        Since SetWindowPos expects client coordinates for child windows,
        convert the coordinates in the rect.
      */
#if !defined(MOTIF)
      if ((ulFlags & WS_CHILD) || (w->parent && w->parent != InternalSysParams.wDesktop))
        WinScreenRectToClient(GetParentOrDT(hWnd), (LPRECT) &r);
#endif

      SetWindowPos(hWnd, NULLHWND,
                   r.left, r.top, RECT_WIDTH(r), RECT_HEIGHT(r),
                   ulSetPosFlags);
      break;


    case SW_MAXIMIZE      :
      if (!(ulFlags & WS_MAXIMIZE))
        WinZoom(hWnd);
      break;

    case SW_SHOWMINIMIZED     :
    case SW_MINIMIZE          :
    case SW_SHOWMINNOACTIVE   :
      /*
                          SW_MIN       SW_SHOWMIN          SW_MINNOACT
                          ------       ----------          -----------
        SetWindowPos      NOACTIVATE   SHOWWINDOW | NOZ    SHOW | NOZ | NOACT
        ActivateFirstTop    X
      */
      if (!(ulFlags & WS_MINIMIZE))
        WinMinimize(hWnd);
      break;

    case SW_RESTORE           :
    case SW_SHOWNORMAL        :
      if (ulFlags & WS_MINIMIZE)
        WinMinimize(hWnd);
      else if (ulFlags & WS_MAXIMIZE)
        WinZoom(hWnd);
      else
      {
        if (nCmdShow == SW_SHOWNORMAL)
        {
          ulSetPosFlags = SWP_SHOWWINDOW | SWP_NOZORDER;
          goto call_setpos;
        }
      }
      break;
  }

  return rc;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinShowWindow()                                              */
/*                                                                          */
/* Purpose  : Alters the internal visibility state of a window.             */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinShowWindow(WINDOW *w, BOOL bVisible)
{
  WINDOW *wm;
  BOOL   bSendWMVIS = (TEST_PROGRAM_STATE(STATE_NO_WMSETVISIBLE) == 0);

  static int iLevel = 0;


  /*
    SPEED NOTE :
      We can speed up things a little bit if we go for a little less
    Windows compatibility and forget about the sending of the WM_SHOWWINDOW
    message. We may want to consider putting support for WM_SHOWWINDOW
    into the SetWindowsCompatibility() flags.
  */

  if (w == (WINDOW *) NULL)
    return;

  if (iLevel == 0)
  {
    SetVisibilityFlags(w, bVisible);
    if (bSendWMVIS)
      SendMessage(w->win_id, WM_SHOWWINDOW, bVisible, 0L);
  }

  /*
    Show/hide the menubar
  */
#if !defined(MOTIF)
  if (w->hMenu && (wm = WID_TO_WIN(w->hMenu)) != (WINDOW *) NULL)
  {
    /*
      Don't recurse since we want to leave the submenus hidden
    */
    SetVisibilityFlags(wm, bVisible);
    if (bSendWMVIS)
      SendMessage(wm->win_id, WM_SHOWWINDOW, bVisible, 0L);
  }
#endif

  /*
    Show/hide the first-level children 
  */
  for (w = w->children;  w;  w = w->sibling)
  {
    /*
      Leave a scrollbar whose range is 0 hidden
    */
#if!defined(MOTIF)
    if (bVisible && w->idClass == SCROLLBAR_CLASS)
    {
      int minpos, maxpos;
      GetScrollRange(w->win_id, SB_CTL, &minpos, &maxpos);
      if (minpos == maxpos)
        continue;
    }
#endif

    /*
      If a child window's state is amorphous (neither hidden nor visible,
      a state which can occur if the window has just been created), then
      set the child's visible flag on.
      However, if a child has a visibility state set, then do not touch
      it, no matter what state we are setting the parent to. It is MEWEL's
      responsibility to prohibit a child window from being drawn if its
      parent is hidden.
    */
    if (!(w->flags & WS_VISIBLE) && !TEST_WS_HIDDEN(w))
      SetVisibilityFlags(w, TRUE);

    /*
      Recurse and show/hide the children of the child window
    */
    if (w->children)
    {
      iLevel++;
      _WinShowWindow(w, bVisible);
      iLevel--;
    }

    /*
      Show/hide the menubar
    */
#if !defined(MOTIF)
    if (w->hMenu && (wm = WID_TO_WIN(w->hMenu)) != (WINDOW *) NULL)
    {
      /*
        Don't recurse since we want to leave the submenus hidden
      */
      SetVisibilityFlags(wm, bVisible);
      if (bSendWMVIS)
        SendMessage(wm->win_id, WM_SHOWWINDOW, bVisible, 0L);
    }
#endif
  }
}


static VOID PASCAL SetVisibilityFlags(WINDOW *w, BOOL bVisible)
{
  if (bVisible)
  {
    w->flags |= WS_VISIBLE;
    CLR_WS_HIDDEN(w);
    XMEWELShowWindow(w, SWP_SHOWWINDOW);
  }
  else
  {
    w->flags &= ~WS_VISIBLE;
    SET_WS_HIDDEN(w);
    XMEWELShowWindow(w, SWP_HIDEWINDOW);
  }
}



/****************************************************************************/
/*                                                                          */
/* Function : IsIconic()                                                    */
/*                                                                          */
/* Purpose  : Determines if a window is currently iconic                    */
/*                                                                          */
/* Returns  : TRUE if iconic, FALSE if not                                  */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsIconic(hWnd)
  HWND hWnd;
{
  return (BOOL) ((WinGetFlags(hWnd) & WS_MINIMIZE) != 0L);
}

/****************************************************************************/
/*                                                                          */
/* Function : OpenIcon()                                                    */
/*                                                                          */
/* Purpose  : Restores an iconic window.                                    */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL OpenIcon(hWnd)
  HWND hWnd;
{
  return IsIconic(hWnd) ? ShowWindow(hWnd, SW_RESTORE) : FALSE;
}

/****************************************************************************/
/*                                                                          */
/* Function : CloseWindow()                                                 */
/*                                                                          */
/* Purpose  : Iconizes a top-level window.                                  */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL CloseWindow(hWnd)
  HWND hWnd;
{
  /*
    Minimize the window only if it's a top-level window.
  */
  if (hWnd && GetParentOrDT(hWnd) == _HwndDesktop)
    ShowWindow(hWnd, SW_MINIMIZE);
}

/****************************************************************************/
/*                                                                          */
/* Function : IsZoomed()                                                    */
/*                                                                          */
/* Purpose  : Determines if a window is zoomed.                             */
/*                                                                          */
/* Returns  : TRUE if maximized, FALSE if not.                              */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsZoomed(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (BOOL) (w && ((w->flags & WS_MAXIMIZE) != 0L));
}

