/*===========================================================================*/
/*                                                                           */
/* File    : WRGNFRAM.C                                                      */
/*                                                                           */
/* Purpose : Implelements the FrameRgn() function                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"


BOOL FAR PASCAL FrameRgn(hDC, hRgn, hBrush, nWidth, nHeight)
  HDC    hDC;
  HRGN   hRgn;
  HBRUSH hBrush;
  INT    nWidth, nHeight;
{
  LPRECT lpRect;

  /*
    Width and height are ignored by FrameRect()
  */
  (void) nHeight;  (void) nWidth;

  /*
    Get a pointer to the simple region to frame
  */
  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;

  /*
    Let FrameRect() do the dirty work
  */
  return FrameRect(hDC, lpRect, hBrush);
}

