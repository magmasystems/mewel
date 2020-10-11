/*===========================================================================*/
/*                                                                           */
/* File    : WSCRLBAR.C                                                      */
/*                                                                           */
/* Purpose : Implements the scrollbar control.                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_SCROLLBAR

#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#define DO_THUMB_OUTLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif
static HWND PASCAL _SBQuery(HWND, INT, SCROLLBARINFO **);
static INT  PASCAL _ScrollBarGetThumbTrackPos(WINDOW *, INT);
static VOID PASCAL ScrollBarDraw(HWND);
static VOID PASCAL InvalidateScrollbar(HWND, SCROLLBARINFO *);
static INT  PASCAL ScrollBarXYtoMessage(HWND, SCROLLBARINFO *, INT, INT, BOOL);
#ifdef MEWEL_GUI
static VOID PASCAL ScrollBarDrawThumb(HWND, HDC);
#else
static VOID PASCAL ScrollBarDrawThumb(HWND, BOOL);
#endif

static UINT lastSBMsg = 0;

#if defined(MEWEL_GUI) && defined(DO_THUMB_OUTLINE)
/*
  Tracks the thumb outline for the GUI version.
*/
static VOID PASCAL ScrollBarDrawOutline(HWND, INT, BOOL);
static RECT rectThumbTrack;
#else
#define ScrollBarDrawOutline(hWnd, xy, bVert)
#endif

#ifdef __cplusplus
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : ScrollBarWinProc()                                            */
/*                                                                          */
/* Purpose  : Window procedure for the scrollbar control.                   */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LONG FAR PASCAL ScrollBarWinProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  INT    row, col;
  INT    msg = -1;
  DWORD  dwFlags;
  BOOL   bIsCtl, bVert;
  WINDOW *sbw;
  WINDOW *wParent;		/* parent window */
  SCROLLBARINFO *sbi;
  RECT   rBar;

  
  /*
    Get a pointer to the scrollbar's WINDOW structure and the
    SCROLLBARINFO structure.
  */
  if ((sbw = WID_TO_WIN(hWnd)) == NULL)
    return FALSE;
  if ((sbi = (SCROLLBARINFO *) sbw->pPrivate) == NULL)
    return TRUE;

  /*
    Get things into local vars for speed
  */
  dwFlags = sbi->sb_flags;
  bIsCtl  = (BOOL) ((dwFlags & SBS_CTL)  != 0L);
  bVert   = (BOOL) ((dwFlags & SBS_VERT) != 0L);
  wParent = sbw->parent;
  rBar    = sbw->rect;


  /*
    Client-area mouse messages should be converted back into screen coords
    for this winproc.
  */
  if (IS_WM_MOUSE_MSG(message))
    lParam = _UnWindowizeMouse(hWnd, lParam);

  switch (message)
  {
    case WM_SETCURSOR   :
      if (TEST_PROGRAM_STATE(STATE_NO_WMSETCURSOR))
        return TRUE;
     return SendMessage(wParent->win_id, WM_SETCURSOR, wParam, lParam);

    case WM_GETDLGCODE:
      /*
        A control scrollbar needs to process arrow keys. A non-ctrl
        scrollbar returns DLGC_STATIC because the IsStaticClass()
        function needs it.
      */
      if (bIsCtl)
        return DLGC_WANTARROWS;
      else
        return DLGC_STATIC;


#if defined(USE_NATIVE_GUI)
    case WM_PAINT      :
    case WM_NCPAINT    :
    case WM_ERASEBKGND :
      sbw->rUpdate  =  RectEmpty;
      sbw->ulStyle  &= ~WIN_UPDATE_NCAREA;
      sbi->sb_flags &= ~SB_NOREDRAWARROWS;
      break;

#else
    case WM_SETFOCUS  :
    case WM_KILLFOCUS :
      /*
        A control scrollbar needs to show that it has the input focus
        somehow, so we use a different thumb character and redraw it.
      */
      if (bIsCtl)
      {
        if (message == WM_SETFOCUS)
          sbi->sb_flags |= SB_HASFOCUS;
        else
          sbi->sb_flags &= ~SB_HASFOCUS;
        InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
        break;
      }
      /* fall through to WM_PAINT... */


#if defined(MEWEL_GUI)
    case WM_NCPAINT    :
    case WM_ERASEBKGND :
      /*
        ScrollBarDraw takes care of the erasing.
      */
      return TRUE;
#endif

    case WM_PAINT  :
      /* Don't touch the scrollbars of a minimized window  */
      if (sbi->minpos != sbi->maxpos && !(wParent->flags & WS_MINIMIZE))
      {
        if (!bIsCtl || IsWindowVisible(hWnd))
          /*
            This should be the only place where ScrollBarDraw() is called.
          */
          ScrollBarDraw(hWnd);
      }
      sbw->rUpdate = RectEmpty;
      sbw->ulStyle &= ~WIN_UPDATE_NCAREA;
      sbi->sb_flags &= ~SB_NOREDRAWARROWS;
      break;
#endif


    case WM_NCHITTEST   :
      return HTCLIENT;


    case WM_LBUTTONDOWN :
      SetCapture(hWnd);
      if (bIsCtl)
        SendMessage(GetParent(hWnd), WM_NEXTDLGCTL, hWnd, 1L);

      /*
        row and col are the coords of the mouse, relative to the scrollbar.
      */
      row = HIWORD(lParam) - rBar.top;
      col = LOWORD(lParam) - rBar.left;

#if defined(MEWEL_GUI) && defined(DO_THUMB_OUTLINE)
      rectThumbTrack = RectEmpty;
#endif

      /*
        Make sure that the button was clicked within the scrollbar
      */
      if (!PtInRect(&rBar, MAKEPOINT(lParam)))
        break;

      /*
        Determine the proper SB_xxx message to send to the parent
      */
      msg = ScrollBarXYtoMessage(hWnd, sbi, row, col, bVert);
      if (msg >= 0)
      {
        lastSBMsg = msg;
        if (msg == SB_THUMBTRACK)
          goto send_thumb_msg;
        else
          goto lbl_repeat;
      }
      break;


    case WM_LBUTTONUP   :
      ReleaseCapture();
      if (lastSBMsg == SB_THUMBTRACK)
      {
        lastSBMsg = SB_THUMBPOSITION;
#if defined(MEWEL_GUI) && defined(DO_THUMB_OUTLINE)
        /*
          Get rid of the thumb outline
        */
        rectThumbTrack = RectEmpty;
        sbi->sb_flags |= SB_NOREDRAWARROWS;
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
#endif
        /* Force an SB_ENDSCROLL message too. */ 
        PostMessage(hWnd, WM_LBUTTONUP, wParam, lParam); 
        goto send_thumb_msg;
      }
      else
      {
        if (wParent)
          SendMessage(wParent->win_id,
                      (bVert) ? WM_VSCROLL : WM_HSCROLL,
                      SB_ENDSCROLL, MAKELONG(sbi->currpos,(bIsCtl?hWnd:0)));
      }
      break;



    case WM_MOUSEMOVE   :
      if (InternalSysParams.hWndCapture != hWnd)
        break;

      /*
        Dragging the mouse in the scrollbar
      */
      lastSBMsg = SB_THUMBTRACK;
      if (!PtInRect(&rBar, MAKEPOINT(lParam)))
        break;

send_thumb_msg:
      /*
        Get the mouse coordinates in client-relative values
      */
      row = HIWORD(lParam) - rBar.top;
      col = LOWORD(lParam) - rBar.left;

#ifdef MEWEL_TEXT   /* 1/14/94 (televoice) */
      row++;
      col++;
#endif

      /*
        Draw the outline of the thumb
      */
      ScrollBarDrawOutline(hWnd, bVert ? row : col, bVert);

      /*
        Notify the parent if the mouse is being dragged within the
        elevator shaft of the scrollbar.
      */
      if (wParent != NULL)
      {
        int iPos = _ScrollBarGetThumbTrackPos(sbw, bVert ? row : col);
        sbi->sb_flags |= SB_FORCEREDRAW;
        SendMessage(wParent->win_id,
                    (bVert) ? WM_VSCROLL : WM_HSCROLL,
                    lastSBMsg, MAKELONG(iPos, (bIsCtl ? hWnd : 0)));
      }
      break;
      

    case WM_MOUSEREPEAT :
lbl_repeat:
      if (wParent)
      {
        if (message == WM_MOUSEREPEAT)  /* cause we can jump into here...*/
        {
          if (!PtInRect(&rBar, MAKEPOINT(lParam)))
            break;

          row = HIWORD(lParam) - rBar.top;
          col = LOWORD(lParam) - rBar.left;

          msg = ScrollBarXYtoMessage(hWnd, sbi, row, col, bVert);

          /*
            Make sure that the mouse is on one of the arrows
          */
          if (msg == SB_LINEUP || msg == SB_LINEDOWN)
            lastSBMsg = msg;
          else
            break;
        }

        SendMessage(wParent->win_id, 
                   (bVert) ? WM_VSCROLL : WM_HSCROLL, 
                   lastSBMsg, MAKELONG(0, (bIsCtl ? hWnd : 0)));

        /*
          Windows sends the SB_ENDSCROLL message if you are using the
          keyboard.
        */
        if (message == WM_KEYDOWN)
          SendMessage(wParent->win_id,
                      (bVert) ? WM_VSCROLL : WM_HSCROLL,
                      SB_ENDSCROLL, 
                      MAKELONG(sbi->currpos, (bIsCtl ? hWnd : 0)));
      }
      break;


    case WM_KEYDOWN :
      /*
        Map arrow keys into SB_xxx messages
      */
      switch (wParam)
      {
        case VK_UP    :
        case VK_LEFT  :
          msg = SB_LINEUP;
          break;
        case VK_DOWN  :
        case VK_RIGHT :
          msg = SB_LINEDOWN;
          break;
        case VK_PRIOR :
          msg = SB_PAGEUP;
          break;
        case VK_NEXT  :
          msg = SB_PAGEDOWN;
          break;
        case VK_HOME  :
          if (bIsCtl)
            msg = SB_TOP;
          break;
        case VK_END   :
          if (bIsCtl)
            msg = SB_BOTTOM;
          break;
        default :
          goto call_dwp;
      }

      if (msg >= 0)
      {
        lastSBMsg = msg;
        goto lbl_repeat;
      }
      break;


    case WM_DESTROY :
      /*
        When we destroy a scrollbar, we have to twiddle with the parent's
        flags in order to tell it that there is no more scrollbar.
      */
      if (wParent && !bIsCtl)
      {
        wParent->flags &= (bVert) ? ~WS_VSCROLL : ~WS_HSCROLL;
        wParent->hSB[(bVert) ? SB_VERT : SB_HORZ] = NULL;
      }
      break;


    case WM_SYSCOLORCHANGE :
      if (wParam == SYSCLR_SCROLLBAR      ||
          wParam == SYSCLR_SCROLLBARTHUMB ||
          wParam == SYSCLR_SCROLLBARARROWS)
        InternalInvalidateWindow(hWnd, FALSE);
      break;


    default :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);
  }
  
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* File    : SBCREATE.C                                                      */
/*                                                                           */
/* Purpose : Procedure to create a scrollbar                                 */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL ScrollBarCreate(hParent, y1,x1,y2,x2, attr, flags, id)
  HWND  hParent;
  INT   y1,x1, y2,x2;
  COLOR attr;
  DWORD flags;
  UINT  id;
{
  HWND          hWnd;
  WINDOW        *sbw;
  SCROLLBARINFO *sbi;
  
  if (WID_TO_WIN(hParent) == NULL)
    return NULLHWND;

  hWnd = WinCreate(hParent, y1,x1, y2, x2, NULL, attr,
                   flags | WS_CLIPSIBLINGS, SCROLLBAR_CLASS, id);
  if (hWnd == NULLHWND)
    return NULLHWND;

  /*
    Set the values in the internal window structure
  */
  sbw = WID_TO_WIN(hWnd);
  sbw->fillchar = WinGetSysChar(SYSCHAR_SCROLLBAR_FILL);
  sbw->pPrivate = emalloc(sizeof(SCROLLBARINFO));

  /*
    Set the values in the scrollbar's private data structure
  */
  sbi = (SCROLLBARINFO *) sbw->pPrivate;
  sbi->currpos = 0;
  sbi->minpos  = 0;     /* MS Windows gives a scrollbar a default range */
  sbi->maxpos  = 100;   /* of 0 - 100 */
  sbi->sb_flags = (flags & SBS_VERT) ? SBS_VERT : SBS_HORZ;
  if (flags & SBS_CTL)
    sbi->sb_flags |= SBS_CTL;
  
  return hWnd;
}


