/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU6.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with the cursor & icon functions in USER.EXE         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


HCURSOR WINAPI CopyCursor(HINSTANCE h, HCURSOR hCursor)  /* USER.369 */
{
  (void) h;
  (void) hCursor;
  return 0;
}

void WINAPI GetClipCursor(RECT FAR *lpRect)
/* USER.309 */
{
  SetRect(lpRect, 0, 0, 0, 0);
}

HICON WINAPI CopyIcon(HINSTANCE h, HICON hIcon)  /* USER.368 */
{
  (void) h;
  (void) hIcon;
  return 0;
}

HCURSOR WINAPI GetCursor(void)
{
  return InternalSysParams.hCursor;
}


