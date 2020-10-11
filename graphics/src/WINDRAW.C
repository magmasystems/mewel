/*===========================================================================*/
/*                                                                           */
/* File    : WINDRAW.C                                                       */
/*                                                                           */
/* Purpose : Contains all functions which draw to windows with clipping      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define USE_DCCLIPPING
#define INCLUDE_LISTBOX


/*
  Thomas Wagner of Ferrari Electronics GMBH added code to handle tabs in
  listboxes.
*/

#include "wprivate.h"
#include "window.h"

#ifdef WAGNER_GRAPHICS
extern FARPROC lpfnWinFillRectHook;
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern VOID PASCAL _WinPutsWithTabs(HWND,HDC,WINDOW*,LPSTR,int,int,int,int,
                                    char[],COLOR,int);
#ifdef __cplusplus
}
#endif

extern BOOL bVirtScreenEnabled;
extern BOOL bDrawingBorder;  /* TRUE if we should clip to w->rect instead
                                of w->rClient. */
extern RECT RectEmpty;

/*
  Some VGA's support up to 160x50 !!!
*/
#define MAXCOLS   (160 + 1)

/*
  Define as a macro in order to speed things up
*/
#define RECTINTERSECT(r, r1, r2)  \
      RECTGEN(r, max(r1.top,r2.top), max(r1.left,r2.left), \
              min(r1.bottom,r2.bottom), min(r1.right,r2.right))
#define RECTISEMPTY(r)  (r.top >= r.bottom || r.left >= r.right)
#define RECTCONTAINSPOINT(r, row, col) \
          (col >= r.left && col < r.right && row >= r.top && row < r.bottom)


/*
  Turbo C++, and maybe some other compilers, will call a far function to
  copy a RECT structure. On the other hand, MSC will do it in-line. So,
  define a macro for a RECT copy operation.
*/
#if defined(__TURBOC__)
#define RECTCOPY(r1,r2)  RECTGEN(r1,(r2).top,(r2).left,(r2).bottom,(r2).right)
#else
#define RECTCOPY(r1,r2)  (r1) = (r2)
#endif

int  iDisplayTabSize = 8;         /* added to support changing tabs */
static BOOL bDrawingTree = FALSE;


/****************************************************************************/
/*                                                                          */
/* Function : WinDrawAllWindows()                                           */
/*                                                                          */
/* Purpose  : Refreshes the entire desktop.                                 */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: _VidInitNewVideoMode(), when the screen is put into another   */
/*            video mode.                                                   */
/*            WinExec() in order to refresh the desktop after swapping.     */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinDrawAllWindows(void)
{
  WinGenInvalidRects(_HwndDesktop, &SysGDIInfo.rectScreen);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : WinClear()                                                      */
/*   All this does is sends a WM_ERASEBKGND message to the window.           */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinClear(hWnd)
  HWND hWnd;
{
  INT    rc;
  HDC    hDC;
  WINDOW *w;
  RECT   rSaved;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  hDC = GetDC(hWnd);

  /*
    Since we want to clear the entire window, and since the default
    processing of WM_ERASEBKGND only used the update rectangle, we want
    to temporarily set the update rect to the entire client area.
  */
  rSaved = w->rUpdate;
  GetClientRect(hWnd, &w->rUpdate);

  rc = (INT) SendMessage(hWnd, 
       (w->flags & WS_MINIMIZE) ? WM_ICONERASEBKGND : WM_ERASEBKGND, hDC, 0L);

  /*
    Restore the original update rect and get rid of the DC
  */
  w->rUpdate = rSaved;
  ReleaseDC(hWnd, hDC);

  return rc;
}


#if !defined(MEWEL_GUI) && !defined(MOTIF)

/*===========================================================================*/
/*                                                                           */
/* Purpose : WinEraseEOP()                                                   */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinEraseEOP(hWnd, row, col, attr)
  HWND  hWnd;
  int   row, col;
  COLOR attr;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    height;

  if (!w || TEST_WS_HIDDEN(w) || !(w->flags & WS_VISIBLE))
    return FALSE;

  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hWnd);

  WinEraseEOL(hWnd, row, col, attr);    /* the remainder of the 1st line */
  height = RECT_HEIGHT(w->rClient) - 1;
