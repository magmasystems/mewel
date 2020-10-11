/*===========================================================================*/
/*                                                                           */
/* File    : WMAPPOIN.C                                                      */
/*                                                                           */
/* Purpose : Implements the MapWindowPoints() function                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL MapWindowPoints(hwndFrom, hwndTo, lppt, cPoints)
  HWND hwndFrom;
  HWND hwndTo;
  POINT FAR *lppt;
  UINT cPoints;
{
  while (cPoints-- > 0)
  {
    /*
      Convert the points to screen coordinates if not in screen coords already
    */
    if (hwndFrom != NULL && hwndFrom != _HwndDesktop)
      ClientToScreen(hwndFrom, lppt);

    /*
      Convert the points to the coords of the other window
    */
    if (hwndTo != NULL && hwndTo != _HwndDesktop)
      ScreenToClient(hwndTo, lppt);

    /*
      Advance to next set of points
    */
    lppt++;
  }
}

