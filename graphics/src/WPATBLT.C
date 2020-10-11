/*===========================================================================*/
/*                                                                           */
/* File    : WPATBLT.C                                                       */
/*                                                                           */
/* Purpose : Implements the PatBlt() function                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#ifdef NOGDI
#undef NOGDI
#endif

#include "wprivate.h"
#include "window.h"


BOOL FAR PASCAL PatBlt(hDC, X, Y, nWidth, nHeight, dwROP)
  HDC hDC;
  int X, Y;
  int nWidth, nHeight;
  DWORD dwROP;
{
  RECT r;

  SetRect((LPRECT) &r, X, Y, X+nWidth, Y+nHeight);

  if (dwROP == PATCOPY)
  {
    FillRect(hDC, (LPRECT) &r, _GetDC(hDC)->hBrush);
  }
  else if (dwROP == PATINVERT || dwROP == DSTINVERT)
  {
    InvertRect(hDC, (LPRECT) &r);
  }
  else if (dwROP == BLACKNESS)
  {
    FillRect(hDC, (LPRECT) &r, GetStockObject(BLACK_BRUSH));
  }
  else if (dwROP == WHITENESS)
  {
    FillRect(hDC, (LPRECT) &r, GetStockObject(WHITE_BRUSH));
  }

  return TRUE;
}

