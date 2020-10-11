/*===========================================================================*/
/*                                                                           */
/* File    : WGDIRGN.C                                                       */
/*                                                                           */
/* Purpose : Stubs for the GDI calls which deal with painting regions        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#ifdef NOGDI
#undef NOGDI
#endif
#define NO_STD_INCLUDES
#define NOKERNEL

#include "wprivate.h"
#include "window.h"
#include "wobject.h"


BOOL FAR PASCAL PtInRegion(hRgn, X, Y)
  HRGN hRgn;
  INT  X, Y;
{
  LPRECT lpRect;
  POINT  pt;

  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;
  pt.x = X;   pt.y = Y;
  return PtInRect(lpRect, pt);
}

INT FAR PASCAL CombineRgn(hRgn, hRgn1, hRgn2, nCombineMode)
  HRGN hRgn;
  HRGN hRgn1, hRgn2;
  INT  nCombineMode;
{
  (void) hRgn;  (void) hRgn1;  (void) hRgn2;  (void) nCombineMode;
  return SIMPLEREGION;
}

BOOL FAR PASCAL EqualRgn(hRgn1, hRgn2)
  HRGN hRgn1, hRgn2;
{
  LPRECT lpRect1, lpRect2;

  if ((lpRect1 = _RgnToRect(hRgn1)) == NULL)
    return FALSE;
  if ((lpRect2 = _RgnToRect(hRgn2)) == NULL)
    return FALSE;
  return (BOOL) (
    lpRect1->left   == lpRect2->left  && 
    lpRect1->top    == lpRect2->top   && 
    lpRect1->right  == lpRect2->right && 
    lpRect1->bottom == lpRect2->bottom);
}

INT  FAR PASCAL OffsetRgn(hRgn, X, Y)
  HRGN hRgn;
  INT  X, Y;
{
  LPRECT lpRect;
  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return FALSE;
  OffsetRect(lpRect, X, Y);
  return TRUE;
}

INT  FAR PASCAL GetRgnBox(hRgn, lpRect)
  HRGN hRgn;
  LPRECT lpRect;
{
  (void) hRgn;  (void) lpRect;
  return SIMPLEREGION;
}

BOOL FAR PASCAL RectInRegion(hRgn, lpRect)
  HRGN   hRgn;
  CONST RECT FAR *lpRect;
{
  LPRECT lpRgn;
  RECT   rDummy;

  if ((lpRgn = _RgnToRect(hRgn)) == NULL)
    return FALSE;
  return IntersectRect((LPRECT) &rDummy, lpRgn, lpRect);
}

HRGN FAR PASCAL CreateEllipticRgnIndirect(lpRect)
  CONST RECT FAR * lpRect;
{
  (void) lpRect;
  return 0;
}

HRGN FAR PASCAL CreateEllipticRgn(X1, Y1, X2, Y2)
  INT X1, Y1, X2, Y2;
{
  return CreateRectRgn(X1, Y1, X2, Y2);
}

HRGN FAR PASCAL CreatePolygonRgn(lpPoint, nCount, nPolyFillMode)
  CONST POINT FAR *lpPoint;
  INT     nCount;
  INT     nPolyFillMode;
{
  (void) lpPoint;  (void) nCount;  (void) nPolyFillMode;
  return 0;
}

HRGN FAR PASCAL CreatePolyPolygonRgn(lpPoint,lpPolyCounts,nCount,nPolyFillMode)
  CONST POINT FAR *lpPoint;
  CONST INT   FAR *lpPolyCounts;
  INT     nCount;
  INT     nPolyFillMode;
{
  (void) lpPoint;  (void) lpPolyCounts;  (void) nCount;  (void) nPolyFillMode;
  return 0;
}

HRGN FAR PASCAL CreateRoundRectRgn(X1, Y1, X2, Y2, X3, Y3)
  INT X1, Y1, X2, Y2, X3, Y3;
{
  (void) X3;  (void) Y3;
  return CreateRectRgn(X1, Y1, X2, Y2);
}

HRGN FAR PASCAL CreateRectRgnIndirect(lpRect)
  CONST RECT FAR * lpRect;
{
  return CreateRectRgn(lpRect->left,lpRect->top,lpRect->right,lpRect->bottom);
}

VOID FAR PASCAL SetRectRgn(hRgn, X1, Y1, X2, Y2)
  HRGN hRgn;
  INT  X1, Y1, X2, Y2;
{
  LPRECT lpRgn;
  if ((lpRgn = _RgnToRect(hRgn)) != NULL)
    SetRect(lpRgn, X1, Y1, X2, Y2);
}