#ifdef WAGNER_GRAPHICS
  if (row < height && lpfnWinFillRectHook)
  {
    RECT fillrect;
    BOOL bCaretPushed;

    fillrect = w->rClient;
    fillrect.top += row + 1;
    WinClientRectToScreen(hWnd, &fillrect);
    bCaretPushed = CaretPush(fillrect->top, fillrect->bottom);
    MouseHide();
    lpfnWinFillRectHook(hWnd, &fillrect, w->fillchar, attr);
    MouseShow();
    if (bCaretPushed)
      CaretPop();
    return TRUE;
  }
#endif
  while (++row <= height)
    WinEraseEOL(hWnd, row, 0, attr);
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : WinEraseEOL() - erase to the end of line                        */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinEraseEOL(hWnd, row, col, attr)
  HWND hWnd;
  int  row, col;
  COLOR attr;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    len;
  char   szBlankLine[133];


  /*
    Create a blank line (with the window's fill pattern) and
      blast it to the screen
  */
  if ((len = (w->rClient.right - (w->rClient.left+col))) > 0)
  {
#ifdef WAGNER_GRAPHICS
   if (lpfnWinFillRectHook)
   {
     RECT fillrect;
     BOOL bCaretPushed;

     fillrect = w->rClient;
     fillrect.top += row;
     fillrect.bottom = fillrect.top;
     fillrect.left += col;
     bCaretPushed = CaretPush(fillrect->top, fillrect->bottom);
     MouseHide();
     if (bCaretPushed)
       CaretPop();
     lpfnWinFillRectHook(hWnd, &fillrect, w->fillchar, attr);
     MouseShow();
     return TRUE;
   }
#endif
    len = min(len, sizeof(szBlankLine) - 1);
    memset(szBlankLine, w->fillchar, len);
    szBlankLine[len] = '\0';
    if (attr == SYSTEM_COLOR)
      attr = WinGetClassBrush(hWnd);
    _WinPuts(hWnd, (HDC) 0, row, col, szBlankLine, attr, len, FALSE);
  }

  return TRUE;
}


INT FAR PASCAL WinEraseRect(hWnd, lpRect, attr)
  HWND   hWnd;
  LPRECT lpRect;
  COLOR  attr;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    row, width;
  char   szBlankLine[133];


  if (!w || (width = lpRect->right - lpRect->left) <= 0)
    return FALSE;

  width = min(width, sizeof(szBlankLine) - 1);

  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hWnd);

#ifdef WAGNER_GRAPHICS
   if (lpfnWinFillRectHook)
   {
     RECT fillrect;
     BOOL bCaretPushed;

     fillrect = *lpRect;
     WinClientRectToScreen (hWnd, &fillrect);
     MouseHide ();
     bCaretPushed = CaretPush(fillrect->top, fillrect->bottom);
     lpfnWinFillRectHook (hWnd, &fillrect, w->fillchar, attr);
     MouseShow ();
     if (bCaretPushed)
       CaretPop();
     return TRUE;
   }
#endif

  /*
    Create a blank line (with the window's fill pattern) and
      blast it to the screen
  */
  memset(szBlankLine, w->fillchar, width);
  szBlankLine[width] = '\0';
  for (row = lpRect->top;  row < lpRect->bottom;  row++)
    _WinPuts(hWnd, (HDC) 0, row, lpRect->left, szBlankLine, attr, width, FALSE);

  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : WinPutsCenter()                                                 */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinPutsCenter(hWnd, row, buf, attr)
  HWND  hWnd;
  int   row;
  LPSTR buf;
  COLOR attr;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    iLen;

  if (w)
  {
    iLen = lstrlen(buf) - ((HilitePrefix && lstrchr(buf, HILITE_PREFIX) != NULL) ? 1 : 0);
    WinPuts(hWnd, row, (WIN_CLIENT_WIDTH(w) - iLen) / 2, buf, attr);
  }
  return TRUE;
}


