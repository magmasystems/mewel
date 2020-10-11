/*===========================================================================*/
/*                                                                           */
/* File    : WRGNINV.C                                                       */
/*                                                                           */
/* Purpose : Implements the InvertRgn() function                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"


BOOL FAR PASCAL InvertRgn(hDC, hRgn)
  HDC  hDC;
  HRGN hRgn;
{
  LPRECT lpRect;

  /*
    Get a pointer to the simple region to invert
  */
  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;

  /*
    Let InvertRect() do the dirty work
  */
  InvertRect(hDC, lpRect);
  return TRUE;
}

