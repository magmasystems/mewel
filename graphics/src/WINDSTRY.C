/*===========================================================================*/
/*                                                                           */
/* File    : WINDSTRY.C                                                      */
/*                                                                           */
/* Purpose : Routines to destroy a window                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static int bInDestroyWindow = 0;

#if defined(XWINDOWS)
static VOID PASCAL XMEWELClearWinVars(HWND hWnd);
#endif

INT FAR PASCAL DestroyWindow(hWnd)
  HWND hWnd;
{
  RECT    rDestroyed;
  WINDOW  *w = WID_TO_WIN(hWnd);
  DWORD   dwFlags;

  if (!w)
    return FALSE;

  dwFlags = w->flags;  /* save for comparison below */

  /*
    See if the window is a child of another window and is contained within
    that parent window. 
  */
#if !defined(MOTIF)
  rDestroyed = w->rect;
  _WinAdjustRectForShadow(w, (LPRECT) &rDestroyed);
#endif

  /*
    Do some low-level destruction, and recurse on the children.
  */
  _WinDestroy(hWnd);

  /*
    Invalidate the area which the *visible* portion occupied
  */
#if !defined(MOTIF)
  if (dwFlags & WS_VISIBLE)
  {
    WinUpdateVisMap();
    WinGenInvalidRects(_HwndDesktop, (LPRECT) &rDestroyed);
  }
#endif

  /*
    If we deleted the window which has the current focus, then we must
    choose a new window to get the focus. We will go through the top level 
    windows and choose the first visible non-static, non-menu window
    which is not disabled.
  */
  if (InternalSysParams.hWndFocus == NULLHWND)
  {
    if (GetActiveWindow())
      SetFocus(GetActiveWindow());
    else
      _WinActivateFirstWindow();
  }

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinDestroy()                                                 */
/*                                                                          */
/* Purpose  : Low level window destruction routine.                         */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinDestroy(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *cw, *wSibling;
  MSG    msg;
  
  if (!w)
    return;

  bInDestroyWindow++;

  /*
    If we are destroying the active window, then null out the active variable
  */
  if (GetActiveWindow() == hWnd)
  {
    InternalSysParams.hWndActive = NULLHWND;
    InternalSysParams.hWndFocus = hWnd;  /* force the test below to succeed */
  }

  if (InternalSysParams.hWndFocus == hWnd)
  {
    InternalSysParams.hWndFocus = NULLHWND;
    SendMessage(hWnd, WM_KILLFOCUS, NULLHWND, 0L);
  }

  /*
    Sever the clipboard 
  */
  if (hWnd == _Clipboard.hWndOwner)
  {
    SendMessage(hWnd, WM_RENDERALLFORMATS, 0, 0L);
    _Clipboard.hWndOwner = NULLHWND;
  }

  /*
    Get rid of the capture
  */
  if (hWnd == InternalSysParams.hWndCapture)
  {
    ReleaseCapture();
  }

  /*
    Get rid of the caret if it's associated with the window
  */
  if (hWnd == InternalSysParams.hWndCaret)
    DestroyCaret();

  /*
    Get rid of the system modal window if it's this window.
  */
  if (hWnd == InternalSysParams.hWndSysModal)
    InternalSysParams.hWndSysModal = NULLHWND;

  /*
    Kill all timers associated with this window, and get rid of the menu.
  */
  KillWindowTimers(hWnd);
  if (w->hMenu)
    DestroyMenu(w->hMenu);
  if (w->hSysMenu)
    DestroyMenu(w->hSysMenu);

  /*
    Recursively destroy the window's scrollbars
  */
  if (w->hSB[SB_HORZ])
    _WinDestroy(w->hSB[SB_HORZ]);
  if (w->hSB[SB_VERT])
    _WinDestroy(w->hSB[SB_VERT]);

#if defined(MEWEL_GUI)
  /*
    If we are destroying an iconic window, then destroy the icon title window
    as well.
  */
  if (IsIconic(hWnd))
    WinDestroyIconTitle(w);
#endif

  /*
    Clear out the message queue to make sure that no messages are pending for 
    the window or any of its children. 
  */
  SET_PROGRAM_STATE(STATE_NONBLOCKING_PEEKMSG);
  while (_PeekMessage(&msg) && (msg.hwnd == hWnd || IsChild(hWnd, msg.hwnd)))
    _WinGetMessage(&msg);
  CLR_PROGRAM_STATE(STATE_NONBLOCKING_PEEKMSG);

  /*
    For Win 3.0 compatibility, we should issue the WM_PARENTNOTIFY
    message
  */
  WinParentNotify(hWnd, WM_DESTROY, 0);

  /*
    Recursively destroy the decendants
  */
  for (cw = w->children;  cw;  cw = wSibling)
  {
    wSibling = cw->sibling; /* save the sibling cause the link gets trashed */
    _WinDestroy(cw->win_id);
  }

  /*
    Destroy any owned windows.
  */
  if (hWnd != _HwndDesktop)
  {
    for (cw = InternalSysParams.wDesktop->children;  cw;  cw = wSibling)
    {
      wSibling = cw->sibling; /* save the sibling cause the link gets trashed */
      if (cw->hWndOwner == hWnd)
        _WinDestroy(cw->win_id);
    }
  }

  /*
    OPT NOTE - we can save some time by removing the WM_SHOWWINDOW message
    at the expense of Windows compatibility
  */
  if (!TEST_PROGRAM_STATE(STATE_NO_WMSETVISIBLE))
    SendMessage(hWnd, WM_SHOWWINDOW, FALSE, 0L);  /* as per Windows... */

  /*
    Send the WM_DESTROY and WM_NCDESTROY messages to the window.
  */
  SendMessage(hWnd, WM_DESTROY,   0, 0L);
  SendMessage(hWnd, WM_NCDESTROY, 0, 0L);

  /*
    Remove all internal pointer to the window and free the memory.
  */
  WinDelete(hWnd);

  bInDestroyWindow--;
}


