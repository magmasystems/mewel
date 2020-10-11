/*===========================================================================*/
/*                                                                           */
/* File    : WCRECURS.C                                                      */
/*                                                                           */
/* Purpose : Implements the CreateCursor and DestroyCursor functions.        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#define INCLUDE_CURSORS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


/****************************************************************************/
/*                                                                          */
/* Function : CreateCursor()                                                */
/*                                                                          */
/* Purpose  : Dynamically creates a cursor.                                 */
/*                                                                          */
/* Returns  : The handle of the cursor if successful, NULL if not.          */
/*                                                                          */
/* OpSys    : DOS - cursors must be 16x16, not 32x32 as in Windows.         */
/*                                                                          */
/****************************************************************************/
HCURSOR FAR PASCAL CreateCursor(hInst, xHotSpot, yHotSpot, nWidth, nHeight,
                                lpvANDplane, lpvXORplane)
  HINSTANCE hInst;
  INT       xHotSpot, yHotSpot;
  INT       nWidth, nHeight;
  CONST VOID FAR *lpvANDplane;
  CONST VOID FAR *lpvXORplane;
{
  LPSYSCURSORINFO lpsysCursorInfo;
  HCURSOR         hCursor;
  INT             iMaskSize;

  (void) hInst;

  /*
    Make sure that the cursor for DOS is 16x16.
  */
#if !defined(USE_32x32_CURSORS)
  if (nWidth != 16 || nHeight != 16)
    return NULL;
#endif

  if ((hCursor = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE,
                        (DWORD) sizeof(SYSCURSORINFO))) == NULL)
    return NULL;
  if ((lpsysCursorInfo = (LPSYSCURSORINFO) GlobalLock(hCursor)) == NULL)
    return NULL;

  lpsysCursorInfo->dwID     = CURSOR_SIGNATURE;
  lpsysCursorInfo->nWidth   = nWidth;
  lpsysCursorInfo->nHeight  = nHeight;
  lpsysCursorInfo->xHotSpot = xHotSpot;
  lpsysCursorInfo->yHotSpot = yHotSpot;
  lpsysCursorInfo->bFromResource = TRUE;

  iMaskSize = (nWidth/8) * nHeight;
  lmemcpy(lpsysCursorInfo->achANDMask, lpvANDplane, iMaskSize);
  lmemcpy(lpsysCursorInfo->achXORMask, lpvXORplane, iMaskSize);

  GlobalUnlock(hCursor);
  return hCursor;
}


/****************************************************************************/
/*                                                                          */
/* Function : DestroyCursor(hCursor)                                        */
/*                                                                          */
/* Purpose  : Destroys a cursor which was created by CreateCursor or        */
/*            LoadCursor.                                                   */
/*                                                                          */
/* Returns  : TRUE if destroyed, FALSE if not.                              */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DestroyCursor(hCursor)
  HCURSOR hCursor;
{
  LPSYSCURSORINFO lpsysCursorInfo;
  LPCSTR          dwID;

  /*
    Peek at what could be the dwID part of a SYSCURSORINFO structure.
  */
  if ((lpsysCursorInfo = (LPSYSCURSORINFO) GlobalLock(hCursor)) == NULL)
    return FALSE;
  dwID = lpsysCursorInfo->dwID;
  GlobalUnlock(hCursor);

  /*
    Can't free a stock cursor
  */
  if ((WORD) FP_OFF(dwID) >= 32512 && (WORD) FP_OFF(dwID) <= 32645)
    return FALSE;

  /*
    See if the object is really a cursor
  */
  if (dwID != CURSOR_SIGNATURE)
    return FALSE;

  /*
    If the ID part of the memory was the magic cursor signature, then
    this cursor was created by CreateCursor. In that case, just free the
    memory. Otherwise, the cursor was created by LoadCursor, so we have
    to use the resource freeing functions.
  */
  if (dwID == CURSOR_SIGNATURE)
    return GlobalFree(hCursor) == NULL;
  else
    return FreeResource(hCursor);
}

