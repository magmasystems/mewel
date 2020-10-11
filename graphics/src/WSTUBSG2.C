/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSG2.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           These are for GDI print functions.                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"

BOOL WINAPI QueryAbort(HDC hDC, int reserved)  /* GDI.155 */
{
  (VOID) hDC;  (VOID) reserved;
  return TRUE;
}

HANDLE  WINAPI SpoolFile(LPSTR lpszPrinter, LPSTR lpszPort, 
                         LPSTR lpszJob, LPSTR lpszFile)
/* GDI.254 */
{
  (void) lpszPrinter;  (void) lpszPort;  (void) lpszJob;  (void) lpszFile;
  return (HANDLE) SP_ERROR;
}

