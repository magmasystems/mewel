/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU1.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with functions in USER.EXE                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


BOOL WINAPI EnableScrollBar(HWND hWnd, int nBar, UINT flags) /* USER.482 */
{
  (VOID) hWnd;  (VOID) nBar;  (VOID) flags;
  return TRUE;
}


BOOL WINAPI RedrawWindow(HWND hwnd, CONST RECT FAR* lprcUpdate, 
                         HRGN hrgnUpdate, UINT flags)  /* USER.290 */
{
  (VOID) hwnd;  (VOID) lprcUpdate;  (VOID) hrgnUpdate;  (VOID) flags;
  return TRUE;
}

