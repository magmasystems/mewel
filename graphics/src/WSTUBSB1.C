/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSB1.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           These stubs deal with KEYBOARD.                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


INT WINAPI GetKBCodePage(void)  /* KEYBOARD.132 */
{
  return 437;  /* hardwired, but the most common codepage */
}

INT WINAPI GetKeyNameText(LONG lParam,LPSTR lpszBuf,INT cb) /* KEYBOARD.133 */
{
  /*
    Why don't we make MEWEL apps really HUGE and keep a table of
    strings around!!!
  */
  (void) lParam;   /* 32-bit lParam of WM_CHAR message */
  (void) lpszBuf;  /* buffer to copy the key name into */
  (void) cb;       /* max # of bytes to copy */
  *lpszBuf = '\0';
  return 0;
}

UINT WINAPI MapVirtualKey(UINT uKeyCode, UINT fuMapType)  /* KEYBOARD.131 */
{
  (void) uKeyCode;   /* VK code or scan code */
  (void) fuMapType;  /* 1 = scan-to-VK, 2 = VK to unshifted ASCII */
  return 0;
}

DWORD WINAPI OemKeyScan(uOemChar) /* KEYBOARD.128 */
{
  (void) uOemChar;  /* ASCII value of the OEM char */
  /*
    Return the scan code in LOWORD, shift state in HIWORD
  */
  return MAKELONG(0, 0);
}

UINT WINAPI ScreenSwitchEnable(UINT flag)  /* KEYBOARD.100 */
{
  (void) flag;
  return 0;
}

INT WINAPI ToAscii(UINT uVirtKey, UINT uScanCode, BYTE FAR *lpbKeyState,
                   DWORD FAR *lpdwTransKey, UINT fuState)  /* KEYBOARD.4 */
{
  (void) uVirtKey;
  (void) uScanCode;
  (void) lpbKeyState;
  (void) lpdwTransKey;
  (void) fuState;
  return 0;
}

UINT WINAPI VkKeyScan(UINT uChar)  /* KEYBOARD.129 */
{
  (void) uChar;
  /*
    Return the VK code in the LOBYTE, and the shift state in the HIBYTE
  */
  return 0;
}

