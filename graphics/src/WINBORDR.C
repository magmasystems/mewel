/*
 *
 * File    : WINBORDR.C
 *
 * Purpose : WinDrawBorder()  - draw a border around a window
 *
 * History :
 *  11/07/89 (maa) - Added support for system colors.
 *  11/07/89 (maa) - Added support for zoomed windows.
 *  08/20/90 (maa) - Added support for the restore icon and cleaned up code.
 *
 * (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved
 *
 * $Log:   E:/vcs/mewel/winbordr.c_v  $
//	
//	   Rev 1.3   30 Nov 1993 17:31:48   Adler
//	Added a few Televoice changes.
//	
//	   Rev 1.2   08 Jul 1993  9:18:26   Adler
//	Entered TELEVOICE ifdef's
//	
//	   Rev 1.1   24 May 1993  8:49:16   unknown
//	Added ifdefs for ZAPP and the no-border stuff for dialogs.
//	
//	   Rev 1.0   23 May 1993 21:06:02   adler
//	Initial revision.
 * 
 *    Rev 1.1   16 Aug 1991 16:28:00   MLOCKE
 * Altered file header block for compatibility with PVCS.
 * 
 * Added support for thick borders around modal dialogs to give
 * the user a visual cue that action outside the dialog is
 * not allowed.  Modal dialogs with WS_POPUP style have outlines
 * of thick lines using character graphic thick line characters.
 */

#include "wprivate.h"
#include "window.h"

#ifdef COMPUSERVE_MEWEL
#define POPUP_COLOR SYSCLR_MODALCAPTION
#define MODAL_COLOR SYSCLR_MODALBORDER
#define WIN_HAS_TITLE(w) (w->title && w->title[0])
#endif

#define CH_TOPLINE    0
#define CH_BOTLINE    1
#define CH_LEFTLINE   2
#define CH_RIGHTLINE  3
#define CH_TOPLEFT    4
#define CH_BOTLEFT    5
#define CH_TOPRIGHT   6
#define CH_BOTRIGHT   7

static VOID PASCAL _WinFastPutc(PWINDOW,int,int,int,COLOR);
static INT  PASCAL WinDrawCaption(HWND, BOOL);


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
VOID FAR PASCAL WinDrawBorder(hWnd, typeBorder)
  HWND hWnd;
  INT  typeBorder;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  COLOR  attr;
  HWND   hHSB, hVSB;
  DWORD  dwFlags;
  
#if defined(MOTIF) || defined(DECWINDOWS)
  return;
