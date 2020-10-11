/*===========================================================================*/
/*                                                                           */
/* File    : WINPRINT.C                                                      */
/*                                                                           */
/* Purpose : Windows 3.1 compatible printing functions                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

int WINAPI StartDoc(hDC, lpDI)
  HDC       hDC;
  DOCINFO FAR *lpDI;
{
  return Escape(hDC, STARTDOC, lstrlen((LPSTR) lpDI->lpszDocName),
                lpDI->lpszDocName, NULL);
}

int WINAPI StartPage(hDC)
  HDC       hDC;
{
  (void) hDC;
  return TRUE;
}

int WINAPI EndPage(hDC)
  HDC hDC;
{
  return Escape(hDC, NEWFRAME, NULL, NULL, NULL);
}

int WINAPI EndDoc(hDC)
  HDC hDC;
{
  return Escape(hDC, ENDDOC, NULL, NULL, NULL);
}

int WINAPI AbortDoc(hDC)
  HDC hDC;
{
  return Escape(hDC, ABORTDOC, NULL, NULL, NULL);
}

int WINAPI SetAbortProc(hDC, lpfnAbort)
  HDC       hDC;
  ABORTPROC lpfnAbort;
{
  return Escape(hDC, SETABORTPROC, NULL, (LPSTR) lpfnAbort, NULL);
}

