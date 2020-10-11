/*===========================================================================*/
/*                                                                           */
/* File    : WINACCEL.C                                                      */
/*                                                                           */
/* Purpose : Accelerator routines                                            */
/*                                                                           */
/* History :                                                                 */
/*   3/6/90 (maa)  - allow AccelSetFocus() to taked NULL as an arg           */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#include "wprivate.h"
#include "window.h"

#define ACC_NOINVERT   0x02
#define ACC_SHIFT      0x04
#define ACC_CTRL       0x08
#define ACC_ALT        0x10
#define ACC_END        0x80
#define ACC_MODS       (ACC_SHIFT | ACC_ALT | ACC_CTRL)

#if !defined(USE_WINDOWS_COMPAT_KEYS)
static UINT AltKeyTable[][2] =
{
  { VK_ALT_A,    'A' },
  { VK_ALT_B,    'B' },
  { VK_ALT_C,    'C' },
  { VK_ALT_D,    'D' },
  { VK_ALT_E,    'E' },
  { VK_ALT_F,    'F' },
  { VK_ALT_G,    'G' },
  { VK_ALT_H,    'H' },
  { VK_ALT_I,    'I' },
  { VK_ALT_J,    'J' },
  { VK_ALT_K,    'K' },
  { VK_ALT_L,    'L' },
  { VK_ALT_M,    'M' },
  { VK_ALT_N,    'N' },
  { VK_ALT_O,    'O' },
  { VK_ALT_P,    'P' },
  { VK_ALT_Q,    'Q' },
  { VK_ALT_R,    'R' },
  { VK_ALT_S,    'S' },
  { VK_ALT_T,    'T' },
  { VK_ALT_U,    'U' },
  { VK_ALT_V,    'V' },
  { VK_ALT_W,    'W' },
  { VK_ALT_X,    'X' },
  { VK_ALT_Y,    'Y' },
  { VK_ALT_Z,    'Z' },
  { VK_ALT_1,    '1' },
  { VK_ALT_2,    '2' },
  { VK_ALT_3,    '3' },
  { VK_ALT_4,    '4' },
  { VK_ALT_5,    '5' },
  { VK_ALT_6,    '6' },
  { VK_ALT_7,    '7' },
  { VK_ALT_8,    '8' },
  { VK_ALT_9,    '9' },
  { VK_ALT_0,    '0' },
  { VK_ALT_PLUS, '+' },
  { VK_ALT_MINUS,'-' },
  { 0,            0  }
};

#if defined(CDPLUS) || defined(aix)
static UINT CtrlKeyTable[][2] =
{
  { (UINT) VK_CTRL_A,    'A' },
  { (UINT) VK_CTRL_B,    'B' },
  { (UINT) VK_CTRL_C,    'C' },
  { (UINT) VK_CTRL_D,    'D' },
  { (UINT) VK_CTRL_E,    'E' },
  { (UINT) VK_CTRL_F,    'F' },
  { (UINT) VK_CTRL_G,    'G' },
  { (UINT) VK_CTRL_H,    'H' },
  { (UINT) VK_CTRL_I,    'I' },
  { (UINT) VK_CTRL_J,    'J' },
  { (UINT) VK_CTRL_K,    'K' },
  { (UINT) VK_CTRL_L,    'L' },
/*{ (UINT) VK_CTRL_M,    'M' }, */  /* Don't xlate carriage return */
  { (UINT) VK_CTRL_N,    'N' },
  { (UINT) VK_CTRL_O,    'O' },
  { (UINT) VK_CTRL_P,    'P' },
  { (UINT) VK_CTRL_Q,    'Q' },
  { (UINT) VK_CTRL_R,    'R' },
  { (UINT) VK_CTRL_S,    'S' },
  { (UINT) VK_CTRL_T,    'T' },
  { (UINT) VK_CTRL_U,    'U' },
  { (UINT) VK_CTRL_V,    'V' },
  { (UINT) VK_CTRL_W,    'W' },
  { (UINT) VK_CTRL_X,    'X' },
  { (UINT) VK_CTRL_Y,    'Y' },
  { (UINT) VK_CTRL_Z,    'Z' },
  { 0,                   0   }
};
#endif
#endif

#if defined(aix)
extern BOOL bUseAltKeyTable;  /* set by TTYAssignKeys in wunixkbd.c */
#endif


