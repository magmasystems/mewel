/*===========================================================================*/
/*                                                                           */
/* File    : WRGNPAIN.C                                                      */
/*                                                                           */
/* Purpose : Implements the PaintRgn() function                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NO_STD_INCLUDES
#define NOKERNEL

#include "wprivate.h"
#include "window.h"
#include "wobject.h"


/****************************************************************************/
/*                                                                          */
/* Function : PaintRgn(hDC, hRgn)                                           */
/*                                                                          */
/* Purpose  : Fills a region using the current brush for the DC.            */
/*                                                                          */
/* Returns  : True if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL PaintRgn(hDC, hRgn)
  HDC  hDC;
  HRGN hRgn;
{
  LPHDC  lphDC;
  LPRECT lpRect;

  /*
    Get a pointer to the DC structure.
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Get a pointer to the rectangle corresponding to the region.
  */
  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;

  /*
    Use FillRect() with the current brush.
  */
  return FillRect(hDC, lpRect, lphDC->hBrush);
}

