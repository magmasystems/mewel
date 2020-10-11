/*===========================================================================*/
/*                                                                           */
/* File    : WGDICLIP.C                                                      */
/*                                                                           */
/* Purpose : Implements GDI clipping routines                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"


#if !defined(XWINDOWS)
HRGN FAR PASCAL CreateRectRgn(int X1, int Y1, int X2, int Y2)
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  if ((hObj = _ObjectAlloc(OBJ_RGN)) == BADOBJECT)
    return (HRGN) NULL;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HRGN) NULL;

  SetRect((LPRECT) &lpObj->uObject.uRgn, X1, Y1, X2, Y2);

  return (HRGN) hObj;
}
#endif


int  FAR PASCAL GetClipBox(HDC hDC, LPRECT lpRect)
{
  LPHDC lphDC = _GetDC(hDC);
  RECT  r;

  if (lphDC)
  {
    r = lphDC->rClipping;
    SPtoLP(hDC, (LPPOINT) &r, 2);
    *lpRect = r;
  }
  return SIMPLEREGION;
}


INT FAR PASCAL ExcludeClipRect(hDC, X1, Y1, X2, Y2)
  HDC  hDC;
  INT  X1, Y1, X2, Y2;
{
  RECT  r;
  (void) hDC;
  SetRect(&r, X1, Y1, X2, Y2);
  return SIMPLEREGION;
}

INT FAR PASCAL IntersectClipRect(hDC, X1, Y1, X2, Y2)
  HDC  hDC;
  INT  X1, Y1, X2, Y2;
{
  RECT  r;
  LPHDC lphDC = _GetDC(hDC);

  SetRect(&r, X1, Y1, X2, Y2);
  LPtoSP(hDC, (LPPOINT) &r, 2);
  if (!IntersectRect(&lphDC->rClipping, &lphDC->rClipping, &r))
    return NULLREGION;
  else
    return SIMPLEREGION;
}

INT FAR PASCAL OffsetClipRect(hDC, X, Y)
  HDC  hDC;
  INT  X, Y;
{
  LPHDC lphDC = _GetDC(hDC);
  POINT pt;

  pt.x = X;  pt.y = Y;
  LPtoSP(hDC, (LPPOINT) &pt, 1);
  OffsetRect(&lphDC->rClipping, pt.x, pt.y);
  return SIMPLEREGION;
}

int WINAPI OffsetClipRgn(hDC, nXOffset, nYOffset)
  HDC hDC;
  INT nXOffset, nYOffset;
{
  return OffsetClipRect(hDC, nXOffset, nYOffset);
}

BOOL FAR PASCAL PtVisible(hDC, X, Y)
  HDC hDC;
  INT X, Y;
{
  LPHDC lphDC = _GetDC(hDC);
  POINT pt;

  pt.x = X;  pt.y = Y;
  LPtoSP(hDC, (LPPOINT) &pt, 1);
  return PtInRect(&lphDC->rClipping, pt);
}

BOOL FAR PASCAL RectVisible(hDC, lpRect)
  HDC hDC;
  CONST RECT FAR *lpRect;
{
  LPHDC lphDC = _GetDC(hDC);
  RECT  r;

  r = *lpRect;
  LPtoSP(hDC, (LPPOINT) &r, 2);
  return IntersectRect(&r, &r, &lphDC->rClipping);
}

#if !defined(XWINDOWS)
int FAR PASCAL SelectClipRgn(HDC hDC, HRGN hRgn)
{
  LPRECT   lpRect;
  RECT     rClip;
  LPHDC    lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return ERROR;

  if ((lpRect = _RgnToRect(hRgn)) == NULL)
    return ERROR;

  rClip = *lpRect;                   /* rClip is in logical coords */
  LPtoSP(hDC, (LPPOINT) &rClip, 2);  /* translate into screen coords */

  if (!IntersectRect(&rClip, &rClip, &(WID_TO_WIN(lphDC->hWnd)->rClient)))
    return NULLREGION;

  lphDC->rClipping = rClip;
#if defined(MEWEL_GUI) && defined(META)
  _GraphicsSetViewport(hDC);
#endif

  return SIMPLEREGION;
}
#endif


HRGN FAR PASCAL GetClipRgn(hDC)  /* GDI.173 */
  HDC hDC;
{
  LPHDC lphDC;
  if ((lphDC = _GetDC(hDC)) == NULL)
    return NULL;
  return lphDC->hRgnClip;
}


LPRECT FAR PASCAL _RgnToRect(HRGN hRgn)
{
  LPOBJECT lpObj;

  if ((lpObj = _ObjectDeref(hRgn)) == NULL)
    return (LPRECT) NULL;
  return (LPRECT) &lpObj->uObject.uRgn;
}