/****************************************************************************/
/*                                                                          */
/* Function : WinDelete()                                                   */
/*                                                                          */
/* Purpose  : Performs most of the low-level stuff required to remove a     */
/*            window from various linked lists and to free up the memory.   */
/*                                                                          */
/* Returns  : Nothing important.                                            */
/*                                                                          */
/****************************************************************************/
extern VOID PASCAL WinFree(HWND);

VOID FAR PASCAL WinDelete(hWnd)
  HWND hWnd;
{
  WINDOW *delw = WID_TO_WIN(hWnd);
  WINDOW *w, *prevw; 
  extern BOOL bIsaWindowZoomed;    /* from WINZOOM.C */
  
  if (!delw)
    return;

  /*
    If we are destroying a maximized window, then reset the bIsaWindowZoomed
    flag, or else we will never be able to maximize again!
  */
  if ((delw->flags & WS_MAXIMIZE))
    bIsaWindowZoomed = FALSE;

  /*
    1) Unlink the window from the window list.
  */
  if (delw == InternalSysParams.WindowList)
    InternalSysParams.WindowList = delw->next;
  else
  {
    for (prevw = w = InternalSysParams.WindowList;
         w && w != delw;
         prevw = w, w = w->next)
      ;
    if (prevw)
      prevw->next = delw->next; 
  } 


  /*
    2) Remove the window from its parent's children chain
  */
  _WinUnlinkChildFromParent(delw);


#if defined(MOTIF)
  if (bInDestroyWindow)
  {
    /*
      Leave the menu destruction for Motif. It should destroy the
      menubar when it destroys the window.

      When we quit an app, and DestroyWindow calls DestroyMenu to get
      rid of the menu bar, it leaves something dangling. So, upon exit,
      Motif will core dump in the XmCallRowColumnUnmapCallback. So,
      in a bit of a kludge, we will not destroy the menu when we are
      in the process of exiting.
    */
    if (IS_MENU(delw))
    {
      goto next;
    }

#if 1
    XMEWELClearWinVars(hWnd);
#endif
    if (delw->widget && delw->widget != delw->widgetShell)
    {
      XtmewelRemoveEventHandlersForWidget(delw->widget);
      XtDestroyWidget(delw->widget);
    }
    if (delw->widgetDrawingArea && delw->widgetDrawingArea != delw->widget)
    {
      XtmewelRemoveEventHandlersForWidget(delw->widgetDrawingArea);
    }
    if (delw->widgetFrame && delw->widgetFrame != delw->widget)
    {
      XtmewelRemoveEventHandlersForWidget(delw->widgetFrame);
    }
    if (delw->widgetShell)
    {
      XtmewelRemoveEventHandlersForWidget(delw->widgetShell);
      XtDestroyWidget(delw->widgetShell);
    }
  }
next:
#elif defined(XWINDOWS)
  if (delw->Xwindow)
    XDestroyWindow(XSysParams.display, delw->Xwindow);
#endif


  /*
    3) Free the memory allocated for the window and the window-extra bytes.
       Also, free the DC if the window class has CS_OWNDC.
  */
  MYFREE_FAR(delw->title);
  MyFree(delw->pWinExtra);
  MyFree(delw->pPrivate);
  MyFree(delw->pMDIInfo);
  if (delw->hDC)
    ReleaseDC(hWnd, delw->hDC);
  MyFree((char *) delw);
  WinFree(hWnd);
}



