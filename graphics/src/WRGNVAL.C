/*===========================================================================*/
/*                                                                           */
/* File    : WRGNVAL.C                                                       */
/*                                                                           */
/* Purpose : Implements the InvalidateRgn(), ValidateRgn(), GetUpdateRgn(),  */
/*           and ExcludeUpdateRgn() functions.                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"


VOID FAR PASCAL InvalidateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
  /*
    Use InvalidateRect() to do the dirty work. Actually, InvalidateRect()
    should call InvalidateRgn()!
  */
  InvalidateRect(hWnd, _RgnToRect(hRgn), bErase);
}

VOID FAR PASCAL ValidateRgn(HWND hWnd, HRGN hRgn)
{
  /*
    Use ValidateRect() to do the dirty work. Actually, ValidateRect()
    should call ValidateRgn()!
  */
  ValidateRect(hWnd, _RgnToRect(hRgn));
}

INT  FAR PASCAL GetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
  RECT   r;
  LPRECT lpRgn;

  /*
    Use GetUpdateRect() to do the dirty work. Actually, GetUpdateRect()
    should call GetUpdateRgn()!
  */
  GetUpdateRect(hWnd, &r, bErase);

  /*
    Get a pointer to the underlying rect structure and copy the update
    rectangle to it.
  */
  if ((lpRgn = _RgnToRect(hRgn)) != NULL)
    *lpRgn = r;

  /*
    Right now, all regions are simple regions.
  */
  return SIMPLEREGION;
}

INT FAR PASCAL ExcludeUpdateRgn(HDC hDC, HWND hWnd)
{
  (void) hDC;  (void) hWnd;
  return SIMPLEREGION;
}

