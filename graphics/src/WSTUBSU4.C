/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU4.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with the message functions in USER.EXE               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


BOOL WINAPI CallMsgFilter(MSG FAR *lpMsg, int nCode)  /* USER.123 */
{
  (void) lpMsg;
  (void) nCode;
  return TRUE;
}

LPARAM WINAPI GetMessageExtraInfo(void)
/* USER.288 */
{
  return 0L;
}

DWORD WINAPI GetQueueStatus(UINT flags)  /* USER.334 */
{
  (void) flags;
  return 0L;
}

BOOL WINAPI PostAppMessage(HTASK hTask, UINT msg, WPARAM wParam, LPARAM lParam)
/* USER.116 */
{
  (void) hTask;
  return PostMessage(_HwndDesktop, msg, wParam, lParam);
}

BOOL WINAPI QuerySendMessage(HANDLE h1, HANDLE h2, HANDLE h3, LPMSG lpMsg)
/* USER.184 */
{
  /*
    Return 0 if the message originated in the current task.
  */
  (void) h1;
  (void) h2;
  (void) h3;
  (void) lpMsg;
  return FALSE;
}

void WINAPI ReplyMessage(LRESULT lResult)
/* USER.115 */
{
  (void) lResult;
}

BOOL WINAPI SetMessageQueue(int nMsg) /* USER.266 */
{
  /*
    Creates a new message queue with 'nMsg' entries. Max is 120 messages.
  */
  (void) nMsg;
  return TRUE;
}


