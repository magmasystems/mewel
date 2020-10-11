/*===========================================================================*/
/*                                                                           */
/* File    : WSTUB3D.C                                                       */
/*                                                                           */
/* Purpose : Stubs for CTL3D functions.                                      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"
#include "ctl3d.h"

BOOL WINAPI Ctl3dColorChange(void)
{
  return FALSE;
}

BOOL WINAPI Ctl3dSubclassDlg(HWND hWnd, WORD w)
{
  return FALSE;
}

BOOL WINAPI Ctl3dRegister(HANDLE hInst)
{
  return FALSE;
}

BOOL WINAPI Ctl3dUnregister(HANDLE handle)
{
  return FALSE;
}

BOOL WINAPI Ctl3dAutoSubclass(HANDLE hInst)
{
  return FALSE;
}

