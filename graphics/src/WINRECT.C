/*===========================================================================*/
/*                                                                           */
/* File    : WINRECT.C                                                       */
/*                                                                           */
/* Purpose : Rectangle routines.                                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  This defines an empty rectangle.
*/
RECT RectEmpty =
{
  0, 0, 0, 0
};


#if !defined(DOS) || !defined(__TURBOC__) || defined(MEWEL_GUI) || defined(__DPMI32__)
/*
  Implemented in assembler...
*/
INT FAR PASCAL IntersectRect(pDest, pRect1, pRect2)
  LPRECT pDest;
  CONST RECT FAR *pRect1;
  CONST RECT FAR *pRect2;
{
  RECT   r;
  
  r.left  = max(pRect1->left,  pRect2->left);
  r.right = min(pRect1->right, pRect2->right);
  r.top   = max(pRect1->top,   pRect2->top);
  r.bottom= min(pRect1->bottom,pRect2->bottom);
  
  if (r.top >= r.bottom || r.left >= r.right)
    r = RectEmpty;

  *pDest = r;
  return !IsRectEmpty(pDest);
}
#endif


/*===========================================================================*/
/*                                                                           */
/* File    : RECTTILE.C                                                      */
/*                                                                           */
/* Purpose : Returns a tiling of two rectangles                              */
/*                                                                           */
/*===========================================================================*/

#if 91392
/*
 * Changed from the scheme below to one where we prefer the horizontal
 * direction to the vertical direction. Better for MS Windows compatibility.
 *
 *      +---------------------+      +---------------------+
 *      |a                    |      |r0                   |
 *      |      +------+       |      +------+------+-------+
 *      |      |b     |       | -->  |r1    |//////| r2    |
 *      |      +------+       |      +------+------+-------+
 *      |                     |      |r3                   |
 *      +---------------------+      +---------------------+
*/
VOID FAR PASCAL RectTile(a, b, r)
  RECT a, b, r[];
{
  r[0] = a;
  r[0].bottom = b.top;
  if (b.top >= a.top)
    a.top = b.top;

  r[3] = a;
  r[3].top = b.bottom;
  if (b.bottom <= a.bottom)
    a.bottom = b.bottom;

  r[1] = a;
  r[1].right = b.left;
  if (b.left >= a.left)
    a.left = b.left;

  r[2] = a;
  r[2].left = b.right;

#if 0
  /* We never use r[4] in the MEWEL kernel */
  r[4] = a;
  r[4].right = b.right;
#endif
}

#else
/*
 *      +---------------------+      +------+------+-------+
 *      |a                    |      |r0    |r1    |r3     |
 *      |      +------+       |      |      +------+       |
 *      |      |b     |       | -->  |      |//////|       |
 *      |      +------+       |      |      +------+       |
 *      |                     |      |      |r2    |       |
 *      +---------------------+      +------+------+-------+
*/
VOID FAR PASCAL RectTile(a, b, r)
  RECT a, b, r[];
{
  r[0] = a;
  r[0].right = b.left;
  if (b.left >= a.left)
    a.left = b.left;

  r[3] = a;
  r[3].left = b.right;
  if (b.right <= a.right)
    a.right = b.right;

  r[1] = a;
  r[1].bottom = b.top;
  if (b.top >= a.top)
    a.top = b.top;

  r[2] = a;
  r[2].top = b.bottom;

#if 0
  /* We never use r[4] in the MEWEL kernel */
  r[4] = a;
  r[4].bottom = b.bottom;
#endif
}
#endif


/***************************************************************************/
/*                                                                         */
/* LOW LEVEL RECTANGLE AND POINT MANIPULATION ROUTINES                     */
/*                                                                         */
/*          (MS-WINDOWS compatible)                                        */
/*                                                                         */
/* CopyRect                                                                */
/* EqualRect                                                               */
/* InflateRect                                                             */
/* IntersectRect                                                           */
/* IsRectEmpty                                                             */
/* OffsetRect                                                              */
/* PtInRect                                                                */
/* SetRect                                                                 */
/* SetRectEmpty                                                            */
/* UnionRect                                                               */
/*                                                                         */
/* ClientToScreen                                                          */
/* ScreenToClient                                                          */
/*                                                                         */
/* WinClientRectToScreen                                                   */
/* WinScreenRectToClient                                                   */
/*                                                                         */
/***************************************************************************/

VOID FAR PASCAL CopyRect(pDestRect, pSrcRect)
  LPRECT pDestRect;
  CONST RECT FAR *pSrcRect;
{
  *pDestRect = *pSrcRect;
}