INT FAR PASCAL WinPutsRight(hWnd, row, buf, attr)
  HWND  hWnd;
  int   row;
  LPSTR buf;
  COLOR attr;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    iLen;

  if (w)
  {
    iLen = lstrlen(buf) - ((HilitePrefix && lstrchr(buf, HILITE_PREFIX) != NULL) ? 1 : 0);
    WinPuts(hWnd, row, (WIN_CLIENT_WIDTH(w) - iLen), buf, attr);
  }
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : WinPuts() - outputs a string to a window                        */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinPuts(hWnd, row, col, buf, attr)
  HWND hWnd;      /* handle of window to write into */
  LPSTR buf;      /* ptr to the string to write */
  int   row, col; /* window-relative coordinates to write */
  COLOR attr;     /* color */
{
  return _WinPuts(hWnd, (HDC) 0, row, col, buf, attr, -1, TRUE);
}

INT FAR PASCAL _WinPuts(
  HWND hWnd,      /* handle of window to write into */
  HDC  hDC,       /* optional device context */
  int  row, int col, /* window-relative coordinates to write */
  LPSTR buf,      /* ptr to the string to write */
  COLOR attr,     /* color */
  int  sLen,      /* length of the string, or -1 if we need to calc it */
  BOOL bCheckTabs
)
{
  WINDOW *w = WID_TO_WIN(hWnd);
  int    absCol;
  int    wRight;
  BOOL   bHasHighlite;

  /*
    bVisible is our output buffer.  If a 0xFF is in bVisible, it means that
    we should *not* write to that screen column.  First, we set all of the
    locations in bVisible to 0xFF.  Then, we copy the string into bVisible
    at the correct offset, clipping to the window, and handling tabs.  Then
    we go through the necessary windows on the tree, writing 0xFF back into
    bVisible where the windows obscure the output window.  What we end up
    with is a buffer which contains the characters to be written, or 0xFF
    to not write to a particular column.
  */
  char bVisible[MAXCOLS];

  /*
   * See if the window is visible 
   */
  if (!buf || !w || TEST_WS_HIDDEN(w) || !(w->flags & WS_VISIBLE))
    return FALSE;

  /*
    Translate the window-relative coords to screen-relative coords
  */
  row += w->rClient.top;
  absCol = w->rClient.left + col;

  /*
    Quick test to see if we're writing off the screen
  */
  if (row < 0 || row >= (int) VideoInfo.length  || 
      row < w->rect.top || row >= w->rect.bottom ||
      absCol >= (int) VideoInfo.width)
    return FALSE;

  /*
    See if we are writing to a y-coordinate outside of the window's
    client area. If so, return.
    Also, see if we are starting writing to the left of the client
    area. If so, bump up the string pointer so we start at the
    left side.
  */
  if (!bDrawingBorder && HAS_BORDER(w->flags))
  {
    if (row >= w->rClient.bottom || row < w->rClient.top)
      return FALSE;
    if (col < 0)
    {
      if (sLen == -1 && 
          (sLen = lstrlen(buf)) < -col)  /* everything off screen */
        return FALSE;
      absCol = w->rClient.left;
      buf += -col;
    }
  }

  /*
    Derive the right-most column we can possible write to.
  */
  wRight = min(w->rClient.right - 1, (int) (VideoInfo.width - 1));
  if (bDrawingBorder)
    wRight++;

#ifdef MULTMONITORS
  MonitorSwitch(w->iMonitor, SAVE);
#endif

  /*
    Initialize bVisible to disable writing in all columns
  */
  memset(bVisible, 0xFF, sizeof(bVisible));


  if (sLen == -1)
    sLen = lstrlen(buf);

  /*
    Compensate for a negative starting column.
    (but don't count highlight characters  7/26/93 CFN/Televoice)
  */
  if (absCol < 0)
  {
    char c;
    while (absCol < 0 && (c = *buf++) != '\0')
      if (c != HilitePrefix)
      {
        absCol++;
        sLen--;
      }
  }

  /*
    Don't let the string overwrite bVisible[MAXCOLS]. Also, see if
    the the clipping resulting from the statements right above left
    us with a null string.
  */
  if ((sLen = min(sLen, MAXCOLS)) < 0)
    goto byebye;

#ifdef USE_DCCLIPPING
  /*
    Clip to the device context's clipping rectangle
  */
  if (hDC)
  {
    LPHDC lphDC = _GetDC(hDC);
    RECT  rClipping;

    rClipping = lphDC->rClipping;
    if (!RECTISEMPTY(rClipping))
    {
      if (row < rClipping.top || row >= rClipping.bottom ||
          absCol >= rClipping.right)
        return FALSE;

      if (absCol < rClipping.left)
      {
        int  iDiff = rClipping.left - absCol;
        if ((sLen -= iDiff) < 0)
          return FALSE;
        buf += iDiff;
        absCol = rClipping.left;
      }

      if (absCol + sLen - 1 > rClipping.right)
      {
        int  iDiff = (absCol + sLen - 1) - rClipping.right;
        if ((sLen -= iDiff) < 0)
          return FALSE;
      }
    }
  }
#endif

  /*
    Copy buffer into bVisible, overwriting 0xFF values with output values
  */
  bHasHighlite = (HilitePrefix != '\0' && lstrchr(buf, HilitePrefix) != NULL);
  if (bCheckTabs)
    bCheckTabs = (lstrchr(buf,'\t') != NULL);
  if ((HilitePrefix == '\0' || !bHasHighlite) && !bCheckTabs)
  {
    /*
      Since the string doesn't contain any tabs or hilite characters, we
      can use memcpy() to slam it into the output area.  This is *fast*,
      and handles most cases where the application is writing to a
      window.
    */
    if ((sLen = min(sLen, wRight-absCol+1)) < 0)
      goto byebye;

    {
    LPSTR  pBuf = buf;
    char  *pVisible = &bVisible[absCol];
    BYTE   ch;
    LPUINT pVisMap;
    int    len = sLen;

    pVisMap = &WinVisMap[row * VideoInfo.width + absCol];
    for (pBuf = buf;  len && (ch = *pBuf++) != '\0';  len--)
      *pVisible++ = (*pVisMap++ == hWnd) ? (BYTE) ch : (BYTE) 0xFF;
    *pVisible = '\0';
    }
  }
  else
  {
    /*
      Special routine to draw a line which has tabs and/or highlites
    */
    _WinPutsWithTabs(hWnd,hDC,w,buf,row,absCol,wRight,sLen,bVisible,attr,1);
  }


  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hWnd);

  /*
   * Blast the string to Video RAM 
   */
  VidBlastToScreen(hDC, row, max(absCol, 0), wRight, attr, bVisible + absCol);

  /*
    Now we can handle the hilite characters.  We do this by recursively
    calling  ourselves...
  */
