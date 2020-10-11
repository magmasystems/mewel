/*===========================================================================*/
/*                                                                           */
/* File    : WPROCINS.C                                                      */
/*                                                                           */
/* Purpose : The MakeProcInstance/FreeProcInstance functions                 */
/*                                                                           */
/* History : 4/12/94 (maa)   Created                                         */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

FARPROC WINAPI MakeProcInstance(farProc, hInst)
  FARPROC farProc;
  HINSTANCE hInst;
{
  (void) hInst;  /* not used */
  return (FARPROC) farProc;
}


VOID WINAPI FreeProcInstance(farProc)
  FARPROC farProc;
{
  (void) farProc;  /* not used */
}