/****************************************************************************/
/*                                                                          */
/* Function : WindowDestroyCallback()                                       */
/*                                                                          */
/* Purpose  : This is called when a MOTIF window is destroyed. If we        */
/*            closed the window by some 'devious' MOTIF method, then        */
/*            we must destroy the corresponding internal MEWEL window.      */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
#if defined(MOTIF) || defined(DECWINDOWS)

static VOID PASCAL XMEWELClearWinVars2(Widget widget);

void WindowDestroyCallback(widget, client_data, call_data)
  Widget  widget;
  XtPointer client_data;
  XtPointer call_data;
{
  (void) client_data;
  (void) call_data;

  if (!bInDestroyWindow)
  {
    HWND hWnd;
    if ((hWnd = _WidgetToHwnd(widget)) != NULL)
      _WinDestroy(hWnd);
  }
  else
    XMEWELClearWinVars2(widget);
#if 0
  if (widget == widgetLastCursor)
    XSysParams.widgetLastCursor = XSysParams.winLastCursor = 0;
  if (widget == widgetCursor)
    XSysParams.widgetCursor = XSysParams.winCursor = 0;
#endif
}

static VOID PASCAL XMEWELClearWinVars(HWND hWnd)
{
  WINDOW *w = WID_TO_WIN(hWnd);
  if (w)
  {
    XMEWELClearWinVars2(w->widgetShell);
    XMEWELClearWinVars2(w->widgetFrame);
    XMEWELClearWinVars2(w->widgetDrawingArea);
    XMEWELClearWinVars2(w->widget);
  }
}

static VOID PASCAL XMEWELClearWinVars2(Widget widget)
{
  Window xWin;
  if (widget == NULL)
    return;
  if ((xWin = XtWindow(widget)) != NULL)
  {
    if (xWin == XSysParams.winLastCursor)
    {
      XSysParams.widgetLastCursor = (Widget) NULL;
      XSysParams.winLastCursor = 0;
    }
    if (xWin == XSysParams.winCursor)
    {
      XSysParams.widgetCursor = (Widget) NULL;
      XSysParams.winCursor = 0;
    }
    if (widget == XSysParams.widgetDesktop)
    {
      XSysParams.widgetDesktop = (Widget) NULL;
      XSysParams.windowDesktop = 0;
    }
  }
}
#endif

