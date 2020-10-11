/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSS1.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with functions in SHELL.DLL                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"

/*
  ExtractIcon (SHELL.34)
*/
HICON FAR PASCAL 
ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex)
{
  (void) hInst;
  (void) lpszExeFileName;
  (void) nIconIndex;

  /*
    Return the default icon
  */
  return LoadIcon(NULL, IDI_APPLICATION);
}