/****************************************************************************/
/*                                                                          */
/* Function : ShowScrollBar(hBar, nBar, fShow)                              */
/*                                                                          */
/* Purpose  : Shows or hides a scrollbar.                                   */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: MEWEL calls this function only from the SetScrollRange()      */
/*            function in order to hide a scrollbar if the range is 0       */
/*            and to show the scrollbar if the range is non-zero.           */
/*                                                                          */
/****************************************************************************/
#ifdef __TURBOC__
#pragma argsused
#endif

VOID FAR PASCAL ShowScrollBar(HWND hBar, INT nBar, BOOL fShow)
{
  WINDOW *sbw, *wParent;
  SCROLLBARINFO *sbi;
  RECT   r;
  DWORD  fOldFlags, ulScrollStyle;
  BOOL   bParentHasBorder;
  HWND   hParent;


  if ((sbw = WID_TO_WIN(hBar)) == NULL)
    return;

  /*
    Get the handle of the scrollbar window.

    In Windows, windows are always 'born' with a scrollbar. This
    is not true in MEWEL. So, if we are showing a scrollbar which
    isn't 'born' yet, then create it and attach it to the window.
  */
  if ((hBar = _SBQuery(hBar, nBar, &sbi)) == NULL) 
  {
    if (fShow)
    {
      if (nBar == SB_VERT && sbw->hSB[SB_VERT] == NULL)
      {
        sbw->flags |= WS_VSCROLL;
        hBar = sbw->hSB[SB_VERT] = ScrollBarCreate(sbw->win_id,
                      sbw->rClient.top,
                      sbw->rClient.right,
                      sbw->rClient.bottom,
                      sbw->rClient.right + IGetSystemMetrics(SM_CXVSCROLL),
                      SYSTEM_COLOR,
                      (DWORD) SBS_VERT, 0);
        _WinSetClientRect(sbw->win_id);

        sbw = WID_TO_WIN(sbw->hSB[SB_VERT]);
        sbi = (SCROLLBARINFO *) sbw->pPrivate;
      }

      else if (nBar == SB_HORZ && sbw->hSB[SB_HORZ] ==NULL)
      {
        sbw->flags |= WS_HSCROLL;
        hBar = sbw->hSB[SB_HORZ] = ScrollBarCreate(sbw->win_id,
                      sbw->rClient.top,
                      sbw->rClient.right,
                      sbw->rClient.bottom,
                      sbw->rClient.right + IGetSystemMetrics(SM_CXVSCROLL),
                      SYSTEM_COLOR,
                      (DWORD) SBS_HORZ, 0);
        _WinSetClientRect(sbw->win_id);

        sbw = WID_TO_WIN(sbw->hSB[SB_HORZ]);
        sbi = (SCROLLBARINFO *) sbw->pPrivate;
      }
      else
        return;
    }
    else
      return;
  }
  else
  {
    if ((sbw = WID_TO_WIN(hBar)) == NULL)
      return;
  }

  wParent   = sbw->parent;
  fOldFlags = sbw->flags;
  hParent   = wParent->win_id;
  bParentHasBorder = (BOOL) ((wParent->flags & WS_BORDER) != 0L);

  /*
    See which scrollbar style we should add/subtract from the parent.
    Assume the scrollbar is a control sb, so start ulScrollStyle off
    at 0.
  */
  ulScrollStyle = 0;
  if (hBar == wParent->hSB[SB_VERT])
    ulScrollStyle = WS_VSCROLL;
  else if (hBar == wParent->hSB[SB_HORZ])
    ulScrollStyle = WS_HSCROLL;


#ifdef MEWEL_TEXT
  /*
    Special processing for MDI maximized child, since MEWEL/TEXT puts
    scrollbars on the window border.
  */
  if (IS_MDIDOC(wParent) && (wParent->flags & WS_MAXIMIZE))
    bParentHasBorder = FALSE;
#endif

  /*
    See if we want to make a hidden scrollbar visible again...
  */
  if (fShow)
  {
    CLR_WS_HIDDEN(sbw);
    sbw->flags |= WS_VISIBLE;
#if defined(MOTIF)
    XMEWELShowWindow(sbw, SWP_SHOWWINDOW);
#endif
    wParent->flags |= ulScrollStyle;
    if (!(fOldFlags & WS_VISIBLE) && wParent)
    {
      _WinSetClientRect(hParent);
      WinUpdateVisMap();
      if (!(sbi->sb_flags & SBS_CTL))
      {
        sbi->sb_flags &= ~SB_NOREDRAWARROWS;
        if (bParentHasBorder)
          InvalidateNCArea(hParent);
        else
          InvalidateRect(hParent, (LPRECT) NULL, TRUE);
      }
    }
  } /* end if (fShow) */

  /*
    See if we want to hide a scrollbar...
  */
  if (!fShow)
  {
    SET_WS_HIDDEN(sbw);
    sbw->flags &= ~WS_VISIBLE;
#if defined(MOTIF)
    XMEWELShowWindow(sbw, SWP_HIDEWINDOW);
#endif
    wParent->flags &= ~ulScrollStyle;
    if ((fOldFlags & WS_VISIBLE) && wParent)
    {
      _WinSetClientRect(hParent);
      WinUpdateVisMap();
      if ((sbi->sb_flags & SBS_CTL))
      {
        r = sbw->rect;  /* cause WinGenInvalid messages up the rect */
        WinGenInvalidRects(hParent, (LPRECT) &r);
      }
      else
      {
        sbi->sb_flags &= ~SB_NOREDRAWARROWS;
        if (bParentHasBorder)
          InvalidateNCArea(hParent);
        else
          InvalidateRect(hParent, (LPRECT) NULL, TRUE);
      }
    }
  }
}