#endif

  /*
    Signal that we're in the middle of a border drawing operation. This
    tells some of the drawing routines in WINDRAW.C that it's legal
    to draw outside the client area in row number "-1" of a window.
  */
  bDrawingBorder = TRUE;

  /*
    Only draw the border if the window has the WS_BORDER flag set
    and if the window is visible.
  */
  if (w && IsWindowVisible(hWnd))
  {
#ifdef MULTMONITORS
    MonitorSwitch(w->iMonitor, SAVE);
#endif

    dwFlags = w->flags;

    /*
      Give the user a chance to draw his own border. If the user's winproc
      returns TRUE, then assume that the user took care of the border.
    */
    if (HAS_BORDER(dwFlags))
    {
      if (!SendMessage(hWnd, WM_BORDER, typeBorder, 0L))
      {
        /*
          Get attribute info.
        */
#ifdef TELEVOICE
        if (dwFlags & WS_SYSCOLOR)
#else
        if ((attr = w->attr) == SYSTEM_COLOR)
#endif
        {
          int idClass = _WinGetLowestClass(w->idClass);
          if (idClass == PUSHBUTTON_CLASS || IsFrameClass(w))
            attr = WinGetClassBrush(hWnd);
          else
          {
#ifdef COMPUSERVE_MEWEL
            if (dwFlags & WS_POPUP)
              attr = WinQuerySysColor(NULLHWND, SYSCLR_MODALBORDER);
            else
#endif
            attr = WinQuerySysColor(NULLHWND,
                  ((dwFlags & WS_DISABLED) != 0L) ? SYSCLR_DISABLEDBORDER :
                  (typeBorder ? SYSCLR_ACTIVEBORDER : SYSCLR_INACTIVEBORDER));
           }
        }
#ifdef TELEVOICE
        else  /* doesn't use system colors */
        {
          attr = w->attr;					 
          /*
            Show modeless dialogs with active or inactive border color.
          */
          if ((dwFlags & (WS_POPUP | WS_DLGFRAME)) == (WS_POPUP | WS_DLGFRAME))
          {
            attr = WinQuerySysColor(NULLHWND,
                  (typeBorder ? SYSCLR_ACTIVEBORDER : SYSCLR_INACTIVEBORDER));
          }
        }
#endif

        WinDrawFrame(hWnd, attr, typeBorder);
      } /* end if WM_BORDER */
    } /* end if WS_BORDER */

    /*
      See if the window has a caption but no side and bottom borders.
    */
    else if (HAS_CAPTION(dwFlags))
    {
      WinDrawCaption(hWnd, (BOOL) ((w->ulStyle & WIN_ACTIVE_BORDER) != 0L));
    }

    /*
      Refresh the scrollbars, since they'll get overwritten by the border.
      Do not refresh the scrollbars if we have a listbox which has the
      LBS_NOREDRAW flag set.
    */
    if (WinGetClass(hWnd) != LISTBOX_CLASS || !(dwFlags & LBS_NOREDRAW))
    {
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
      if (bHorz && bVert && !(dwFlags & WS_BORDER))
        WinPutc(hWnd, w->rect.bottom - w->rect.top  - 1,
                      w->rect.right  - w->rect.left - 1, 
                      w->fillchar, WinQuerySysColor(NULL, SYSCLR_SCROLLBAR));
    }

#ifdef MULTMONITORS
    MonitorSwitch(w->iMonitor, RESTORE);
#endif
  } /* end if (w && IsWindowVisible()) */

  bDrawingBorder = FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : WinDrawFrame()                                                */
