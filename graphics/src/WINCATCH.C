/*===========================================================================*/
/*                                                                           */
/* File    : WINCATCH.C                                                      */
/*                                                                           */
/* Purpose : Implements Catch() and Throw()                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


INT FAR PASCAL Catch(lpCatchBuf)
  LPCATCHBUF lpCatchBuf;
{
#if defined(MSC)
#if defined(DOS286X) && defined(_MSC_VER) && defined(setjmp) && (setjmp == _setjmp)
#undef setjmp
extern int  far cdecl setjmp(LPCATCHBUF);
#endif
#endif
  setjmp(lpCatchBuf);
  return 0;
}

VOID FAR PASCAL Throw(lpCatchBuf, nThrowBack)
  LPCATCHBUF lpCatchBuf;
  int        nThrowBack;
{
  longjmp(lpCatchBuf, nThrowBack);
}

