/*===========================================================================*/
/*                                                                           */
/* File    : WHFILEIO.C                                                      */
/*                                                                           */
/* Purpose : Huge file-reading functions, _hread and _hwrite.                */
/*           _hread and _hwrite are used in OWL 2.0 by DIB.CPP               */
/*                                                                           */
/* History :                                                                 */
/* $Log:   E:/vcs/mewel/whfileio.c_v  $                                      */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


LONG FAR PASCAL _hreadwrite(HFILE fd, VOID _huge *lpBuf, LONG dwBytes, BOOL bRead)
{
  LONG  nTotal = 0L;

  while (dwBytes > 0)
  {
    INT wBytes = (UINT) (min(dwBytes, 0x7FFF));

    /*
      Read at most 32K bytes
    */
    INT n = (bRead) ? _lread(fd,  lpBuf, wBytes)
                    : _lwrite(fd, lpBuf, wBytes);

    /*
      Error-check
    */
    if (n < 0)
      return -1L;

    /*
      Advance to the next 32K seg, and bump down the byte count by 32K
    */
    lpBuf = (char _huge *) lpBuf + n;
    dwBytes -= n;
    nTotal += n;

    if (n < wBytes)
      break;
  }
  return nTotal;
}

LONG FAR PASCAL _hread(HFILE fd, VOID _huge *lpBuf, LONG dwBytes)
{
  return _hreadwrite(fd, lpBuf, dwBytes, TRUE);
}

LONG FAR PASCAL _hwrite(HFILE fd, CONST VOID _huge *lpBuf, LONG dwBytes)
{
  return _hreadwrite(fd, (VOID _huge *) lpBuf, dwBytes, FALSE);
}