/*                                                                          */
/* Purpose  : Responsible for drawing the frame of a "normal" window.       */
/*                                                                          */
/* Returns  : TRUE if it's drawn, FALSE if not.                             */
/*                                                                          */
/* Called by: WinDrawBorder(). Also StaticDraw() when a frame window is to  */
/*            be drawn.                                                     */
/****************************************************************************/
INT FAR PASCAL WinDrawFrame(hWnd, attr, typeBorder)
  HWND     hWnd;
  COLOR    attr;
  INT      typeBorder;
{
  char     szLineBuf[MAXSCREENWIDTH+1];
  WINDOW   *w = WID_TO_WIN(hWnd);
  INT      height, width;
  INT      originTop, originLeft;
  BOOL     bHasVertSB = FALSE, bHasHorzSB = FALSE;
  PSTR     pszBorder;
  int      idClass;
  DWORD    dwFlags;
  
#ifdef VAXC
#pragma nostandard
#endif
#define  WINPUTC(h,r,c,ch,attr)   _WinFastPutc(w,r,c,ch,attr)
#ifdef VAXC
#pragma standard
#endif

#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#else

#if defined(ZAPP) && !defined(MEWEL_GUI)
  BOOL noBorder = !strcmp(WinGetClassName(hWnd), "DIALOGFRAMECL");
#endif

  if (!w || !(HAS_BORDER(w->flags)) || !IsWindowVisible(hWnd))
    return FALSE;

  dwFlags = w->flags;
  idClass = _WinGetLowestClass(w->idClass);

#ifdef COMPUSERVE_MEWEL
  if (dwFlags & WS_POPUP)
    typeBorder = BORDER_POPUP;
#endif

  /*
    Calculate the height and width of the border
  */
  height = RECT_HEIGHT(w->rect) - 2;
  width  = RECT_WIDTH(w->rect)  - 2;
  if (height <= 0 || width <= 0)
    return FALSE;

  /*
    Calculate the origin of the border.
    If the client area is not the same as the window area, the offset
    for WinPutc() must be -1 in order to draw a border outside the
    client area.
  */
  originTop  = w->rect.top - w->rClient.top;
  originLeft = (w->rect.left == w->rClient.left) ? 0 : -1;

  /*
    Calculate the color attribute
  */
  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(w->win_id);

  pszBorder = SysBoxDrawingChars[typeBorder];  /* point to the proper array */

  /*
    Draw the top corners of the border
  */
  WINPUTC(hWnd, originTop,  originLeft,         pszBorder[CH_TOPLEFT],  attr);
  WINPUTC(hWnd, originTop,  width+originLeft+1, pszBorder[CH_TOPRIGHT], attr);
  /*
    Draw the bottom corners of the border
  */
#ifdef ZAPP
  /* changed for zapp no borders */  
  WINPUTC(hWnd, height+originTop+1,  originLeft,         (noBorder) ? ' ' : pszBorder[CH_BOTLEFT],  attr);
  WINPUTC(hWnd, height+originTop+1,  width+originLeft+1, (noBorder) ? ' ' : pszBorder[CH_BOTRIGHT], attr);
#else
  WINPUTC(hWnd, height+originTop+1,  originLeft,         pszBorder[CH_BOTLEFT],  attr);
  WINPUTC(hWnd, height+originTop+1,  width+originLeft+1, pszBorder[CH_BOTRIGHT], attr);
#endif

  /*
    In order to speed up drawing time, we see if the window has visible
    scrollbars. If it does, then don't draw the border where the
    scrollbars intersect it.
  */
  if (WinHasScrollbars(hWnd, SB_BOTH))
  {
    HWND hHSB, hVSB;

    WinGetScrollbars(hWnd, &hHSB, &hVSB);
    if (hHSB && IsWindowVisible(hHSB))
      bHasHorzSB = TRUE;
    if (hVSB && IsWindowVisible(hVSB))
      bHasVertSB = TRUE;
  }

  /*
    Draw the top line of the frame...
  */
  if (HAS_CAPTION(dwFlags))
    WinDrawCaption(hWnd, (BOOL) ((w->ulStyle & WIN_ACTIVE_BORDER) != 0L));
  else if (idClass==PUSHBUTTON_CLASS || idClass==BUTTON_CLASS || IS_MENU(w))
  {
    INT width2 = min(width, sizeof(szLineBuf)-1);
    memset(szLineBuf, pszBorder[CH_TOPLINE], width2);
    szLineBuf[width] = '\0';
    WinPuts(hWnd, originTop, originLeft+1, szLineBuf, attr);
  }
  else
    WinDrawCaption(hWnd, (BOOL) (typeBorder != 0 &&
                                ((w->ulStyle & WIN_ACTIVE_BORDER) != 0 || 
                                 InternalSysParams.hWndFocus==hWnd || 
                                 GetActiveWindow()==hWnd)));

  /*
    Draw the bottom line of the frame...
  */
  if (!bHasHorzSB)    /* don't draw over the horiz scrollbar */
  {
    INT width2 = min(width, sizeof(szLineBuf)-1);
#ifdef ZAPP
    memset(szLineBuf, (noBorder) ? ' ' : pszBorder[CH_BOTLINE], width2);
#else
    memset(szLineBuf, pszBorder[CH_BOTLINE], width2);
#endif
    szLineBuf[width] = '\0';
    WinPuts(hWnd, height+originTop+1, originLeft+1, szLineBuf, attr);
  }

  /*
    Fill in the sides
  */
  while (--height >= 0)
  {
    int row = height+originTop+1;
#ifdef ZAPP
    WINPUTC(hWnd, row, originLeft, (noBorder) ? ' ' : pszBorder[CH_LEFTLINE], attr);
    if (!bHasVertSB || height == 0)
      WINPUTC(hWnd, row, width+originLeft+1, (noBorder) ? ' ' : pszBorder[CH_RIGHTLINE], attr);
#else
    WINPUTC(hWnd, row, originLeft, pszBorder[CH_LEFTLINE], attr);
    if (!bHasVertSB || height == 0)
      WINPUTC(hWnd, row, width+originLeft+1, pszBorder[CH_RIGHTLINE], attr);
#endif
  }

  return TRUE;

#endif /* MOTIF */
}


