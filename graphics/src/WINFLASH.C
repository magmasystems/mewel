/*===========================================================================*/
/*                                                                           */
/* File    : WINFLASH.C                                                      */
/*                                                                           */
/* Purpose : Implements the silly FlashWindow() call                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

BOOL FAR PASCAL FlashWindow(HWND hWnd, BOOL bInvert)
{
  BOOL rc = TRUE;  /* return code. Whether window was active before the call */
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  /*
    Windows will always flash an iconic window and return TRUE. Otherwise
    get the activation state of the window.
  */
  if (IsIconic(hWnd))
    bInvert = FALSE;
  else
    rc = (BOOL) ((w->ulStyle & WIN_ACTIVE_BORDER) != 0L);

  /*
    Flash the caption bar
  */
  SendMessage(hWnd, WM_NCACTIVATE, !rc, 0L);

  /*
    If bInvert is FALSE, the window is returned to its original state
  */
  if (!bInvert)
    SendMessage(hWnd, WM_NCACTIVATE, rc, 0L);

  return rc;
}

