/*===========================================================================*/
/*                                                                           */
/* File    : WRGNFILL.C                                                      */
/*                                                                           */
/* Purpose : Implements the FillRgn() function                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"


BOOL FAR PASCAL FillRgn(hDC, hRgn, hBrush)
  HDC    hDC;
  HRGN   hRgn;
  HBRUSH hBrush;
{
  LPRECT lpRect;

  /*
    Get a pointer to the simple region to fill
  */
  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;

  /*
    Let FillRect() do the dirty work
  */
  return FillRect(hDC, lpRect, hBrush);
}

