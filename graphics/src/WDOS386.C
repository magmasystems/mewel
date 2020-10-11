/*===========================================================================*/
/*                                                                           */
/* File    : WDOS386.C                                                       */
/*                                                                           */
/* Purpose : Replacement "C" routines for Zortech's DOSX                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if defined(__GNUC__)
#include <pc.h>  /* for kbhit */
#else
#include <conio.h>
#endif

VOID PASCAL limemset(dest, cell, len)
  UINT FAR *dest;
  UINT   cell;
  UINT   len;
{
  while (len-- > 0)
    *dest++ = cell;
}

VOID FAR PASCAL lmemshr(LPSTR dest, LPSTR src, UINT len)
{
  dest += len-1;
  src  += len-1;
  while (len-- > 0)
    *dest-- = *src--;
}

LPSTR FAR PASCAL lcgacpy(LPSTR dest, LPSTR src, UINT len)
{
  return memcpy(dest, src, len);
}

VOID FAR PASCAL lmemcpy386(BYTE TRUEFAR *dest, LPSTR src, UINT len)
{
  while (len-- > 0)
    *dest++ = *src++;
}

INT FAR CDECL keyready(UINT wEnhanced)
{
  (void) wEnhanced;
  return kbhit();
}

VOID FAR CDECL DosSleep(int nMillisecs)
{
  (void) nMillisecs;
}

/*
  Find the length of the string or the right border, whichever comes first
*/
INT FAR PASCAL lmemscan(LPSTR s, int ch, UINT len)
{
  LPSTR pFound;

  if (len == 0)
    return 0;
  pFound = memchr(s, ch, len);
  return (pFound) ? (pFound - s) : len;
}

#if defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)

/*
  From winalt.c
*/
VOID Int9Init(void)
{
}
VOID Int9Terminate(void)
{
}

/*
  From winbreak.c
*/
volatile char BrkFound = 0;
int FAR PASCAL getbrk(void)
{
  return 0;
}
VOID FAR PASCAL rstbrk(void)
{
}
VOID FAR PASCAL setbrk(void)
{
}
VOID CDECL int23ini(char FAR *pBrkSemaphore)
{
  (void) pBrkSemaphore;
}
VOID CDECL int23res(void)
{
}

/*
  From winint24.c
*/
UINT    Int24Err = 0;
PSTR    Int24ErrMsg;
VOID FAR PASCAL Int24Install(void)
{
}
VOID CDECL Int24Restore(void)
{
}

#endif

UINT FAR PASCAL SetErrorMode(UINT wMode)
{
  (void) wMode;
  return 0;
}

