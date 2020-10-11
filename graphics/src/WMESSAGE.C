/*===========================================================================*/
/*                                                                           */
/* File    : WMESSAGE.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*           10/14/90 (maa) - Added logic for WM_SETCURSOR                   */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#if 0
#define DEBUG
#endif

/*
  Thomas Wagner's international keyboard driver does not work correctly
  with MS-DOS 5. As I am not able to find the bug, switching it totally off
  is the easiest solution -- Wolfgang Lorenz
*/
#undef INTERNATIONAL_MEWEL

#include "wprivate.h"
#include "window.h"
#include "winevent.h"


static HWND PASCAL DetermineKeyWindow(void);
static VOID PASCAL Process_WM_ALT(void);

#ifdef DEBUG
static VOID PASCAL DumpMouseMessage(HWND, int, WPARAM, LPARAM);
#endif

/****************************************************************************/
/*                                                                          */
/*   Flow of event polling                                                  */
/*                                                                          */
/*                                          -----> GetEvent()               */
/*                                          |                               */
/*                                          |                               */
/*   GetMessage --->  PeekMessage ---> _PeekMessage  <---                   */
/*                                |                     |                   */
/*                     PM_REMOVE? |--> _WinGetMessage ---                   */
/*                                |                                         */
/*                                ---> Filtering                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* Function : _PeekMessage()                                                */
/*                                                                          */
/* Purpose  : Calls the low level input device poller to retrieve an input  */
/*            event (if not message is already at the front of the queue).  */
/*            Responsible for determine which window the message is meant   */
/*            for. Also fills the wParam and lParam fields with the proper  */
/*            values.                                                       */
/*                                                                          */
/* Returns  : The WM_xxx message value, if there is a message. 0 if not.    */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL _PeekMessage(lpMsg)
  LPMSG lpMsg;
{
  LPMSG  e;
  HWND   hWnd;
  DWORD  lParam;
  UINT   rc;
  INT    wHitCode;

#if defined(OS2) && !defined(NOTHREADS)
  if (DOSSEMWAIT(&EventQueue.semQueueEmpty, SEM_IMMEDIATE_RETURN) != 0)
  {
    RefreshInvalidWindows(_HwndDesktop);
    return FALSE;
  }

#else
  /*
    If the event queue is empty, go and check for an event. Also check
    for an event if we are doing a PeekMessage() for a certain message
    and that message hasn't occured yet.
  */
  if (EventQueue.semQueueEmpty || TEST_PROGRAM_STATE(STATE_MUST_CHECK_EVENT))
  {
    if (TEST_PROGRAM_STATE(STATE_NONBLOCKING_PEEKMSG))
    {
      CLR_PROGRAM_STATE(STATE_MUST_CHECK_EVENT);
      return FALSE;
    }

    /*
      Call the input device poller to check for a raw event.
    */
#if !defined(XWINDOWS)
    if (SysEventInfo.pfExternalEventChecker)
      rc = (*SysEventInfo.pfExternalEventChecker)(lpMsg);
#else
    RefreshInvalidWindows(_HwndDesktop);
    rc = !EventQueue.semQueueEmpty;
#endif

    /*
      Still empty? No event, so return FALSE. Don't rely solely on
      the semaphore, cause there might be a message stuck at the
      from of the queue due to a PeekMessage() on a certain event.
    */
    if (EventQueue.semQueueEmpty || !rc)
    {
      RefreshInvalidWindows(_HwndDesktop);
      /* Check again, the above routine might have posted messages */
      if (EventQueue.semQueueEmpty)
      {
        CLR_PROGRAM_STATE(STATE_MUST_CHECK_EVENT);
        return FALSE;
      }
    }
  }
#endif

  /*
    Copy the event structure into the passed event structure

    11/13/91 (maa) There is a BUG here which one day we will fix....
    If you are doing a PeekMessage(&msg, hWnd, WM_CHAR, WM_CHAR, PM_REMOVE)
    and there is something ahead of the WM_CHAR message on the queue, then
    the line below retrieves the message at the head of the queue, *not*
    the WM_CHAR message which was just inserted into the queue by PostEvent().
    Therefore, the 'event.hWnd' member for the WM_CHAR message will remain 
    NULLHWND and will not be set to the proper window. The workaround is
    to do something like this in your code :
      if (PeekMessage(&msg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE))
  */
#ifndef RAPID
  if (TEST_PROGRAM_STATE(STATE_MUST_CHECK_EVENT))
    e = (LPMSG) QueueGetData(&EventQueue, 
                             QueuePrevElement(&EventQueue, EventQueue.qhead));
  else
#endif
    e = (LPMSG) QueueGetData(&EventQueue, EventQueue.qtail);

  /*
    _PeekMsg must return non-zero if there is a message, but if we have 
    a WM_NULL message, then 0 will be mistakenly returned. So, change
    WM_NULL messages into WM_REMOVEDMSG messages.
  */
  if (e->message == WM_NULL)
    e->message = WM_REMOVEDMSG;

  *lpMsg = *e;


  if ((hWnd = lpMsg->hwnd) == NULLHWND)
  {
    switch (lpMsg->message)
    {
      case WM_CHAR       :
      case WM_KEYDOWN    :
      case WM_SYSKEYDOWN :
      case WM_KEYUP      :
      case WM_SYSKEYUP   :
        hWnd = DetermineKeyWindow();
        break;


      case WM_LBUTTONDOWN :
        hWnd = 0;
      case WM_LBUTTONDBLCLK:
      case WM_MOUSEMOVE   :
      case WM_LBUTTONUP   :
      case WM_RBUTTONDOWN :
      case WM_RBUTTONUP   :
      case WM_RBUTTONDBLCLK:
      case WM_MOUSEREPEAT :
        /*
          If we have the mouse captured, send the mouse msg to that window.
        */
        if (InternalSysParams.hWndCapture && 
            IsWindow(InternalSysParams.hWndCapture))
        {
          hWnd = InternalSysParams.hWndCapture;
        }
        else
        {
          /*
            See which window the mouse is over. In the process, issue a
            WM_NCHITTEST message.
          */
          lParam = lpMsg->lParam;
          hWnd = DetermineClickOwner(HIWORD(lParam), LOWORD(lParam), &wHitCode);

          /*
            Possibly transform the mouse message into a WM_NCxxx message if
            the hitcode returned indicated that the mouse was not in the
            client area.
            Note :
            Don't use WM_NCxxx messages if the mouse was captured. All messages
            are client-area messages, according to Petzold.
          */
          if (hWnd != NULLHWND)
          {
            if (wHitCode != HTCLIENT && wHitCode != HTERROR)
            {
              lpMsg->message = WM_MOUSE_TO_NCMOUSE(lpMsg->message);
              e->message = lpMsg->message;
              e->wParam = lpMsg->wParam = wHitCode;
            }
          }
        }

#if 0
#ifdef DEBUG
        DumpMouseMessage(hWnd, lpMsg->message, wHitCode, lpMsg->lParam);
#endif
#endif
        /*
          Client-area mouse messages should be client-relative
        */
        if (IS_WM_MOUSE_MSG(lpMsg->message))
          e->lParam = lpMsg->lParam = _WindowizeMouse(hWnd, lpMsg->lParam);

        break;

    } /* end switch (message) */

    e->hwnd = lpMsg->hwnd = hWnd;
  } /* end if (hWnd == NULL) */


  /*
    Return the message value
  */
  return lpMsg->message;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinGetMessage()                                              */
/*                                                                          */
/* Purpose  : Waits for a message by repeatedly calling _PeekMessage().     */
/*            Removes the message from the queue by advancing the tail.     */
/*            Also in charge of setting a clearing various message queue    */
/*            semaphores.                                                   */
/*                                                                          */
/* Returns  : The WM_xxx message value.                                     */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL _WinGetMessage(lpMsg)
  LPMSG lpMsg;
{
  /*
    If there are no messages in the queue, then wait for a message
  */
  DOSSEMWAIT(&EventQueue.semQueueEmpty, -1L);

#if defined(XWINDOWS)
  if (_PeekMessage(lpMsg) == FALSE)
    return 0;
#else
  while (_PeekMessage(lpMsg) == FALSE)
    ;
#endif

  /* 
    Advance to the next element in the message queue. If the queue is
    empty, then set a semaphore telling the system about the emptiness.
  */
  EventQueue.qtail = QueueNextElement(&EventQueue, EventQueue.qtail);
  if (EventQueue.qhead == EventQueue.qtail)
    DOSSEMSET(&EventQueue.semQueueEmpty);
  
  /* Let other processes play with the event queue */
  DOSSEMCLEAR(&EventQueue.semQueueMutex);

  /* The queue is not full anymore */
  DOSSEMCLEAR(&EventQueue.semQueueFull);
  
  return SysEventInfo.lastMessage = lpMsg->message;
}


/****************************************************************************/
/*                                                                          */
/* Function : PeekMessage()                                                 */
/*                                                                          */
/* Purpose  : Sees if there is a message in the queue by calling _PeekMsg().*/
/*            If there is, the messages is subjected to the filtering       */
/*            criteria. If no filtering is needed, or the message fits the  */
/*            criteria, then the message is removed from the queue.         */
/*                                                                          */
/* Returns  : TRUE if there was message                                     */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL PeekMessage(lpMsg, hWnd, wFilterMin, wFilterMax, wRemove)
  LPMSG lpMsg;
  HWND  hWnd;
  UINT  wFilterMin;
  UINT  wFilterMax;
  UINT  wRemove;
{
  LPMSG e;
  INT   idxEvent = EventQueue.qtail;

#if defined(XWINDOWS)
  /*
    Every time we call GetMessage(), the X Windows event loop is done.
    We then check the MEWEL event queue to see if anything has been
    generated by MEWEL.
  */
  XEvent       xevent;
  int          rc;

  /*
    Do the _PeekMessage first so we can take care of invalid windows. If
    there are message in the queue, _PeekMessage returns TRUE. However,
    if there are invalid rectangles, and we posted a bunch of WM_PAINT
    messages, then _PeekMessage returns FALSE. We do not want to block
    on AppNextEvent if there are paint messages pending, so bypass
    AppNextEvent if the queue is not empty after _PeekMessage returns.
  */
peek_queue:
  rc = _PeekMessage(lpMsg);

  if (rc == 0)
  {
#if defined(MOTIF)
    if (EventQueue.semQueueEmpty && XSysParams.appContext != 0)
    {
      /*
        If there are no messages in the event queue, then we must wait for
        one. If we *DO NOT* want to block, then check the X queue, and
        if nothing is there, return. If we *do* want to block, or if
        there is something in the queue, then fetch the event.
      */
      if (!(XSysParams.ulOptions & XOPT_USEBLOCKINGWAIT) ||
           (wRemove == PM_NOREMOVE))
        if (!XtAppPending(XSysParams.appContext))
          return 0;

      XtAppNextEvent(XSysParams.appContext, &xevent);
      if (XSysParams.appContext && !(XSysParams.ulFlags & XFLAG_QUITTING_APP))
        XtDispatchEvent(&xevent);
      /*
        If XtDispatchEvent caused a message to be inserted into the
        MEWEL queue, then go retrieve the message.
      */
      if (!EventQueue.semQueueEmpty)
        goto peek_queue;
      return FALSE;
    }
#else
    if (EventQueue.semQueueEmpty && XPending(XSysParams.display))
    {
      XNextEvent(XSysParams.display, &xevent);
      XMEWEL_DispatchEvent(&xevent);
    }
#endif
    return 0;
  }

#else
  /*
    Use _PeekMessage() to see if there is a message in the queue. _PeekMessage()
    will do all of the low-level queue stuff.
  */
  if (!_PeekMessage(lpMsg))
  {
    /*
      If we are in a menu, send the WM_ENTERIDLE message
    */
    if (InternalSysParams.hWndFocus != NULLHWND)
    {
      WINDOW *w = WID_TO_WIN(InternalSysParams.hWndFocus);
      if (w && IS_MENU(w))
      {
        while (w && IS_MENU(w))
          w = w->parent;
        if (w)
          SendMessage(w->win_id, WM_ENTERIDLE, MSGF_MENU, 
                      MAKELONG(InternalSysParams.hWndFocus, 0));
      }
    }
    return 0;
  }
#endif

  /*
    No filtering??? Return the retrieved message.
  */
  if (wFilterMin == 0 && wFilterMax == 0)
  {
    if (lpMsg->message == WM_REMOVEDMSG)
    {
      _WinGetMessage(lpMsg);
      return FALSE;
    }
    if (wRemove & PM_REMOVE)
      _WinGetMessage(lpMsg);
    return TRUE;
  }


  /*
    There is filtering and we got a removed message. This means that
    there was a removed message at the tail of the event queue. So,
    get rid of this message and advnace the queue pointers so it
    is at the next message.
  */
  if (lpMsg->message == WM_REMOVEDMSG)
  {
    _WinGetMessage(lpMsg);
#if defined(XWINDOWS)
    goto peek_queue;
#endif
    return FALSE;
  }


  /*
    Lock other apps out of the message queue
  */
  DOSSEMSET(&EventQueue.semQueueMutex);
  SET_PROGRAM_STATE(STATE_MUST_CHECK_EVENT);

  /*
    If the window handle to examine is NULL, then return the first
    message in the queue (idxEvent == EventQueue.qtail). Otherwise
    search the event queue from tail to head, looking for an event
    which meets our criteria.
  */

  for (;;)
  {
    e = (LPMSG) QueueGetData(&EventQueue, idxEvent);
    if ((hWnd == NULLHWND || e->hwnd == hWnd || IsChild(hWnd, e->hwnd)) &&
        ((wFilterMin == 0 && wFilterMax == 0) ||        /* no filtering  */
         (e->message >= wFilterMin && e->message <= wFilterMax)))
    {
      lmemcpy((LPSTR) lpMsg, (LPSTR) e, sizeof(MSG));
      break;
    }
    /*
      Did we just examine the last element? If so, no messages were
      worthy of being returned.
    */
    if (idxEvent == EventQueue.qhead)
    {
      DOSSEMCLEAR(&EventQueue.semQueueMutex);
      CLR_PROGRAM_STATE(STATE_MUST_CHECK_EVENT);
      return FALSE;
    }

    /*
      Move onto the next message.
    */
    idxEvent = QueueNextElement(&EventQueue, idxEvent);
  }


  /*
    Instead of physically shifting the other messages, simply turn the
    retrieved message into an invalid message.
  */
  if (wRemove & PM_REMOVE)
    e->message = WM_REMOVEDMSG;

  /*
    Let other processes play with the event queue
  */
  DOSSEMCLEAR(&EventQueue.semQueueMutex);
  CLR_PROGRAM_STATE(STATE_MUST_CHECK_EVENT);

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetMessage() / WaitMessage()                                  */
/*                                                                          */
/* Purpose  : Calls PeekMessage repeatedly to wait for a message            */
/*                                                                          */
/* Returns  : TRUE if the message was not WM_QUIT                           */
/*                                                                          */
/****************************************************************************/
static DWORD ulLastMsgTime;
static POINT ptLastMessage;

BOOL FAR PASCAL GetMessage(lpMsg, hWnd, wFilterMin, wFilterMax)
  LPMSG lpMsg;
  HWND  hWnd;
  UINT  wFilterMin;
  UINT  wFilterMax;
{
  do
  {
    while (!PeekMessage(lpMsg, hWnd, wFilterMin, wFilterMax, PM_REMOVE))
      ;
  } while (lpMsg->message == WM_REMOVEDMSG);
  ulLastMsgTime = lpMsg->time;
  ptLastMessage = lpMsg->pt;
  return (BOOL) !(lpMsg->message == WM_QUIT);
}

LONG FAR PASCAL GetMessageTime(void)
{
  return (LONG) ulLastMsgTime;
}

DWORD FAR PASCAL GetMessagePos(void)
{
  return MAKELONG(ptLastMessage.x, ptLastMessage.y);
}

VOID FAR PASCAL WaitMessage(void)
{
  MSG msg;

  while (!PeekMessage(&msg, NULLHWND, 0, 0, PM_NOREMOVE))
    ;
}


/****************************************************************************/
/*                                                                          */
/* Function : SendMessage() / InSendMessage()                               */
/*                                                                          */
/* Purpose  : Sends a message directly to a window proc.                    */
/*                                                                          */
/* Returns  : The result from the winproc.                                  */
/*                                                                          */
/****************************************************************************/
static int iSendMsgCount = 0;   /* for InSendMessage() */

LRESULT FAR PASCAL SendMessage(hWnd, msg, wParam, lParam)
  HWND   hWnd;
  UINT   msg;
  WPARAM wParam;
  LPARAM lParam;
{
#ifdef DDE_SHMEM
  DWORD   rc = 0;
 /*
  * Tricky stuff to avoid us from DDE broadcasting an incoming DDE broadcast.
  * It so happens that Mewel-generated broadcasts use window handle 0xffff.
  * As long as application-generated broadcasts use -1 (Oxffffffff) we can
  * distinguish them from each other. Mewel-generated broadcasts are not
  * forwarded via DDE.
  */
  if (hWnd & ~0xffff)
  {
    rc = dde_send(hWnd, msg, wParam, lParam);
    if (hWnd != -1)
      return rc;
    hWnd = 0xffff;
  }
  {
#endif

  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD   rc = 0;
  
  iSendMsgCount++;

  /*
    Broadcast message to all top-level windows
  */
  if (hWnd == 0xFFFF)
  {
    rc = SendMessage(_HwndDesktop, msg, wParam, lParam);
    if (IsWindow(_HwndDesktop))
      for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
        if (w->winproc)
          rc = SendMessage(w->win_id, msg, wParam, lParam);
  }

  /*
    Call the window proc directly...
  */
  else if (hWnd && w && w->winproc)
  {
    if (InternalSysParams.pHooks[WH_CALLWNDPROC])
      _WinProcessWndProcHook(&hWnd, &msg, &wParam, &lParam);
    if (InternalSysParams.pHooks[WH_COMMDLG])
      if (msg != WM_INITDIALOG)
        (* (COMMDLGHOOKPROC *) InternalSysParams.pHooks[WH_COMMDLG]->lpfnHook)
           (hWnd,msg,wParam,lParam);
    rc = (DWORD) (*(w->winproc))(hWnd, msg, wParam, lParam);
  }

  iSendMsgCount--;
  return rc;
#ifdef DDE_SHMEM
  }
#endif
}

BOOL FAR PASCAL InSendMessage(void)
{
  return (BOOL) (iSendMsgCount != 0);
}


/****************************************************************************/
/*                                                                          */
/* Function : CallWindowProc()                                              */
/*                                                                          */
/* Purpose  : Calls the passed window procedure                             */
/*                                                                          */
/* Returns  : Whatever the window procedure returns                         */
/*                                                                          */
/****************************************************************************/
LONG FAR PASCAL CallWindowProc(lpPrevWndFunc, hWnd, message, wParam, lParam)
  FARPROC lpPrevWndFunc;
  HWND    hWnd;
  UINT    message;
  WPARAM  wParam;
  LPARAM  lParam;
{
  if (lpPrevWndFunc)
  {
    if (InternalSysParams.pHooks[WH_CALLWNDPROC])
      _WinProcessWndProcHook(&hWnd, &message, &wParam, (LONG *) &lParam);
    return (* (WINPROC *) lpPrevWndFunc)(hWnd, message, wParam, lParam);
  }
  else
    return 0L;
}


/*===========================================================================*/
/*                                                                           */
/* This code handles the WH_CALLWNDPROC hook which is needed by message      */
/* dumping routines and by things like Microsoft's AFX.                      */
/*                                                                           */
/*===========================================================================*/
typedef struct tagCallWndProcHookData
{
#ifdef MEWEL_32BITS
  UINT   lParam;
#else
  USHORT hlParam;
  USHORT llParam;
#endif
  WPARAM wParam;
  UINT   wMsg;
  HWND   hWnd;
} CALLWNDPROCHOOKDATA, FAR *LPCALLWNDPROCHOOKDATA;


VOID FAR PASCAL _WinProcessWndProcHook(phWnd, pmessage, pwParam, plParam)
  HWND   *phWnd;
  UINT   *pmessage;
  WPARAM *pwParam;
  LPARAM *plParam;
{
  CALLWNDPROCHOOKDATA hookData;

  if (InternalSysParams.pHooks[WH_CALLWNDPROC])
  {
    hookData.hWnd    = *phWnd;
    hookData.wMsg    = *pmessage;
    hookData.wParam  = *pwParam;
#ifdef MEWEL_32BITS
    hookData.lParam  = *plParam;
#else
    hookData.llParam = LOWORD(*plParam);
    hookData.hlParam = HIWORD(*plParam);
#endif

    (* (WNDPROCHOOKPROC *) InternalSysParams.pHooks[WH_CALLWNDPROC]->lpfnHook)
         (0, 1, (DWORD) (LPCALLWNDPROCHOOKDATA) &hookData);

    *phWnd           = hookData.hWnd;
    *pmessage        = hookData.wMsg;
    *pwParam         = hookData.wParam;
#ifdef MEWEL_32BITS
    *plParam         = hookData.lParam;
#else
    *plParam         = MAKELONG(hookData.llParam, hookData.hlParam);
#endif
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : PostMessage()                                                 */
/*                                                                          */
/* Purpose  : Places a message onto the message queue for subsequent        */
/*            message.                                                      */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL PostMessage(hWnd, msg, wParam, lParam)
  HWND   hWnd;
  UINT   msg;
  WPARAM wParam;
  LPARAM lParam;
{
#ifdef DDE_SHMEM
  DWORD   rc = 0;
 /*
  * Tricky stuff to avoid us from DDE broadcasting an incoming DDE broadcast.
  * It so happens that Mewel-generated broadcasts use window handle 0xffff.
  * As long as application-generated broadcasts use -1 (Oxffffffff) we can
  * distinguish them from each other. Mewel-generated broadcasts are not
  * forwarded via DDE.
  */
  if (hWnd & ~0xffff) {
    rc = dde_post(hWnd, msg, wParam, lParam);
    if (hWnd != -1)
      return rc;
    hWnd = 0xffff;
  }
  {
#endif

  /* Broadcast message to all top-level windows */
  if (hWnd == 0xFFFF)
  {
    INT rc;
    WINDOW *w;

    rc = PostEvent(_HwndDesktop, msg, wParam, lParam, BIOSGetTime());
    if (IsWindow(_HwndDesktop))
      for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
        if (w->winproc)
          rc = PostEvent(w->win_id, msg, wParam, lParam, BIOSGetTime());
    return rc;
  }

  return PostEvent(hWnd, msg, wParam, lParam, BIOSGetTime());
#ifdef DDE_SHMEM
  }
#endif
}



/****************************************************************************/
/*                                                                          */
/* Function : TranslateMessage()                                            */
/*                                                                          */
/* Purpose  : Translates ALT-keys into WM_COMMAND messages for menus        */
/*                                                                          */
/* Returns  : TRUE if it was translated.                                    */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL TranslateMessage(lpMsg)
  LPMSG lpMsg;
{
  UINT    key;
  int     id, iPos, chAltKey;
  HWND    hRoot;
  HMENU   hMenu;
  WINDOW  *fw;
  UINT    fsState;
  HWND    hFocus;

#if defined(XWINDOWS)
  return TRUE;
#else

  /*
    Only handle WM_CHAR messages
  */
  if (lpMsg->message != WM_CHAR && lpMsg->message != WM_KEYDOWN && 
      lpMsg->message != WM_SYSKEYDOWN)
    return FALSE;

  key = lpMsg->wParam;

  /*
    Make sure that we have a window which has the focus or is the active window

    9/3/92 (maa)
      If lpMsg->hwnd points to a window, use that window as the root
    for the menu search.
  */
  if ((hFocus = lpMsg->hwnd) == NULL)
  {
    if ((hFocus = InternalSysParams.hWndFocus) == NULLHWND)
      if ((hFocus = GetActiveWindow()) == NULLHWND)
        return FALSE;
  }

  /*
    If the key code is above 127, then it is possibly an ALT-key combo.
    If it is an ALT-key combo, then it may invoke a pulldown-menu item.
    Trace up from the focus window and see if it leads to a menubar.
  */
#if defined(INTERNATIONAL_MEWEL)
  if ((key & 0xFF00) && 
#elif defined(CDPLUS)
  /* we at CDPLUS want to be able to translate CTRL_keys as well! */
  if (
#else
  if (key >= 128 && 
#endif
      (hRoot = WinGetMenuRoot(hFocus)) != NULLHWND &&
      (fw = WID_TO_WIN(hRoot)) != NULL && (hMenu = fw->hMenu) != NULLHWND)
  {
    /*
      It leads to a menubar. See if the key corresponds to one of the
      menubar items. If it does, invoke that menubar item.
    */
    if ((id = IsMenuBarAccel(hMenu, key, &chAltKey, &iPos)) >= 0)
    {
      /*
        If we are already in a menu and we press an accelerator key,
        then first kill the current menu.
      */
      if (WinGetStyle(hFocus) & WIN_IS_MENU)
        SendMessage(hFocus, WM_KILLFOCUS, 0, 0L);


do_newitem:
      /*
         Don't translate an accelerator for a disabled menu item
      */
      fsState = GetMenuState(hMenu, (UINT) iPos, MF_BYPOSITION);
      if (fsState & MF_DISABLED)
      {
        MessageBeep(0);
        return TRUE;
      }

      /*
        Now set the focus to the menu, but only if it contains a popup.
      */
      if (fsState & MF_POPUP)
        SetFocus(hMenu);

      /*
        Change the message into a WM_COMMAND message which will directed
        at the menubar.
      */
      lpMsg->hwnd    = (fsState & MF_POPUP) ? hMenu : hRoot;
      lpMsg->message = WM_COMMAND;
      lpMsg->wParam  = (WPARAM) id;
      lpMsg->lParam  = (fsState & MF_POPUP) ? MAKELONG(hMenu, 1) : 0L;
      return TRUE;
    }
    else
    {
      /*
        It's not a menubar item and not an accelerator.
        Send a WM_MENUCHAR message to the menu owner as per MS-Windows.
        The return value is a long value composed of a return code
        in the HIWORD and a possible menu id in the LOWORD.
        If the parent window returns 0, discard the char and beep.
        If 1 is returned, then close the menu. If a number > 1 is
        returned, then LOWORD is interpreted as a menuitem id to invoke.
      */
      if (chAltKey)
      {
        DWORD ulRet;
        ulRet = (DWORD) SendMessage(hRoot, WM_MENUCHAR, (WPARAM) chAltKey,
                                    MAKELONG(MF_POPUP, hMenu));
        if (HIWORD(ulRet) >= 1)
        {
          if (WinGetStyle(hFocus) & WIN_IS_MENU)
            SendMessage(hFocus, WM_KILLFOCUS, 0, 0L);
          if (HIWORD(ulRet) == 1)
            return TRUE;            /* close the menu... */
          else
          {
            id = LOWORD(ulRet);
            /*
              id is a position number. We must change it into a command id.
            */
            id = GetMenuItemID(hMenu, id);
            goto do_newitem;
          }
        } /* end if (HIWORD...) */
      } /* end if (chAltKey) */
    } /* end else */
  } 


  /*
    Beep if we pressed an ALT key which doesn't invoke a menu
  */
#ifdef INTERNATIONAL_MEWEL
  if ((key & 0xFF00) && (key & ALT_SHIFT))
#else
  if (key >= 128 && AltKeytoLetter(key))
#endif
    MessageBeep(0);
  return FALSE;
#endif /* XWINDOWS */
}


/****************************************************************************/
/*                                                                          */
/* Function : DispatchMessage()                                             */
/*                                                                          */
/* Purpose  : Sends a message to the proper window proc                     */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL DispatchMessage(lpMsg)
  LPMSG  lpMsg;
{
  WINDOW *w;
  HWND   hNew, hParent;
  HWND   hWnd;
  INT    row, col;
  UINT   message;
  WPARAM wParam;
  DWORD  lParam;
  
  /*
    Process all messages which originated from an input device (WM_CHAR and
    the mouse messages), and determine the proper owner.
  */
  SysEventInfo.lastMessage = message = lpMsg->message;
  wParam  = lpMsg->wParam;
  lParam  = lpMsg->lParam;


  switch (message)
  {
    case WM_ALT        :
      Process_WM_ALT();
      break;

    case WM_CHAR       :
    case WM_SYSCHAR    :
    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
#if !defined(MOTIF)
      /*
        See if we got <ALT-SPACE>. If so, try and invoke a system menu.
      */
#if defined(DOS) || defined(OS2)
      if (wParam == VK_SPACE && (lParam & 0x20000000L) != 0L)
#else
      if (wParam == VK_CTRL_F1)
#endif
      {
        if (_WinFindandInvokeSysMenu(lpMsg->hwnd))
          return FALSE;
      }

      /*
        Both ALT and F10 (alone) invoke the menubar
      */
      else if (wParam == VK_F10)
      {
        PostMessage(lpMsg->hwnd, WM_ALT, 0, 0L);
        return TRUE;  /* don't pass the WM_CHAR/VK_F10 message on */
      }

      else
#endif /* MOTIF */

      /*
        Send out a WM_SYSCOMMAND message with SC_NEXT/PREVWINDOW if we
        got an <CTRL-F6> or <CTRL-SH-F6> key.
        Send out a WM_SYSCOMMAND message with SC_CLOSE if we got an <ALT-F4>.
      */
#if defined(USE_WINDOWS_COMPAT_KEYS)
      if (wParam == VK_F6  && GetKeyState(VK_CONTROL) || 
          wParam == VK_TAB && GetKeyState(VK_CONTROL) ||
          message == WM_SYSKEYDOWN && wParam == VK_F4  && GetKeyState(VK_MENU))
#else
      if ((wParam & ~(LEFT_SHIFT | RIGHT_SHIFT)) == VK_CTRL_F6  || 
           (wParam & ~(LEFT_SHIFT | RIGHT_SHIFT)) == VK_CTRL_TAB ||
            wParam == VK_ALT_F4)
#endif
      {
        SysEventInfo.lastMessage = message = lpMsg->message = WM_SYSCOMMAND;
#if defined(USE_WINDOWS_COMPAT_KEYS)
        if (wParam == VK_F4)
#else
        if (wParam == VK_ALT_F4)
#endif
        {
          wParam = SC_CLOSE;

          /*
            If we are in a MDI window or another child window, make sure
            that the ALT_F4 is sent to the first window with a system menu.
          */
          w = WID_TO_WIN(lpMsg->hwnd);
          while (w && w != InternalSysParams.wDesktop && 
                   (w->hSysMenu == NULL || IS_MDIDOC(w)))
            w = w->parent;
          lpMsg->hwnd = (w) ? w->win_id : NULL;
        }
        else
#if defined(USE_WINDOWS_COMPAT_KEYS)
          wParam = (GetKeyState(VK_SHIFT))
#else
          wParam = (LOBYTE(wParam) & (LEFT_SHIFT | RIGHT_SHIFT))
#endif
                            ? (WPARAM) SC_PREVWINDOW : (WPARAM) SC_NEXTWINDOW;
        lpMsg->wParam = wParam;
        lParam = lpMsg->lParam  = 0L;
      }
      break;


    /*
      Note - these two case's look like this so we can debug mouse messages
      easier in Codeview...
    */
    case WM_LBUTTONDBLCLK:
      row = (INT) HIWORD(lParam);
      /* fall into */

    case WM_LBUTTONDOWN :
      row = (INT) HIWORD(lParam);
      /* fall into */

    case WM_NCLBUTTONDOWN :
      row = (INT) HIWORD(lParam);
      /* fall into */

    case WM_MOUSEMOVE   :
    case WM_LBUTTONUP   :
    case WM_RBUTTONDOWN :
    case WM_RBUTTONUP   :
    case WM_RBUTTONDBLCLK:

    case WM_NCMOUSEMOVE   :
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONUP   :
    case WM_NCRBUTTONDOWN :
    case WM_NCRBUTTONUP   :
    case WM_NCRBUTTONDBLCLK:

    case WM_MOUSEREPEAT :
      /*
        If we have the mouse captured, send the mouse msg to that window.
      */
      if (lpMsg->hwnd == InternalSysParams.hWndCapture && IsWindow(lpMsg->hwnd))
        break;

      row = (INT) HIWORD(lParam);
      col = (INT) LOWORD(lParam);

      /*
        Find out which window the mouse was over...
      */
      if ((hWnd = lpMsg->hwnd) != NULLHWND)
      {
        INT hitCode;

        w = WID_TO_WIN(hWnd);

        /*
          Get the hit-test code. Always use the screen-based coordinates.
        */
        if (IS_WM_MOUSE_MSG(message))
        {
#if defined(MOTIF)
          /*
            Hitcode should only be used for NC mouse message
          */
          hitCode = HTCLIENT;
#else
          /*
            Derive screen coords from client coords.
          */
          DWORD lP2 = _UnWindowizeMouse(lpMsg->hwnd, lParam);
          hitCode = (INT) SendMessage(hWnd, WM_NCHITTEST, message, lP2);
#endif
        }
        else
          hitCode = wParam;

        /*
          Processing for button-down messages (yes, Windows uses the
          right button too!)
        */
        if (message==WM_LBUTTONDOWN   || message==WM_RBUTTONDOWN ||
            message==WM_NCLBUTTONDOWN || message==WM_NCRBUTTONDOWN)
        {
          /*
            Send a WM_NCHITTEST message to the window to see if we should
            set the focus to it. (Use col as a scrap var for the ret code)
          */
          BOOL bIsScrollbar = (BOOL) (w->idClass == SCROLLBAR_CLASS);

          /*
            We should set the focus to the control if the hit code was
            either HTCLIENT or a border/caption/sysmenu/sizebox/...
          */
          if (bIsScrollbar || 
              (hitCode != HTERROR && hitCode != HTNOWHERE && 
              hitCode != HTTRANSPARENT))
          {
            hParent = GetParentOrDT(hWnd);
            hNew = (bIsScrollbar && !(w->flags & SBS_CTL)) ? hParent : hWnd;

            /*
              If we are changing the focus within a dialog box, then do the
              MEWEL-specific data validation.
            */
            if (IS_DIALOG(WID_TO_WIN(hParent)))
            {
              extern DWORD _dwValidateReason;
              _dwValidateReason = MAKELONG(hWnd, 
                (message == WM_LBUTTONDOWN) ? VR_LBUTTONDOWN : VR_RBUTTONDOWN);
              if (!_DlgSetFocus(hWnd,FALSE))
                return TRUE;
            }
                
            if (IS_MENU(w))
            {
              SET_PROGRAM_STATE(STATE_MOUSED_ON_MENU);
              if (hWnd != InternalSysParams.hWndFocus)
              {
                /*
                  If we are in a cascaded submenu which we invoked via
                  the keyboard, and we click the mouse on the menubar,
                  we must make sure that the submenu and its parent
                  menus are killed. The lparam must be 0L (KILL_PARENT_FOCUS)
                */
                if (WinGetStyle(InternalSysParams.hWndFocus) & WIN_IS_MENU)
                  SendMessage(InternalSysParams.hWndFocus,WM_KILLFOCUS,hNew,0L);
                SetFocus(hNew);
              }
            }
            else
            {
              HWND hOldActive = GetActiveWindow();
              HWND hWndRoot   = _WinGetRootWindow(hNew);
              INT  rcACT      = MA_ACTIVATE;

              /*
                We can have the situation where we click and release
                the mouse on a menubar; the pulldown menu has the
                focus but not the capture. Then we click on the client
                area of an MDI window. If these two things happen,
                the caption bars of the MDI child windows will be
                screwed up. So, let's watch for that here.
              */
              if (WinGetStyle(InternalSysParams.hWndFocus) & WIN_IS_MENU)
              {
                SendMessage(InternalSysParams.hWndFocus, WM_KILLFOCUS, 0,
                            MAKELONG(hNew,0));
                hOldActive = GetActiveWindow();
              }

              /*
                Activate the root window if it is not the currently
                active window.
              */
              if (message != WM_RBUTTONDOWN && message != WM_NCRBUTTONDOWN)
                if (hWndRoot != hOldActive)
                  rcACT = (INT) SendMessage(hNew, WM_MOUSEACTIVATE, hWndRoot, 
                                            MAKELONG(hitCode, message));

#if 101992
              (void) rcACT;
              if (message != WM_RBUTTONDOWN && message != WM_NCRBUTTONDOWN)
                SetFocus(hNew);
#else
/*
  Taking out this code makes MEWEL work the way that Windows does, even in
  the Child Demo of MEWLDEMO. If we click on the client area of one of the
  children, the child should *not* get activation messages. Only the root
  window should get the activation messages. MEWEL handles the WM_MOUSEACTIVATE
  message by calling SetActiveWindow() to the root window, so we do not need
  to do this code anymore.
*/
              /*
                If the activation succeeded, then we need to make sure
                that the window which we clicked on is the new window
                which has the input focus. 
              */
              if (rcACT != MA_NOACTIVATE && hNew != InternalSysParams.hWndFocus)
              {
                SendMessage(hNew, WM_ACTIVATE, 2, MAKELONG(hOldActive, 0));
                SendMessage(hNew, WM_NCACTIVATE, TRUE, 0L);
              }
#endif
            }
          }
          else if (hitCode == HTERROR)
            MessageBeep(0);

          if (message == WM_LBUTTONDOWN                    &&
              (hitCode == HTNOWHERE || hitCode == HTERROR) &&
               (WinGetStyle(InternalSysParams.hWndFocus) & WIN_IS_MENU))
          {
            /*
              If we are in a menu and we click the button, then we should
              dismiss the menu no matter what the hit-test results were.
            */
            SendMessage(InternalSysParams.hWndFocus, message, wParam, lParam);
          }

          /*
            For dialog boxes, let the WM_LBUTTONDOWN message through so
            the dlg box can be moved and the system menu invoked.
          */
          if ((hitCode == HTNOWHERE && !IS_DIALOG(w) && !bIsScrollbar) || 
               hitCode == HTERROR)
            return TRUE;

        }  /* message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN */

        /*
          Set the mouse cursor to the appropriate shape, and pass the
          mouse message on to the window. Do this only if the mouse
          is not being captured.
        */
        if (InternalSysParams.hWndCapture == NULLHWND)
          SendMessage(hWnd, WM_SETCURSOR, hWnd, MAKELONG(hitCode,message));

        /*
          Send the WM_PARENTNOTIFY message.
        */
        if (message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN ||
                                       message==WM_MBUTTONDOWN)
          WinParentNotify(hWnd, message, MAKELONG(col, row));

        lpMsg->hwnd  = hWnd;
        SysEventInfo.lastMessage = message;
      } /* hWnd != NULLHWND */
      else
      {
        /*
          hWnd is NULL, which probably means it's the desktop window.
          Remember to reset the cursor.
        */
        SendMessage(_HwndDesktop, WM_SETCURSOR, _HwndDesktop, MAKELONG(HTCLIENT,message));
      }
      break;

  } /* switch (message) */


  /*
    Send the message onto the window's winproc
  */
  if (lpMsg->hwnd)
  {
    WINDOW *w = WID_TO_WIN(lpMsg->hwnd);
    if (w)
      CallWindowProc((FARPROC)w->winproc, lpMsg->hwnd, message, wParam, lParam);
  }

  CLR_PROGRAM_STATE(STATE_MOUSED_ON_MENU);
  return TRUE;
}



/*===========================================================================*/
/*                                                                           */
/* Purpose : PostQuitMessage()                                               */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL PostQuitMessage(exitcode)
  INT exitcode;
{
#if defined(MOTIF)
  XSysParams.ulFlags |= XFLAG_QUITTING_APP;
#if 0
  XtDestroyApplicationContext(XSysParams.appContext);
  XSysParams.appContext = 0;
  exit(0);
#endif
#endif

  return (INT) PostMessage(0xFFFF, WM_QUIT, (WPARAM) exitcode, 0L);
}


/*===========================================================================*/
/*                                                                           */
/* File    : WDEFPROC.C                                                      */
/*                                                                           */
/* Purpose : Implements the DefWinProc() function                            */
/*                                                                           */
/*===========================================================================*/
LRESULT FAR PASCAL DefWindowProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  WINDOW  *w;
  WINPROC *defproc;

  /*
    Make sure we are passed a valid handle. If so, get a pointer to the
    default winproc for the window class and call it.
  */
  if ((w = WID_TO_WIN(hWnd)) != (WINDOW *) NULL)
  {
    /*
      In an effort to speed up the message processing path, let's try
      bring some of the window-class code in-line. This way, we won't
      be slowed down by calls to ClassIDToDefProc and ClassIDToClassStruct.
    */
    if (w->idClass < MAXCLASSES)
      return (*ClassIndex[w->idClass]->lpfnDefWndProc)(hWnd,message,wParam,lParam);

    if ((defproc = (WINPROC *) ClassIDToDefProc(w->idClass)) != (WINPROC *) 0)
    {
      if (InternalSysParams.pHooks[WH_CALLWNDPROC])
        _WinProcessWndProcHook(&hWnd, &message, &wParam, &lParam);
      return (*defproc)(hWnd, message, wParam, lParam);
    }
  }
  return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WindowizeMouse()                                             */
/*                                                                          */
/* Purpose  : Translates mouse coords from screen to window relative        */
/*                                                                          */
/* Returns  : Mouse coordinates in window-relative terms                    */
/*                                                                          */
/****************************************************************************/
LONG FAR PASCAL _WindowizeMouse(HWND hwnd, LONG lMouse)
{
  int  xMouse;
  int  yMouse;
  RECT rc;

  if (hwnd == NULLHWND)
    return lMouse;

  /*
    x is in low-order of lMouse
    y is in high-order of lMouse
  */
  WinGetClient(hwnd, &rc);
  xMouse = LOWORD(lMouse) - rc.left;
  yMouse = HIWORD(lMouse) - rc.top;
  return MAKELONG(xMouse, yMouse);
}

LONG FAR PASCAL _UnWindowizeMouse(HWND hwnd, LONG lMouse)
{
  int  xMouse;
  int  yMouse;
  RECT rc;

  if (hwnd == NULLHWND)
    return lMouse;

  /*
    x is in low-order of lMouse
    y is in high-order of lMouse
  */
  WinGetClient(hwnd, &rc);
  xMouse = LOWORD(lMouse) + rc.left;
  yMouse = HIWORD(lMouse) + rc.top;
  return MAKELONG(xMouse, yMouse);
}


/*
  If there is a window in focus, then send the keystroke to that
  window. If not, try the active window. If not, then pass it onto
  the desktop window.
*/
static HWND PASCAL DetermineKeyWindow(void)
{
  return InternalSysParams.hWndFocus ? InternalSysParams.hWndFocus :
           (InternalSysParams.hWndActive ? InternalSysParams.hWndActive :
               _HwndDesktop);
}


/*===========================================================================*/
/*                                                                           */
/* WinGetMenuRoot() - Given the handle to a window, returns the handle of    */
/*   the first ancestor which has a menubar associated with it.              */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL WinGetMenuRoot(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  
  /*
    Go up the parent chain until we hit a window with a menu attached to it.
  */
  while (w && !w->hMenu)
  {
    /* 
      Make sure we are not a menu ourselves
    */
    if (IS_MENU(w))
      return NULLHWND;

    /*
      If the window or any of its ancestors are disabled, then the menu cannot
      be accessed.
    */
    if (!IsWindowEnabled(hWnd))
      return NULLHWND;

    /*
      Go up the parent chain to the next highest ancestor
    */
    if ((hWnd = w->hWndOwner) == NULLHWND)
      return NULLHWND;
    w = WID_TO_WIN(hWnd);
  }

  /*
    Final check for a disabled state.
  */
  return IsWindowEnabled(hWnd) ? hWnd : NULLHWND;
}


static VOID PASCAL Process_WM_ALT(void)
{
  HWND hRoot;

  /*
    See if the current focus (or active) window has a menubar. If so,
    then set the focus to the menubar.
  */
  if ((hRoot = InternalSysParams.hWndFocus) == NULL)
    hRoot = GetActiveWindow();

  /*
    Don't activate an iconic or disabled window
  */
  if (IsIconic(hRoot) || !IsWindowEnabled(hRoot))
    return;

  if (hRoot && (hRoot = WinGetMenuRoot(hRoot)) != NULL && GetMenu(hRoot))
  {
    SysEventInfo.lastMessage = WM_ALT;   /* need this for MenuWndProc(WM_SETFOCUS) */
    SetFocus(GetMenu(hRoot));
    return;
  }

  /*
    If a menu has the focus when the ALT key is tapped, then remove
    the focus from the menu.
  */
  if (InternalSysParams.hWndFocus && 
      IS_MENU(WID_TO_WIN(InternalSysParams.hWndFocus)))
    SendMessage(InternalSysParams.hWndFocus, WM_KILLFOCUS, 0, 0L);
}


#ifdef DEBUG
static VOID PASCAL DumpMouseMessage(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  PSTR pMouseMsg;
  PSTR pHitCode;
  char  szBuf[96];
  HDC   hDC;
  RECT  r;
  TEXTMETRIC tm;

static char *aszHitCode[] =
{
"HTNOWHERE",
"HTCLIENT",
"HTCAPTION",
"HTSYSMENU",
"HTSIZE",
"HTMENU",
"HTHSCROLL",
"HTVSCROLL",
"HTMINBUTTON",
"HTMAXBUTTON",
"HTLEFT",
"HTRIGHT",
"HTTOP",
"HTTOPLEFT",
"HTTOPRIGHT",
"HTBOTTOM",
"HTBOTTOMLEFT",
"HTBOTTOMRIGHT",
"HTBORDER",
};

  hDC = GetDC(_HwndDesktop);
  GetClientRect(_HwndDesktop, &r);
  GetTextMetrics(hDC, &tm);

  r.top = r.bottom - (tm.tmHeight + tm.tmExternalLeading);
  FillRect(hDC, &r, GetStockObject(WHITE_BRUSH));

  switch (message)
  {
    case WM_LBUTTONDBLCLK:
      pMouseMsg = "WM_LBUTTONDBLCLK";
      break;
    case WM_LBUTTONDOWN :
      pMouseMsg = "WM_LBUTTONDOWN";
      break;
    case WM_MOUSEMOVE   :
      pMouseMsg = "WM_MOUSEMOVE";
      break;
    case WM_LBUTTONUP   :
      pMouseMsg = "WM_LBUTTONUP";
      break;
    case WM_RBUTTONDOWN :
      pMouseMsg = "WM_RBUTTONDOWN";
      break;
    case WM_RBUTTONUP   :
      pMouseMsg = "WM_RBUTTONUP";
      break;
    case WM_RBUTTONDBLCLK:
      pMouseMsg = "WM_RBUTTONDBLCLK";
      break;
    case WM_NCMOUSEMOVE   :
      pMouseMsg = "WM_NCMOUSEMOVE";
      break;
    case WM_NCLBUTTONDOWN :
      pMouseMsg = "WM_NCLBUTTONDOWN";
      break;
    case WM_NCLBUTTONDBLCLK:
      pMouseMsg = "WM_NCLBUTTONDBLCLK";
      break;
    case WM_NCLBUTTONUP   :
      pMouseMsg = "WM_NCLBUTTONUP";
      break;
    case WM_NCRBUTTONDOWN :
      pMouseMsg = "WM_NCRBUTTONDOWN";
      break;
    case WM_NCRBUTTONUP   :
      pMouseMsg = "WM_NCRBUTTONUP";
      break;
    case WM_NCRBUTTONDBLCLK:
      pMouseMsg = "WM_NCRBUTTONDBLCLK";
      break;
    case WM_MOUSEREPEAT :
      pMouseMsg = "WM_MOUSEREPEAT";
      break;

    default :
      pMouseMsg = "Unknown";
      break;
  }

  if (wParam>=HTNOWHERE && wParam<=HTBORDER)
    pHitCode = aszHitCode[wParam];
  else if (wParam == HTERROR)
    pHitCode = "HTERROR";
  else if (wParam == HTTRANSPARENT)
    pHitCode = "HTTRANSPARENT";
  else
    pHitCode = "???";

  sprintf(szBuf, "Hwnd=%d,  x=%d,  y=%d,  msg=%s,  hit=%s",
          hWnd, LOWORD(lParam), HIWORD(lParam), pMouseMsg, pHitCode);

  TextOut(hDC, 0, r.top, szBuf, strlen(szBuf));

  ReleaseDC(_HwndDesktop, hDC);
}
#endif

VOID PASCAL MEWELWriteDebugString(pStr)
  PSTR pStr;
{
  HDC   hDC;
  RECT  r;
  TEXTMETRIC tm;

  hDC = GetDC(_HwndDesktop);
  GetClientRect(_HwndDesktop, &r);
  GetTextMetrics(hDC, &tm);

  r.top = r.bottom - (tm.tmHeight + tm.tmExternalLeading);
  FillRect(hDC, &r, GetStockObject(WHITE_BRUSH));
  TextOut(hDC, 0, r.top, pStr, strlen(pStr));

  ReleaseDC(_HwndDesktop, hDC);
}

