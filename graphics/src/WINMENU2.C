/*===========================================================================*/
/*                                                                           */
/* File    : WINMENU2.C                                                      */
/*                                                                           */
/* Purpose : Routines to implement floating popups as defined in WIN 3.0     */
/*                                                                           */
/* History :                                                                 */
/*          3/31/91 (maa) Separated out from WINMENU.C                       */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"

/***************************************************************************/
/*                                                                         */
/*         POPUP MENU STUFF FOR WINDOWS 3.0                                */
/*                                                                         */
/***************************************************************************/

HMENU FAR PASCAL CreatePopupMenu(void)
{
  HMENU hPop;
  if ((hPop = CreateMenu()) != NULLHWND)
    ChangeMenu(hPop, hPop, NULL, hPop, MF_POPUP | MF_CHANGE);
  return hPop;
}

HMENU FAR PASCAL LoadPopupMenu(hModule, idMenu)
  HMODULE hModule;
  LPCSTR idMenu;
{
  HMENU hPop = LoadMenu(hModule, idMenu);

  if (hPop)
    ChangeMenu(hPop, hPop, NULL, hPop, MF_POPUP | MF_CHANGE);
  return hPop;
}

BOOL FAR PASCAL TrackPopupMenu(hMenu, wFlag, x, y, nReserved, hParent, lpRect)
  HMENU hMenu;
  UINT  wFlag;
  int   x, y;       /* Screen coordinates */
  int   nReserved;
  HWND  hParent;
  CONST RECT *lpRect;
{
  /*
    1) Save the old dimensions and parent ptr of the popup
    2) If the old parent is not the same as hparent, set the new parent
    3) Move the popup to [x,y]
    4) If cx > 0, calculate the new size of the popup
    n) Restore the old dimensions and parent
  */
  MENU  *mPopup;
  HWND  hOldFocus, hOldMenu;
  MSG   msg, msg2;
  int   rc;

  (void) lpRect;
  (void) wFlag;
  (void) nReserved;

#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif

  if (hMenu == NULL)
    return FALSE;

  hOldFocus = InternalSysParams.hWndFocus;

  hOldMenu = GetMenu(hParent);
  SetMenu(hParent, hMenu);

  /*
    Move and resize the popup
  */
  mPopup = _MenuHwndToStruct(hMenu);
  MenuAdjustPopupSize(mPopup);
  WinMove(hMenu, y, x);
  _MenuFixPopups(NULL, hMenu);

  /*
    Direct all attention to the popup
  */
  SetFocus(hMenu);
  InternalDrawMenuBar(hMenu);
  SetCapture(hMenu);

  while ((rc = _PeekMessage(&msg)) != WM_COMMAND)
  {
    if (rc == FALSE)
    {
      SendMessage(hParent, WM_ENTERIDLE, MSGF_MENU, (DWORD) hMenu);
      continue;
    }

    _WinGetMessage(&msg);

    if (msg.hwnd == NULLHWND  ||
        msg.hwnd == hParent   ||
        IS_MENU(WID_TO_WIN(msg.hwnd)))
    {
      DispatchMessage(&msg);
      if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        break;
      if (InternalSysParams.hWndFocus == hParent)  /* we got out of the popup */
        break;
    }
  }

  /*
    We could have messages remaining in the queue for the popup (such as
    when we have a multi-level floating popup). Let's gobble up all of
    the messages at the head of the queue which are meant for menus.
  */
  SET_PROGRAM_STATE(STATE_NONBLOCKING_PEEKMSG);
  while (_PeekMessage(&msg2) && msg2.hwnd && 
         IS_MENU(WID_TO_WIN(msg2.hwnd)))
    _WinGetMessage(&msg2);
  CLR_PROGRAM_STATE(STATE_NONBLOCKING_PEEKMSG);

  ReleaseCapture();
  SetMenu(hParent, hOldMenu);    /* Restore the owner's old menu   */
  SetFocus(hOldFocus);           /* Restore the old focus */
  SetParent(hMenu, NULLHWND);    /* Restore the popup's old parent */

  if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
    return (BOOL) 0;
  return (BOOL) msg.wParam;
}