#if defined(MEWEL_TEXT) && !defined(USE_NATIVE_GUI)
static VOID PASCAL ScrollBarDraw(hBar)
  HWND hBar;
{
  WINDOW *sbw;
  SCROLLBARINFO *sbi;
  COLOR  attr;
  COLOR  saveAttr;
  DWORD  dwFlags;  /* to let sliders be color-defined by user */

  if (!IsWindowVisible(hBar))
    return;

  if ((sbw = WID_TO_WIN(hBar)) == NULL)
    return;

  saveAttr = sbw->attr;
  sbi = (SCROLLBARINFO *) sbw->pPrivate;
  dwFlags = sbi->sb_flags;   /* to let sliders be color defined by user */

  /* if we want system colors and we don't have an SB_CTL SCROLLBAR
   * or if we have a disabled SB_CTL scrollbar, pick the system-defined
   * colors */
  if ((bUseSysColors && !(dwFlags & SBS_CTL)) ||
      (bUseSysColors &&  (dwFlags & SBS_CTL) && (sbw->flags & WS_DISABLED)))
  {
    BOOL bDisabled = ((sbw->flags & WS_DISABLED) != 0L);

    attr = WinQuerySysColor(hBar,
          bDisabled ? SYSCLR_DISABLEDSCROLLBARARROWS : SYSCLR_SCROLLBARARROWS);
    sbw->attr = WinQuerySysColor(hBar,
          bDisabled ? SYSCLR_DISABLEDSCROLLBAR : SYSCLR_SCROLLBAR);
  }
  else
  {
    if (dwFlags & SBS_CTL)
    {
      _PrepareWMCtlColor(hBar, CTLCOLOR_SCROLLBAR, 0);
      saveAttr = sbw->attr;
    }
    attr = sbw->attr;
  }


  /*
    Do the actual drawing. Output the shaft, the arrows, and the thumb.
  */
  bDrawingBorder++;
  WinClear(hBar);
  if (!(sbi->sb_flags & SBS_VERT))
  {
    int row = WIN_CLIENT_HEIGHT(sbw) / 2;
    WinPutc(hBar, row, 0, WinGetSysChar(SYSCHAR_SCROLLBAR_LEFT), attr);
    WinPutc(hBar, row, WIN_CLIENT_WIDTH(sbw)-1, 
                          WinGetSysChar(SYSCHAR_SCROLLBAR_RIGHT), attr);
  }
  else
  {
    int col = WIN_CLIENT_WIDTH(sbw) / 2;
    WinPutc(hBar, 0, col, WinGetSysChar(SYSCHAR_SCROLLBAR_UP), attr);
    WinPutc(hBar, WIN_CLIENT_HEIGHT(sbw)-1, col, 
                          WinGetSysChar(SYSCHAR_SCROLLBAR_DOWN), attr);
  }

  sbw->attr = saveAttr;
  ScrollBarDrawThumb(hBar, TRUE);
  bDrawingBorder--;
}


