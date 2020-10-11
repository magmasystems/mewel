/*===========================================================================*/
/*                                                                           */
/* File    : WFILETIT.C                                                      */
/*                                                                           */
/* Purpose : GetFileTitle()                                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


int WINAPI GetFileTitle(lpszFile, lpszTitle, cbBuf)
  LPCSTR lpszFile;
  LPSTR  lpszTitle;
  UINT   cbBuf;
{
  int    sLen;

  /*
    Find the last directory separator
  */
  LPCSTR  pSlash = strrchr(lpszFile, CH_SLASH);

  /*
    Make pSlash point to the start of the file name
  */
  if (pSlash)
    pSlash++;
  else
    pSlash = lpszFile;

  sLen = lstrlen((LPSTR) pSlash) + 1;  /* +1 for the null terminator */
  /*
    Not enough room to copy the file title? Return the amount 
    of bytes required.
  */
  if ((UINT) sLen > cbBuf)
    return sLen;

  /*
    Copy the title portion of the file (without path info) to lpszTitle.
  */
  lstrncpy(lpszTitle, (LPSTR) pSlash, sLen);
  lpszTitle[sLen] = '\0';

  /*
    Returning 0 means success
  */
  return 0;
}

