/*===========================================================================*/
/*                                                                           */
/* File    : WINEVENT.C                                                      */
/*                                                                           */
/* Purpose : Low-level event-handling routines.                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
/*
  Thomas Wagner's international keyboard driver does not work correctly
  with MS-DOS 5. As I am not able to find the bug, switching it totally off
  is the easiest solution -- Wolfgang Lorenz
*/
#undef INTERNATIONAL_MEWEL

#include "wprivate.h"
#include "window.h"
#include "winevent.h"


#ifdef INTERNATIONAL_MEWEL
#define ALT_TAP MAKEKEY(0x38, ALT_SHIFT)
#endif

SYSEVENTINFO SysEventInfo =
{
  0,        /* chLastKey   */
  0,        /* chLastShift */
  0,        /* chLastScan  */
  WM_NULL,  /* lastMessage */
  0L,       /* lParam      */
  0,        /* nAltKeyPresses */
  NULL,     /* event check proc */
  0,        /* dbl click pending msg */
};

static VOID PASCAL PostKeyEvent(UINT);
extern VOID FAR PASCAL _MEWELtoWindows_WMCHAR(WPARAM *, LPARAM *);

/***************************************************************************/
/*                                                                         */
/*               ROUTINE TO GET A RAW EVENT FROM THE SYSTEM                */
/*                                                                         */
/***************************************************************************/

UINT FAR PASCAL GetEvent(lpMsg)
  LPMSG lpMsg;
{
  INT  c;
  BOOL bPostIt;
  UINT wMsg;
  LONG lParam;

  static UINT bSendWMKEYUP = 0;  /* either 0, WM_KEYUP, or WM_SYSKEYUP */

  /*
    If we have an old image on the screen, then refresh it.
  */
  RefreshInvalidWindows(_HwndDesktop);

  /*
    See if we should actually post events, or if we are just checking
    the hardware queue.
  */
  bPostIt = (BOOL) (TEST_PROGRAM_STATE(STATE_JUST_POLL_EVENTS) == 0L);

  /*
    Is there a keyboard event waiting?
  */
  if (TEST_PROGRAM_STATE(STATE_PUSHBACKCHAR_WAITING) && (c = ngetc()) != 0 ||
      (c = KBDRead()) != 0)
  {
    if (bPostIt)
    {
#ifdef INTERNATIONAL_MEWEL
      if (c == ALT_TAP)
         PostEvent(NULLHWND, WM_ALT, 0, 0L, BIOSGetTime());
      else
#endif
      {
        /*
          If we pressed a different key than the last key, then send
          the WM_KEYUP for the old key before we send the WM_KEYDOWN
          for the new one.
        */
        if (bSendWMKEYUP && ((UINT) c) != SysEventInfo.chLastKey)
        {
          PostKeyEvent(bSendWMKEYUP);
          bSendWMKEYUP = FALSE;
          SysEventInfo.chLastKey = 0;
        }

        SysEventInfo.chLastKey   = c;
        SysEventInfo.chLastShift = KBDGetShift();
        lParam = MAKELONG(SysEventInfo.chLastShift, 0);

        /*
          Translate into a Windows-compatible key message?
        */
        _MEWELtoWindows_WMCHAR((WPARAM *) &c, (LPARAM *) &lParam);
        SysEventInfo.lParam = lParam;

        /*
          If the ALT key was pressed, issue a WM_SYSKEYDOWN, Otherwise,
          issue a WM_KEYDOWN message.
        */
        wMsg = (lParam & 0x20000000L) ? WM_SYSKEYDOWN : WM_KEYDOWN;

        /*
          Post a WM_(SYS)KEYDOWN message
        */
        PostKeyEvent(wMsg);


#ifdef RAPID
        /* 
          Some codes can only be generated in conjunction with the alt key
          on german / international keyboards. In such cases we have to
          clear the Alt-shift-flag, so that MEWEL doesn't assume that this
          is a Mnemonic or Accelerator key.
        */
        if ((c == 178)
            || (c == 'ü')
            || (c == '|')
            || (c == '{')
            || (c == '[')
            || (c == ']')
            || (c == '}')
            || (c == '\\')
            || (c == '~')
            || (c == 181))
          SysEventInfo.chLastShift &= ~ALT_SHIFT;
#endif

        /* 
          Only send WM_CHAR when it is a ANSI char

          Note : if (wMsg == WM_SYSKEYDOWN), then we should post
                 a WM_SYSCHAR message, not a WM_CHAR message.
                 Also, MS Windows sends a WM_CHAR for all of
                 the character below 32.
        */
        if ((c & 0xFF00) == 0 /*&& (c >= 32 || c == 21)*/)
          PostKeyEvent(WM_CHAR);

        /*
          We may need to post a corresponding WM_KEYUP message.
        */
        bSendWMKEYUP = (wMsg == WM_KEYDOWN) ? WM_KEYUP : WM_SYSKEYUP;
      }
    }
    return TRUE;
  }


  /*
    If there is no keyboard event pending, and we need to send a
    WM_KEYUP message, then do so.
  */
  if (bSendWMKEYUP)
  {
    PostKeyEvent(bSendWMKEYUP);
    bSendWMKEYUP = FALSE;
    SysEventInfo.chLastKey = 0;
    return TRUE;
  }

  /*
    Was the ALT key tapped?
  */
#ifndef INTERNATIONAL_MEWEL
  if (SysEventInfo.nAltKeyPresses > 0)
  {
    if (bPostIt)
    {
      PostEvent(NULLHWND, WM_ALT, 0, 0L, BIOSGetTime());
      SysEventInfo.nAltKeyPresses--;
    }
    else
      return TRUE;
  }
#endif


  /*
    Check for mouse events.
  */
  if (GetMouseMessage(lpMsg))
  {
    /*
      wParam is the shift status
      lParam is column (LOWORD) and row (HIWORD)
    */
    if (bPostIt)
      PostEvent(NULL,lpMsg->message,lpMsg->wParam,lpMsg->lParam,BIOSGetTime());
    return TRUE;
  }


#ifdef DDE_SHMEM /* DDE extension */
  /* Look for incoming DDE messages. */
  if (dde_receive())
    return TRUE;
#endif /* DDE_SHMEM */

  /* At this point, we have no pending events. Check the time */
  if (TimerCheck(lpMsg, bPostIt))
    return TRUE;
  
  return FALSE;
}