#ifndef DUMB_CURSES	/* regular and inverse video, no highlite */
  if (HilitePrefix != '\0' && bHasHighlite)
  {
#ifdef MULTMONITORS
    MonitorSwitch(w->iMonitor,RESTORE);
#endif
    wRight = min(w->rClient.right - 1, (int) (VideoInfo.width - 1));
    attr = HilitePrefixAttr ? HilitePrefixAttr : ((COLOR) attr^0x08);
    _WinPutsWithTabs(hWnd,hDC,w,buf,row,absCol,wRight,sLen,bVisible,attr,2);
    return TRUE;
  }
#endif /* DUMB_CURSES */

byebye:
#ifdef MULTMONITORS
  MonitorSwitch(w->iMonitor, RESTORE);
#endif
  return TRUE;
}


VOID PASCAL 
_WinPutsWithTabs(hWnd,hDC,w,pString,row,col,wRight,sLen,bVisible,attr,iPass)
  HWND   hWnd;
  HDC    hDC;
  WINDOW *w;
  LPSTR pString;
  int   row, col;
  int   wRight;
  char  bVisible[];
  int   sLen;
  COLOR attr;
  int   iPass;
{
  int   i;
  int   nSpaces;
  BOOL  bHi = FALSE;
  char *pVisible = &bVisible[col];

  /*
    Well...  There are tabs and/or hilites, so we have to process it
    a character at a time.  This is slow, but hey, what can we do?
  */
  for (i = 0;  i < sLen && col <= wRight;  i++, pString++)
  {
    if (*pString == HilitePrefix)
    {
      /*
        Ignore hilites for the 1st pass. They are processed at the end by
        recursively calling WinPuts()...  The first time through, we
        print the "hilited" character in the same attribute as the rest
        of the string.  At the end, we'll go back and overprint it in its
        hilited attribute.
      */
      bHi = TRUE;
    }
    else if (*pString == '\t')
    {
      int relCol = col - w->rClient.left;

      /*
        Process tabstops in listboxes which have the LBS_USETABSTOPS
        style set.
      */
      if ((w->flags & LBS_USETABSTOPS) && w->idClass == LISTBOX_CLASS)
      {
        LISTBOX *lbp = (LISTBOX *) w->pPrivate;
        LPINT    pStops;

        if ((pStops = lbp->pTabStops) != NULL)
        {
          /*
            Search for the tabstop closest to column n.
          */
          while (*pStops && ((int) *pStops) <= relCol)
            pStops++;
          if (!*pStops)
            pStops = NULL;
          else
            nSpaces = w->rClient.left + *pStops - col;
        }

        /*
          If we have no tabstops set, or if we are writing past the last 
          tabstop, then the tab char just expands to a single space.
        */
        if (pStops == NULL)
          nSpaces = 1;
      }
      else
      {
        /*
          Just a regular old tab in a non-listbox window
        */
        nSpaces = iDisplayTabSize - (relCol % iDisplayTabSize);
      }

      /*
        Don't go beyond the maximum column
      */
      if (col+nSpaces > wRight)
        nSpaces = wRight-col+1;

      /*
        We could make tabs non-destructive by just removing this memset().
        Does Windows interpret tabs as destructive or non-destructive?
      */
      if (iPass == 1)
      {
        LPUINT pVisMap = &WinVisMap[row * VideoInfo.width + col];
        col += nSpaces;
        while (nSpaces-- > 0)
          *pVisible++ = (BYTE) ((*pVisMap++ == hWnd) ? ' ' : 0xFF);
      }
      else
      {
        /*
          For the second pass, just put out nSpaces worth of blanks
        */
        if (bHi)
        {
          memset(bVisible, ' ', nSpaces);
          bVisible[nSpaces] = '\0';
          bHi = FALSE;
          _WinPuts(hWnd, hDC, row - w->rClient.top, col - w->rClient.left,
                   bVisible, attr, nSpaces, FALSE);
        }
        col += nSpaces;
      }
    }

    /*
      If the char is not a tab nor a highlite char ....
    */
    else
    {
      if (iPass == 1)
      {
        *pVisible++ = (WinVisMap[row*VideoInfo.width+col] == hWnd)
                       ? (BYTE) *pString : (BYTE) 0xFF;
      }
      else if (bHi)
      {
        /*
          For the second pass, just put out the highlited character
        */
        bVisible[0] = *pString;
        bVisible[1] = '\0';
        bHi = FALSE;
        _WinPuts(hWnd, hDC, row - w->rClient.top, col - w->rClient.left,
                 bVisible, attr, 1, FALSE);
      }
      col++;
    }
  }
  *pVisible = '\0';
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : WinPutc()                                                       */
/*                                                                           */
/*===========================================================================*/
int FAR PASCAL WinPutc(hWnd, row, col, c, attr)
  HWND hWnd;
  COLOR attr;
  int  row, col, c;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  char   buf[2];
  int    leftCol;

  if (w && !TEST_WS_HIDDEN(w) && WinIsPointVisible(hWnd, row, col))
  {
    buf[0] = (char) c;  buf[1] = '\0';

#ifdef MULTMONITORS
    MonitorSwitch(w->iMonitor, SAVE);
#endif

    if (attr == SYSTEM_COLOR)
      attr = WinGetClassBrush(hWnd);

    leftCol = w->rClient.left + col;
    if (leftCol >= 0)
      VidBlastToScreen((HDC) 0, w->rClient.top + row, leftCol, leftCol, attr, buf);

#ifdef MULTMONITORS
    MonitorSwitch(w->iMonitor, RESTORE);
#endif
    return TRUE;
  }
  else
    return FALSE;
}

