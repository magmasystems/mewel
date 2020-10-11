/*===========================================================================*/
/*                                                                           */
/* File    : WSCROLDC.C                                                      */
/*                                                                           */
/* Purpose : Implements the ScrollDC() function                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

BOOL FAR PASCAL ScrollDC(hDC,dx,dy,lprcScroll,lprcClip,hrgnUpdate,lprcUpdate)
  HDC hDC;
  INT dx, dy;
  CONST RECT FAR *lprcScroll;
  CONST RECT FAR *lprcClip;
  HRGN    hrgnUpdate;
  LPRECT  lprcUpdate;
{
  RECT  rScroll, rClip;
  LPHDC lphDC;

  /*
    Not used...
  */
  (void) hrgnUpdate;


  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  rScroll = *lprcScroll;
  rClip   = *lprcClip;

  /*
    Scroll it!
  */
  return ScrollWindowOrDC(lphDC->hWnd,hDC,dx,dy,&rScroll,&rClip,lprcUpdate);
}