INT FAR PASCAL AltKeytoLetter(key)
  int key;
{
  UINT *a;

#if defined(USE_WINDOWS_COMPAT_KEYS)
#ifdef CD_PLUS
  if (key >= 1 && key <= 26)
    return (key-1) + 'A';
#else
  BOOL bAlt = (BOOL) (GetKeyState(VK_MENU) != 0);
  return bAlt ? key : 0;
#endif

#else

#if defined(CDPLUS) && !defined(aix)
  a = (UINT *) CtrlKeyTable;
#elif defined(aix)
  if (bUseAltKeyTable)
    a = (UINT *) AltKeyTable;
  else
    a = (UINT *) CtrlKeyTable;
#else
  a = (UINT *) AltKeyTable;
#endif

  while (a[0] && a[0] != (UINT) key)
    a += 2;
  return a[1];
#endif
}


/*
  LetterToAltKey()
    Called by STdWindowWinProc() when processing WM_SYSCOMMAND/SC_KEYMENU
*/
INT FAR PASCAL LetterToAltKey(key)
  int key;
{
  UINT *a;

#if defined(USE_WINDOWS_COMPAT_KEYS)
  return key;

#else

#if defined(CDPLUS) && !defined(aix)
  a = (UINT *) CtrlKeyTable;
#elif defined(aix)
  if (bUseAltKeyTable)
    a = (UINT *) AltKeyTable;
  else
    a = (UINT *) CtrlKeyTable;
#else
  a = (UINT *) AltKeyTable;
#endif
  while (a[0] && a[1] != (UINT) key)
    a += 2;
  return a[0];
#endif
}


BOOL FAR PASCAL TranslateAccelerator(hWnd, hAccel, lpMsg)
  HWND   hWnd;
  HACCEL hAccel;
  LPMSG  lpMsg;
{
  UINT   key;
  HMENU  hMenu;
  UINT   message;


  if (hAccel == NULL)
    return FALSE;

  message = lpMsg->message;
  key     = lpMsg->wParam;

  /*
    Search the specified accelerator table for the key in wParam.
  */
  if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && hAccel)
  {
    UINT idAccel;
    LPACCEL aTable;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    UINT    wMask = 0;
    if (GetKeyState(VK_SHIFT))
      wMask |= ACC_SHIFT;
    if (GetKeyState(VK_CONTROL))
      wMask |= ACC_CTRL;
    if (GetKeyState(VK_MENU))
      wMask |= ACC_ALT;
#endif

    /* 
      If we press an ALT key while in a menu, then pass back the letter
      to the menu proc. For example, if we press ALT-F (to get to the
      "&File" pulldown), hold the ALT key down, and press 'S', then the
      "&Save..." menuitem should be invoked.
    */
    if (InternalSysParams.hWndFocus &&
        (WinGetStyle(InternalSysParams.hWndFocus) & WIN_IS_MENU))
    {
      WPARAM wParam;
#if !defined(USE_WINDOWS_COMPAT_KEYS)
      lpMsg->wParam = (((wParam = AltKeytoLetter(key)) != 0) ? wParam : key);
#endif
      return FALSE;
    }

    /*
      Get a pointer to the accelerator table and search it for the key.
    */
    if ((aTable = (LPACCEL) GlobalLock(hAccel)) == (LPACCEL) NULL)
      return FALSE;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    while (!(aTable->fFlags & ACC_END) && (aTable->wEvent != key ||
            (aTable->fFlags & ACC_MODS) != wMask))
#else
    while (!(aTable->fFlags & ACC_END) && aTable->wEvent != key)
#endif
    {
      aTable++;
    }

#if defined(USE_WINDOWS_COMPAT_KEYS)
    if (aTable->wEvent == key && (aTable->fFlags & ACC_MODS) == wMask)
#else
    if (aTable->wEvent == key)
#endif
    {
      idAccel = (UINT) aTable->wId;
      GlobalUnlock(hAccel);
    }
    else
    {
      GlobalUnlock(hAccel);
      return FALSE;
    }

    /*
      Do not pass accelerators through to a disabled window.
    */
    if (!IsWindowEnabled(hWnd))
      return TRUE;

    /*
       Don't translate an accelerator for a disabled menu item
    */
    if ((hMenu = GetMenu(hWnd)) != NULLHWND)
    {
      UINT fState;

      SendMessage(hWnd, WM_INITMENU, hMenu, 0L);
      /*
        We also should send a WM_INITPOPUP message too...
      */
      fState = GetMenuState(hMenu, idAccel, MF_BYCOMMAND);
      if (fState != (UINT) -1 && (fState & (MF_DISABLED | MF_GRAYED)))
        return TRUE;
    }

    /*
      Transform the mesage into a WM_COMMAND message and dispatch it.
    */
    SendMessage(hWnd, WM_COMMAND, idAccel, MAKELONG(0, 1));
    return TRUE;
  }
  else
    return FALSE;
}