#endif /* GUI && MOTIF */


/*===========================================================================*/
/*                                                                           */
/* Purpose : WinIsPointVisible()                                             */
/*           Tests if a certain point within a window is obscured ir visible.*/
/*           Returns TRUE if visible, FALSE if obscured.                     */
/*                                                                           */
/*===========================================================================*/
BOOL FAR PASCAL WinIsPointVisible(hWnd, row, col)
  HWND hWnd;
  int  row, col;
  /* row and col are 0-based coords relative to the client rectangle */
{
  register WINDOW *w = WID_TO_WIN(hWnd);

#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif


  if (!w)
    return FALSE;

  /*
    Translate from window-based to screen-based coordinates
  */
  row += w->rClient.top;
  col += w->rClient.left;

  /*
    Make sure that the point is within the screen boundaries.
  */
  if (row < 0 || row >= (int) VideoInfo.length || 
      col < 0 || col >= (int) VideoInfo.width)
    return FALSE;

#ifdef MEWEL_GUI
  {
  POINT pt;
  pt.x = col;
  pt.y = row;
  return IsWindowVisible(hWnd) && PtInRect(&w->rClient, pt);
  }
#else
  return (WinVisMap[row * VideoInfo.width + col] == hWnd);
#endif
}



/*===========================================================================*/
/*                                                                           */
/* File    : WINISVIS.C                                                      */
/*                                                                           */
/* Purpose : IsWindowVisible()                                               */
/*                                                                           */
/*===========================================================================*/
BOOL FAR PASCAL IsWindowVisible(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *wOwner;
  DWORD  dwFlags;
  BOOL   bIconTitle;

  if (w == (WINDOW *) NULL)
    return FALSE;

  bIconTitle = IS_ICONTITLE(w);

  /*
    If the child or any parent is invisible, the child must be invisible
  */
  while (w && w != InternalSysParams.wDesktop)
  {
    dwFlags = w->flags;

    /*
      If the visible flag is off or the hidden flag is on, then the
      window is not visible.
    */
    if (!(dwFlags & WS_VISIBLE) || TEST_WS_HIDDEN(w))
      return FALSE;

    /*
      Consider a window invisible if it occupies no space (an object window)
    */
    if (IsRectEmpty(&w->rect))
      return FALSE;

    /*
      Child windows should not be visible if an ancestor is iconic.
      However, if hWnd is iconic, it's visible, since we have to show the
      icon.
    */
    if (w->win_id != hWnd && (dwFlags & WS_MINIMIZE))
    {
      /*
        A iconized window's system menu should always be shown, as well
        as the title attached to the icon.
      */
      if (hWnd == w->hSysMenu || bIconTitle)
        return TRUE;
      return FALSE;
    }

    /*
      The next ancestor to look at is the owner of the window.
    */
    wOwner = WID_TO_WIN(w->hWndOwner);

    /*
      10/13/92 (maa)
        It seems that even if the dialog box's owner is a hidden,
      iconic window, the dialog box is still shown. (IMS program)

      12/13/93 (maa)
        If a dialog is being shown and the owner is in that amorphous
      state between visibility and invisibility, then show the dialog. This
      situation occurs when people put up a sign-on screen at the start
      of a program.
    */
    if (wOwner != NULL && IS_DIALOG(w))
    {
      /*
        If the owner is visible or not explicitly hidden, then show
        the dialog.
      */
      if ((wOwner->flags & WS_VISIBLE) || !TEST_WS_HIDDEN(wOwner))
        return TRUE;
    }

    /*
      Go to the next highest ancestor. For dialog boxes and icon titles,
      this is the owner window (the parent window is always HwndDesktop).
      For regular windows, this is the also the owner window, since the
      owner is the same as the parent.
    */
    w = wOwner;
  }

  return TRUE;  /* If we reach here, it's visible! */
}