static VOID PASCAL ScrollBarDrawThumb(HWND hBar, BOOL bDrawThumb)
{
  WINDOW *sbw;
  SCROLLBARINFO *sbi;
  INT    thumbpos, winlen;
  COLOR  attr;
  BYTE   chThumb;
  DWORD  dwFlags;
  COLOR  saveAttr;

  if ((sbw = WID_TO_WIN(hBar)) == NULL)
    return;

  sbi = (SCROLLBARINFO *) sbw->pPrivate;
  dwFlags  = sbi->sb_flags;
  saveAttr = sbw->attr;

  /*
    Determine the size of the elevator shaft.
  */
  winlen = (dwFlags & SBS_VERT) ? WIN_CLIENT_HEIGHT(sbw) : WIN_CLIENT_WIDTH(sbw);
  winlen -= 2;  /* adjust so that it doesn't include the arrows */
  if (winlen <= 0)
    return;

  /*
    Determine the thumb character to use. If we are erasing the old
    thumb position, then use the scrollbar's background character.
  */
  if (bDrawThumb)
  {
    if ((dwFlags & SBS_CTL) && (dwFlags & SB_HASFOCUS))
#ifdef ASCII_ONLY
      chThumb = ' ';
#else
      chThumb = 0xFE;
#endif
    else
      chThumb = WinGetSysChar(SYSCHAR_SCROLLBAR_THUMB);
  }
  else
    chThumb = (BYTE) sbw->fillchar;

  /*
    Figure out the thumb position
  */
  thumbpos = (int)(((long)(sbi->currpos - sbi->minpos) * (long) winlen)
            / (long)max((sbi->maxpos - sbi->minpos), 1));

  /*
    Insure that the thumb position is below the top arrow and above the
    top arrow. Thumbpos will range from 0 to the length of the scrollbar
    client area-1. It will be incremented in the WinPutc()'s below so
    that the thumb will be drawn at position 1 to client_area_len;
  */
  if (thumbpos < 0)
    thumbpos = 0;
  else if (thumbpos >= winlen)
    thumbpos = winlen - 1;

  /*
    Determine the prpoer color to use
  */
#if 0
  /*
    9/3/91 (maa)
      Steve Rogers put this in to show the thumb in a highlighted color
      if the scrollbar is a control scrollbar which has the focus.
      I took it out because the thumb of a scrollbar which has SYSTEM_COLOR
      as the attribute would keep getting drawn with attribute 0xFF!
  */
  if ((bUseSysColors && !(dwFlags & SBS_CTL)) ||
      (bUseSysColors &&  (dwFlags & SBS_CTL) && (sbw->flags & WS_DISABLED)))
#elif defined(DUMB_CURSES) /* Use standout mode only when scrollbar has focus */
  if ((dwFlags & SB_HASFOCUS) == 0)
#else
  if (sbw->attr == SYSTEM_COLOR)
#endif
  {
    BOOL bDisabled = ((sbw->flags & WS_DISABLED) != 0L);
    INT  idxColor;

    if (bDrawThumb)
      idxColor = bDisabled ? SYSCLR_DISABLEDSCROLLBARTHUMB
                           : SYSCLR_SCROLLBARTHUMB;
    else
      idxColor = bDisabled ? SYSCLR_DISABLEDSCROLLBAR
                           : SYSCLR_SCROLLBAR;
    attr = WinQuerySysColor(hBar, idxColor);
  }
  else  /* not using a system color */
  {
    attr = (bDrawThumb) ? MAKE_HIGHLITE(sbw->attr) : sbw->attr;
  }


  /*
    Draw the thumb using the 'chThumb' characters
  */
  if (dwFlags & SBS_VERT)
  {
    for (winlen = WIN_CLIENT_WIDTH(sbw) - 1;  winlen >= 0;  winlen--)
      WinPutc(hBar, thumbpos+1, winlen, chThumb, attr);
  }
  else
  {
    for (winlen = WIN_CLIENT_HEIGHT(sbw) - 1;  winlen >= 0;  winlen--)
      WinPutc(hBar, winlen, thumbpos+1, chThumb, attr);
  }

  sbw->attr = saveAttr;
  sbi->thumbpos = thumbpos;
}
#endif /* MEWEL_TEXT || NATIVE_GUI */


/****************************************************************************/
/*                                                                          */
/* Function : GetScrollPos()                                                */
/*                                                                          */
/* Purpose  : Retrieves the current thumb position in the scrollbar         */
/*                                                                          */
/* Returns  : The current thumb position                                    */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL GetScrollPos(hBar, nBar)
  HWND hBar;
  INT  nBar;
{
  SCROLLBARINFO *sbi;

  if ((hBar = _SBQuery(hBar, nBar, &sbi)) == NULLHWND)
    return 0;
  return sbi->currpos;
}

/****************************************************************************/
/*                                                                          */
/* Function : SetScrollPos()                                                */
/*                                                                          */
/* Purpose  : Sets the current thumb position on the scroll bar.            */
/*                                                                          */
/* Returns  : The old thumb position.                                       */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL SetScrollPos(HWND hBar, INT nBar, INT pos, BOOL fRedraw)
{
  INT    oldpos;
  INT    maxpos, minpos;
  SCROLLBARINFO *sbi;
  
  if ((hBar = _SBQuery(hBar, nBar, &sbi)) == NULLHWND)
    return 0;
  maxpos = sbi->maxpos;
  minpos = sbi->minpos;

  /*
    We can only redraw if the min position is different than the max position.
    Otherwise, the scrollbar is hidden.
  */
  fRedraw = (fRedraw && minpos != maxpos);

  /*
    Make sure that the position is within the min and max range
  */
  if (pos < minpos)
    pos = minpos;
  else if (pos > maxpos)
    pos = maxpos;

  /*
    Erase the old thumb
  */
#if !defined(MEWEL_GUI) && !defined(USE_NATIVE_GUI)
  if (fRedraw && !(sbi->sb_flags & SB_FORCEREDRAW))
    ScrollBarDrawThumb(hBar, FALSE);
#endif

  /*
    Set the new current position
  */
  oldpos = sbi->currpos;
  sbi->currpos = pos;
  
  /*
    Draw the whole scrollbar only if the min and max pos are different.
  */
  if (fRedraw)
  {
#ifdef MEWEL_GUI
    InvalidateScrollbar(hBar, sbi);
#elif !defined(USE_NATIVE_GUI)
    if (sbi->sb_flags & SB_FORCEREDRAW)
    {
      sbi->sb_flags &= ~SB_FORCEREDRAW;
      InvalidateRect(hBar, (LPRECT) NULL, FALSE);
    }
    else
      ScrollBarDrawThumb(hBar, TRUE);
#endif
  }

#if defined(MOTIF) || defined(DECWINDOWS)
  _XSetScrollPos(hBar, pos);
#endif

  return oldpos;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetScrollRange()                                              */
/*                                                                          */
/* Purpose  : Returns the minimum and maximum values of the scrollbar       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL GetScrollRange(hBar, nBar, minpos, maxpos)
  HWND hBar;
  int  nBar;
  int  *minpos, *maxpos;
{
  SCROLLBARINFO *sbi;
  
  if ((hBar = _SBQuery(hBar, nBar, &sbi)) == NULLHWND)
  {
    *minpos = *maxpos = 0;
  }
  else
  {
    *minpos = sbi->minpos;
    *maxpos = sbi->maxpos;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : SetScrollRange()                                              */
/*                                                                          */
/* Purpose  : Sets the minimum and maximum values of the scrollbar          */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL SetScrollRange(HWND hBar, INT nBar, INT minpos, INT maxpos, 
                               BOOL fRedraw)
{
  WINDOW *sbw;
  SCROLLBARINFO *sbi;
  INT    oldMin, oldMax;
  BOOL   bShowingHiddenSB = FALSE;
  
  /*
    If minimum position is greater than the maximum, then this is an
    illegal range.
  */
  if (minpos > maxpos)
    return;
    
  /*
    Get the handle and window structure of the scrollbar we want to change
  */
  if ((hBar = _SBQuery(hBar, nBar, &sbi)) == NULLHWND ||
      (sbw = WID_TO_WIN(hBar)) == NULL)
    return;

  /*
    Save the old range and set the new range
  */
  oldMin      = sbi->minpos;
  oldMax      = sbi->maxpos;
  sbi->minpos = minpos;
  sbi->maxpos = maxpos;

  /*
    See if we need to hide or show the scrollbar
  */
  if (minpos == maxpos && !(sbw->flags & SBS_CTL))
  {
    /*
      If the minimum and maximum ranges are equal, then hide the scrollbar.
      We tell the parent of the scrollbar to redraw its border.
    */
    ShowScrollBar(hBar, SB_CTL, FALSE);
    return;
  }
  else if (TEST_WS_HIDDEN(sbw))
  {
    /*
      If minpos and maxpos are different, then we want to show the scrollbar.
      We do this by letting the parent redraw its border.
    */
    ShowScrollBar(hBar, SB_CTL, TRUE);
    bShowingHiddenSB = TRUE;
  }

  if (fRedraw)
  {
    InvalidateScrollbar(hBar, sbi);
#ifdef MEWEL_GUI
    /*
      Make sure the arrows are drawn if we are showing a previously
      hidden scrollbar.
    */
    if (bShowingHiddenSB)
      sbi->sb_flags &= ~SB_NOREDRAWARROWS;
#endif
  }
  else
    if (oldMin != minpos || oldMax != maxpos)
      sbi->sb_flags |= SB_FORCEREDRAW;  /* force thumb to be redrawn */

#if defined(MOTIF) || defined(DECWINDOWS)
  /*
    Make sure that the current position is valid. (Motif screams
    about this!)
  */
  if (oldMin == minpos && oldMax == maxpos)
    return;
  if (sbi->currpos < minpos || sbi->currpos > maxpos)
    SetScrollPos(hBar, SBS_CTL, sbi->currpos, FALSE);
  _XSetScrollRange(hBar, minpos, maxpos);
#endif
}


static VOID PASCAL InvalidateScrollbar(hBar, sbi)
  HWND hBar;
  SCROLLBARINFO *sbi;
{
  /*
    If the scrollbar is already being shown and if the whole thing
    isn't up for refreshing, then only draw the shaft and thumb.
  */
#ifdef MEWEL_GUI
  if (IsWindowVisible(hBar))
    if (GetUpdateRect(hBar, (LPRECT) NULL, FALSE))
      sbi->sb_flags &= ~SB_NOREDRAWARROWS;
    else
      sbi->sb_flags |=  SB_NOREDRAWARROWS;
#endif
  (void) sbi;

  /*
    Cause the scrollbar to be redrawn.
  */
#if !defined(MOTIF)
  InvalidateRect(hBar, (LPRECT) NULL, FALSE);
#endif
}


/*===========================================================================*/
/*                                                                           */
/* File    : WinGetScrollbars()                                              */
/*                                                                           */
/* Purpose : Retrieves the handles of a window's vertical & horz scrollbars  */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL WinGetScrollbars(hWnd, hHorizSB, hVertSB)
  HWND hWnd;
  HWND *hHorizSB;
  HWND *hVertSB;
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != NULL)
  {
    *hHorizSB = w->hSB[SB_HORZ];
    *hVertSB  = w->hSB[SB_VERT];
  }
}

