/*===========================================================================*/
/*                                                                           */
/* File    : WINMODAL.C                                                      */
/*                                                                           */
/* Purpose : Implements the Get/SetSysModalWindow() functions                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*                                                                           */
/* Notes :                                                                   */
/*   The concept of a system modal dialog box is only valid when there is    */
/* more than one task. So, this stuff is not really used internally within   */
/* MEWEL. But, as an exercise, let's see where Windows uses it, and see the  */
/* changes which would need to be done to MEWEL to implement system modal    */
/* windows.....                                                              */
/*                                                                           */
/* Here's where Windows uses MODAL stuff...                                  */
/* MessageBox styles - MB_APPLMODAL, MB_SYSTEMMODAL, MB_TASKMODAL            */
/* DialogBox  styles - DS_SYSMODAL                                           */
/*                                                                           */
/* and it has two calls...                                                   */
/* GetSysModalWindow()                                                       */
/* SetSysModalWindow(hWnd)                                                   */
/*                                                                           */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


HWND FAR PASCAL GetSysModalWindow(void)
{
  return InternalSysParams.hWndSysModal;
}

HWND FAR PASCAL SetSysModalWindow(hWnd)
  HWND hWnd;
{
  HWND hOld = InternalSysParams.hWndSysModal;
  InternalSysParams.hWndSysModal = hWnd;
  return hOld;
}