static VOID PASCAL PostKeyEvent(wMsg)
  UINT wMsg;
{
  PostEvent(NULLHWND, wMsg, (WPARAM) SysEventInfo.chLastKey, 
            SysEventInfo.lParam, BIOSGetTime());
}


EVENTCHECKPROC FAR PASCAL EventSetHandler(pf)
  EVENTCHECKPROC pf;
{
  EVENTCHECKPROC pfOld;

  pfOld = SysEventInfo.pfExternalEventChecker;
  SysEventInfo.pfExternalEventChecker = pf;
  return pfOld;
}


INT FAR PASCAL PostEvent(hWnd, message, wParam, lParam, time)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
{
  MSG   msg;
  int   button;
  
  /* If the event queue is full, wait until an entry has been removed */
  DOSSEMWAIT(&EventQueue.semQueueFull, -1L);

  /*
    Get the mouse status before the interrupts are disabled
  */
  msg.hwnd    = hWnd;
  msg.message = message;
  msg.wParam  = wParam;
  msg.lParam  = lParam;
  msg.time    = time;
  msg.pt.x = msg.pt.y = 0;
  MouseGetStatus(&button, &msg.pt.x, &msg.pt.y);

  /* Lock out other processes from messing up the event queue */
  disable();
  DOSSEMREQUEST(&EventQueue.semQueueMutex, -1L);
  QueueAddData(&EventQueue, (BYTE *) &msg);
  enable();
  
  /* Set a flag if the queue is now full */
  if (EventQueue.qhead == EventQueue.qtail)
    DOSSEMSET(&EventQueue.semQueueFull);

  /* Let other processes play with the event queue */
  DOSSEMCLEAR(&EventQueue.semQueueMutex);
  
  /* If GetMessage() is waiting for an entry, let him know there is one now */
  DOSSEMCLEAR(&EventQueue.semQueueEmpty);
  return TRUE;
}


BOOL FAR PASCAL GetInputState(void)
{
  BOOL  rc;
  MSG   msg;

  SET_PROGRAM_STATE(STATE_JUST_POLL_EVENTS);
  rc = (*SysEventInfo.pfExternalEventChecker)((LPMSG) &msg);
  CLR_PROGRAM_STATE(STATE_JUST_POLL_EVENTS);
  return rc;
}

BOOL FAR PASCAL EnableHardwareInput(BOOL bEnable)
{
  if (bEnable)
    CLR_PROGRAM_STATE(STATE_HARDWARE_OFF);
  else
    SET_PROGRAM_STATE(STATE_HARDWARE_OFF);
  return bEnable;
}
