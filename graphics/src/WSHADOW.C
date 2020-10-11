/*===========================================================================*/
/*                                                                           */
/* File    : WSHADOW.C                                                       */
/*                                                                           */
/* Purpose : WinDrawShadow()                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

typedef INT (FAR PASCAL SHADOWPROC)(HWND,HDC,LPRECT,INT,COLOR);

/*
  Use this variable to "erase" the shadow then the button is being pushed
  with the mouse.
*/
extern BOOL _fPushButtonPushed;


INT FAR PASCAL WinDrawShadow(hWnd)
  HWND hWnd;
{
  WINDOW *w;
  RECT   r;
  BYTE   chShadow;
  COLOR  attrShadow;
  BOOL   bIsButton;
  BOOL   bDrawRightside = TRUE;
  int    idClass;
  SHADOWPROC *lpfnShadow;
  int    iCol;
  int    iStartCol, iEndCol, iRightLimit;
  int    iRow;


  w = WID_TO_WIN(hWnd);
  if (!w || !(w->flags & WS_SHADOW) || !IsWindowVisible(hWnd) ||
             (w->flags & WS_MINIMIZE))
    return FALSE;

  /*
    Give the user a chance to drawn his own shadow.
  */
  if (SendMessage(hWnd, WM_SHADOW, 0, 0L))
    return TRUE;

  /*
    See if the window is a button. If so, we need special processing.
  */
  idClass = _WinGetLowestClass(w->idClass);
  bIsButton = (idClass == PUSHBUTTON_CLASS  || idClass == BUTTON_CLASS ||
               idClass == RADIOBUTTON_CLASS || idClass == CHECKBOX_CLASS);

  /*
    Determine the proper shadow-drawing function to use.
  */
  lpfnShadow = (SHADOWPROC *) ((bIsButton) ? WinFillRect : WinColorRect);

  /*
    Get the shadow character and the color
  */
  chShadow = WinGetSysChar((bIsButton) ? SYSCHAR_BUTTONBOTTOMSHADOW
                                       : SYSCHAR_SHADOW);
  attrShadow = (COLOR) WinQuerySysColor(hWnd, SYSCLR_SHADOW);


#if defined(DOS386)
  /*
    KLUDGE WARNING....
    For some reason, DOSX gives a protection fault when dealing with
    shadowed pushbutton!!! So, return if we are dealing with a button.
  */
  if (bIsButton)
    return TRUE;
#endif


  /*
    Let the blitting routines know thats it's OK to draw outside of the window
  */
  SET_PROGRAM_STATE(STATE_DRAWINGSHADOW);

  /*
    Draw the bottom horizontal strip
  */
  r = w->rect;
  OffsetRect(&r, 1, 1);
  r.top = r.bottom-1;
#ifdef TWO_COLUMN_SHADOWS
  if (!bIsButton)
    OffsetRect(&r, 1, 0);
#endif

  if (bIsButton)
  {
    HWND  hParent;
    COLOR attrParent;

    /*
      When we draw a shadow, we use the 1/2-height extended ASCII
      shadow chars. So, the other 1/2 of the char which is not
      used for ther shadow should be drawn with the parent's attribute.
    */
    hParent    = GetParentOrDT(hWnd);
    attrParent = WinGetAttr(hParent);

    if (attrParent == SYSTEM_COLOR)
      attrParent = WinGetClassBrush(hParent);
    attrShadow = (COLOR) ((attrParent & 0xF0) | BLACK);

    if (_fPushButtonPushed == 2 && ((CHECKBOXINFO*)w->pPrivate)->bHasFocus)
      attrShadow = 0x100 |            /* Make fg and bk the same */
          MAKE_ATTR(GET_BACKGROUND(attrShadow), GET_BACKGROUND(attrShadow));
    /*
      Note for the above code (Televoice) :
      When the background is intense white, making foreground and backgound
      the same (0xFF). Then a lower level routine (WinFillRect or WinColorRect)
      would think (0xFF) means look up a color. Adding 0x100 lets intense
      white on intense white work for shadows that disappear.
    */
  }


  /*
    The button might be totally or partially obscured by another window.
    So, scan the button, and for each column that is obscured, slide the
    the left side of the shadow over by one.
  */
  iStartCol = 0x7FFF;
  iEndCol   = -1;

  /*
    We scan this row to see if each cell in the row is visible. If it
    is, then we'll draw the shadow char.
  */
  iRow = w->rect.bottom-1;

  /*
    iRightLimit is the column where we stop comparing the cells.
  */
  iRightLimit = r.right - ((bIsButton) ? 1 : 2);

  for (iCol = r.left-1;  iCol < iRightLimit;  iCol++)
  {
    /*
      See if the cell is visible and not obscurred
    */
    if (VisMapPointToHwnd(iRow, iCol) == hWnd)
    {
      if (iCol < iStartCol)
        iEndCol = iStartCol = iCol;
      iEndCol++;
    }
    else
    {
      /*
        See if we must abstain from drawing the vertical strip. We do if the
        right edge of the window is obscurred.
      */
      if (iCol == iRightLimit-1)
      {
        bDrawRightside = FALSE;
        iEndCol--;
      }
    }
  }

  /*
    Adjust the horizontal extent of the shadow to match the visibility
  */
  r.left  = iStartCol + 1;
  r.right = iEndCol + 1;


  /*
    Draw the shadow
  */
  if (!IsRectEmpty(&r))
    (*lpfnShadow)(hWnd, (HDC) 0, &r, chShadow, attrShadow);

  /*
    If the button's right side is obscured, don't draw the vertical strip
  */
  if (!bDrawRightside)
    goto bye;

  /*
    Draw the vertical right strip
  */
  r = w->rect;
  r.left = r.right;
  r.right++;
  if (r.top < r.bottom-1)
    OffsetRect(&r, 0, 1);
#ifdef TWO_COLUMN_SHADOWS
  if (!bIsButton)
    ++r.right;
#endif
  (*lpfnShadow)(hWnd, (HDC) 0, &r,
               bIsButton ? WinGetSysChar(SYSCHAR_BUTTONRIGHTSHADOW) : chShadow,
               attrShadow);

bye:
  CLR_PROGRAM_STATE(STATE_DRAWINGSHADOW);
  return TRUE;
}

