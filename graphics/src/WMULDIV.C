/*===========================================================================*/
/*                                                                           */
/* File    : WMULDIV.C                                                       */
/*                                                                           */
/* Purpose : Implements the MulDiv() function                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

INT FAR PASCAL MulDiv(nNum, nNumer, nDenom)
  int nNum, nNumer, nDenom;
{
   return (nDenom == 0) ? 32767 : (int) (((long) nNum * nNumer) / nDenom);
}