/****************************************************************************/
/*                                                                          */
/* Function : WinDrawCaption()                                              */
/*                                                                          */
/* Purpose  : Draws the caption bar of a window, including the icons which  */
/*            are part of the caption bar. If the window does not have the  */
/*            WS_CAPTION style, a line with the frame style is drawn.       */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
static INT PASCAL WinDrawCaption(HWND hWnd, BOOL bActive)
{
  char   szLineBuf[MAXSCREENWIDTH+1];
  COLOR  attrBar;
  INT    width, width2;
  INT    originTop, originLeft;
  BYTE   chFill;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags;
  BOOL   bIsDisabled;
  int    iClass;

#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#else

  /*
    Calculate the width of the caption and the coordinates...
  */
  if ((width = RECT_WIDTH(w->rect)) <= 0)
    return FALSE;
  dwFlags = w->flags;


  /*
    These values can be 0 or -1
  */
  originTop  = w->rect.top  - w->rClient.top;
  originLeft = w->rect.left - w->rClient.left;

  bIsDisabled = ((dwFlags & WS_DISABLED) != 0L);
  iClass = _WinGetLowestClass(w->idClass);

  /*
    Increment the border-drawing level. This signals WinPuts() that it's
    allright to draw outside of the window's client area.
  */
  bDrawingBorder++;


  /*
    If the window has the WS_CAPTION style, then draw a caption in the
    active or inactive color.
  */
  if (HAS_CAPTION(dwFlags))
  {
    chFill = WinGetSysChar(bActive ? SYSCHAR_ACTIVE_CAPTION
                                   : SYSCHAR_INACTIVE_CAPTION);
    width2 = min(width, sizeof(szLineBuf)-1);
    memset(szLineBuf, chFill, width2);
    szLineBuf[width] = '\0';
    attrBar = WinQuerySysColor(NULLHWND, bActive ? SYSCLR_ACTIVECAPTION
                                                 : SYSCLR_INACTIVECAPTION);

    /*
      Output the caption bar
    */
    WinPuts(hWnd, originTop,  originLeft, szLineBuf, attrBar);
  }
  else
  {
    /*
      Don't count the corner chars in the width...so decrement 2
    */
#ifdef COMPUSERVE_MEWEL
    if (dwFlags & WS_POPUP)
      bActive = WIN_HAS_TITLE(w) ? BORDER_MODAL : BORDER_POPUP;
#endif
    width2 = min(width-2, sizeof(szLineBuf)-1);
    memset(szLineBuf, SysBoxDrawingChars[bActive][CH_TOPLINE], width2);
    szLineBuf[width-2] = '\0';


    /*
      Determine the color of the top line of the frame
    */
    if ((attrBar = w->attr) == SYSTEM_COLOR)
    {
#ifdef COMPUSERVE_MEWEL
      if (dwFlags & WS_POPUP)
        attrBar = WinQuerySysColor(NULLHWND,
	    	bActive ? (WIN_HAS_TITLE(w) ? POPUP_COLOR : MODAL_COLOR)
                                                   : SYSCLR_INACTIVECAPTION);
      else
#endif
#ifdef TELEVOICE
      attrBar = WinQuerySysColor(NULLHWND, SYSCLR_WINDOW);
#else
      attrBar = WinQuerySysColor(NULLHWND,
                   bIsDisabled ? SYSCLR_DISABLEDBORDER :
                   (bActive ? SYSCLR_ACTIVEBORDER : SYSCLR_INACTIVEBORDER));
#endif
    }

    /*
      Output the top of the frame starting to the right of the top-left
      corner character.
    */
    WinPuts(hWnd, originTop,  originLeft+1, szLineBuf, attrBar);
  }


  /*
    Display the window text centered in the frame or caption...
  */
  if (w->title && w->title[0])
  {
    COLOR oldHilitePrefixAttr = HilitePrefixAttr;
    HilitePrefixAttr = WinQuerySysColor(hWnd, SYSCLR_DLGACCEL);
    HilitePrefix = HILITE_PREFIX;

    if (w->attr == SYSTEM_COLOR)
    {
      if (IsFrameClass(w))
        attrBar = WinQuerySysColor(NULLHWND,
                      bIsDisabled ? SYSCLR_DISABLEDBUTTON : SYSCLR_WINDOWTEXT);
#ifdef COMPUSERVE_MEWEL
      else if (dwFlags & WS_POPUP)
	attrBar = WinQuerySysColor(NULLHWND,
		bActive ? (WIN_HAS_TITLE(w) ? POPUP_COLOR : MODAL_COLOR)
                                                 : SYSCLR_INACTIVECAPTION);
      else if (HAS_CAPTION(dwFlags))
	attrBar = WinQuerySysColor(NULLHWND, bActive ? SYSCLR_ACTIVECAPTION
                                                 : SYSCLR_INACTIVECAPTION);
#endif
      else if (iClass == LISTBOX_CLASS)
        attrBar = WinQuerySysColor(NULLHWND,
                      bIsDisabled ? SYSCLR_DISABLEDLISTBOX : SYSCLR_LISTBOX);
      else if (iClass == EDIT_CLASS)
        attrBar = WinQuerySysColor(NULLHWND,
                      bIsDisabled ? SYSCLR_DISABLEDEDIT : SYSCLR_EDIT);
      else
        attrBar = WinQuerySysColor(NULLHWND, SYSCLR_WINDOWSTATICTEXT);
    }
    HilitePrefixAttr = MAKE_ATTR(GET_FOREGROUND(HilitePrefixAttr),
                                 GET_BACKGROUND(attrBar));

#ifndef COMPUSERVE_MEWEL
    if (HAS_CAPTION(dwFlags))
      attrBar = WinQuerySysColor(NULLHWND, 
            bIsDisabled ? SYSCLR_INACTIVECAPTIONTEXT : SYSCLR_CAPTIONTEXT);
#endif

#if !defined(CENTER_BOXTEXT)
    /*
      For frames, edit controls, and listboxes, left-justify the caption
    */
    if (iClass == BOX_CLASS  || iClass == FRAME_CLASS || 
	iClass == EDIT_CLASS || iClass == LISTBOX_CLASS)
    {
      if (bIsDisabled)
        HilitePrefixAttr = attrBar;
      WinPuts(hWnd, originTop, originLeft+1, w->title, attrBar);
    }
    else
#endif
    WinPutsCenter(hWnd, originTop, w->title, attrBar);


    HilitePrefix = '\0';
    HilitePrefixAttr = oldHilitePrefixAttr;
  }



  /*
     Draw the min/max box
  */
  attrBar = WinQuerySysColor(NULLHWND, SYSCLR_SYSMENU);
  if ((dwFlags & WS_MINIMIZEBOX) && 
      (iClass == NORMAL_CLASS || iClass == DIALOG_CLASS))
  {
#if defined(MULTICHAR_SYSICONS)
    PSTR sIcon  = WinGetSysCharStr((dwFlags&WS_MINIMIZE) ? SYSCHARSTR_MAXICON
                                                         : SYSCHARSTR_MINICON);
    int sLen    = strlen(sIcon);
    int iOffset = ((dwFlags & WS_MAXIMIZEBOX) && !(dwFlags & WS_MINIMIZE)) 
                   ? sLen*2 : sLen;
    WinPuts(hWnd, originTop, RECT_WIDTH(w->rClient) - iOffset - 1, 
            sIcon, attrBar);
#else
    int iOffset = ((dwFlags & WS_MAXIMIZEBOX) && !(dwFlags & WS_MINIMIZE)) ? 1 : 0;
    WINPUTC(hWnd, originTop, RECT_WIDTH(w->rClient) - iOffset,
            WinGetSysChar((dwFlags & WS_MINIMIZE) ? SYSCHAR_MAXICON
                                                  : SYSCHAR_MINICON),
            attrBar);
#endif
  }

  /*
    The test for NORMAL_CLASS is here cause LBS_USETABSTOPS is the same
    style bit as WS_MAXIMIZEBOX.
  */
  if ((dwFlags & WS_MAXIMIZEBOX) && !(dwFlags & WS_MINIMIZE) &&
      (iClass == NORMAL_CLASS || iClass == DIALOG_CLASS))
  {
#if defined(MULTICHAR_SYSICONS)
    PSTR sIcon = 
            (dwFlags & WS_MAXIMIZE) ? WinGetSysCharStr(SYSCHARSTR_RESTOREICON)
                                    : WinGetSysCharStr(SYSCHARSTR_MAXICON);
    int  iOffset = 1;
    WinPuts(hWnd, originTop, RECT_WIDTH(w->rClient) - strlen(sIcon) - iOffset,
            sIcon, attrBar);
#else
    WINPUTC(hWnd, originTop, RECT_WIDTH(w->rClient),
            (dwFlags & WS_MAXIMIZE) ? WinGetSysChar(SYSCHAR_RESTOREICON)
                                    : WinGetSysChar(SYSCHAR_MAXICON),
            attrBar);
#endif
  }

  /*
    Draw the system-menu icon
  */
  if (dwFlags & WS_SYSMENU)
  {
#if defined(MULTICHAR_SYSICONS)
    /* leave corner and 1 char intact */
    originLeft += 2;
    WinPuts(hWnd,originTop,originLeft,WinGetSysCharStr(SYSCHARSTR_SYSMENU),attrBar);
#else
    WINPUTC(hWnd,originTop,originLeft,WinGetSysChar(SYSCHAR_SYSMENU),attrBar);
#endif
  }

  /*
    Decrement the border-drawing level and return...
  */
  bDrawingBorder--;
  return TRUE;

#endif /* MOTIF */
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinFastPutc()                                                */
/*                                                                          */
/* Purpose  : Hack to bypass the standard WinPutc() on border drawing       */
/*            in order to speed things up when drawing the border of a      */
/*            window.                                                       */
/*                                                                          */
/* Returns  : TRUE if the char was drawn, FALSE if not.                     */
/*                                                                          */
/****************************************************************************/
#if !defined(MOTIF) && !defined(DECWINDOWS)
static VOID PASCAL _WinFastPutc(w, row, col, c, attr)
  WINDOW *w;
  int    row, col, c;
  COLOR  attr;
{
  register int scrRow, scrCol;

  /*
     Client-to-screen
  */
  scrCol = col + w->rClient.left;
  scrRow = row + w->rClient.top;

  /*
    Clip to the physical screen
  */
  if (scrCol < 0 || scrCol >= (int) VideoInfo.width ||
      scrRow < 0 || scrRow >= (int) VideoInfo.length)
    return;

  /*
    If the point is not obscured, write the character
  */
  if (WinVisMap[scrRow * VideoInfo.width + scrCol] == w->win_id)
  {
    char buf[2];
    buf[0] = (char) c;  buf[1] = '\0';
    VidBlastToScreen((HDC) 0, scrRow, scrCol, scrCol, attr, buf);
  }
}
#endif