BOOL FAR PASCAL WinHasScrollbars(HWND hWnd, UINT which)
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != NULL)
  {
    HWND hHorizSB = w->hSB[SB_HORZ];
    HWND hVertSB  = w->hSB[SB_VERT];
    switch (which)
    {
      case SB_HORZ :
        return (BOOL) (hHorizSB != NULL);
      case SB_VERT :
        return (BOOL) (hVertSB  != NULL);
      case SB_BOTH :
        return (BOOL) (hHorizSB != NULL || hVertSB != NULL);
    }
  }
  else
    return FALSE;
}


static HWND PASCAL _SBQuery(hBar, nBar, pSBI)
  HWND hBar;
  int  nBar;
  SCROLLBARINFO **pSBI;
{
  WINDOW *w;
  HWND   hHSB, hVSB;

  if ((w = WID_TO_WIN(hBar)) == (WINDOW *) NULL)
    return NULLHWND;

  if (w->idClass != SCROLLBAR_CLASS && nBar != SB_CTL)
  {
    WinGetScrollbars(hBar, &hHSB, &hVSB);
    switch (nBar)
    {
      case SB_HORZ :
        hBar = hHSB;
        break;
      case SB_VERT :
        hBar = hVSB;
        break;
      default      :
        return NULLHWND;
    }
  }

  if ((w = WID_TO_WIN(hBar)) == (WINDOW *) NULL)
    return NULLHWND;

  *pSBI = (SCROLLBARINFO *) w->pPrivate;
  return hBar;
}


/****************************************************************************/
/*                                                                          */
/* Function : _ScrollBarGetThumbTrackPos()                                  */
/*                                                                          */
/* Purpose  : Given a mouse coordinate, determines the thumbpos             */
/*                                                                          */
/* Returns  : The thumb position                                            */
/*                                                                          */
/* Called by: ScrollBarWinProc(), in reponse to WM_MOUSEMOVE                */
/*                                                                          */
/****************************************************************************/
static INT PASCAL _ScrollBarGetThumbTrackPos(wSB, xy)
  WINDOW *wSB;
  INT    xy;  /* relative screen coords */
{
  RECT r;
  SCROLLBARINFO *sbi;
  BOOL bVert;
  long length;
  int  maxpos, minpos;
  int  iArrowSize, side2;

  sbi = (SCROLLBARINFO *) wSB->pPrivate;
  bVert = (BOOL) ((wSB->flags & SBS_VERT) != 0);

  GetClientRect(wSB->win_id, &r);

  /*
    Get values into local vars for speed
  */
  maxpos = sbi->maxpos;
  minpos = sbi->minpos;
  iArrowSize = IGetSystemMetrics(bVert ? SM_CYVSCROLL : SM_CXHSCROLL);
  side2 = bVert ? r.bottom : r.right;

  /*
    Determine the approximate position of the thumb.
  */
  if (xy <= iArrowSize)
    return minpos;
  if (xy >= side2 - iArrowSize)
    return maxpos;

#ifdef MEWEL_GUI
   if (bVert  && xy >= sbi->rectThumb.top  && xy < sbi->rectThumb.bottom ||
       !bVert && xy >= sbi->rectThumb.left && xy < sbi->rectThumb.right)
      return sbi->currpos;
#endif

  /*
    Get the length of the shaft
  */
  length = (long) side2 - 2*iArrowSize;
  return (int)((long)(xy-iArrowSize) * (long)(maxpos-minpos) / length) + minpos;
}


/****************************************************************************/
/*                                                                          */
/* Function : ScrollBarXYtoMessage()                                        */
/*                                                                          */
/* Purpose  : Given a mouse coordinate, returns the SB_xxx message.         */
/*                                                                          */
/* Returns  : The SB_xxx message, or -1 if no message generated.            */
/*                                                                          */
/* Called by: ScrollBarWinProc() in response to WM_LBUTTONDOWN.             */
/*                                                                          */
/****************************************************************************/
static INT PASCAL ScrollBarXYtoMessage(HWND hWnd, SCROLLBARINFO *sbi, 
                                       INT row, INT col, BOOL bVert)
{
#if defined(XWINDOWS) || defined(MOTIF)
  return 0;
#else

#ifdef MEWEL_GUI
  RECT r;
#endif
  RECT rSB;
  INT  bound1, bound2, bound3, xy;
  INT  iArrowSize;
#ifdef MEWEL_TEXT
  INT  posArrow, xy2;
#endif

  GetClientRect(hWnd, &rSB);
  iArrowSize = IGetSystemMetrics(bVert ? SM_CYVSCROLL : SM_CXHSCROLL);

#ifdef MEWEL_GUI
  r = sbi->rectThumb;
  bound1 = (bVert) ? r.top      : r.left;
  bound2 = (bVert) ? r.bottom   : r.right;
#else
  posArrow = (bVert ? rSB.right : rSB.bottom) >> 1;
  bound1 = bound2 = iArrowSize + sbi->thumbpos;
#endif
  bound3 = (bVert) ? rSB.bottom : rSB.right;
  xy     = (bVert) ? row : col;
#ifdef MEWEL_TEXT
  xy2    = (bVert) ? col : row;
#endif


#ifdef MEWEL_GUI
  if (xy >= bound1 && xy < bound2)
    return SB_THUMBTRACK;
#endif

  if (xy < iArrowSize)
  {
#ifdef MEWEL_TEXT
    if (xy2 == posArrow)
#endif
    return SB_LINEUP;
  }

  else if (xy >= bound3 - iArrowSize)
  {
#ifdef MEWEL_TEXT
    if (xy2 == posArrow)
#endif
    return SB_LINEDOWN;
  }

  else if (xy > bound2)
    return SB_PAGEDOWN;

  else if (xy < bound1)
    return SB_PAGEUP;

  return -1;

#endif
}



