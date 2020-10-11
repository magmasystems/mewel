/*===========================================================================*/
/*                                                                           */
/* File    : WLIBRARY.C                                                      */
/*                                                                           */
/* Purpose : Stubs for some of the Windows library loading routines          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


FARPROC FAR PASCAL GetProcAddress(hModule, lpProcName)
  HANDLE hModule;
  LPCSTR  lpProcName;
{
  (void) hModule;
  (void) lpProcName;
  return (FARPROC) 0;
}

HINSTANCE FAR PASCAL LoadLibrary(lpLibFileName)
  LPCSTR lpLibFileName;
{
  (void) lpLibFileName;
  return (HANDLE) 2;
}

VOID FAR PASCAL FreeLibrary(hLibModule)
  HINSTANCE hLibModule;
{
  (void) hLibModule;
}

