/*===========================================================================*/
/*                                                                           */
/* File    : WDLGCTID.C                                                      */
/*                                                                           */
/* Purpose : The GetDlgCtrlID() function                                     */
/*                                                                           */
/* History : 4/12/94 (maa)   Created                                         */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

INT WINAPI GetDlgCtrlID(hWnd)
  HWND hWnd;
{
  return (INT) GetWindowWord(hWnd, GWW_ID);
}