/****************************************************************************/
/*                                                                          */
/* Function : ScrollbarDraw(Thumb)                                          */
/*                                                                          */
/* Purpose  : Routines to render GUI scrollbars                             */
/*                                                                          */
/****************************************************************************/

#ifdef MEWEL_GUI

static VOID PASCAL ScrollBarDraw(HWND hWnd)
{
  RECT   r, rBev;
  HDC    hDC;
  HBRUSH hOldBrush, hBrSB, hBrArrow;
  POINT  pt[4];
  HWND   hParent;
  DWORD  dwFlags;
  SCROLLBARINFO *sbi;
  BOOL   bNoArrows;

  if (!IsWindowVisible(hWnd))
    return;

  /*
    Don't take the time to redraw the arrows if we are doing something
    like dragging the thumb or setting the scroll pos or range.
  */
  sbi = (SCROLLBARINFO *) WID_TO_WIN(hWnd)->pPrivate;
  bNoArrows = (BOOL)
   ((sbi->sb_flags & (SB_NOREDRAWARROWS|SB_FORCEREDRAW)) == SB_NOREDRAWARROWS);

  dwFlags = WinGetFlags(hWnd);

  /*
    Get a window DC and figure out the scrollbar rect in pixels
  */
  hDC = GetWindowDC(hWnd);
  WindowRectToPixels(hWnd, (LPRECT) &r);

  /*
    Draw the frame and interior of the scrollbar.
  */
  hBrSB = SysBrush[COLOR_SCROLLBAR];
  hOldBrush = SelectObject(hDC, hBrSB);
  if (bNoArrows)
    if (dwFlags & SBS_VERT)
      InflateRect(&r, 0, -IGetSystemMetrics(SM_CYVSCROLL));
    else
      InflateRect(&r, -IGetSystemMetrics(SM_CXHSCROLL), 0);
  Rectangle(hDC, r.left, r.top, r.right, r.bottom);
  SelectObject(hDC, hOldBrush);

  if (bNoArrows)
    goto draw_thumb;


  /*
    Draw the arrows in black. The face of the thumb and arrows are drawn
    using the BTNFACE attribute.
  */
  InflateRect(&r, -1, -1);
  hBrArrow = GetStockObject(BLACK_BRUSH);
  hOldBrush = SelectObject(hDC, hBrArrow);
  hBrSB = SysBrush[COLOR_BTNFACE];

  if (dwFlags & SBS_VERT)
  {
    /*
      Top arrow
    */
    rBev = r;
    rBev.bottom = r.top + IGetSystemMetrics(SM_CYVSCROLL);
    DrawBeveledBox(hWnd, hDC, (LPRECT) &rBev, 1, 1, FALSE, hBrSB);
    InflateRect(&rBev, -1, -1);

#ifdef ZAPP
    /* draw the upper triangle */
    pt[0].x = rBev.left+2;   pt[0].y = rBev.bottom - 8;     /* left  */
    pt[1].x = rBev.left + (rBev.right-rBev.left)/2;         /* top   */
    pt[1].y = rBev.top+3; 
    pt[2].x = rBev.right-3;  pt[2].y = pt[0].y;             /* right */
    Polygon(hDC, (LPPOINT) &pt, 3);

    MoveTo(hDC, pt[0].x, pt[0].y);
    LineTo(hDC, pt[2].x, pt[2].y);
    /* draw the lower rectangle */
    pt[0].x += 3;
    pt[1].x = pt[0].x;      pt[1].y = pt[0].y + 4;
    pt[2].x = pt[2].x - 3;  pt[2].y = pt[1].y;
    pt[3].x = pt[2].x;      pt[3].y = pt[0].y;
    SetRect(&rBev, pt[0].x, pt[0].y, pt[2].x, pt[2].y);
    FillRect(hDC, &rBev, hBrArrow);
#else
    pt[0].x = rBev.left  + 2;
    pt[1].x = rBev.left  + (rBev.right-rBev.left)/2;
    pt[2].x = rBev.right - 2;

    pt[0].y = rBev.bottom - 3;            /* left  */
    pt[1].y = rBev.top+3;                 /* top   */
    pt[2].y = pt[0].y;                    /* right */
    Polygon(hDC, (LPPOINT) &pt, 3);
#endif /* ZAPP */

    /*
      Bottom arrow
    */
    rBev = r;
    rBev.top = r.bottom - IGetSystemMetrics(SM_CYHSCROLL);
    DrawBeveledBox(hWnd, hDC, (LPRECT) &rBev, 1, 1, FALSE, hBrSB);
    InflateRect(&rBev, -1, -1);

#ifdef ZAPP
    pt[0].x = rBev.left+2;	 pt[0].y = rBev.top + 8; /* left */
    pt[1].x = rBev.left + (rBev.right-rBev.left)/2;  /* top */
    pt[1].y = rBev.bottom-3; 
    pt[2].x = rBev.right-3;  pt[2].y = pt[0].y;      /* right */
    Polygon(hDC, (LPPOINT) &pt, 3);

    MoveTo(hDC, pt[0].x, pt[0].y);
    LineTo(hDC, pt[2].x, pt[2].y);

    /* draw the upper rectangle */
    pt[0].x += 3;
    pt[1].x = pt[0].x;      pt[1].y = pt[0].y - 4;
    pt[2].x = pt[2].x - 3;  pt[2].y = pt[1].y;
    pt[3].x = pt[2].x;      pt[3].y = pt[0].y;
    SetRect(&rBev, pt[1].x, pt[1].y, pt[3].x, pt[3].y);
    FillRect(hDC, &rBev, hBrArrow);
#else	
    pt[0].y = rBev.top + 3;                  /* left */
    pt[1].y = rBev.bottom - 3;               /* bottom */
    pt[2].y = pt[0].y;                       /* right */
    Polygon(hDC, (LPPOINT) &pt, 3);
#endif /* ZAPP */
  }
  else
  {
    /*
      Left arrow
    */
    rBev = r;
    rBev.right = r.left + IGetSystemMetrics(SM_CXHSCROLL);
    DrawBeveledBox(hWnd, hDC, (LPRECT) &rBev, 1, 1, FALSE, hBrSB);
    InflateRect(&rBev, -1, -1);

#ifdef ZAPP
    pt[0].x = rBev.left+3;  
    pt[0].y = rBev.top + (rBev.bottom-rBev.top)/2;
    pt[1].x = rBev.right-8;
    pt[1].y = rBev.top+3; 
    pt[2].x = pt[1].x;  pt[2].y = rBev.bottom-3;
    Polygon(hDC, (LPPOINT) &pt, 3);
    MoveTo(hDC, pt[1].x, pt[1].y);
    LineTo(hDC, pt[2].x, pt[2].y);

    /* draw the right rectangle */
    {
    int origy = pt[1].y;
    pt[0].x = pt[2].x;       pt[0].y = pt[2].y - 2;
    pt[1].x = pt[0].x + 4;   pt[1].y = pt[0].y;
    pt[2].x = pt[1].x;       pt[2].y = origy + 2;
    pt[3].x = pt[0].x;       pt[3].y = pt[2].y;
    SetRect(&rBev, pt[0].x, pt[0].y, pt[2].x, pt[2].y);
    FillRect(hDC, &rBev, hBrArrow);
    }
#else
    pt[0].x = rBev.left + 3;   pt[0].y = rBev.top + (rBev.bottom-rBev.top)/2; /* left */
    pt[1].x = rBev.right - 3;  pt[1].y = rBev.top+2;                    /* top  */
    pt[2].x = pt[1].x;         pt[2].y = rBev.bottom-2;                 /* bottom */
    Polygon(hDC, (LPPOINT) &pt, 3);
#endif /* ZAPP */

    /*
      Right arrow
    */
    rBev = r;
    rBev.left = r.right - IGetSystemMetrics(SM_CXHSCROLL);
    DrawBeveledBox(hWnd, hDC, (LPRECT) &rBev, 1, 1, FALSE, hBrSB);
    InflateRect(&rBev, -1, -1);

#ifdef ZAPP
    pt[0].x = rBev.right-3;  
    pt[0].y = rBev.top + (rBev.bottom-rBev.top)/2;
    pt[1].x = rBev.left+8;
    pt[1].y = rBev.top+3; 
    pt[2].x = pt[1].x;  pt[2].y = rBev.bottom-3;
    Polygon(hDC, (LPPOINT) &pt, 3);
    MoveTo(hDC, pt[1].x, pt[1].y);
    LineTo(hDC, pt[2].x, pt[2].y);

    /*draw the left rectangle */
    {
    int origy = pt[1].y;
    pt[0].x = pt[2].x;       pt[0].y = pt[2].y - 2;
    pt[1].x = pt[0].x - 4;   pt[1].y = pt[0].y;
    pt[2].x = pt[1].x;       pt[2].y = origy + 2;
    pt[3].x = pt[0].x;       pt[3].y = pt[2].y;
    SetRect(&rBev, pt[1].x, pt[1].y, pt[3].x, pt[3].y);
    FillRect(hDC, &rBev, hBrArrow);
    }
#else	
    pt[0].y = rBev.top + (rBev.bottom-rBev.top)/2; /* left */
    pt[1].y = rBev.top+2;                          /* top  */
    pt[2].y = rBev.bottom-2;                       /* bottom */

    pt[0].x = rBev.right - 3;   /* right */
    pt[1].x = rBev.left + 3;    /* top  */
    pt[2].x = pt[1].x;          /* bottom */
    Polygon(hDC, (LPPOINT) &pt, 3);
#endif /* ZAPP */
  }

  SelectObject(hDC, hOldBrush);

  /*
    Draw the thumb
  */
draw_thumb:
  ScrollBarDrawThumb(hWnd, hDC);

  ReleaseDC(hWnd, hDC);
}



