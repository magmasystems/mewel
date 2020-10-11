/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU5.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with the little-used functions in USER.EXE           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"



BOOL WINAPI ExitWindowsExec(LPCSTR lpszExe, LPCSTR lpszParams)  /* USER.246 */
{
  (void) lpszExe;
  (void) lpszParams;
  return FALSE;
}

UINT WINAPI GetFreeSystemResources(UINT fuResource) /* USER.284 */
{
#define GFSR_SYSTEMRESOURCES   0x0000
#define GFSR_GDIRESOURCES      0x0001
#define GFSR_USERRESOURCES     0x0002
  (void) fuResource;
  return 0;
}

LONG WINAPI GetSystemDebugState(void)  /* USER.231 */
{
  return 0L;
}

BOOL WINAPI LockInput(HANDLE hReserved, HWND hWndInput, BOOL fLock) /*USER.226*/
{
  (void) hReserved;  (void) hWndInput;  (void) fLock;
  return FALSE;
}

BOOL WINAPI LockWindowUpdate(HWND hwndLock) /* USER.294 */
{
  (void) hwndLock;
  return TRUE;
}

BOOL WINAPI SystemParametersInfo(UINT uAction, UINT uParam, 
                                 VOID FAR *lpvParam, UINT fuWinIni)
/* USER.483 */
{
  (void) uAction;  (void) uParam;  (void) lpvParam;  (void) fuWinIni;
  return FALSE;
}

