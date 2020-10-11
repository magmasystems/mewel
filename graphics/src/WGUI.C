/*===========================================================================*/
/*                                                                           */
/* File    : WGUI.C                                                          */
/*                                                                           */
/* Purpose : Code for drawing a Motif-style border around a window.          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#define INCLUDE_SYSBITMAPS

#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if defined(MOTIF)
static int bDrawingNCArea = 0;
#endif

#define IS_PEN(obj)  ((obj) == BLACK_PEN || (obj) == WHITE_PEN)


/****************************************************************************/
/*                                                                          */
/* Function : DrawBeveledBox()                                              */
/*                                                                          */
/* Purpose  : Draws a 3-d bevelled rectangle around the specified window.   */
/*            The frame of the rectangle is drawn with two different colored*/
/*            pens (iTopBrush and iBotBrush) and the interior is filled     */
/*            with iFillBrush (unless it's a NULL brush).                   */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: Used to draw the frames of checkboxes, radiobuttons, pushbuts,*/
/*            static frames, edit controls, and scrollbar thumbs and arrows.*/
/*                                                                          */
/****************************************************************************/
VOID PASCAL DrawBeveledBox(
  HWND   hWnd,
  HDC    hOrigDC,
  LPRECT lpRect,
  int    cxBorder, int cyBorder,
  BOOL   bHasFocus,
  HBRUSH iFillBrush)            /* handle of brush for interior of rect */
{
#if !62292
  RECT rFill;
#endif
  RECT r;
  HDC  hDC;
  int  i, iClass;
  HPEN hTop, hBot;

  static HBRUSH hNullBrush = 0;

  /*
    Store the handle of the NULL_BRUSH for future comparsions
  */
  if (hNullBrush == NULL)
    hNullBrush = GetStockObject(NULL_BRUSH);

  /*
    Get a window DC so that we can draw outside the client area
  */
  if ((hDC = hOrigDC) == NULL)
    hDC = GetWindowDC(hWnd);

  /*
    Set the rectangle to fill.
  */
  if (lpRect == NULL)
    WindowRectToPixels(hWnd, (LPRECT) &r);
  else
    SetRect(&r, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

  {
  LPHDC lphDC = GDISetup(hDC);
  if (lphDC)
    lphDC->fFlags |= DC_ATOMICOPERATION;
  }

  /*
    Fill the box with the iFillBrush color
  */
  if (iFillBrush != hNullBrush)
    FillRect(hDC, &r, iFillBrush);

#if 62292
  (void) cxBorder;

  /*
    Don't include lower-right corner as-per Windows
  */
  r.bottom--;
  r.right--;

  /*
    Get handles to the pens used for drawing the sides
  */
  hTop = SysPen[bHasFocus ? SYSPEN_BTNSHADOW : SYSPEN_BTNHIGHLIGHT];
  hBot = SysPen[bHasFocus ? SYSPEN_BTNHIGHLIGHT : SYSPEN_BTNSHADOW];
#if defined(USE_MOTIF_LOOK)
  if (bDrawingNCArea || (XSysParams.ulFlags & XFLAG_RUBBERBANDING))
  {
    hTop = GetStockObject(bHasFocus ? BLACK_PEN : WHITE_PEN);
    hBot = GetStockObject(bHasFocus ? WHITE_PEN : BLACK_PEN);
  }
#endif

  if (SysGDIInfo.fFlags & GDISTATE_NO_BEVELS)
  {
    iClass = WinGetClass(hWnd);
    iClass = _WinGetLowestClass(iClass);
    if (iClass == EDIT_CLASS || iClass == LISTBOX_CLASS)
    {
      HBRUSH hOldBr = SelectObject(hDC, GetStockObject(NULL_BRUSH));
      SelectObject(hDC, SysPen[SYSPEN_WINDOWFRAME]);
      Rectangle(hDC, r.left, r.top, r.right, r.bottom);
      SelectObject(hDC, hOldBr);
      goto bye;
    }
  }

  for (i = 0;  i < cyBorder;  i++)
  {
    SelectObject(hDC, hTop);
    MoveToEx(hDC, r.left + i,   r.bottom - i, NULL);  /* go to bot left  */
    LineTo(hDC, r.left + i,     r.top + i);           /* draw left side  */
    LineTo(hDC, r.right - i,    r.top + i);           /* draw top        */
  }

  for (i = 0;  i < cyBorder;  i++)
  {
    SelectObject(hDC, hBot);
    MoveToEx(hDC, r.right - i,    r.top + i + 1, NULL); /* go to top-right */
    LineTo(hDC, r.right - i,      r.bottom - i);     /* draw right side */
    LineTo(hDC, r.left + i + 1,   r.bottom - i);     /* draw bottom     */
  }
#else

  /*
    Draw the top and left borders in the iTopBrush color
  */
  if (cyBorder)
  {
    if (cyBorder == 1 && IS_PEN(iTopBrush))
    {
      SelectObject(hDC, GetStockObject(iTopBrush));  /* brush is really a pen */
      MoveToEx(hDC, r.left, r.top, NULL);
      LineTo(hDC, r.right-1, r.top);
    }
    else
    {
      SetRect(&rFill, r.left, r.top, r.right, r.top + cyBorder);
      FillRect(hDC, &rFill, 
        (iTopBrush & 0x8000) ? (iTopBrush & 0x7FFF): GetStockObject(iTopBrush));
    }
  }
  if (cxBorder)
  {
    if (cxBorder == 1 && IS_PEN(iTopBrush))
    {
      SelectObject(hDC, GetStockObject(iTopBrush));  /* brush is really a pen */
      MoveToEx(hDC, r.left, r.top, NULL);
      LineTo(hDC, r.left, r.bottom-1);
    }
    else
    {
      SetRect(&rFill, r.left, r.top, r.left + cxBorder, r.bottom);
      FillRect(hDC, &rFill, 
        (iTopBrush & 0x8000) ? (iTopBrush & 0x7FFF): GetStockObject(iTopBrush));
    }
  }

  /*
    Draw the bottom and right borders in the iBotBrush color
  */
  if (cyBorder)
  {
    if (cyBorder == 1 && IS_PEN(iBotBrush))
    {
      SelectObject(hDC, GetStockObject(iBotBrush));  /* brush is really a pen */
      MoveToEx(hDC, r.left + cxBorder, r.bottom-cyBorder, NULL);
      LineTo(hDC, r.right-1, r.bottom-cyBorder);
    }
    else
    {
      SetRect(&rFill, r.left + cxBorder, r.bottom - cyBorder, r.right, r.bottom);
      FillRect(hDC, &rFill,
        (iBotBrush & 0x8000) ? (iBotBrush & 0x7FFF): GetStockObject(iBotBrush));
    }
  }
  if (cxBorder)
  {
    if (cxBorder == 1 && IS_PEN(iBotBrush))
    {
      SelectObject(hDC, GetStockObject(iBotBrush));  /* brush is really a pen */
      MoveToEx(hDC, r.right-cxBorder, r.top, NULL);
      LineTo(hDC, r.right-cxBorder, r.bottom-1);
    }
    else
    {
      SetRect(&rFill, r.right - cxBorder, r.top, r.right, r.bottom);
      FillRect(hDC, &rFill, 
        (iBotBrush & 0x8000) ? (iBotBrush & 0x7FFF): GetStockObject(iBotBrush));
    }
  }
#endif

  /*
    Get rid of the window DC and return
  */
bye:
  _GetDC(hDC)->fFlags &= ~DC_ATOMICOPERATION;
  if (hOrigDC == NULL)
    ReleaseDC(hWnd, hDC);
}


/****************************************************************************/
/*                                                                          */
/* Function : GUIEraseBackground()                                          */
/*                                                                          */
/* Purpose  : Erases the client area of a window. The clipping area to      */
/*            erase is controlled by the passed DC.                         */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/* Called by: StdWindowWinProc() calls this in response to a                */
/*            WM_ERASEBKGND or WM_ICONERASEBKGND message.                   */
/*                                                                          */
/****************************************************************************/
#if !defined(MOTIF)
VOID PASCAL GUIEraseBackground(hWnd, hDC)
  HWND   hWnd;
  HDC    hDC;
{
  RECT   r;
  BOOL   bCreatedBrush = FALSE;
  HBRUSH hBr;
  HWND   hBrushWnd;
  LPHDC  lphDC;


  /*
    Get a pointer to the DC structure
  */
  lphDC = _GetDC(hDC);

  /*
    Determine the window whose background color we should query. For
    iconic windows or icon titles, we use the color of the parent.
  */
  if (IsIconic(hWnd) || (WID_TO_WIN(hWnd)->ulStyle & WIN_IS_ICONTITLE))
    hBrushWnd = GetParentOrDT(hWnd);
  else
    hBrushWnd = hWnd;

  /*
    Get the brush to erase with.
  */
  if ((hBr = GetClassWord(hBrushWnd, GCW_HBRBACKGROUND)) == BADOBJECT || !hBr)
  {
    /*
      If we were passed a valid DC, then the window's background color
      might have been diddled with by the WM_CTLCOLOR. So, use the
      background color for the erasing.
    */
    (void) bCreatedBrush;
    hBr = lphDC->hBrush;
  }

  /*
    See if the brush is a system brush. It is a system brush if
    the app did something like 
           wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
  */
  if (!(hBr & OBJECT_SIGNATURE) && hBr <= COLOR_BTNHIGHLIGHT+1)
  {
    hBr = SysBrush[hBr-1];
  }


  /*
    Determine the rect to erase
  */
  r = lphDC->rClipping;             /* screen coords of bad area */
  WinScreenRectToClient(hWnd, &r);  /* now in client coords */

  /*
    Perform the erasure
  */
  FillRect(hDC, (LPRECT) &r, hBr);

  /*
    Destroy the newly created brush
  */
  if (bCreatedBrush)
    DeleteObject(hBr);
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : WinDrawBorder()                                               */
/*                                                                          */
/* Purpose  : Draws the non-client area (border, caption, scrollbars) of    */
/*            a window.                                                     */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinDrawBorder(hWnd, bActive)
  HWND hWnd;
  INT  bActive;
{
  WINDOW *w = WID_TO_WIN(hWnd);

  /*
    Only draw the border if the window has the WS_BORDER flag set
    and if the window is visible.
  */
  if (w && IsWindowVisible(hWnd) && !IsIconic(hWnd))
  {
    DWORD dwFlags = w->flags;
    INT   iClass;

    /*
      Give the user a chance to draw his own border. If the user's winproc
      returns TRUE, then assume that the user took care of the border.
      The WM_BORDER message will get routed to GUIDrawBorder below.
    */
    if (HAS_BORDER(dwFlags))
#if defined(MOTIF)
      if (w->dwXflags & WSX_USEMEWELSHELL)
        GUIDrawBorder(hWnd, bActive);
#else
      SendMessage(hWnd, WM_BORDER, bActive, 0L);
#endif

    /*
      Refresh the scrollbars, since they'll get overwritten by the border.
      Do not refresh the scrollbars if we have a listbox which has the
      LBS_NOREDRAW flag set.
    */
    iClass = _WinGetLowestClass(WinGetClass(hWnd));
    if (iClass != LISTBOX_CLASS || !(dwFlags & LBS_NOREDRAW))
    {
      HWND hHSB, hVSB;
      BOOL bVert, bHorz;

      WinGetScrollbars(hWnd, &hHSB, &hVSB);
      bHorz = (BOOL) (hHSB != NULL && IsWindowVisible(hHSB));
      bVert = (BOOL) (hVSB != NULL && IsWindowVisible(hVSB));
      if (bHorz)
        SendMessage(hHSB, WM_PAINT, 0, 0L);
      if (bVert)
        SendMessage(hVSB, WM_PAINT, 0, 0L);

      /*
        If we have both vertical and horizontal scrollbars, then we
        must fill in the growbox area in the lower right corner.
      */
      if (bHorz && bVert)
      {
        RECT r;
        HDC  hDC;

        /*
          Get the growbox rectangle in 0-based coordinates.
        */
        SetRect(&r,
           w->rClient.right  - w->rect.left, 
           w->rClient.bottom - w->rect.top,
           w->rClient.right  + IGetSystemMetrics(SM_CXVSCROLL) - w->rect.left,
           w->rClient.bottom + IGetSystemMetrics(SM_CYHSCROLL) - w->rect.top);

        /*
          Fill the growbox area with the background brush
        */
        hDC = GetWindowDC(hWnd);
        FillRect(hDC, (LPRECT) &r, SysBrush[COLOR_SCROLLBAR]);
        ReleaseDC(hWnd, hDC);
      }
    }
  } /* end if (w && IsWindowVisible() && !IsIconic()) */
}


/****************************************************************************/
/*                                                                          */
/* Function : GUIDrawBorder(hWnd, bActive)                                  */
/*                                                                          */
/* Purpose  : Draws a border for a standard overlapped window.              */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: StdWindowWinProc() in response to the WM_BORDER message.      */
/*                                                                          */
/****************************************************************************/
VOID PASCAL GUIDrawBorder(hWnd, bActive)
  HWND hWnd;
  UINT bActive;
{
  RECT  r;
  INT   i;
  HPEN  hOldPen;
  DWORD dwFlags;
  INT   iClass;
  HDC   hDC;
  LPHDC lphDC;

  dwFlags = GetWindowLong(hWnd, GWL_STYLE);
  iClass  = WinGetClass(hWnd);
  iClass  = _WinGetLowestClass(iClass);

  /*
    Draw the interior of the frame of a pre-defined control.
  */
  if (iClass == LISTBOX_CLASS || iClass == EDIT_CLASS)
  {
    /*
      Do not draw the border of a listbox whose 'redraw' flag is off.
    */
    if (iClass == EDIT_CLASS || !(dwFlags & LBS_NOREDRAW))
      DrawBeveledBox(hWnd, (HDC) NULL, (LPRECT) NULL,
                     IGetSystemMetrics(SM_CXBORDER),
                     IGetSystemMetrics(SM_CYBORDER),
                     (InternalSysParams.hWndFocus == hWnd),
                     GetStockObject(NULL_BRUSH));
  }
  else
  {
    /*
      Get the window rectangle in 0-based coords
    */
    WindowRectToPixels(hWnd, (LPRECT) &r);
  
#if defined(MOTIF)
    XSysParams.ulFlags |= XFLAG_UPDATING_NCAREA;
#endif

    /*
      Get a window DC so we can draw in the non-client area
    */
    if ((hDC = GetWindowDC(hWnd)) == NULL)
      return;

    lphDC = GDISetup(hDC);
    if (lphDC)
      lphDC->fFlags |= DC_ATOMICOPERATION;
    else
    {
      /*
        The window is totally obscured.... then don't draw
      */
      ReleaseDC(hWnd, hDC);
      return;
    }

#if defined(MOTIF)
    bDrawingNCArea++;
#endif

#if defined(MOTIF)
    if ((dwFlags & (WS_THICKFRAME              )) == WS_THICKFRAME)
#else
    if ((dwFlags & (WS_THICKFRAME | WS_MAXIMIZE)) == WS_THICKFRAME)
#endif
    {
      RECT r2;

      int    cxFrame = IGetSystemMetrics(SM_CXFRAME);
      int    cyFrame = IGetSystemMetrics(SM_CYFRAME);
#if defined(USE_MOTIF_LOOK)
      int    cySize  = IGetSystemMetrics(SM_CYSIZE);
      int    cxSize  = IGetSystemMetrics(SM_CYSIZE);
#endif
      HBRUSH hBrush  = SysBrush[bActive ? COLOR_ACTIVEBORDER 
                                        : COLOR_INACTIVEBORDER];

      /*
        OPTIMIZATION NOTE :
        This could be done by a single call to FrameRect(), but the
        problem is that the underlying GUI's only take a certain
        sized pen (they do not have a width of 4, which is the value
        of SM_CXFRAME).
      */

      /*
        Draw the top of the thick-frame
      */
      SetRect(&r2, r.left, r.top, r.right, r.top + cyFrame);
      FillRect(hDC, &r2, hBrush);

      /*
        Draw left side of the thick-frame
      */
      SetRect(&r2, r.left, r.top, r.left + cxFrame, r.bottom);
      FillRect(hDC, &r2, hBrush);

      /*
        Draw bottom of the thick-frame
      */
      SetRect(&r2, r.left + cxFrame, r.bottom - cyFrame, r.right, r.bottom);
      FillRect(hDC, &r2, hBrush);

      /*
        Draw right side of the thick-frame
      */
      SetRect(&r2, r.right - cxFrame, r.top, r.right, r.bottom);
      FillRect(hDC, &r2, hBrush);

      /*
        Draw a 1-pixel black rectangle around the outside of the frame.
        Then, draw a 1-pixel black rect around the inside of the frame.
      */
#if !defined(USE_MOTIF_LOOK)
      SelectObject(hDC, GetStockObject(NULL_BRUSH));
      Rectangle(hDC, r.left, r.top, r.right, r.bottom);
      r2 = r;
      InflateRect(&r2, -(IGetSystemMetrics(SM_CXFRAME)-1),
                       -(IGetSystemMetrics(SM_CYFRAME)-1));
      Rectangle(hDC, r2.left, r2.top, r2.right, r2.bottom);
#endif

#if defined(USE_MOTIF_LOOK)
      /*
        This code draws bevels around the edges of the border, something
        which is done in GeoWorks but which takes too much time here.
      */
      DrawBeveledBox(hWnd, hDC, (LPRECT) &r, 2, 2,
                     FALSE, GetStockObject(NULL_BRUSH));
      InflateRect((LPRECT) &r, -(IGetSystemMetrics(SM_CXFRAME)-2),
                               -(IGetSystemMetrics(SM_CYFRAME)-2));
      DrawBeveledBox(hWnd, hDC, (LPRECT) &r, 2, 2,
                     TRUE, GetStockObject(NULL_BRUSH));
      InflateRect((LPRECT) &r, (IGetSystemMetrics(SM_CXFRAME)-2),
                               (IGetSystemMetrics(SM_CYFRAME)-2));
#endif

      /*
        Draw the resize markers. The vertical resize markers are spaced
        cyFrame*2 pixels away from the corners, and the horizontal
        markers are spaced cxFrame*2 pixels away from the corners.
      */

      hOldPen = SelectObject(hDC, SysPen[SYSPEN_WINDOWFRAME]);

/*
  This was defined by the people at C-Case to draw the resize markers
  better in MSC
*/

#if defined(USE_MOTIF_LOOK)
#define EINS  2
#define IEND  1
#else
#define EINS  1
#define IEND  0
#endif

      for (i = 0;  i <= IEND;  i++)
      {
        /*
          Top-left
        */
#if defined(USE_MOTIF_LOOK)
        MoveToEx(hDC, r.left+1,                 r.top + (cyFrame+cySize) + i, NULL);
        LineTo(hDC,   r.left + cxFrame - EINS,  r.top + (cyFrame+cySize) + i);
        MoveToEx(hDC, r.left + (cxFrame+cxSize) + i,  r.top+1, NULL);
        LineTo(hDC,   r.left + (cxFrame+cxSize) + i,  r.top + cyFrame - EINS);
    
        /*
          Bottom-left
        */
        MoveToEx(hDC, r.left+1,                 r.bottom - (cyFrame+cySize) + i, NULL);
        LineTo(hDC,   r.left + cxFrame - EINS,  r.bottom - (cyFrame+cySize) + i);
        MoveToEx(hDC, r.left + (cxFrame+cxSize) + i,  r.bottom - cyFrame + 2, NULL);
        LineTo(hDC,   r.left + (cxFrame+cxSize) + i,  r.bottom - EINS);
    
        /*
          Top-right
        */
        MoveToEx(hDC, r.right - cxFrame + 2, r.top + (cyFrame+cySize) + i, NULL);
        LineTo(hDC,   r.right - EINS,        r.top + (cyFrame+cySize) + i);
        MoveToEx(hDC, r.right - (cxFrame+cxSize) + i - 2, r.top+1, NULL);
        LineTo(hDC,   r.right - (cxFrame+cxSize) + i - 2, r.top + cyFrame - EINS);
    
        /*
          Bottom-right
        */
        MoveToEx(hDC, r.right - cxFrame + 2, r.bottom - (cyFrame+cySize) + i, NULL);
        LineTo(hDC,   r.right - EINS,        r.bottom - (cyFrame+cySize) + i);
        MoveToEx(hDC, r.right - (cxFrame+cxSize) + i, r.bottom - cyFrame + 2, NULL);
        LineTo(hDC,   r.right - (cxFrame+cxSize) + i, r.bottom - EINS);

#else
        MoveToEx(hDC, r.left,              r.top + (cyFrame*2) + i, NULL);
        LineTo(hDC, r.left + cxFrame - EINS,  r.top + (cyFrame*2) + i);
        MoveToEx(hDC, r.left + (cxFrame*2) + i,  r.top, NULL);
        LineTo(hDC, r.left + (cxFrame*2) + i,  r.top + cyFrame - EINS);
    
        /*
          Bottom-left
        */
        MoveToEx(hDC, r.left,                r.bottom - (cyFrame*2) + i, NULL);
        LineTo(hDC, r.left + cxFrame - EINS,  r.bottom - (cyFrame*2) + i);
        MoveToEx(hDC, r.left + (cxFrame*2) + i,  r.bottom - cyFrame + 1, NULL);
        LineTo(hDC, r.left + (cxFrame*2) + i,  r.bottom - EINS);
    
        /*
          Top-right
        */
        MoveToEx(hDC, r.right - cxFrame + 1, r.top + (cyFrame*2) + i, NULL);
        LineTo(hDC, r.right - EINS,        r.top + (cyFrame*2) + i);
        MoveToEx(hDC, r.right - (cxFrame*2) + i, r.top, NULL);
        LineTo(hDC, r.right - (cxFrame*2) + i, r.top + cyFrame - EINS);
    
        /*
          Bottom-right
        */
        MoveToEx(hDC, r.right - cxFrame + 1, r.bottom - (cyFrame*2) + i, NULL);
        LineTo(hDC, r.right - EINS,        r.bottom - (cyFrame*2) + i);
        MoveToEx(hDC, r.right - (cxFrame*2) + i, r.bottom - cyFrame + 1, NULL);
        LineTo(hDC, r.right - (cxFrame*2) + i, r.bottom - EINS);
#endif


#if defined(USE_MOTIF_LOOK)
      SelectObject(hDC, GetStockObject(WHITE_PEN));
#endif
      }
  
      SelectObject(hDC, hOldPen);
    }
    else if (!(dwFlags & WS_MAXIMIZE))
    {
      /*
        The window does not have a thick-frame. Just draw a black rectangle
        and don't fill it in.
        Note : The width of the border should really be governed by
        SM_CXBORDER and SM_CYBORDER, but these are usually 1.
      */
      HBRUSH hOldBr;
      hOldPen = SelectObject(hDC, SysPen[SYSPEN_WINDOWFRAME]);
      hOldBr  = SelectObject(hDC, GetStockObject(NULL_BRUSH));
      Rectangle(hDC, r.left, r.top, r.right, r.bottom);
      SelectObject(hDC, hOldPen);
      SelectObject(hDC, hOldBr);
    }

    /*
      Draw the caption
    */
    if (HAS_CAPTION(dwFlags))
      GUIDrawCaption(hWnd, hDC, (BOOL) bActive);

#if defined(MOTIF)
    bDrawingNCArea--;
#endif
    if (lphDC)
      lphDC->fFlags &= ~DC_ATOMICOPERATION;
    ReleaseDC(hWnd, hDC);
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GUIDrawCaption()                                              */
/*                                                                          */
/* Purpose  : Draws a caption bar in a window.                              */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/* Called by: GUIDrawBorder().                                              */
/*                                                                          */
/****************************************************************************/
VOID PASCAL GUIDrawCaption(HWND hWnd, HDC hDC, BOOL bActive)
{
  TEXTMETRIC tm;
  RECT   r;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags;
  HBRUSH hOldBr;
  HPEN   hOldPen;

  /*
    Make sure that there is enough room for the caption
  */
  if (w->rect.right - w->rect.left <= 0)
    return;

  dwFlags = w->flags;

  /*
    Never draw the caption bar of a maximized MDI document
  */
#if !defined(MOTIF)
  if ((dwFlags & WS_MAXIMIZE) && IS_MDIDOC(w))
    return;
#endif

  /*
    Get a rectangle which is equal to the top row, and adjusted for the
    frame. This is where the caption background will be drawn.
  */
  WindowRectToPixels(hWnd, (LPRECT) &r);
#if defined(MOTIF)
  if ((dwFlags & (WS_BORDER | WS_THICKFRAME))                            )
#else
  if ((dwFlags & (WS_BORDER | WS_THICKFRAME)) && !(dwFlags & WS_MAXIMIZE))
#endif
  {
    if (dwFlags & WS_THICKFRAME)
      InflateRect(&r, -IGetSystemMetrics(SM_CXFRAME),
                      -IGetSystemMetrics(SM_CYFRAME));
#if 81993
    else
    {
      r.top += IGetSystemMetrics(SM_CYBORDER);
      InflateRect(&r, -IGetSystemMetrics(SM_CXBORDER), 0);
    }
#else
    else
      InflateRect(&r, -IGetSystemMetrics(SM_CXBORDER),
                      -IGetSystemMetrics(SM_CYBORDER));
#endif
  }
  r.bottom = (MWCOORD) (r.top + IGetSystemMetrics(SM_CYCAPTION));


#if defined(MOTIF)
  if (XSysParams.ulFlags & XFLAG_RUBBERBANDING)
    goto just_draw_caption_bevel;
#endif


  /*
    Draw a black box around the caption and fill the caption bar with
    the active or inactive color
  */
  hOldBr  = SelectObject(hDC, 
              SysBrush[bActive ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION]);
  hOldPen = SelectObject(hDC, GetStockObject(NULL_PEN));
  Rectangle(hDC, r.left, r.top, r.right, r.bottom);
  SelectObject(hDC, hOldPen);

#if !defined(USE_MOTIF_LOOK)
  /*
    11/1/93 (maa)
    If we do not have a menu, then the window will erase the bottom line
    of the caption. So, bump up the line by one pixel.
  */
/*if (w->hMenu == NULL)*/
    r.bottom--;
  MoveTo(hDC, r.left,  r.bottom);
  LineTo(hDC, r.right, r.bottom);
#endif

  SelectObject(hDC, hOldBr);

  /*
    Output the caption text
  */
  if (w->title && w->title[0])
  {
#if defined(MOTIF)
    int sLen = (int) LOWORD(GetTextExtent(hDC, w->title, lstrlen(w->title)));
#else
    int sLen = (int) LOWORD(_GetTextExtent(w->title));
#endif
    GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);
    SetTextColor(hDC, 
        GetSysColor(bActive ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));
    SetBkMode(hDC, TRANSPARENT);
    TextOut(hDC,
            r.left + (RECT_WIDTH(r)  - sLen)/2, 
            r.top  + ((RECT_HEIGHT(r) - 2 - tm.tmHeight) / 2),
            (LPSTR) w->title, lstrlen(w->title));
    SetTextAlign(hDC, TA_TOP | TA_LEFT);
  }

  /*
    Draw the min/max box
  */
  if ((dwFlags & WS_MINIMIZEBOX) && !(dwFlags & WS_MINIMIZE))
    DrawSysTransparentBitmap(hWnd, hDC, SYSBMPT_MINIMIZE);
  if (dwFlags & WS_MAXIMIZEBOX)
  {
    if (dwFlags & WS_MAXIMIZE)
      DrawSysTransparentBitmap(hWnd, hDC, SYSBMPT_RESTORE);
    else
      DrawSysTransparentBitmap(hWnd, hDC, SYSBMPT_MAXIMIZE);
  }

  /*
    Draw the system-menu icon
  */
  if (dwFlags & WS_SYSMENU)
#if defined (USE_BITMAPS_IN_MENUS)
    DrawSysTransparentBitmap(hWnd, hDC, SYSBMPT_SYSMENU + IS_MDIDOC(w));
#else
    DrawSysTransparentBitmap(hWnd, hDC, SYSBMPT_SYSMENU);
#endif


#if defined(USE_MOTIF_LOOK)
just_draw_caption_bevel:
  /*
    Draw a bevel around the captioned text
  */
  if (dwFlags & WS_SYSMENU)
    r.left += IGetSystemMetrics(SM_CXSIZE);
  if ((dwFlags & WS_MINIMIZEBOX) && !(dwFlags & WS_MINIMIZE))
    r.right -= IGetSystemMetrics(SM_CXSIZE);
  if (dwFlags & WS_MAXIMIZEBOX)
    r.right -= IGetSystemMetrics(SM_CXSIZE);
  DrawBeveledBox(hWnd, hDC, (LPRECT) &r, 1, 1,
      ((XSysParams.ulFlags & XFLAG_RUBBERBANDING) && bActive) ? TRUE : FALSE, 
      GetStockObject(NULL_BRUSH));
#endif
}


/****************************************************************************/
/*                                                                          */
/*                          SYSTEM ICON STUFF                               */
/*                                                                          */
/****************************************************************************/
extern SYSCAPTIONBITMAPINFO HUGEDATA SysCaptionBitmapInfo[9];

#if !defined(MOTIF)
VOID FAR PASCAL DrawSysTransparentBitmap(HWND hWnd, HDC hOrigDC, INT idxIcon)
{
  RECT   r;
  PSYSCAPTIONBITMAPINFO psi;
  HDC    hDC;
  LPHDC  lphDC;
  BITMAPINFOHEADER bmi;
  LPSTR  lpBitmapBytes;
  INT    nTotalBytesPerLine;
  INT    x, y, row;
  BOOL   bHasFrame;
  DWORD  dwFlags;
  HBRUSH hOldBrush;
  WINDOW *w;


  /*
    Get the window's style bits
  */
  w = WID_TO_WIN(hWnd);
  dwFlags = w->flags;
  bHasFrame = (BOOL) ((dwFlags & WS_THICKFRAME) != 0L);

  /*
    Get a pointer to the info for the bitmap we are drawing
  */
  psi = &SysCaptionBitmapInfo[idxIcon];

  /* 
    Figure out how far left from the window frame we must start.
  */
  if ((dwFlags & WS_MAXIMIZE) || idxIcon == SYSBMPT_COMBO)
    x = 0;
  else
    x = -IGetSystemMetrics(bHasFrame ? SM_CXFRAME : SM_CXBORDER);
  psi->cxPixelOffset =
      (idxIcon == SYSBMPT_SYSMENU || idxIcon == SYSBMPT_MDISYSMENU) ? -x : x;

  /* 
    Figure out how far down from the window frame we must start.
  */
  if ((dwFlags & WS_MAXIMIZE) || idxIcon == SYSBMPT_COMBO)
    y = 0;
  else
    y = IGetSystemMetrics(bHasFrame ? SM_CYFRAME : SM_CYBORDER) + 1;
  psi->cyPixelOffset = y;


  /*
    If we have a minimize box without a maximize box, then the min bitmap
    goes flush against the right side.
  */
  SysCaptionBitmapInfo[SYSBMPT_MINIMIZE].cxRightColOffset    = 
  SysCaptionBitmapInfo[SYSBMPT_MINIMIZEDEP].cxRightColOffset = 
     ((dwFlags & WS_MAXIMIZEBOX) != 0L) ? 2 : 1;


  /*
    Copy the relevant parts of the psi structure to the bitmap header
  */
  bmi.biWidth    = psi->biWidth;
  bmi.biHeight   = psi->biHeight;
  bmi.biBitCount = psi->biBitCount;

  /*
    Figure out the position of the bevelled box
  */
  WindowRectToPixels(hWnd, (LPRECT) &r);

  /*
    If cxRightColOffset is non-zero, then the icon should be right-justified
    within the window.
    cxRightColOffset is the number (minus 1) of SM_CXSIZE units we should
    slide over leftwards before the icon is drawn.
  */
  if (psi->cxRightColOffset)
  {
    r.left  = (MWCOORD) (r.right - (psi->cxRightColOffset * IGetSystemMetrics(SM_CXSIZE)));
    r.right = (MWCOORD) (r.left + IGetSystemMetrics(SM_CXSIZE));
  }
  else
  {
    r.right = (MWCOORD) (r.left + (int) bmi.biWidth);
  }

  r.left  += (MWCOORD) psi->cxPixelOffset;
  r.right += (MWCOORD) psi->cxPixelOffset;
  r.top   += (MWCOORD) psi->cyPixelOffset;
  r.bottom = (MWCOORD) (r.top + (int) bmi.biHeight);


  /*
    Figure out how many pixels one byte translates into
  */
  switch (bmi.biBitCount)
  {
    case 1 : /* 1 byte = 8 pixels */
      nTotalBytesPerLine = (int) (bmi.biWidth >> 3);
      break;
    case 4 : /* 1 byte = 2 pixels */
      nTotalBytesPerLine = (int) (bmi.biWidth >> 1);
      break;
    case 8 : /* 1 byte = 1 pixels */
      nTotalBytesPerLine = (int) (bmi.biWidth);
      break;
    default :
      return;
  }


  /*
    Get a window DC to draw into if one wasn't passed in. Also, set the
    viewport.
  */
  if ((hDC = hOrigDC) == NULL)
    hDC = GetWindowDC(hWnd);
  if (!_GraphicsSetViewport(hDC))
    goto bye;


  /*
    Offset the drawing area by the clipping region.
  */
  lphDC = _GetDC(hDC);
  OffsetRect(&r, w->rect.left - lphDC->rClipping.left,
                 w->rect.top  - lphDC->rClipping.top);

  /*
    Draw a bevelled box to hold the icon. Just a simple filled gray rectangle.
  */
#if defined(USE_MOTIF_LOOK)
  DrawBeveledBox(hWnd, hDC, (LPRECT) &r, 1, 1,
                 FALSE, GetStockObject(NULL_BRUSH));
#else
  InflateRect(&r, 1, 1);
  hOldBrush = SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
  Rectangle(hDC, r.left, r.top, r.right, r.bottom);
  SelectObject(hDC, hOldBrush);
  InflateRect(&r, -1, -1);
#endif


#if !defined(USE_MOTIF_LOOK)
  /*
    Draw the bitmap centered within the bevelled box
  */
  if (lphDC->hRgnVis)
  {
    RECT r3, rTmp;
    INT  i;
    PREGION pRegion;

    /*
      Get 'r' into screen coordinates.
    */
    OffsetRect(&r, lphDC->rClipping.left, lphDC->rClipping.top);
  
    /*
      Clip the bits to the screen. 'r' is in screen coordinates.
    */
    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);
    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      if (IntersectRect(&rTmp, &pRegion->rects[i], &r))
      {
        SetRect(&r3, rTmp.left, rTmp.top, SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
        if (!IntersectRect(&r3, &r3, &rTmp))
          continue;
        _setviewport(r3.left, r3.top, r3.right, r3.bottom);

        /*
          x and y are the 0-based offsets within rectangle 'r' which the
          bitmap should be draw at.
        */
        x =  ((RECT_WIDTH(r)  - (int) bmi.biWidth)  / 2);
        y =  ((RECT_HEIGHT(r) - (int) bmi.biHeight) / 2);

        /*
          If we are drawing partially off the screen, then the starting
          x and y coordinates should be off the screen.
        */
        if (r.left - r3.left < 0)
          x = r.left - r3.left;
        if (r.top  - r3.top < 0)
          y = r.top  - r3.top;

        lpBitmapBytes = (LPSTR) psi->lpBits;
        for (row = y;  row < y + (int) bmi.biHeight;  row++)
        {
          int col, j, iBit, ch;
      
          col = x;
          for (j = 0;  j < nTotalBytesPerLine;  j++)
          {
            switch (bmi.biBitCount)
            {
              case 1 :
                for (iBit = 0;  iBit < 8;  iBit++)
                {
                  if ((*lpBitmapBytes & (0x80 >> iBit)) != 0)
                    SETPIXEL(hDC, col, row, BLACK);
                  col++;
                }
                break;
      
              case 4 :
                if ((ch = (*lpBitmapBytes >> 4) & 0x0F) != WHITE)
                  SETPIXEL(hDC, col, row, ch);
                col++;
                if ((ch = *lpBitmapBytes & 0x0F) != WHITE)
                  SETPIXEL(hDC, col, row, ch);
                col++;
                break;
      
              case 8 :
                if ((ch = *lpBitmapBytes) != WHITE)
                  SETPIXEL(hDC, col, row, ch);
                col++;
                break;
            }
            lpBitmapBytes++;
          } /* for i */
        } /* for row */

      }
    }
    UNLOCKREGION(lphDC->hRgnVis);
  }
#endif


bye:
  /*
    Reset the original viewport and get rid of the window DC.
  */
  _setviewport(SysGDIInfo.rectLastClipping.left, 
               SysGDIInfo.rectLastClipping.top, 
               SysGDIInfo.rectLastClipping.right, 
               SysGDIInfo.rectLastClipping.bottom);
  if (hOrigDC == NULL)
    ReleaseDC(hWnd, hDC);
}
#endif /* !Motif */


/****************************************************************************/
/*                                                                          */
/* Function : WinTrackSysIcon(hWnd, iHitCode)                               */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL WinTrackSysIcon(hWnd, iHitCode)
  HWND hWnd;
  INT  iHitCode;
{
  MSG    msg;
  RECT   r;
  LPRECT lprWindow;
  INT    idxIcon;
  BOOL   bDepressed;

  lprWindow = WinCalcWindowRects(hWnd);
  r = lprWindow[iHitCode];

  switch (iHitCode)
  {
    case HTMINBUTTON :
      idxIcon = SYSBMPT_MINIMIZE;
      break;
    case HTMAXBUTTON :
      idxIcon = (IsZoomed(hWnd) || IsIconic(hWnd)) ? SYSBMPT_RESTORE
                                                   : SYSBMPT_MAXIMIZE;
      break;
    default :
      return FALSE;
  }

  /*
    Before we go into this modal loop, make sure that any
    windows which need to be refreshed are refreshed.
  */
  RefreshInvalidWindows(_HwndDesktop);

  /*
    Note :
      We cannot do hDC=GetWindowDC(hWnd) here, because the mouse will
    stay hidden throughout the loop below until we ReleaseDC.
  */

  bDepressed = TRUE;
  DrawSysTransparentBitmap(hWnd, 0, idxIcon+bDepressed);

  while (!GetMouseMessage(&msg) || msg.message != WM_LBUTTONUP)
  {
    if (msg.message == WM_MOUSEMOVE)
    {
      BOOL bInRect = PtInRect(&r, MAKEPOINT(msg.lParam));
      if (!bDepressed && bInRect)
      {
        bDepressed = TRUE;
        DrawSysTransparentBitmap(hWnd, 0, idxIcon+bDepressed);
      }
      else if (bDepressed && !bInRect)
      {
        bDepressed = FALSE;
        DrawSysTransparentBitmap(hWnd, 0, idxIcon+bDepressed);
      }
    }
  }

  /*
    Redraw the system icon in its undepressed state
  */
  if (bDepressed)
    DrawSysTransparentBitmap(hWnd, 0, idxIcon+FALSE);

  /*
    Return TRUE if the mouse was released within the icon, FALSE if not.
  */
  return PtInRect(&r, MAKEPOINT(msg.lParam));
}


/****************************************************************************/
/*                                                                          */
/*                   SYSTEM BITMAP RENDERING                                */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL InitSysBitmaps(void);
extern SYSBITMAPINFO HUGEDATA SysBitmapInfo[5];


static VOID PASCAL InitSysBitmaps(void)
{
  char achBitmap[sizeof(BITMAPINFO) + (16 * sizeof(RGBQUAD))];
  int  i;

  extern BITMAPINFO SysIconBMInfo16;
  extern RGBQUAD SysDefaultRGB[16];


  /*
    Copy the default bitmap header and RGBQUAD array
  */
  lmemcpy(achBitmap, (LPSTR) &SysIconBMInfo16, sizeof(BITMAPINFOHEADER));
  lmemcpy(((BITMAPINFO *) achBitmap)->bmiColors, (LPSTR) SysDefaultRGB, 16*sizeof(RGBQUAD));


  /*
    Create each of the system bitmaps.
  */
  for (i = 0;  i < sizeof(SysBitmapInfo)/sizeof(SysBitmapInfo[0]);  i++)
  {
    ((LPBITMAPINFOHEADER) achBitmap)->biWidth  = SysBitmapInfo[i].biWidth;
    ((LPBITMAPINFOHEADER) achBitmap)->biHeight = SysBitmapInfo[i].biHeight;

    SysBitmapInfo[i].hBitmap =
      CreateDIBitmap((HDC) 0, 
                     (LPBITMAPINFOHEADER) achBitmap,
                     CBM_INIT,
                     (LPSTR) SysBitmapInfo[i].lpBits,
                     (LPBITMAPINFO) achBitmap,
                     DIB_RGB_COLORS);
  }
}


VOID FAR PASCAL DrawSysBitmap(HDC hDC, INT idxBitmap, INT x, INT y)
{
  static BOOL bInitBitmaps = FALSE;

  if (!bInitBitmaps)
  {
    InitSysBitmaps();
    bInitBitmaps = TRUE;
  }
  DrawBitmapToDC(hDC, x, y, SysBitmapInfo[idxBitmap].hBitmap, SRCCOPY);
}