/****************************************************************************/
/*                                                                          */
/* Function : ScrollBarDrawThumb(hWnd, hDC)                                 */
/*                                                                          */
/* Purpose  : Calculates the thumb coordinates and draws the thumb.         */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: ScrollBarDraw() in the graphics mode.                         */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL ScrollBarDrawThumb(hWnd, hDC)
  HWND hWnd;
  HDC  hDC;
{
  RECT rSB;
  SCROLLBARINFO *sbi;
  BOOL bVert;
  INT  iScrollSize, iThumbSize;
  INT  iGradient, iRange;
  INT  thumbpos, shaftLen;
  INT  side1, bound1, bound2;
  INT  iMinPos, iMaxPos, iCurrPos;

  /*
    Get a pointer to the scrollbar's private info.
  */
  sbi = (SCROLLBARINFO *) WID_TO_WIN(hWnd)->pPrivate;

  /*
    See if we are dealing with a horizontal or vertical scrollbar.
  */
  bVert = (BOOL) ((WinGetFlags(hWnd) & SBS_VERT) != 0L);

  /*
    Get the client rectangle of the scrollbar.
  */
  GetClientRect(hWnd, &rSB);

  /*
    Get the size of the arrows, thumb, and scrollbar shaft into local
    variables.
  */
  iScrollSize = IGetSystemMetrics(bVert ? SM_CYVSCROLL : SM_CXHSCROLL);
  iThumbSize  = IGetSystemMetrics(bVert ? SM_CYVTHUMB  : SM_CXHTHUMB);
  shaftLen    = (bVert ? rSB.bottom : rSB.right) - 2*iScrollSize;

  /*
    If the thumb is bigger than the shaft, then don't draw the thumb.
  */
  if (shaftLen < iThumbSize)
    return;

  /*
    Get scrollbar's min, max and current positions into local vars
  */
  iMinPos  = sbi->minpos;
  iMaxPos  = sbi->maxpos;
  iCurrPos = sbi->currpos;

  /*
    Figure out the gradient. Take the length of the shaft and divide
    it by the range. Also, perform rounding. (Special case for range == 2).
  */
  iRange = (iMaxPos - iMinPos + 1);
  if (iRange == 2)
    iGradient = (iCurrPos <= iMinPos) ? 0 : shaftLen;
  else
    iGradient = shaftLen / iRange;
  if ((shaftLen % iRange) > (iRange >> 1))
    iGradient++;

  /*
    Figure out the thumb position. Make sure to add half the gradient
    to the position so that the thumb is centered around the gradient.
  */
  thumbpos = (int)
      ((long) (iCurrPos - iMinPos) * (long) shaftLen / (long) iRange) +
      (iGradient >> 1);

  /*
    Insure that the thumb position is below the top arrow and above the
    bottom arrow. Thumbpos will range from 0 to the length of the scrollbar
    client area.
  */
  if (thumbpos < 0 || iCurrPos == iMinPos)
    thumbpos = 0;
  else if (thumbpos >= shaftLen || iCurrPos == iMaxPos)
    thumbpos = shaftLen;

  /*
    Reduce the dimensions of the scrollbar rect so that it just
    encompasses the shaft, not including the arrows.
  */
  if (bVert)
    InflateRect(&rSB, 0, -iScrollSize);
  else
    InflateRect(&rSB, -iScrollSize, 0);

  /*
    Make sure that the thumb does not go outside of the shaft.
  */
  bound1 = (bVert) ? rSB.top    : rSB.left;
  bound2 = (bVert) ? rSB.bottom : rSB.right;

  /*
    The equation (thumbpos + iScrollSize) gives the 0-based coordinate
    of the top of the thumb, based from the top of the scrollbar.
    We want the thumb to be centered around 'thumbpos', so subtract
    half of the thumb's size.
  */
  side1 = (thumbpos + iScrollSize) - (iThumbSize >> 1);
  side1 = max(side1, bound1);
  side1 = min(side1, bound2-iThumbSize);


  /*
    Leave an iota of space between the thumb and the shaft
  */
  if (bVert)
    SetRect(&rSB, rSB.left+1, side1, rSB.right-1, side1 + iThumbSize);
  else
    SetRect(&rSB, side1, rSB.top+1, side1 + iThumbSize, rSB.bottom-1);

  /*
    Believe it or not, the thumb is drawn using the BTNFACE attribute
  */
  DrawBeveledBox(hWnd, hDC, &rSB, 1, 1, FALSE, SysBrush[COLOR_BTNFACE]);

  /*
    sbi->rectThumb is the coordinates of the thumb, 0-based from the
    scrollbar window.
  */
  sbi->rectThumb = rSB;
  sbi->thumbpos = thumbpos;

#if defined(DO_THUMB_OUTLINE)
  rectThumbTrack = RectEmpty;
#endif
}


