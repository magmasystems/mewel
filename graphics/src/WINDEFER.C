/*===========================================================================*/
/*                                                                           */
/* File    : WINDEFER.C                                                      */
/*                                                                           */
/* Purpose : Implements the DeferWindowPos() family of functions             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

typedef struct tagDeferStruct
{
  INT  nElements;
  INT  nMaxElements;
/*
  DEFERELEMENT [1];
*/
} DEFERSTRUCT, FAR *LPDEFERSTRUCT;

typedef struct tagDeferElement
{
  HWND hWnd;
  HWND hWndInsertAfter;
  INT  x, y, cx, cy;
  UINT wFlags;
} DEFERELEMENT, FAR *LPDEFERELEMENT;

#define ELEMENT_CHUNK  4


HDWP FAR PASCAL BeginDeferWindowPos(nWindows)
  INT nWindows;
{
  HDWP h;
  LPDEFERSTRUCT lpD;

  h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
             (DWORD) (sizeof(DEFERSTRUCT) + nWindows*sizeof(DEFERELEMENT)));
  if (!h)
    return 0;

  lpD = (LPDEFERSTRUCT) GlobalLock(h);
  lpD->nElements    = 0;
  lpD->nMaxElements = max(nWindows, ELEMENT_CHUNK);
  GlobalUnlock(h);
  return h;
}


HDWP FAR PASCAL DeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter,
                                 x, y, cx, cy, wFlags)
  HDWP hWinPosInfo;
  HWND   hWnd;
  HWND   hWndInsertAfter;
  INT    x, y, cx, cy;
  UINT   wFlags;
{
  LPDEFERSTRUCT  lpD;
  LPDEFERELEMENT lpE;
  LPSTR          lp;

  if ((lpD = (LPDEFERSTRUCT) GlobalLock(hWinPosInfo)) == NULL)
    return 0;

  /*
    No more entries? Reallocate space.
  */
  if (lpD->nElements >= lpD->nMaxElements)
  {
    GlobalUnlock(hWinPosInfo);
    if ((hWinPosInfo = GlobalReAlloc(hWinPosInfo,
         (DWORD) (sizeof(DEFERSTRUCT) + 
                      (lpD->nMaxElements+ELEMENT_CHUNK)*sizeof(DEFERELEMENT)),
                                      GMEM_MOVEABLE)) == NULL)
      return 0;
    if ((lpD = (LPDEFERSTRUCT) GlobalLock(hWinPosInfo)) == NULL)
      return 0;
    lpD->nMaxElements += ELEMENT_CHUNK;
  }

  /*
    Point to the proper element in the memory block
  */
  lp = ((LPSTR) lpD) + sizeof(DEFERSTRUCT);
  lpE = ((LPDEFERELEMENT) lp) + lpD->nElements;

  /*
    Fill the element structure
  */
  lpE->hWnd = hWnd;
  lpE->hWndInsertAfter = hWndInsertAfter;
  lpE->x  = x;
  lpE->cx = cx;
  lpE->y  = y;
  lpE->cy = cy;
  lpE->wFlags = wFlags;

  /*
    Tally one more element
  */
  lpD->nElements++;

  GlobalUnlock(hWinPosInfo);
  return hWinPosInfo;
}


BOOL FAR PASCAL EndDeferWindowPos(hWinPosInfo)
  HDWP hWinPosInfo;
{
  LPDEFERSTRUCT  lpD;
  LPDEFERELEMENT lpE;
  LPSTR          lp;
  int            i;

  if ((lpD = (LPDEFERSTRUCT) GlobalLock(hWinPosInfo)) == NULL)
    return FALSE;

  /*
    Point to the proper element in the memory block
  */
  lp = ((LPSTR) lpD) + sizeof(DEFERSTRUCT);
  lpE = ((LPDEFERELEMENT) lp);

  /*
    Do the multiple calls to SetWindowPos()
  */
  for (i = 0;  i < lpD->nElements;  i++, lpE++)
  {
    SetWindowPos(lpE->hWnd,lpE->hWndInsertAfter,lpE->x,lpE->y,lpE->cx,lpE->cy,
                 lpE->wFlags);
  }

  GlobalUnlock(hWinPosInfo);
  GlobalFree(hWinPosInfo);
  return TRUE;
}

