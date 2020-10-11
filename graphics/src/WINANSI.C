/*===========================================================================*/
/*                                                                           */
/* File    : WINANSI.C                                                       */
/*                                                                           */
/* Purpose : Implements the AnsiXXX and OEMxxx series of functions           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


LPSTR FAR PASCAL AnsiLower(lpStr)
  LPSTR lpStr;
{
  if (HIWORD(lpStr) == 0)   /* single character passed */
  {
    BYTE ch = (BYTE) (((DWORD) lpStr) & 0x00FFL);
    return (LPSTR) MAKELONG(tolower(ch), 0);
  }
  else
  {
    LPSTR lpOrig = lpStr;
    while (*lpStr)
      *lpStr++ = (BYTE) tolower(*lpStr);
    return lpOrig;
  }
}

LPSTR FAR PASCAL AnsiUpper(lpStr)
  LPSTR lpStr;
{
  if (HIWORD(lpStr) == 0)   /* single character passed */
  {
    BYTE ch = (BYTE) (((DWORD) lpStr) & 0x00FFL);
    return (LPSTR) MAKELONG(lang_upper(ch), 0);
  }
  else
  {
    LPSTR lpOrig = lpStr;
    while (*lpStr)
      *lpStr++ = (BYTE) lang_upper(*lpStr);
    return lpOrig;
  }
}

UINT FAR PASCAL AnsiLowerBuff(lpString, nLength)
  LPSTR lpString;
  UINT  nLength;
{
  LPSTR lpOrig = lpString;

  if (nLength == 0)
    nLength = 0xFFFF;

  for (  ;  nLength > 0 && *lpString != '\0';  nLength--)
    *lpString++ = (BYTE) tolower(*lpString);
  return lpString - lpOrig;
}

UINT FAR PASCAL AnsiUpperBuff(lpString, nLength)
  LPSTR lpString;
  UINT  nLength;
{
  LPSTR lpOrig = lpString;

  if (nLength == 0)
    nLength = 0xFFFF;

  for (  ;  nLength > 0 && *lpString != '\0';  nLength--)
    *lpString++ = (BYTE) lang_upper(*lpString);
  return lpString - lpOrig;
}

LPSTR FAR PASCAL AnsiNext(lpCurrentChar)
  LPCSTR lpCurrentChar;
{
  if (*lpCurrentChar)
    lpCurrentChar++;
  return (LPSTR) lpCurrentChar;
}

LPSTR FAR PASCAL AnsiPrev(lpStart, lpCurrentChar)
  LPCSTR lpStart;
  LPCSTR lpCurrentChar;
{
  if (lpCurrentChar != lpStart)
    lpCurrentChar--;
  return (LPSTR) lpCurrentChar;
}

VOID FAR PASCAL AnsiToOem(lpAnsiStr, lpOemStr)
  CONST char HUGEDATA *lpAnsiStr;
  char       HUGEDATA *lpOemStr;
{
  lstrcpy((LPSTR) lpOemStr, (LPSTR) lpAnsiStr);
}

VOID FAR PASCAL AnsiToOemBuff(lpAnsiStr, lpOemStr, nLength)
  LPCSTR lpAnsiStr;
  LPSTR  lpOemStr;
  UINT   nLength;
{
  lstrncpy(lpOemStr, (LPSTR) lpAnsiStr, (nLength == 0) ? 0xFFFF : nLength);
}

VOID FAR PASCAL OemToAnsi(lpOemStr, lpAnsiStr)
  CONST char HUGEDATA *lpOemStr;
  char       HUGEDATA *lpAnsiStr;
{
  lstrcpy((LPSTR) lpAnsiStr, (LPSTR) lpOemStr);
}

VOID FAR PASCAL OemToAnsiBuff(lpOemStr, lpAnsiStr, nLength)
  LPCSTR lpOemStr;
  LPSTR  lpAnsiStr;
  UINT   nLength;
{
  lstrncpy(lpAnsiStr, (LPSTR) lpOemStr, (nLength == 0) ? 0xFFFF : nLength);
}