#if defined(MEWEL_GUI) && defined(DO_THUMB_OUTLINE)
static VOID PASCAL ScrollBarDrawOutline(hWnd, xy, bVert)
  HWND hWnd;
  INT  xy;     /* row (if bVert) or col of mouse, relative to scrollbar */
  BOOL bVert;
{
  RECT rSB;
  HDC  hDC;
  INT  iArrowSize = IGetSystemMetrics(bVert ? SM_CYVSCROLL : SM_CXHSCROLL);
  INT  iThumbSize = IGetSystemMetrics(bVert ? SM_CYVTHUMB  : SM_CXHTHUMB);
  INT  side1, bound1, bound2;

  GetClientRect(hWnd, &rSB);

  if (bVert)
    InflateRect(&rSB, 0, -iArrowSize);
  else
    InflateRect(&rSB, -iArrowSize, 0);

  bound1 = (bVert) ? rSB.top    : rSB.left;
  bound2 = (bVert) ? rSB.bottom : rSB.right;

  side1 = xy - (iThumbSize >> 1);
  side1 = max(side1, bound1);
  side1 = min(side1, bound2-iThumbSize);

  if (bVert)
    SetRect(&rSB, rSB.left+1, side1, rSB.right-1, side1 + iThumbSize);
  else
    SetRect(&rSB, side1, rSB.top+1, side1 + iThumbSize, rSB.bottom-1);

  /*
    Erase the old outline and draw the new outline.
  */
  hDC = GetDC(hWnd);
  if (!IsRectEmpty(&rectThumbTrack))
    DrawFocusRect(hDC, &rectThumbTrack);
  DrawFocusRect(hDC, &rSB);
  ReleaseDC(hWnd, hDC);

  /*
    Save the coordinates of the outline
  */
  rectThumbTrack = rSB;
}
#endif


#if 0
/****************************************************************************/
/*                                                                          */
/*  This scrollbar-drawing function uses icons for the arrows, but can't be */
/* used cause scrollbars can be any height or width.                        */
/*                                                                          */
/****************************************************************************/
int PASCAL GScrollbar(void)
{
  HDC hDC;
  HBRUSH hBrush, hOldBrush;
  RECT   r;
  int    x1, x2, y1, y2;
  BITMAPINFOHEADER bmi;
  extern HWND hMain;


  static BYTE pUpArrow[8] =
  {
    0x08,/* 00001000 */
    0x1C,/* 00011100 */
    0x3E,/* 00111110 */
    0x7F,/* 01111111 */
    0x08,/* 00001000 */
    0x08,/* 00001000 */
    0x08,/* 00001000 */
    0x08 /* 00001000 */
  };
  static BYTE pDownArrow[8] =
  {
    0x08,/* 00001000 */
    0x08,/* 00001000 */
    0x08,/* 00001000 */
    0x08,/* 00001000 */
    0x7F,/* 01111111 */
    0x3E,/* 00111110 */
    0x1C,/* 00011100 */
    0x08,/* 00001000 */
  };


  x1 = 10;
  x2 = x1 + 1;
  y1 = 5;
  y2 = 20;

  hDC = GetDC(hMain);

  hBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0x00, 0xFF, 0xFF));
  hOldBrush = SelectObject(hDC, hBrush);
  Rectangle(hDC, x1*FONT_WIDTH, y1*VideoInfo.yFontHeight, 
                 x2*FONT_WIDTH, y2*VideoInfo.yFontHeight);
  SelectObject(hDC, hOldBrush);
  DeleteObject(hBrush);

  /*
    Top Arrow
  */
  SetRect(&r, x1*FONT_WIDTH, (y1+0) * VideoInfo.yFontHeight,
              x2*FONT_WIDTH, (y1+1) * VideoInfo.yFontHeight);
  Draw3DBox(hDC, (LPRECT) &r, FALSE);
  bmi.biWidth = bmi.biHeight = 8;
  bmi.biBitCount = 1;
  DrawBitmap(hDC, &bmi, pUpArrow, x1*8, y1*VideoInfo.yFontHeight+4);

  /*
   Bottom Arrow
  */
  SetRect(&r, x1*FONT_WIDTH, (y2-1) * VideoInfo.yFontHeight,
              x2*FONT_WIDTH, (y2-0) * VideoInfo.yFontHeight);
  Draw3DBox(hDC, (LPRECT) &r, FALSE);
  DrawBitmap(hDC, &bmi, pDownArrow, x1*FONT_WIDTH, (y2-1)*VideoInfo.yFontHeight);

  /*
    Thumb
  */
  SetRect(&r, x1*FONT_WIDTH, (y1+5) * VideoInfo.yFontHeight,
              x2*FONT_WIDTH, (y1+6) * VideoInfo.yFontHeight);
  Draw3DBox(hDC, (LPRECT) &r, FALSE);

  ReleaseDC(hMain, hDC);
  return TRUE;
}


#define BORDER_WIDTH   1
#define BORDER_HEIGHT  1

/****************************************************************************/
/*                                                                          */
/* Function : Draw3DBox()                                                   */
/*                                                                          */
/* Purpose  : Draws a 3-d rectangle. Used by ScrollbarDraw to enclose the   */
/*            scrollbar's arrows and thumb.                                 */
/*                                                                          */
/****************************************************************************/
VOID PASCAL Draw3DBox(HDC hDC, LPRECT lpRect, BOOL bSelected)
{
  RECT r;

  r = *lpRect;  /* for speed... */

  /*
    Draw it...
  */
  SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
  SelectObject(hDC, GetStockObject(BLACK_PEN));
  Rectangle(hDC, 
             r.left   + BORDER_WIDTH,
             r.top    + BORDER_HEIGHT,
             r.right  - BORDER_WIDTH,
             r.bottom - BORDER_HEIGHT);

  /*
    Do the bottom and right in black
  */
  SelectObject(hDC, GetStockObject(bSelected ? WHITE_BRUSH : BLACK_BRUSH));
  Rectangle(hDC, 
             r.left,
             r.bottom - BORDER_HEIGHT,
             r.right,
             r.bottom);
  Rectangle(hDC, 
             r.right - BORDER_WIDTH,
             r.top,
             r.right,
             r.bottom);

  /*
    Do the top and left in white
  */
  SelectObject(hDC, GetStockObject(bSelected ? BLACK_BRUSH : WHITE_BRUSH));
  Rectangle(hDC, 
             r.left,
             r.top, 
             r.right - BORDER_WIDTH,
             r.top   + BORDER_HEIGHT);
  Rectangle(hDC, 
             r.left,
             r.top,
             r.left  + BORDER_WIDTH,
             r.bottom);
}


#endif

#endif /* MEWEL_GUI */
