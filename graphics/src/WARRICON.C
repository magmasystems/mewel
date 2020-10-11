/*===========================================================================*/
/*                                                                           */
/* File    : WARRICON.C                                                      */
/*                                                                           */
/* Purpose : Implements the ArrangeIconicWindows() function.                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wmdilib.h"


/****************************************************************************/
/*                                                                          */
/* Function : ArrangeIconicWindows(hWnd)                                    */
/*                                                                          */
/* Purpose  : Arranges the iconic windows belong to window 'hWnd'.          */
/*                                                                          */
/* Returns  : The height of one row of icons.                               */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL ArrangeIconicWindows(hWnd)
  HWND hWnd;
{
  return MEWELArrangeIconicWindows(hWnd, FALSE);
}

UINT FAR PASCAL MEWELArrangeIconicWindows(HWND hWnd, 
         BOOL bDoOnlyMDI)  /* TRUE if called from WINMDI.C, FALSE from above */
{
  HWND hChild;
  INT  x, y, cx, cy;
  INT  nIcons = 0;
  INT  cxVSB = 0;

#if defined(MEWEL_TEXT)
  if (WinHasScrollbars(hWnd, SB_VERT))
    cxVSB = IGetSystemMetrics(SM_CXVSCROLL);
#endif

  cy = ICONHEIGHT;  /* for return value, in case nothing is iconic */

  /*
    Go through all of the MDI document windows. For each iconic window,
    determine the correct position of the icon and move it there.
  */
  for (hChild=GetTopWindow(hWnd); hChild; hChild=GetWindow(hChild,GW_HWNDNEXT))
  {
    if (IsIconic(hChild) &&
        (!bDoOnlyMDI || MdiGetProp(hChild, PROP_ISMDI)))
    {
      /*
        Get the new icon position in screen-relative coordinates
      */
      _GetIconXY(nIcons++, hChild, &x, &y, &cx, &cy);
      WinMove(hChild, y, x - cxVSB);
    }
  }

  /*
    Invalidate the window's client area. We need to do this here since
    the low-level WinMove() function doesn't do any invalidation.
  */
  if (nIcons)
  {
    RECT rClient;
    WinGetClient(hWnd, &rClient);
    WinGenInvalidRects(hWnd, (LPRECT) &rClient);
  }

  return cy;
}