BOOL FAR PASCAL EqualRect(pRect1, pRect2)
  CONST RECT FAR *pRect1;
  CONST RECT FAR *pRect2;
{
  return (BOOL) 
        (pRect1->top    == pRect2->top     &&
         pRect1->bottom == pRect2->bottom  &&
         pRect1->left   == pRect2->left    &&
         pRect1->right  == pRect2->right);
}

VOID FAR PASCAL InflateRect(pRect, x, y)
  LPRECT pRect;
  INT    x, y;
{
  pRect->left   -= x;
  pRect->top    -= y;
  pRect->right  += x;
  pRect->bottom += y;
}


BOOL FAR PASCAL IsRectEmpty(pRect)
  CONST RECT FAR *pRect;
{
  return (BOOL) (pRect->top >= pRect->bottom || pRect->left >= pRect->right);
}

VOID FAR PASCAL OffsetRect(pRect, x, y)
  LPRECT pRect;
  INT    x, y;
{
  pRect->left   += x;
  pRect->top    += y;
  pRect->right  += x;
  pRect->bottom += y;
}

BOOL FAR PASCAL PtInRect(pRect, pt)
  CONST RECT FAR *pRect;
  POINT  pt;
{
  return (BOOL) (pt.x >= pRect->left && pt.x <  pRect->right && 
                 pt.y >= pRect->top  && pt.y <  pRect->bottom);
}

VOID FAR PASCAL SetRect(pRect, x1, y1, x2, y2)
  LPRECT pRect;
  INT    x1, y1, x2, y2;
{
  pRect->left   = x1;
  pRect->top    = y1;
  pRect->right  = x2;
  pRect->bottom = y2;
}

VOID FAR PASCAL SetRectEmpty(pRect)
  LPRECT pRect;
{
  *pRect = RectEmpty;
}

BOOL WINAPI SubtractRect(lpDest, lpRect1, lpRect2)
  RECT FAR *lpDest;
  CONST RECT FAR *lpRect1;
  CONST RECT FAR *lpRect2;
{
  (void) lpRect2;

  /*
    Fast kludge
  */
  *(lpDest) = *(lpRect1);

  return TRUE;
}

BOOL FAR PASCAL UnionRect(pDest, pRect1, pRect2)
  LPRECT pDest;
  CONST RECT FAR *pRect1;
  CONST RECT FAR *pRect2;
{
  RECT r;
  
  if (IsRectEmpty(pRect1))
    CopyRect(pDest, pRect2);
  else if (IsRectEmpty(pRect2))
    CopyRect(pDest, pRect1);
  else
  {  
    r.left   = min(pRect1->left,   pRect2->left);
    r.right  = max(pRect1->right,  pRect2->right);
    r.top    = min(pRect1->top,    pRect2->top);
    r.bottom = max(pRect1->bottom, pRect2->bottom);
    CopyRect(pDest, &r);
  }
  return !IsRectEmpty(pDest);
}

/*-------------------------------------------------------------------------*/

VOID FAR PASCAL GetClientRect(hWnd, pRect)
  HWND   hWnd;
  LPRECT pRect;
{
  RECT   rClient;
  WinGetClient(hWnd, &rClient);
  OffsetRect(&rClient, -rClient.left, -rClient.top);
  *pRect = rClient;
}


VOID FAR PASCAL ClientToScreen(hWnd, pPt)
  HWND    hWnd;
  LPPOINT pPt;
{
  RECT rClient;
  WinGetClient(hWnd, &rClient);
  pPt->x += rClient.left;
  pPt->y += rClient.top;
}

VOID FAR PASCAL ScreenToClient(hWnd, pPt)
  HWND    hWnd;
  LPPOINT pPt;
{
  RECT rClient;
  WinGetClient(hWnd, &rClient);
  pPt->x -= rClient.left;
  pPt->y -= rClient.top;
}

/*
  WinClientRectToScreen()
    Translates the client-relative points of a rectangle to screen coords
*/
VOID FAR PASCAL WinClientRectToScreen(hWnd, pRect)
  HWND   hWnd;
  LPRECT pRect;
{
  RECT rClient;
  WinGetClient(hWnd, &rClient);
  OffsetRect(pRect, rClient.left, rClient.top);
}

/*
  WinScreenRectToClient()
    Translates the screen relative points of a rectangle to client-relative pts
*/
VOID FAR PASCAL WinScreenRectToClient(hWnd, pRect)
  HWND   hWnd;
  LPRECT pRect;
{
  RECT rClient;
  WinGetClient(hWnd, &rClient);
  OffsetRect(pRect, -rClient.left, -rClient.top);
}

