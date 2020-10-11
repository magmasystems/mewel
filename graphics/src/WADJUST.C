/*===========================================================================*/
/*                                                                           */
/* File    : WADJUST.C                                                       */
/*                                                                           */
/* Purpose : Implements the AdjustWindowRect() style of functions            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL AdjustWindowRectEx(LPRECT lpRect, DWORD flStyle, 
                                   BOOL fMenu, DWORD dwExStyle)
{
  /*
    Check for a bordered window. If so, increase its left flank.
  */
#if defined(MEWEL_TEXT)
  if (HAS_BORDER(flStyle) || (dwExStyle & WS_EX_DLGMODALFRAME))
  {
    flStyle |= WS_HSCROLL | WS_VSCROLL;
    --lpRect->left;
  }
#endif


  /*
    If a window has scrollbars, increase its boundaries
  */
  if (flStyle & WS_HSCROLL)
    lpRect->bottom += (MWCOORD) IGetSystemMetrics(SM_CYHSCROLL);
  if (flStyle & WS_VSCROLL)
    lpRect->right  += (MWCOORD) IGetSystemMetrics(SM_CXVSCROLL);

  /*
    Adjust the top. We increase it if the window is bordered or has any
    window decorations
  */
#if defined(MOTIF)
#elif defined(MEWEL_GUI) 
  if ((flStyle & (WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) ||
       HAS_CAPTION(flStyle))
    lpRect->top -= IGetSystemMetrics(SM_CYCAPTION);
  if (((flStyle & WS_BORDER) || (dwExStyle & WS_EX_DLGMODALFRAME)) &&
      !(flStyle & WS_MAXIMIZE))
  {
    if (flStyle & WS_THICKFRAME)  /* thick frame? */
      InflateRect(lpRect,IGetSystemMetrics(SM_CXFRAME),IGetSystemMetrics(SM_CYFRAME));
    else
      InflateRect(lpRect,IGetSystemMetrics(SM_CXBORDER),IGetSystemMetrics(SM_CYBORDER));
  }
#else
  if ((flStyle &
      (WS_BORDER | WS_DLGFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))
      || (dwExStyle & WS_EX_DLGMODALFRAME))
    --lpRect->top;
#endif

  /*
    If the window has a menubar, increase its top by the height of the bar.
  */
  if (fMenu)
    lpRect->top -= (MWCOORD) IGetSystemMetrics(SM_CYMENU);
}


VOID FAR PASCAL AdjustWindowRect(LPRECT lpRect, DWORD flStyle, BOOL fMenu)
{
  AdjustWindowRectEx(lpRect, flStyle, fMenu, 0L);
}

