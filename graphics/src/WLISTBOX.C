/*
 *
 * File    :  LISTBOX.C
 *
 * Purpose :  Contains all routines which implement the listbox class.
 *
 * History :
 *
 * (C) Copyright 1989 Marc Adler/Magma Systems           All Rights Reserved
 *
 * $Log:   E:/vcs/mewel/wlistbox.c_v  $
//	
//	   Rev 1.22   25 Oct 1993 13:51:08   Adler
//	Changed lstrcmp() to lstricmp() in listbox sorting.
//	Added new arg to PrepareWMCtlColor()
//	
//	   Rev 1.21   04 Oct 1993 13:04:32   Adler
//	Fixed bug where focus rectangle was being shown in addition to the
//	selected item in a listbox which did not have the focus (ie - the
//	listbox portion of a combobox when the edit portion of the combo
//	has the focus).
//	
//	   Rev 1.20   24 Sep 1993 15:15:16   Adler
//	Added test for WM_HSCROLL in addition to testing for MULTICOLUMN so that
//	listboxes do not scroll redundantly on an hscroll message.
//	Cleaned up selection-state processing in owner-drawn listboxes.
//	
//	   Rev 1.19   31 Aug 1993 11:45:12   Adler
//	No change.
//	
//	   Rev 1.18   19 Aug 1993 10:44:58   Adler
//	
//	   Rev 1.17   18 Aug 1993 17:13:50   Adler
//	Changed WORD to UINT.
//	Added call to TabbedTextOut in the GUI version to account for tabs
//	in a listbox.
//	Added the nTabStops member to the LISTBOX structure.
//	
//	   Rev 1.16   12 Aug 1993  9:55:08   Adler
//	Eliminated test for WM_CHAR. How only use WM_KEYDOWN.
//	
//	   Rev 1.15   19 Jul 1993 11:54:28   Adler
//	No change.
//	
//	   Rev 1.14   19 Jul 1993 11:34:02   Adler
//	Lorenz fix in ListBoxRefresh. Test to see if the listbox is the
//	current focus window in 'bMultiSelWithFocus = ...'
//	
//	   Rev 1.13   07 Jul 1993 11:15:22   Adler
//	In ListBoxCreate, changed GetTextMetrics so that it actually uses
//	a real live DC. Otherwise (under MetaWindows), the text metrics
//	might refer to a font which was not the MEWEL system font (rather
//	the default MetaWindows font).
//	
//	   Rev 1.12   07 Jul 1993  8:07:48   Adler
//	Put in Pargeter code in ListBoxDrawOwnerDrawnItem (#if 70693).
//	Added support for LB_SET/GETITEMHEIGHT messages.
//	
//	   Rev 1.11   04 Jul 1993 10:14:18   Adler
//	Fixed bug at line 338 in _ListBoxMaybeRefresh().
//	
//	   Rev 1.10   21 Jun 1993  8:04:58   Adler
//	Fixed bug with multicolumn listboxes not scrolling when we reach a
//	column which is partially off the window. (Lorenz)
//	
//	   Rev 1.9   14 Jun 1993 12:09:56   Adler
//	Major surgery on ListBoxMaybeRefresh().
//	
//	   Rev 1.8   14 Jun 1993 11:24:46   Adler
//	Fixed bug in text mode where listbox selection wasn't getting highlighted.
//	
//	   Rev 1.7   07 Jun 1993 10:44:06   Adler
//	Added #ifndef MEEL_TEXT around the manipulation of bIgnoreErase. This
//	was due to the fact that the text mode version did not erase anything
//	in ListBoxRefresh() when the WM_ERASEBKGND message was sent (because
//	the listbox's area was totally validated by the call to BeginPaint).
//	
//	   Rev 1.6   04 Jun 1993 23:38:48   Adler
//	Changes to make owner-drawn combos work.
//	
//	   Rev 1.5   04 Jun 1993 20:14:06   Adler
//	Removed processing of WM_GETTEXT message.
//	
//	   Rev 1.4   04 Jun 1993 16:26:20   Adler
//	Removed call to ListBoxClearSelections when an LB_ADD/INSERTSTRING is done.
//	
//	   Rev 1.3   02 Jun 1993 12:05:44   Adler
//	No change.
//	
//	   Rev 1.2   27 May 1993 20:42:34   Adler
//	Got rid of redundant WM_ERASEBKGND in caused by BeginPaint().
//	
//	   Rev 1.1   24 May 1993  9:14:40   unknown
//	Added ifdefs for ZAPP. Drawing code and LBS_NOREDRAW.
//	
//	   Rev 1.0   23 May 1993 21:06:14   adler
//	Initial revision.
 * 
 *    Rev 1.1   16 Aug 1991 15:51:36   MLOCKE
 * Altered header to use normal C comment form for compatiblity
 * with PVCS.
 * 
 * Allow multiple selection list box to return index to currently
 * selected item.
 * 
 * Modified _ListBoxNotifyParent to use SendMessage rather than
 * PostMessage.  PostMessage can cuase processing to occur out
 * of sequence.
 * 
 * Modified presentation of items in multiple selection listboxes
 * to show arrow head 'cursors' at left and right of item.  All
 * items are indented by one space to avoid movement of text as
 * the user scrolls through the selection.
 * 
 * Modified _ListBoxOwnerDrawnItem not to change the attribute of
 * the device context structure.  MS Windows does not change it.
 * Doing so causes a FillRect, TextOut, InvertRect sequence to
 * incorrectly draw the highlight.
 */
#define INCLUDE_CURSES
#define NOKERNEL
#define INCLUDE_LISTBOX
#define INCLUDE_COMBOBOX

/*
  Thomas Wagner of Ferrari Electronics GMBH added code to handle tabs in
  listboxes, and to use itemdata in non-ownerdrawn listboxes.

  91-04-25: Changed user-interface of listboxes to be more like Windows,
  i.e. using the Mouse to scroll through the box does not change the
  selection.
*/

#include "wprivate.h"
#if defined(MEWEL_GUI) || defined(MOTIF)
#include "wgraphic.h"
#endif

#define WAGNER   1  /* KLUDGE for now ... put here so lang_upper is not used */

/*
  CY_LISTBOXOFFSET defines the amount of space to start drawing strings
  from the top border
*/
#if defined(MEWEL_GUI) || defined(MOTIF)
#define CY_LISTBOXOFFSET  4
#else
#define CY_LISTBOXOFFSET  0
#endif



/* 
  Define SZITEMDATA as sizeof(DWORD) for Win3.0 compatible handling of
  LB_SETITEMDATA and LB_GETITEMDATA in non-userdraw Listboxes. 
  Define as 0 is you don't use this feature, so as to not waste space.
*/
#define SZITEMDATA   (sizeof(DWORD))

#if defined(WORD_ALIGNED)
static VOID  PASCAL LB_SET_ITEMDATA(LPSTR, DWORD);
static DWORD PASCAL LB_GET_ITEMDATA(LPSTR);
#else
#define LB_SET_ITEMDATA(pDest, dw)   (* ((LPDWORD) ((pDest)+1)) = (dw))
#define LB_GET_ITEMDATA(pSrc)        (* (LPDWORD) ((pSrc) + 1))
#endif


#ifdef __cplusplus
extern "C" {
#endif

static INT  PASCAL ListBoxChar(PWINDOW,int);
static INT  PASCAL ListBoxDeleteString(LISTBOX *,int);
static LPSTR PASCAL ListBoxGetString(LISTBOX *,int);
static INT  PASCAL ListBoxInsertString(LISTBOX *,LPSTR,int,WPARAM);
static INT  PASCAL ListBoxScroll(WINDOW *, int, int);
static INT  PASCAL ListBoxSelect(LISTBOX *,int,int);
static VOID PASCAL _ListBoxSetScrollBars(PWINDOW,LISTBOX *);
static VOID PASCAL _ListBoxMaybeRefresh(PWINDOW,LISTBOX *,int,int);
static INT  PASCAL _ListBoxGetSelections(LISTBOX *,int,LPINT);
static VOID PASCAL ListboxInvalidateRect(HWND);
static BOOL PASCAL CheckExtendedSelections(WINDOW *, LISTBOX *, BOOL *, BOOL, LPRECT);

/*
  These are extern'ed because they are used by the spin button control
  and by DED.
*/
extern VOID PASCAL ListBoxClearSelections(LISTBOX *);
extern INT  PASCAL ListBoxFindPrefix(LISTBOX *,LPSTR,int,BOOL);
extern VOID PASCAL ListBoxFreeStrings(LISTBOX *);
extern VOID PASCAL ListBoxRefresh(LISTBOX *, HDC, LPRECT);
extern VOID PASCAL _ListBoxNotifyParent(PWINDOW,int);

#ifdef __cplusplus
}
#endif

/*
  This controls the InvalidateRect call for the LB_SETSEL message. If
  we just toggle a single listbox item with the space bar, or if we
  drag the mouse in a multisel listbox, we don't want the *entire*
  listbox being rendered invalid.
*/
static BOOL bDontInvalidateOnSETSEL = FALSE;

static bForceMaybeRefresh = FALSE;


/*
  Extended selection listbox stuff
*/
#define USE_EXTSEL
/*
  Index when bSelecting became TRUE. Used with LBS_EXTENDEDSEL
*/
static int xSelecting = -1;
static VOID PASCAL ListBoxSelectExtended(LISTBOX *, INT);


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxWinProc()                                                */
/*                                                                           */
/*===========================================================================*/
LONG FAR PASCAL ListBoxWinProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  int     key;
  LPSTR   s;
  int     i;
  WINDOW  *lbw;
  LISTBOX *lbi;
  RECT    rClient;
  DWORD   dwFlags;
  BOOL    bIsMultiSel;

  static BOOL bSelecting = FALSE;
  BOOL bIsExtSel;
  static int iCurrListBoxMouseRow;  /* so we know if we moved vertically */
  static BOOL bIgnoreErase = FALSE;

#if defined(ZAPP) && defined(MEWEL_GUI)
  static int  firstBorder;
  static RECT zCliRect;
#endif

  if ((lbw = WID_TO_WIN(hWnd)) == NULL)
    return -1;
  lbi = (LISTBOX *) lbw->pPrivate;

  rClient = lbw->rClient;
  dwFlags = lbw->flags;
  bIsMultiSel = (dwFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0L;
  bIsExtSel   = (dwFlags & LBS_EXTENDEDSEL) != 0L;

  /*
    Client-area mouse messages should be converted back into screen coords
    for this winproc.
  */
  if (IS_WM_MOUSE_MSG(message))
    lParam = _UnWindowizeMouse(hWnd, lParam);


  switch (message)
  {
#if defined(ZAPP) && defined(MEWEL_GUI)	  
    case WM_BORDER:
      if (dwFlags & WS_BORDER)
      {
        WINDOW *w;	  
        HPEN oldPen, boxPen;
        int x1, y1, x2, y2;
        HDC hDC;

        w = WID_TO_WIN(hWnd);
        hDC = GetWindowDC(hWnd);

        x1 = 0;
        y1 = 0;
        x2 = w->rect.right - 2 - w->rect.left;
        y2 = w->rect.bottom - 2 - w->rect.top;
        if (!firstBorder)
        {
          w->rClient.left   += 1;
          w->rClient.top    += 1;
          w->rClient.bottom -= 2;
          w->rClient.right  -= 2;
          firstBorder++;
        }
		  
        boxPen = CreatePen(PS_SOLID, 1, RGB(0xff,0xff,0xff));
        oldPen = SelectObject(hDC,boxPen);
        MoveTo(hDC,x1,y1);
        LineTo(hDC,x2-1,y1);
        LineTo(hDC,x2-1,y2-1);	/* draw first rectangle white */
        LineTo(hDC,x1,y2-1);
        LineTo(hDC,x1,y1);
        SelectObject(hDC,oldPen);
        DeleteObject(boxPen);

        boxPen = CreatePen(PS_SOLID,1,RGB(0x40,0x40,0x40));
        oldPen = SelectObject(hDC, boxPen);
        MoveTo(hDC,x2,y1+1);
        LineTo(hDC,x2,y2);
        LineTo(hDC,x1-1,y2);
        MoveTo(hDC,x1+1,y2-2);
        LineTo(hDC,x1+1,y1+1);
        LineTo(hDC,x2-2,y1+1);
        SelectObject(hDC,oldPen);
        DeleteObject(boxPen);

        ReleaseDC(hWnd,hDC);
      }
      return TRUE;
#endif /* ZAPP */


    case WM_CREATE :
      /*
        10/9/92 (maa)
          Upon creation, we should initially set the scrollbar range
        to 0,0 so they will not get shown by the painting proceedure
        if the listbox is empty. Otherwise, the default scrollbar range
        is 0,100 and the scrollbars will be erroneously shown.
      */
      if (lbi)  /* lbi can be NULL if window rect is 0,0,0,0 */
        _ListBoxSetScrollBars(lbw, lbi);
#if defined(ZAPP) && defined(MEWEL_GUI)	  
      firstBorder = 0;
      SetRect(&zCliRect, 0, 0, 0, 0);
#endif	  
      break;

    case WM_ERASEBKGND :
      /*
        10/9/92 (maa)
        Do not erase the background of the listbox if the redraw flag
        is off.
      */
#if !defined(ZAPP) && !20195
      /* zapp - commented this out since it was not redrawing at times */
      if ((dwFlags & LBS_NOREDRAW) || bIgnoreErase)
        return TRUE;
      else
#endif
      {
        _PrepareWMCtlColor(hWnd, CTLCOLOR_LISTBOX, (HDC) wParam);
        goto call_dwp;
      }

    case WM_PAINT  :
    {
      PAINTSTRUCT ps;

#if defined(ZAPP) && defined(MEWEL_GUI)	  
      WINDOW * w = WID_TO_WIN(hWnd);		  
      if (w->rClient.bottom != zCliRect.bottom)
      {
        w->rClient.left   += 1;
        w->rClient.top    += 1;
        w->rClient.bottom -= 2;
        w->rClient.right  -= 2;
        zCliRect = w->rClient;
      }
#endif

      /*
        Tell ListBoxWinProc to ignore the WM_ERASEBKGND message
        which is generated by the BeginPaint() since ListBoxRefresh()
        will send it's own WM_ERASEBKGND.
        We need to do the WM_ERASEBKGND always in text mode. If we
        try to do it at the time of the ListBoxRefresh(), then the
        listbox's area will be entirely valid, and the erasing won't
        do anything. (See WM_ERASEBKGND logic for text mode in
        StdWindowWinProc).
      */
#ifndef MEWEL_TEXT
      bIgnoreErase++;
#endif

      BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
      _PrepareWMCtlColor(hWnd, CTLCOLOR_LISTBOX, ps.hdc);


      /*
        OK to process WM_ERASEBKGND.
      */
#ifndef MEWEL_TEXT
      bIgnoreErase--;
#endif

      ListBoxRefresh(lbi, ps.hdc, &ps.rcPaint);
      EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
      break;
    }

    case WM_SETFOCUS :
    case WM_KILLFOCUS :
    {
      InvalidateNCArea(hWnd);
      _ListBoxNotifyParent(lbw, (message == WM_SETFOCUS) ? LBN_SETFOCUS
                                                         : LBN_KILLFOCUS);
      /*
        Let the owner-drawn listbox add or remove the focus indicator
        from the current item.
      */
      bForceMaybeRefresh++;
      _ListBoxMaybeRefresh(lbw, lbi, lbi->iCurrSel, lbi->iTopLine);
      bForceMaybeRefresh--;
      break;
    }

    case WM_DESTROY    :
      /*
        When we destroy a listbox, we must also free up the allocated
        string list and the tab-stop array.
      */
      if (lbi != NULL)    /* lbi can be NULL if window rect is 0,0,0,0 */
      {
        ListBoxFreeStrings(lbi);
        if (lbi->pTabStops)
          MYFREE_FAR(lbi->pTabStops);
      }
      break;

    case WM_GETDLGCODE :
      return DLGC_WANTARROWS;

    case WM_SIZE :
      /*
        Calculate the number of strings which can be shown within
        the listbox. Refresh the scrollbar if we changed the size,
        cause now, we might be able to hide or show the scrollbar.
      */
      lbi->nVisibleStrings = (RECT_HEIGHT(rClient) - CY_LISTBOXOFFSET) /
                              lbi->tmHeightAndSpace;
      _ListBoxSetScrollBars(lbw, lbi);
      break;


    case WM_KEYDOWN :
    case WM_CHAR    : /* needed for the generation of WM_CHARTOITEM */
      i = -1;

      /*
        See if the listbox has the LBS_WANTKEYBOARDINPUT style. If so,
        then send the parent the notification message.
      */
      if ((dwFlags & LBS_WANTKEYBOARDINPUT) && !(lbw->ulStyle&LBS_IN_COMBOBOX))
      {
        i = (int) SendMessage(lbw->hWndOwner,
                   (message == WM_KEYDOWN) ? WM_VKEYTOITEM : WM_CHARTOITEM,
                   wParam, MAKELONG(hWnd, lbi->iCurrSel));
        /*
          If i is -1, then do the default action.
          If i is -2, then the app handled the processing.
          If i >= 0, then do the default processing on the ith item.
        */
        if (i == -2)
        {
          _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
          break;
        }
        if (i >= 0)
          lbi->iCurrSel = i;
      }

      /*
        Process the character. If the selection changed, either by 
        ListBoxChar() returning TRUE or by the app returning i>=0 in
        response to the WM_CHARTOITEM message, then notify the parent.
      */
      if ((message == WM_KEYDOWN && ListBoxChar(lbw, wParam)) || i >= 0)
        _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
      break;


    case WM_HSCROLL :
    case WM_VSCROLL :
      /* The user touched the listbox's vertical scroll bar */
      key = 0;
      switch (wParam)
      {
        case SB_LINEUP         :
          key = (message == WM_VSCROLL) ? VK_UP : VK_LEFT;
          break;
        case SB_LINEDOWN       :
          key = (message == WM_VSCROLL) ? VK_DOWN : VK_RIGHT;
          break;
        case SB_PAGEUP         :
          key = VK_PRIOR;
          break;
        case SB_PAGEDOWN       :
          key = VK_NEXT;
          break;
        case SB_THUMBTRACK    :
        case SB_THUMBPOSITION :
        {
          HWND hVSB, hHSB;
          int  minpos, maxpos, currpos;

          /*
            Kludge to prevent flickering in multicolumn listboxes
          */
          if ((dwFlags & LBS_MULTICOLUMN) || message == WM_HSCROLL)
            break;

          WinGetScrollbars(hWnd, &hHSB, &hVSB);
          GetScrollRange(hVSB, SB_CTL, &minpos, &maxpos);

          /*
            lParam has a value between minpos and maxpos
          */
          currpos = LOWORD(lParam);
#if defined(MEWEL_GUI) || defined(MOTIF)
          if (currpos == GetScrollPos(hVSB, SB_CTL))
            break;
#endif

          SetScrollPos(hVSB, SB_CTL, max(min(currpos, maxpos), minpos), 
#if defined(MEWEL_GUI) || defined(MOTIF)
                       (wParam == SB_THUMBPOSITION));
#else
                       TRUE);  /* always move in text mode - no outline */
#endif
          currpos = GetScrollPos(hVSB, SB_CTL);

          /*
            Set the top visible entry
          */
          if ((lbi->iTopLine = min(lbi->nStrings - lbi->nVisibleStrings, currpos-1)) < 0)
            lbi->iTopLine = 0;

          /*
            To make the listboxes behave more like MS Windows, we don't
            actually scroll the listbox on a THUMBTRACK message
          */
          if (wParam == SB_THUMBTRACK)
          {
            ListboxInvalidateRect(hWnd);
            UpdateWindow(hWnd);
          }
          else
          {
            lbi->iCurrSel = min(currpos-1, lbi->nStrings-1);
            ListboxInvalidateRect(hWnd);
            UpdateWindow(hWnd);
            _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
          }
          key = 0;
        }
        break;
      }

      if (key)
      {
        if (dwFlags & LBS_MULTICOLUMN)
          ListBoxChar(lbw, key);
        else
          ListBoxScroll(lbw, key, message);
      }
      break;


    case WM_LBUTTONDOWN :
    {
      /*
        Get the mouse position as relative from the client area
      */
      int mouserow = HIWORD(lParam) - rClient.top;
      int mousecol = LOWORD(lParam) - rClient.left;
      int iNewSel, iOldSel;

#ifdef USE_EXTSEL
      BOOL bCtrlKeyDown = (BOOL) ((KBDGetShift() & CTL_SHIFT) != 0);
#endif

      /*
        If we are going to get a double-click message, then defer the
        processing of the second mouse-down message.
      */
      if (SysEventInfo.DblClickPendingMsg)
        break;

      /*
        Ignore clicks on the listbox border
      */
      if (!PtInRect(&rClient,MAKEPOINT(lParam)) || mouserow < 0 || mousecol < 0)
        break;

#if defined(MEWEL_GUI) || defined(MOTIF)
      mouserow -= CY_LISTBOXOFFSET;
      mouserow /= lbi->tmHeightAndSpace;
#endif

      iNewSel = (dwFlags & LBS_MULTICOLUMN)
                 ? (lbi->iLeftCol + mousecol/lbi->iColWidth) * 
                                            lbi->nVisibleStrings + mouserow
                 : lbi->iTopLine + mouserow;

      if (iNewSel > lbi->nStrings)
        break;

      iCurrListBoxMouseRow = mouserow;

      /*
        If Extended, a new selection clears all old selections if the
        CTRL key is not being pressed.
      */
#ifdef USE_EXTSEL
      if (bIsExtSel && !bCtrlKeyDown)
        SendMessage(lbi->hListBox, LB_SETSEL, 0, MAKELONG(-1, 0));
#endif

      /*
        Select the new string and refresh the window to show the
        new selection. Also, capture the mouse in case we want to
        drag through the strings in the listbox.
      */
      iOldSel = lbi->iCurrSel;

      /*
        If extended, just set, never toggle.
      */
      ListBoxSelect(lbi, iNewSel, (bIsMultiSel && !bIsExtSel) ? -1 : TRUE);

      _ListBoxMaybeRefresh(lbw, lbi, iOldSel, lbi->iTopLine);
      SetCapture(hWnd);
      bSelecting = TRUE;

      /*
        Keep track of index of first selection
      */
      xSelecting = iNewSel;

      /*
        Don't notify parent until all selections are made.
      */
      if (!bIsExtSel || bCtrlKeyDown)
        _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
      break;
    }


    case WM_NCLBUTTONUP :
    case WM_LBUTTONUP   :
      /*
        If extended, notify parent now that all selections are made
      */
      if (bSelecting && bIsExtSel)
        _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
      ReleaseCapture();
      bSelecting = FALSE;
      iCurrListBoxMouseRow = -1;
      xSelecting = -1;
      break;


    case WM_MOUSEREPEAT :
    case WM_MOUSEMOVE   :
#ifdef DOS
      if (bSelecting && IsMouseLeftButtonDown())
#else
      if (bSelecting)
#endif
      {
        int mouserow = HIWORD(lParam) - rClient.top;
        int mousecol = LOWORD(lParam) - rClient.left;
        int lbheight = lbi->nVisibleStrings;
        int lbwidth  = RECT_WIDTH(rClient);

#if defined(MEWEL_GUI) || defined(MOTIF)
        mouserow -= CY_LISTBOXOFFSET;
        mouserow /= lbi->tmHeightAndSpace;
#endif
        i = lbi->iCurrSel;  /* record the current selected item */

        if (mouserow < 0)
        {
          if (lbi->iTopLine > 0)
            ListBoxChar(lbw, VK_UP);
        }
        else if (mouserow >= lbheight)
        {
          if (lbi->iTopLine + mouserow < lbi->nStrings)
            ListBoxChar(lbw, VK_DOWN);
        }
        else if (mousecol < 0)
        {
          if (lbi->iLeftCol > 0)
            ListBoxChar(lbw, VK_LEFT);
        }
        else if (mousecol >= lbwidth)
        {
          ListBoxChar(lbw, VK_RIGHT);
        }
        else
        {
          int  iNewSel; /* current selection */

          iNewSel = (dwFlags & LBS_MULTICOLUMN)
                        ? (lbi->iLeftCol + mousecol/lbi->iColWidth) * 
                             lbi->nVisibleStrings + mouserow
                        : lbi->iTopLine + mouserow;
          if (iNewSel > lbi->nStrings)
            break;

          /*
            Don't change the selection for a mouse-repeat msg inside of the
            listbox, nor if we moved the mouse horizontally.
          */
          if (message == WM_MOUSEREPEAT || mouserow == iCurrListBoxMouseRow ||
              iNewSel == i)
            break;

          /* if extended, allow to shrink as well as grow selection range. */
          if (bIsExtSel)
          {
            ListBoxSelectExtended(lbi, iNewSel);
          }
          else  /* not extended sel */
            ListBoxSelect(lbi, iNewSel, bIsMultiSel ? -1 : TRUE);

          _ListBoxMaybeRefresh(lbw, lbi, i, lbi->iTopLine);
          iCurrListBoxMouseRow = mouserow;
        }

        if (lbi->iCurrSel != i)  /* did the selection change? */
        {
          /*
             Windows comboboxes don't send LBN_SELCHANGE if the user is
             scrolling through the listbox portion of a combobox with
             the mouse.
          */
          if (!(lbw->ulStyle & LBS_IN_COMBOBOX) && !bIsExtSel)
            _ListBoxNotifyParent(lbw, LBN_SELCHANGE);
        }
      }
      break;


    case WM_LBUTTONDBLCLK :
      if (dwFlags & LBS_NOTIFY)
        _ListBoxNotifyParent(lbw, LBN_DBLCLK);
      else
        SendMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0L);
      break;

  /* ---------------------------------------------------- */

    case LB_ADDSTRING         :
      /* lParam is a FAR ptr to the string */
      i = ListBoxInsertString(lbi, (LPSTR) lParam, ATEND, LB_ADDSTRING);
      if (i == LB_ERRSPACE)
        _ListBoxNotifyParent(lbw, LBN_ERRSPACE);
      return i;

    case LB_INSERTSTRING :
      /* wParam is the index of the place to insert into, -1 if at the end */
      /* lParam is a FAR ptr to the string */
      i = ListBoxInsertString(lbi, (LPSTR) lParam, wParam, LB_INSERTSTRING);
      if (i == LB_ERRSPACE)
        _ListBoxNotifyParent(lbw, LBN_ERRSPACE);
      return i;

    case LB_DELETESTRING :
      /* wParam is the index of the string */
      return ListBoxDeleteString(lbi, wParam);

    case LB_SETCURSEL         :
    {
      int  oldCurrSel = lbi->iCurrSel;

      /* wParam is the index of the string */
      if (ListBoxSelect(lbi, wParam, TRUE) == LB_ERR)
        return LB_ERR;

      _ListBoxMaybeRefresh(lbw, lbi, oldCurrSel, lbi->iTopLine);
      if (lbw->ulStyle & LBS_IN_COMBOBOX)
        _ListBoxNotifyParent(lbw, CBN_INTERNALSELCHANGE);
      return TRUE;
    }

    case LB_SELECTSTRING     :
    case LB_FINDSTRING       :
    case LB_FINDSTRINGEXACT  :
      /* wParam is the index where to start searching (-1 if we should
         start at the top). The searching actually starts from the index+1
         place. lParam is a ptr to the prefix string.
      */
      if ((bIsMultiSel) && message == LB_SELECTSTRING)
        return LB_ERR;
      if ((i = ListBoxFindPrefix(lbi, (LPSTR) lParam,
                 (wParam == 0xFFFF || wParam == 0) ? 0 : wParam+1,
                 (message == LB_FINDSTRINGEXACT))) < 0)
        return LB_ERR;
      if (message == LB_SELECTSTRING)
      {
        i = ListBoxSelect(lbi, i, TRUE);
#ifdef WAGNER
        /* scroll into view */
        _ListBoxMaybeRefresh(lbw, lbi, lbi->iCurrSel, lbi->iTopLine);
#else
        ListboxInvalidateRect(hWnd);
#endif
      }
      return i;

    case LB_SELITEMRANGE :
      /*
        Select or deselect a range of items in a multiple selection
        listbox. wParam is the selection state, LOWORD(lParam) is the
        first item and HIWORD(lParam) is the last item.
      */
      if (!(bIsMultiSel))
        return LB_ERR;
      for (i = (int) LOWORD(lParam);  i <= (int) HIWORD(lParam);  i++)
      {
        bDontInvalidateOnSETSEL = TRUE;
        SendMessage(hWnd, LB_SETSEL, wParam, MAKELONG(i, 0));
      }
      ListboxInvalidateRect(hWnd);
      break;

    case LB_SETSEL         :
    {
      RECT   rInvalid;
      LPRECT lpRect = NULL;

      /*
         This can only be used with multiple selection listboxes.
         wParam is 0 if the string should be deselected, non-zero otherwise.
         LOWORD(lParam) is the index of the string, -1 if the selection
         should be removed from all strings.
      */
      if (!bIsMultiSel)
        return LB_ERR;

      if ((i = (short) LOWORD(lParam)) == -1)
      {
        LIST *p;
        BOOL bSelVisible;
        BOOL bAnythingSelected = CheckExtendedSelections(lbw, lbi, &bSelVisible, wParam, &rInvalid);
        if (!bAnythingSelected || !bSelVisible)
          bDontInvalidateOnSETSEL = TRUE;
        else
          lpRect = &rInvalid;
        for (p = lbi->strList;  p;  p = p->next)
          * (LPSTR) p->data = (BYTE) wParam;
      }
      else
      {
        LIST *p;
        if ((p = ListGetNth(lbi->strList, i)) != NULL)
          * (LPSTR) p->data = (BYTE) wParam;
      }
      /*
        Invalidate the entire listbox if we changed all of the entries
        or if the 'bDontInvalidateOnSETSEL'variable wasn't set.
      */
      if (i == -1)
      {
        if (bDontInvalidateOnSETSEL == FALSE)
#if defined(USE_NATIVE_GUI)
         if (IS_OWNERDRAWN(dwFlags))
#endif
           InvalidateRect(hWnd, lpRect, TRUE);
      }
      else
      {
        if (bDontInvalidateOnSETSEL == FALSE)
          _ListBoxMaybeRefresh(lbw, lbi, lbi->iCurrSel, lbi->iTopLine);
      }

      bDontInvalidateOnSETSEL = FALSE;
      break;
    }


    case LB_GETCARETINDEX  :
      return (lbi->nStrings <= 0) ? -1 : lbi->iCurrSel;

    case LB_SETCARETINDEX  :
    {
      int iOldCurrSel = lbi->iCurrSel;
      int iOldTopLine = lbi->iTopLine;

      /*
        Use a value of -2 for ListBoxSelect in order to tell it not to
        alter the 'selected' state of the item.
      */
      if (ListBoxSelect(lbi, wParam, -2) == LB_ERR)
        return LB_ERR;
      _ListBoxMaybeRefresh(lbw, lbi, iOldCurrSel, iOldTopLine);
      return 0L;
    }


    case LB_GETCOUNT         :
      return lbi->nStrings;

    case LB_GETCURSEL         :
    /*
     * Allow the multiple selection listbox to return the index
     * to the currently selected item.  Windows SDK says you
     * aren't supposed to be able to do this but it lets you
     * do it anyway so we allow MEWEL to do it also.
     */
      return (lbi->iCurrSel < 0 ? LB_ERR : lbi->iCurrSel);

    case LB_GETSEL         :
      /* wParam is the index of the string to query */
      if (bIsMultiSel)
      {
        LIST *p = ListGetNth(lbi->strList, wParam);
        if (p && * (LPSTR) p->data != 0)
          return TRUE;
        else
          return FALSE;
      }

      return (lbi->iCurrSel == (int) wParam);


    case LB_GETTEXT         :
      /*
        wParam is the index of the string to retrieve
        lParam is the FAR buffer to copy it into.
      */
      if ((int) wParam < 0 || (int) wParam >= lbi->nStrings)
        return LB_ERR;

      /*
        If we have an owner-drawn, non-string item, then copy
        the 32-bit item identifier to the buffer.
      */
      if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbw->flags))
      {
        LIST *p = ListGetNth(lbi->strList, wParam);
        * (LPDWORD) lParam = LB_GET_ITEMDATA(p->data);
        return sizeof(DWORD);
      }

      if ((s = ListBoxGetString(lbi, wParam)) != NULL)
      {
        lstrcpy((LPSTR) lParam, s);
        return lstrlen(s);
      }
      else
        return LB_ERR;

    case LB_GETTEXTLEN         :
      /* wParam is the index of the string to retrieve        */
      if ((int) wParam < 0 || (int) wParam >= lbi->nStrings)
        return LB_ERR;
      s = ListBoxGetString(lbi, wParam);
      return (s) ? lstrlen(s) : 0;

#if !60493
    /*
      6/4/93 (maa)
        Removed the WM_GETTEXT logic. The DDLIST sample shows that
        the WM_GET/SETTEXT messages have nothing at all to do with
        the text corresponding to the listbox items.
    */
    case WM_GETTEXT            :
      if (bIsMultiSel || lbi->iCurrSel < 0)
        return LB_ERR;
      if ((s = ListBoxGetString(lbi,lbi->iCurrSel)) == NULL)
        return LB_ERR;
      lstrncpy((LPSTR) lParam, s, wParam);
      return lstrlen((LPSTR) lParam);
#endif

    case WM_GETTEXTLENGTH      :
      return SendMessage(hWnd,
                         LB_GETTEXTLEN,
                         (WPARAM) SendMessage(hWnd,LB_GETCURSEL,0,0L),
                         lParam);


    case LB_RESETCONTENT :
      ListBoxFreeStrings(lbi);
      lbi->nStrings = 0;
      lbi->iCurrSel = (bIsMultiSel) ? 0 : -1;
      lbi->iTopLine = 0;
      lbi->iLeftCol = 0;
      _ListBoxSetScrollBars(lbw, lbi);
      ListboxInvalidateRect(hWnd);
      break;

    case LB_DIR          :
      ListBoxDir(lbi, (LPSTR ) lParam, wParam);
      break;

    case LB_GETSELCOUNT :
      return _ListBoxGetSelections(lbi, 0, (LPINT) NULL);
    case LB_GETSELITEMS :
      return _ListBoxGetSelections(lbi, wParam, (LPINT) lParam);
    case LB_GETTOPINDEX :
      return lbi->iTopLine;
    case LB_SETTOPINDEX :
      if ((int) wParam < 0 || (int) wParam >= lbi->nStrings)
        return LB_ERR;
      lbi->iTopLine = wParam;
      lbi->iCurrSel = wParam;
      ListboxInvalidateRect(hWnd);
      return lbi->iTopLine;

    case LB_SETCOLUMNWIDTH       :
      if (!(dwFlags & LBS_MULTICOLUMN) || wParam < 1)
        return LB_ERR;
      lbi->iColWidth = wParam;
      ListboxInvalidateRect(hWnd);
      return TRUE;

    case LB_SETTABSTOPS          :
      /*
        wParam is the number of tabstops and lParam is a pointer to a
        buffer containing the tabstops.
        If wParam is 1, then the first number in the buffer is considered
        to be the width. If wParam > 1, then each number in the buffer
        represents a tabstop column.
        If there is one width, then we will negate the number and place
        it into the first word of pTabStops.
        Question : should the tabstops be in client or screen coordinates?
      */
      if (!(dwFlags & LBS_USETABSTOPS))
        return LB_ERR;

      /* 
        I allocate a tab stop buffer even if wParam is 1. The buffer is
        filled with tab stops for every <width> bytes, up to the column
        length of the box. This should make it easier to handle tabs
        when outputting the data (no checks for special cases).
        Tab stops are relative to the start of the box (WIN3 compatible).
        The tab stops are terminated with a 0, the count is not stored.
      */
      {
      int   n;
      LPINT p;
      UINT  w;

      if (lbi->pTabStops)
      {
        MYFREE_FAR(lbi->pTabStops);
        lbi->pTabStops = NULL;
      }

      if (!wParam)
        return FALSE;

      if (wParam == 1)
        n = RECT_WIDTH(rClient) / *((UINT *)lParam) + 2;
      else
        n = wParam + 1;

      if ((p = lbi->pTabStops = (LPINT)EMALLOC_FAR_NOQUIT(n*sizeof(INT))) == NULL)
        return FALSE;
      lbi->nTabStops = --n;

      if (wParam == 1)
        for (w = *((UINT FAR *)lParam);  n > 0;  w += *((UINT FAR *)lParam), n--)
          *p++ = w;
      else
        for (i = 0;  n > 0;  i++, n--)
          *p++ = ((UINT FAR *) lParam)[i];
      *p = 0;
      }
      return TRUE;  /* return FALSE if tabstops not set, TRUE if they are */


    case LB_GETHORIZONTALEXTENT  :
      return lbi->wHorizExtent;
    case LB_SETHORIZONTALEXTENT  :
      if (WinHasScrollbars(hWnd, SB_HORZ))  /* is there a horz scrollbar? */
      {
        lbi->wHorizExtent = wParam;
      }
      return lbi->wHorizExtent;


    case WM_SETREDRAW :
      ListBoxSetRedraw(hWnd, wParam);
      break;

    case WM_SYSCOLORCHANGE :
      if (wParam == SYSCLR_LISTBOX)
      {
        lbw->ulStyle |= WIN_UPDATE_NCAREA;
        ListboxInvalidateRect(hWnd);
      }
      break;

#ifdef OWNERDRAWN
    case WM_COMPAREITEM :
    case WM_DELETEITEM  :
    case WM_DRAWITEM    :
    case WM_MEASUREITEM :
      return SendMessage(GetParent(hWnd), message, wParam, lParam);
#endif

    case LB_SETITEMDATA :
    case LB_GETITEMDATA :
    {
      LIST *p;

      if ((p = ListGetNth(lbi->strList, wParam)) != NULL)
      {
        if (message == LB_SETITEMDATA)
          LB_SET_ITEMDATA(p->data, lParam);
        return LB_GET_ITEMDATA(p->data);
      }
      else
        return LB_ERR;
    }

    case LB_GETITEMHEIGHT :
      return lbi->tmHeightAndSpace;
    case LB_SETITEMHEIGHT :
      lbi->tmHeight = lbi->tmHeightAndSpace = wParam;
      break;


    default :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);

  } /* switch */

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _ListBoxNotifyParent()                                        */
/*                                                                          */
/* Purpose  : Sends parent of listbox a notification code in HIWORD(lParam) */
/*                                                                          */
/****************************************************************************/
VOID PASCAL _ListBoxNotifyParent(lbw, notifyCode)
  WINDOW  *lbw;
  int     notifyCode;
{
  if (lbw->hWndOwner && (lbw->flags & LBS_NOTIFY))
    SendMessage(lbw->hWndOwner, WM_COMMAND, lbw->idCtrl,
                                     MAKELONG(lbw->win_id, notifyCode));
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxCreate()                                                 */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL ListBoxCreate(hParent,row1,col1,row2,col2,title,attr,flFlags,id)
  HWND  hParent;
  int   row1, row2, col1, col2;
  LPSTR title;
  COLOR attr;
  DWORD flFlags;
  UINT  id;
{
  TEXTMETRIC tm;
  HWND    hLB;
  HDC     hDC;
  WINDOW  *lbw;
  LISTBOX *lbi;
  DWORD   dwStyle = flFlags | WS_VSCROLL | WS_CLIPSIBLINGS | WS_GROUP;


  /*
    Multi-column listboxes should not have a horizontal scrollbar, unless
    the app specifically mentions WS_HSCROLL in the style. Pkus, they
    should *not* have a vertical scrollbar.
  */
  if (flFlags & LBS_MULTICOLUMN)
    dwStyle &= ~WS_VSCROLL;

  /*
    Call the low level window creation routine.
  */
  hLB = WinCreate(hParent, row1,col1,row2,col2, title,attr, dwStyle,
                  LISTBOX_CLASS, id);
  if (hLB == NULL)
    return NULL;

  /*
    Fill in the listbox info structure
  */
  lbw = WID_TO_WIN(hLB);
  lbw->pPrivate = emalloc(sizeof(LISTBOX));
  lbi = (LISTBOX *) lbw->pPrivate;
  memset((BYTE *) lbi, 0, sizeof(LISTBOX));
  lbi->iCurrSel = (flFlags & (LBS_MULTICOLUMN | LBS_EXTENDEDSEL)) ? 0 : -1;
  lbi->hListBox = hLB;
  lbi->wListBox = lbw;

  /*
    Get the height of the font
    (Note : this should really be the font returned by WM_GETFONT)
  */
#if defined(MOTIF)
  hDC = GetDC(_HwndDesktop);
  GetTextMetrics(hDC, &tm);
  ReleaseDC(_HwndDesktop, hDC);
#else
  hDC = GetDC(hLB);
  GetTextMetrics(hDC, &tm);
  ReleaseDC(hLB, hDC);
#endif

  lbi->tmWidth  = tm.tmAveCharWidth;
  lbi->tmHeight = tm.tmHeight;
  lbi->tmHeightAndSpace = tm.tmHeight + tm.tmExternalLeading;

  /*
    The default column width is the entire client area
  */
  lbi->iColWidth = lbw->rClient.right - lbw->rClient.left;

#if defined(OWNERDRAWN) && (defined(MEWEL_GUI) || defined(MOTIF))
  if (IS_OWNERDRAWN(flFlags))
  {
    MEASUREITEMSTRUCT mis;

    mis.CtlType    = ODT_LISTBOX;
    mis.CtlID      = id;
    mis.itemID     = 0;
    mis.itemData   = 0L;
    mis.itemHeight = lbi->tmHeightAndSpace;
    mis.itemWidth  = tm.tmAveCharWidth;
    if (SendMessage(hParent, WM_MEASUREITEM, id, (LONG) (LPSTR) &mis))
    {
      lbi->tmHeight = lbi->tmHeightAndSpace = mis.itemHeight;
      lbi->tmWidth  = mis.itemWidth;
    }
  }
#endif

  /*
    Calculate the number of visible strings in the listbox
  */
  lbi->nVisibleStrings = (WIN_CLIENT_HEIGHT(lbw) - CY_LISTBOXOFFSET) /
                         lbi->tmHeightAndSpace;

  return hLB;
}



/*===========================================================================*/
/*                                                                           */
/* Function: _ListBoxSetScrollBars()                                         */
/*                                                                           */
/* Called by: ListBoxRefresh() and ListBoxMaybeRefresh()                     */
/*                                                                           */
/*===========================================================================*/
static VOID PASCAL _ListBoxSetScrollBars(lbw, lbi)
  WINDOW *lbw;
  LISTBOX *lbi;
{
  HWND hLB;
  HWND hHSB, hVSB;
  BOOL bRefresh;
  int  oldMin, oldMax, oldPos;
  int  nStrings = lbi->nStrings;

#if defined(MOTIF)
  /*
    Only set the scrollbars on ownerdrawn listboxes
  */
  if (!IS_OWNERDRAWN(lbw->flags))
    return;
#endif

  hLB = lbw->win_id;
  WinGetScrollbars(hLB, &hHSB, &hVSB);

  if (hVSB)
  {
#ifdef WAGNER
    int lbheight = lbi->nVisibleStrings;

    GetScrollRange(hVSB, SB_CTL, &oldMin, &oldMax);
    oldPos = GetScrollPos(hVSB, SB_CTL);

    bRefresh = (oldPos != lbi->iTopLine+1) ||
               (oldMax != max(nStrings - lbheight + 1, 2));
    /*
      Show the scrollbar only if the number of strings exceeds the height
      of the listbox.
    */
    if (lbi->nStrings > lbi->nVisibleStrings)
    {
      if (lbw->flags & LBS_DISABLENOSCROLL)
        EnableWindow(hVSB, TRUE);
      SetScrollRange(hVSB, SB_CTL, 1, max(nStrings-lbheight+1, 2), FALSE);
      SetScrollPos(hVSB, SB_CTL, lbi->iTopLine + 1, bRefresh);
      if (bRefresh)
        InvalidateNCArea(hLB);
    }
    else
    {
      if (lbw->flags & LBS_DISABLENOSCROLL)
        EnableWindow(hVSB, FALSE);
      else
        SetScrollRange(hVSB, SB_CTL, 0, 0, FALSE);
    }

#else
    SetScrollRange(hVSB, SB_CTL, 1, max(nStrings, 2), FALSE);
    SetScrollPos(hVSB, SB_CTL, lbi->iCurrSel + 1, !bVirtScreenEnabled);
#endif
  }

  if (hHSB)
  {
    int lbheight = lbi->nVisibleStrings;
    if (lbheight)
    {
      UINT iMax = (lbw->flags & LBS_MULTICOLUMN) ? nStrings/lbheight
                                                 : lbi->wHorizExtent;

      SetScrollRange(hHSB, SB_CTL, 1, iMax+1, FALSE);
      SetScrollPos(hHSB, SB_CTL, lbi->iLeftCol + 1, !bVirtScreenEnabled);
      InvalidateNCArea(hLB);

      /*
        Re-calculate the number of strings which can be shown within
        the listbox, since we might have changed the visibility state
        of the horizontal scrollbar.
      */
      lbi->nVisibleStrings = (RECT_HEIGHT(lbw->rClient) - CY_LISTBOXOFFSET) /
                              lbi->tmHeightAndSpace;
    }
  }
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxSetRedraw()                                              */
/*                                                                           */
/* Purpose : Called to enable/disable listbox refreshing.                    */
/*                                                                           */
/* Returns : TRUE if successful, -1 if bad window handle.                    */
/*                                                                           */
/*===========================================================================*/
int FAR PASCAL ListBoxSetRedraw(HWND hBox, BOOL fRedraw)
{
  WINDOW  *lbw;

  if ((lbw = WID_TO_WIN(hBox)) == NULL)
    return -1;
  if (fRedraw)
  {
    lbw->flags &= ~LBS_NOREDRAW;
    if (IsWindowVisible(hBox))
      ListboxInvalidateRect(hBox);
  }
  else
    lbw->flags |= LBS_NOREDRAW;
  return TRUE;
}


static VOID PASCAL ListboxInvalidateRect(HWND hLB)
{
#if defined(USE_NATIVE_GUI)
  WINDOW *w = WID_TO_WIN(hLB);
  if (!IS_OWNERDRAWN(w->flags))
    return;
#endif

  InvalidateRect(hLB, (LPRECT) NULL, TRUE);
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxInsertString()                                           */
/*                                                                           */
/* Purpose : This function is in charge of inserting a string within a       */
/*           listbox.                                                        */
/*           NOTE : If any changes are made to optimize memory handling      */
/*                  within listboxes, this is the place to make it.          */
/*                                                                           */
/* Returns : 0-based position where string was inserted.                     */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL ListBoxInsertString(lbi, str, pos, message)
  LISTBOX *lbi;  /* listbox structure */
  LPSTR   str;   /* string to insert        */
  int     pos;   /* 0-based position to insert the string at */
  WPARAM  message; /* LB_INSERTSTRING or LB_ADDSTRING */
{
  LIST    *p, *pNth;
  WINDOW  *lbw;
  LPSTR   s;

  lbw = lbi->wListBox;

  /*
    Save a copy of the string
  */
#ifdef OWNERDRAWN
  if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbw->flags))
  {
    if ((s = (LPSTR) EMALLOC_FAR_NOQUIT(SZITEMDATA + 1)) == NULL)
      return LB_ERRSPACE;
    LB_SET_ITEMDATA(s, (DWORD) str);
  }
  else
#endif
  {
    if ((s = (LPSTR) EMALLOC_FAR_NOQUIT(lstrlen(str)+2+SZITEMDATA)) == NULL)
      return LB_ERRSPACE;
    lstrcpy(s + 1 + SZITEMDATA, str);
#ifdef sparc
    memset((LPDWORD) (s+1), 0, sizeof(DWORD));
#else
    LB_SET_ITEMDATA(s, 0L);
#endif
  }
  str = s;


  /*
    Create a list structure
  */
  if ((p = ListCreate(s)) == NULL)  /* really should be (LPSTR) str */
    return LB_ERRSPACE;

  if (pos < 0 ||  /* (pos == -1) => insert the string to the end of the list */
      pos >= lbi->nStrings)
  {
    /*
      Only perform sorting if we are ADDING, not INSERTING.
    */
    if ((lbw->flags & LBS_SORT) && message == LB_ADDSTRING)
    {
      int idxList = 0;

      /*
        Locate the proper place to insert the string
      */
      for (pNth = lbi->strList;  pNth;  pNth = pNth->next)
      {
#ifdef OWNERDRAWN
        if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbw->flags))
        {
          COMPAREITEMSTRUCT cis;
          BOOL  bIsCB = (BOOL) ((lbw->ulStyle & LBS_IN_COMBOBOX) != 0L);
          cis.CtlType  = (bIsCB) ? ODT_COMBOBOX : ODT_LISTBOX;
          cis.CtlID    = lbw->idCtrl;
          cis.hwndItem = lbi->hListBox;
          cis.itemID1  = idxList;
          cis.itemID2  = pos;
          cis.itemData1= LB_GET_ITEMDATA(pNth->data);
          cis.itemData2= LB_GET_ITEMDATA(str);
          if (SendMessage(GetParent(lbi->hListBox), WM_COMPAREITEM, lbw->idCtrl,
                                 (DWORD) (LPCOMPAREITEMSTRUCT) &cis) >= 0)
            break;
        }
        else
#endif
        {
          if (lstricmp((LPSTR) pNth->data+1+SZITEMDATA, str+1+SZITEMDATA) >= 0)
            break;
        }
        idxList++;
      }
      ListInsert(&lbi->strList, p, pNth);
      pos = idxList;
    }
    else
    {
addtoend:
      ListAdd(&lbi->strList, p);
      pos = lbi->nStrings;
    }
  }

  /*
    Add the string to the end of the list.
  */
  else if ((pNth = ListGetNth(lbi->strList, pos)) == NULL)
    goto addtoend;

  /*
    Insert the string before the nth item in the list.
  */
  else
    ListInsert(&lbi->strList, p, pNth);

  /*
   Increase the count of the number of strings.
  */
  lbi->nStrings++;

  /*
    Redraw the listbox if the REDRAW bit is on.
  */
  if (!(lbw->flags & LBS_NOREDRAW))
  {
    _ListBoxSetScrollBars(lbw, lbi);
    ListboxInvalidateRect(lbi->hListBox);
  }

  return pos;  /* return the position where it was inserted */
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxDeleteString()                                           */
/*                                                                           */
/* Purpose : Deletes the string at index 'pos' from the listbox.             */
/*                                                                           */
/* Returns : TRUE if the string was freed, FALSE if not.                     */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL ListBoxDeleteString(lbi, pos)
  LISTBOX *lbi;
  int     pos;
{
  LIST    *pNth;
  WINDOW  *lbw;
  INT     iCurrSel;

  lbw = lbi->wListBox;

  /*
    Get a pointer to the proper list element
  */
  if ((pNth = ListGetNth(lbi->strList, pos)) == NULL)
    return FALSE;

#ifdef OWNERDRAWN
  /*
    Let the app do some app-defined behavior associated with freeing
    the string.
  */
  if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbw->flags))
    _ListBoxDeleteOwnerDrawnItem(lbw, pos, pNth);
#endif


  /*
    Free the element and decrease the count of strings in the listbox.
  */
  ListDelete(&lbi->strList, pNth);
  lbi->nStrings--;

  /*
    See if we deleted before the currently selected item, decrement the
    current selection.
    Don't decrement if it was the current selection.
    Also, see if we deleted the last item and that item was the selected one.
  */
  iCurrSel = lbi->iCurrSel;  /* get into local var for speed */
  if (pos < iCurrSel || (pos == iCurrSel && pos == lbi->nStrings && pos >= 0))
  {
    iCurrSel = (--lbi->iCurrSel);
    /*
      We may have just decremented the cursel off screen.
      Need to check to see that Topline is adjusted also.
    */
    if (lbi->iTopLine > iCurrSel && lbi->iTopLine > 0)
      lbi->iTopLine--;
  }

  /*
    If the listbox is in a refreshable state, then show it.
  */
  _ListBoxSetScrollBars(lbw, lbi);
  ListboxInvalidateRect(lbi->hListBox);

  return TRUE;
}

/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxFreeStrings()                                            */
/*                                                                           */
/*===========================================================================*/
VOID PASCAL ListBoxFreeStrings(lbi)
  LISTBOX *lbi;
{
#ifdef OWNERDRAWN
  /*
    If we have an owner-drawn listbox with user-defined items, then call
    the app's window procedure to take care of any app-specific behavior
    associated with freeing the item. For instance, the app may want
    to free any memory associated with the memory handle.
  */
  if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbi->wListBox->flags))
  {
    LIST   *p;
    int    idxItem = 0;
    for (p = lbi->strList;  p;  p = p->next)
      _ListBoxDeleteOwnerDrawnItem(lbi->wListBox, idxItem++, p);
  }
#endif

  ListFree(&lbi->strList, TRUE);
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxGetString()                                              */
/*                                                                           */
/* Purpose : Retrieves a pointer to the 'index'th string in the listbox.     */
/*                                                                           */
/* Returns : The string, or NULL if index is bad.                            */
/*                                                                           */
/*===========================================================================*/
static LPSTR PASCAL ListBoxGetString(lbi, index)
  LISTBOX  *lbi;
  int      index;
{
  LIST *pNth;

#ifdef OWNERDRAWN
  if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbi->wListBox->flags))
    return NULL;
#endif

  if ((pNth = ListGetNth(lbi->strList, index)) == NULL)
    return (LPSTR) NULL;
  return (LPSTR) pNth->data + 1 + SZITEMDATA;
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxChar()                                                   */
/*                                                                           */
/* Purpose : Handles keystrokes in listboxes                                 */
/*                                                                           */
/* Returns : TRUE if the current selection has changed, FALSE if not.        */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL ListBoxChar(lbw, key)
  WINDOW *lbw;
  int     key;
{
  LISTBOX *lbi       = (LISTBOX *) lbw->pPrivate;
  DWORD   dwFlags    = lbw->flags;
  INT     lbheight   = lbi->nVisibleStrings;
  INT     oldCurrSel = lbi->iCurrSel;
  INT     oldTopLine = lbi->iTopLine;
  INT     nStrings   = lbi->nStrings;
  INT     nColsToScroll = 1;
  INT     wExtent    = lbi->wHorizExtent;
  BOOL    bChanged   = FALSE;
  BOOL    bExtSel    = (BOOL)((dwFlags & LBS_EXTENDEDSEL) != 0);
  BOOL    bMultiSel  = (BOOL)((dwFlags & LBS_MULTIPLESEL) != 0);
  BOOL    bMultiCol  = (BOOL)((dwFlags & LBS_MULTICOLUMN) != 0);
  HWND    hLB        = lbi->hListBox;
  BOOL    bIsExtKey  = FALSE;


#ifdef USE_EXTSEL
  switch (key)
  {
#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_UP       :
    case VK_DOWN     :
    case VK_LEFT     :
    case VK_RIGHT    :
    case VK_PRIOR    :
    case VK_NEXT     :
    case VK_HOME     :
    case VK_END      :
      if (!GetKeyState(VK_SHIFT))
        break;
#else
    case VK_SH_UP    :
    case VK_SH_DOWN  :
    case VK_SH_LEFT  :
    case VK_SH_RIGHT :
    case VK_SH_PGUP  :
    case VK_SH_PGDN  :
    case VK_SH_HOME  :
    case VK_SH_END   :
#endif
      bIsExtKey = TRUE;
      /*
        If we haven't anchored a selection, then do so now.
      */
      if (xSelecting == -1)
        xSelecting = lbi->iCurrSel;
      break;

    default          :
      bIsExtKey = FALSE;
      /*
        If we had some items selected, and we press something like
        DOWN ARROW, then we first must deselect all items.
      */
      if (xSelecting != -1)
        SendMessage(hLB, LB_SELITEMRANGE, FALSE, MAKELONG(0, lbi->nStrings));
      xSelecting = -1;
      break;
  }
#endif


  switch (key)
  {
    case VK_UP        :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_UP     :
#endif
      if (oldCurrSel > 0)
      {
        lbi->iCurrSel--;
        bChanged = TRUE;
      }
      break;

    case VK_DOWN      :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_DOWN   :
#endif
      if (oldCurrSel + 1 < nStrings)
      {
        lbi->iCurrSel++;
#ifndef WAGNER
        if (!bMultiCol && lbi->iCurrSel == lbi->iTopLine + lbheight)
          lbi->iTopLine++;
#endif
        bChanged = TRUE;
      }
      break;

    case VK_LEFT       :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_LEFT    :
    case VK_SH_TAB     :
#endif
do_left:
      if (bMultiCol)
      {
        int i;
        int iSel = oldCurrSel;
        for (i = nColsToScroll;  i > 0;  i--)
        {
          if (iSel - lbheight >= 0)
          {
            iSel = (lbi->iCurrSel -= lbheight);
            bChanged = TRUE;
          }
        }
      }
      else if (wExtent)
      {
        return ListBoxScroll(lbw, VK_LEFT, WM_HSCROLL);
      }
      break;

    case VK_RIGHT     :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_RIGHT  :
    case VK_TAB       :
#endif
do_right:
      if (bMultiCol)
      {
        int i;
        int iSel = oldCurrSel;
        for (i = nColsToScroll;  i > 0;  i--)
        {
          if (iSel + lbheight < nStrings)
          {
            iSel = (lbi->iCurrSel += lbheight);
            bChanged = TRUE;
          }
        }
      }
      else if (wExtent)
      {
        return ListBoxScroll(lbw, VK_RIGHT, WM_HSCROLL);
      }
      break;

    case VK_PRIOR   :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_PGUP :
#endif
      if (bMultiCol)
      {
        nColsToScroll = 3;
        goto do_left;
      }
      if (oldCurrSel > 0)
      {
        lbi->iCurrSel = max(0, oldCurrSel - lbheight);
        bChanged = TRUE;
      }
      break;

    case VK_NEXT    :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_PGDN :
#endif
      if (bMultiCol)
      {
        nColsToScroll = 3;
        goto do_right;
      }
      lbi->iCurrSel = min(nStrings - 1, oldCurrSel + lbheight);
      bChanged = TRUE;
      break;

    case VK_HOME    :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_HOME :
#endif
      lbi->iCurrSel = 0;
      bChanged = TRUE;
      break;

    case VK_END     :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_SH_END  :
#endif
      lbi->iCurrSel = nStrings - 1;
      bChanged = TRUE;
      break;

    case VK_RETURN  :
      break;

    case ' '  :
      /*
        Make sure that we have a selected item
      */
      if (oldCurrSel == -1)
        lbi->iCurrSel = 0;

      /*
        If we hit the space bar in a multiple selection listbox, then
        we toggle the selection state of the current item.
      */
      if (bMultiSel)
      {
        int state = (int) SendMessage(hLB, LB_GETSEL, lbi->iCurrSel, 0L);
        bDontInvalidateOnSETSEL = TRUE;
        SendMessage(hLB, LB_SETSEL, !state, MAKELONG(lbi->iCurrSel, 0));
      }
      bChanged = TRUE;
      break;


#ifdef USE_EXTSEL
    case '/'         :   /* CTRL+/ selects everything   */
    case '\\' & 0x1F :   /* CTRL+\ deselects everything */
      if (bExtSel)
        SendMessage(hLB, LB_SELITEMRANGE, (key == '/'),
                    MAKELONG(0, lbi->nStrings));
      break;
#endif


    default   :
      /*
        Handle the "printable" keystrokes
        (Careful... isprint() returns TRUE on chars above 128)
      */
      if (key > 32 && key < 256 && isprint(key))  
      {                                            
        char prefix[2];
        int  iSel, iStartPos;

#ifdef OWNERDRAWN
        if (IS_OWNERDRAWN_WITHOUT_STRINGS(dwFlags))
          return FALSE;
#endif

        prefix[0] = (BYTE) key;  prefix[1] = '\0';

        /*
          Start searching at the next item...
        */
        iStartPos = lbi->iCurrSel + 1;
        if (dwFlags & LBS_SORT)
        {
          /*
            If we have a sorted listbox, and if the user pressed a key
            which is less than the first letter of the currently
            selected item, then start searching at the beginning of
            the list (courtesy of Chad Yost, West Publishing).
          */
          LPSTR s = ListBoxGetString(lbi, max(lbi->iCurrSel, 0));
          if (s != NULL && lang_upper(key) < lang_upper((int) *s))
            iStartPos = 0;
        }

        /*
          Search for the first entry starting with the typed key. 
        */
        if ((iSel = ListBoxFindPrefix(lbi, prefix, iStartPos, 0)) != LB_ERR ||
            (iSel = ListBoxFindPrefix(lbi, prefix, 0, 0))         != LB_ERR)
        {
          ListBoxSelect(lbi, iSel, TRUE);
          bChanged = TRUE;
        }
      }
      break;
  }


#ifdef USE_EXTSEL
  if (bIsExtKey)
  {
    ListBoxSelectExtended(lbi, lbi->iCurrSel);
  }
#endif


  /*
    If we changed the current selection, refresh the listbox.
  */
  if (bChanged)
    _ListBoxMaybeRefresh(lbw, lbi, oldCurrSel, oldTopLine);

  return bChanged;
}


#ifdef WAGNER
/****************************************************************************/
/*                                                                          */
/* Function : ListBoxScroll()                                               */
/*                                                                          */
/* Purpose  : Scrolls the listbox in response to a key                      */
/*                                                                          */
/* Returns  : TRUE if the topline changed, FALSE if not.                    */
/*                                                                          */
/****************************************************************************/
static INT PASCAL ListBoxScroll(lbw, key, message)
  WINDOW *lbw;
  INT     key;      /* VK_xxx */
  INT     message;  /* WM_VSCROLL or WM_HSCROLL */
{
  LISTBOX *lbi       = (LISTBOX *) lbw->pPrivate;
  int     lbheight   = lbi->nVisibleStrings;
  int     oldTopLine = lbi->iTopLine;
  int     nStrings   = lbi->nStrings;
  BOOL    bChanged   = FALSE;
  BOOL    bMultiCol  = (BOOL) ((lbw->flags & LBS_MULTICOLUMN) != 0x0000);
  int     iNewTopLine = oldTopLine;  /* local var for speed */

  INT     wOldLeft   = lbi->iLeftCol;
  INT     wExtent    = lbi->wHorizExtent;
  INT     wWidth     = RECT_WIDTH(lbw->rClient);

  /*
    Cannot scroll a multicolumn listbox...
  */
  if (bMultiCol)
    return 0;

  switch (key)
  {
    case VK_UP        :
      if (oldTopLine > 0)
      {
        iNewTopLine--;          /* scroll up one line */
        bChanged = TRUE;
      }
      break;

    case VK_DOWN :
      if (oldTopLine < nStrings - lbheight)
      {
        iNewTopLine++;          /* scroll down one line */
        bChanged = TRUE;
      }
      break;

    case VK_LEFT :
      if (wExtent)
        lbi->iLeftCol -= SysGDIInfo.tmAveCharWidth;
      break;

    case VK_RIGHT :
      if (wExtent)
        lbi->iLeftCol += SysGDIInfo.tmAveCharWidth;
      break;

    case VK_PRIOR :
      if (message == WM_VSCROLL)
      {
        if (oldTopLine > 0)       /* scroll up by one window */
        {
          iNewTopLine = max(0, oldTopLine - lbheight);
          bChanged = TRUE;
        }
      }
      else
      {
        if (wExtent)
          lbi->iLeftCol -= wWidth;
      }
      break;

    case VK_NEXT :
      if (message == WM_VSCROLL)
      {
        if (oldTopLine < nStrings - lbheight)
        {
          iNewTopLine = min(nStrings - lbheight, oldTopLine + lbheight);
          bChanged = TRUE;
        }
      }
      else
      {
        if (wExtent)
          lbi->iLeftCol += wWidth;
      }
      break;

    case VK_HOME :
      if (oldTopLine)
      {
        iNewTopLine = 0;
        lbi->iCurrSel = 0;
        bChanged = TRUE;
      }
      break;

    case VK_END  :
      if (oldTopLine < nStrings - lbheight)
      {
        iNewTopLine = nStrings - lbheight;
        bChanged = TRUE;
      }
      break;

    default   :
      break;
  }

  /*
    Make sure that the left col is within range
  */
  if (wExtent && wOldLeft != lbi->iLeftCol)
  {
    if (lbi->iLeftCol > wExtent-wWidth)
      lbi->iLeftCol = wExtent-wWidth;
    else if (lbi->iLeftCol < 0)
      lbi->iLeftCol = 0;
    if (wOldLeft != lbi->iLeftCol)  /* see if it's changed after adjusting */
      bChanged = TRUE;
  }

  if (bChanged)
  {
    lbi->iTopLine = iNewTopLine;
    ListboxInvalidateRect(lbw->win_id);
  }

  return bChanged;
}
#endif


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxClearSelections()                                        */
/*                                                                           */
/*===========================================================================*/
VOID PASCAL ListBoxClearSelections(lbi)
  LISTBOX *lbi;
{
  DWORD flFlags = lbi->wListBox->flags;

  lbi->iTopLine = 0;
  if (flFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
  {
    lbi->iCurrSel = 0;
    SendMessage(lbi->hListBox, LB_SETSEL, FALSE, MAKELONG(-1, 0));
  }
  else
    lbi->iCurrSel = -1;
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxFindPrefix()                                             */
/*                                                                           */
/* Purpose : Starting from a certain item, finds the next listbox string     */
/*           which matches the prefix.                                       */
/*                                                                           */
/* Returns : The 0-based index of the matched string, LB_ERR if no match     */
/*                                                                           */
/*===========================================================================*/
INT PASCAL ListBoxFindPrefix(
  LISTBOX  *lbi,      /* listbox structure */
  LPSTR    prefix,    /* string containing the prefix */
  int      start,     /* position to start searching  */
  BOOL     bExact)    /* exact match? */
{
  LIST *p;
  int  index;
  LPSTR str, psz;

  /*
    At the end?
  */
  if (start >= lbi->nStrings || !prefix)
    return LB_ERR;

  /*
    Go through all of the strings starting at position 'start'
  */
  for (p = ListGetNth(lbi->strList, index = start);  p;  p = p->next, index++)
  {
#ifdef OWNERDRAWN
    /*
      If we are dealing with an owner-drawn item, then try matching the
      user-defined field.
    */
    if (IS_OWNERDRAWN_WITHOUT_STRINGS(lbi->wListBox->flags))
    {
      DWORD ul = LB_GET_ITEMDATA(p->data);
      if (ul == (DWORD) prefix)
        return index;
      continue;
    }
#endif

    /*
      Try a case-insensitive string comparison
    */
    str = (LPSTR) p->data + SZITEMDATA + 1;
    psz = prefix;
    while (*psz && *str && (lang_upper(*psz) == lang_upper(*str)))
      psz++, str++;

    if (*psz == '\0')    /* a match! */
    {
      if (bExact) /* match the word exactly? */
      {
        if (*str == '\0')
          return index;
      }
      else  /* we did a prefix match, not an exact match */
        return index;
    }
  }

  return LB_ERR;
}


/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxSelect()                                                 */
/*                                                                           */
/* Purpose : Sets or removes the current selection from a listbox.           */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL ListBoxSelect(lbi, index, bHilite)
  LISTBOX  *lbi;
  int      index;
  int      bHilite;
{
  DWORD flFlags = lbi->wListBox->flags;

  /*
    Sending LB_SETCURSEL to -1 will remove a selection from a listbox
  */
  if (index == -1)
  {
    lbi->iCurrSel = (flFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) ? 0 : -1;
    return TRUE;
  }

  /*
    Make sure that the index is valid
  */
  if (index < 0 || index >= lbi->nStrings)
    return LB_ERR;

  /*
    Set the current selection
  */
  if (bHilite)
    lbi->iCurrSel = index;

  /*
    For multiple selection listboxes, send the LB_SETSEL message and
    let the winproc do the hard work.
  */
  if (flFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
  {
    if (bHilite == -1)        /* toggle the state */
      bHilite = !SendMessage(lbi->hListBox, LB_GETSEL, index, 0L);
    if (bHilite != -2)       /* -2 from LB_SETCARETINDEX */
    {
      bDontInvalidateOnSETSEL = TRUE;
      SendMessage(lbi->hListBox, LB_SETSEL, bHilite, MAKELONG(index, 0));
    }
  }
  return index;
}


/****************************************************************************/
/*                                                                          */
/* Function : _ListBoxGetSelections()                                       */
/*                                                                          */
/* Purpose  : Processing for LB_GETSELCOUNT and LB_GETSELITEMS messages.    */
/*                                                                          */
/* Returns  : # of items selected or placed in buffer                       */
/*                                                                          */
/****************************************************************************/
static INT PASCAL _ListBoxGetSelections(lbi, nMax, pIndexBuffer)
  LISTBOX *lbi;
  int     nMax;
  LPINT   pIndexBuffer;
{
  LIST   *pItems;
  int    iCnt = 0;
  INT    iSel = 0;
  LPINT  origIndexBuffer = pIndexBuffer;

  /*
    Insure that we are dealing with a multiple selection listbox
  */
  if (!(lbi->wListBox->flags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)))
    return LB_ERR;

  for (pItems = lbi->strList;  pItems;  pItems = pItems->next, iSel++)
  {
    /*
      If an item is selected, then bump up the selection count and place
      the index of the item in the buffer.
    */
    if (* (LPSTR) pItems->data != 0)
    {
      iCnt++;
      if (pIndexBuffer && iCnt <= nMax)
        *pIndexBuffer++ = iSel;
    }
  }

  /*
    Return the number of items placed in the buffer, or the number of items
    selected.
  */
  if (pIndexBuffer)
    return pIndexBuffer - origIndexBuffer;
  else
    return iCnt;
}


/****************************************************************************/
/*                                                                          */
/* Function : Owner-draw listbox processing.                                */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/****************************************************************************/
#ifdef OWNERDRAWN

HDC FAR PASCAL _ListBoxSetupDIS(lbw, pdis, hDC, lphDC)
  DRAWITEMSTRUCT *pdis;
  WINDOW         *lbw;
  HDC            hDC;
  LPHDC          *lphDC;
{
  BOOL bIsCB = (BOOL) ((lbw->ulStyle & LBS_IN_COMBOBOX) != 0L || 
                       (lbw->idClass != LISTBOX_CLASS));

  pdis->CtlType  = (bIsCB) ? ODT_COMBOBOX : ODT_LISTBOX;
  pdis->CtlID    = lbw->idCtrl;
  pdis->hwndItem = (bIsCB) ? GetParent(lbw->win_id) : lbw->win_id;
  pdis->itemID   = pdis->itemAction = 0;
  pdis->hDC      = hDC;
  *lphDC         = _GetDC(hDC);
  return hDC;
}


/****************************************************************************/
/*                                                                          */
/* Function : _ListBoxDrawOwnerDrawnItem()                                  */
/*                                                                          */
/* Purpose  : Sends the WM_DRAWITEM message to the dialog box               */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/* Compatibility Note :                                                     */
/*                                                                          */
/*    Windows sends the following sequence of WM_DRAWITEM messages when     */
/*    you click on an item in a listbox                                     */
/*                                                                          */
/*  1) WM_DRAWITEM  (index = old focus item)                                */
/*     ODS_SELECTED  ODA_FOCUS                                              */
/*                                                                          */
/*  2) WM_DRAWITEM  (index = old focus item)                                */
/*                   ODA_SELECT                                             */
/*                                                                          */
/*  3) WM_DRAWITEM  (index = new focus item)                                */
/*     ODS_SELECTED  ODA_SELECT                                             */
/*                                                                          */
/*  4) WM_DRAWITEM  (index = new focus item)                                */
/*     ODS_SELECTED  ODA_FOCUS  ODS_FOCUS                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _ListBoxDrawOwnerDrawnItem(
  WINDOW *lbw,
  int   row, int col,
  COLOR attr,
  BOOL  bIsMultisel,
  BOOL  bSelected,
  BOOL  bHasFocus,
  LIST  *pItem,
  PDRAWITEMSTRUCT lpdis,
  INT   idxItem,
  BOOL  bDrawingAll,
  BOOL  bIsEdit)
{
  INT   cyHeight;
  INT   cyOffset = (bIsEdit) ? 0 : CY_LISTBOXOFFSET;
  LPHDC lphDC;

  /*
    5/26/94 (maa)
      It's easier if we just send ODA_DRAWENTIREITEM all of the time....
  */

  /*
    Set the color attribute
  */
  lphDC = _GetDC(lpdis->hDC);
  lphDC->attr = attr;

/*
    MS Windows does not change the attribute of the device context.
    Doing so makes a FillRect, TextOut, InvertRect sequence incorrectly
    draw the highlight.
*/
#if 0
  _GetDC(lpdis->hDC)->attr = attr;
#endif

  /*
    Set the drawing rectangle
  */
  cyHeight = ((LISTBOX *) lbw->pPrivate)->tmHeightAndSpace;
  SetRect(&lpdis->rcItem,
          col, 
          (row * cyHeight) + cyOffset,
          RECT_WIDTH(lbw->rClient), 
          ((row+1) * cyHeight) + cyOffset);


  /*
    Set the item-state flags
  */
  lpdis->itemState  = 0;
  lpdis->itemAction = ODA_DRAWENTIRE;
  if (bIsMultisel)
  {
    if (*(LPSTR) pItem->data)
    {
      lpdis->itemState = ODS_SELECTED;
      if (!bDrawingAll)
#if 0
        lpdis->itemAction  = ODA_SELECT;
#elif 0
        lpdis->itemAction |= ODA_SELECT;
#else
        ;
#endif
    }
  }
  else if (bSelected)
  {
    lpdis->itemState = ODS_SELECTED;
    if (!bDrawingAll)
#if 0
      lpdis->itemAction  = ODA_SELECT;
#elif 0
      lpdis->itemAction |= ODA_SELECT;
#else
        ;
#endif
  }

#if 1
  if (bHasFocus)
  {
    lpdis->itemState  |= ODS_FOCUS;
    if (!bDrawingAll)
      /* 
        A single sel listbox can't have both selection and focus
      */
#if 0
      if (bIsMultisel || !bSelected)
        lpdis->itemAction |= ODA_FOCUS;
#else
        ;
#endif
  }
#endif

  /*
    Point to the listbox item data
  */
  lpdis->itemData = LB_GET_ITEMDATA(pItem->data);
  lpdis->itemID   = idxItem;


  /*
    Finally, send the WM_DRAWITEM message to the listbox's owner
  */
  SendMessage(GetParent(lpdis->hwndItem), WM_DRAWITEM,
              lpdis->CtlID, (DWORD)(LPSTR) lpdis);

  /*
    Send the ODA_FOCUS action.
    A single selection listbox can't have both the focus and selection.
  */
#if 0
  if (bHasFocus && !bDrawingAll && (bIsMultisel || !bSelected))
  {
    lpdis->itemAction = ODA_FOCUS;
    SendMessage(GetParent(lpdis->hwndItem), WM_DRAWITEM, 
                lpdis->CtlID, (DWORD)(LPSTR)lpdis);
  }
#endif
}


VOID FAR PASCAL _ListBoxDeleteOwnerDrawnItem(lbw, idxItem, pItem)
  WINDOW *lbw;
  int    idxItem;
  LIST   *pItem;
{
  DELETEITEMSTRUCT dis;

  BOOL bIsCB = (BOOL) ((lbw->ulStyle & LBS_IN_COMBOBOX) != 0L || 
                       (lbw->idClass != LISTBOX_CLASS));

  dis.CtlType  = (bIsCB) ? ODT_COMBOBOX : ODT_LISTBOX;
  dis.CtlID    = lbw->idCtrl;
  dis.itemID   = idxItem;
  dis.hwndItem = lbw->win_id;
  dis.itemData = LB_GET_ITEMDATA(pItem->data);
  SendMessage(GetParent(lbw->win_id), WM_DELETEITEM, 0,
                                      (DWORD) (LPDELETEITEMSTRUCT) &dis);
}

#endif



/*===========================================================================*/
/*                                                                           */
/* Function: ListBoxRefresh()                                                */
/*                                                                           */
/* Purpose : This function is in charge of drawing the client area of a      */
/*           listbox.                                                        */
/*                                                                           */
/* Called by: The only place where this is called from is the WM_PAINT       */
/*            case in ListboxWinProc.                                        */
/*                                                                           */
/*===========================================================================*/
#if defined(MEWEL_GUI) || defined(MOTIF)
static VOID PASCAL ListBoxDrawItem(HDC, LISTBOX *, INT, INT, INT, INT, LPSTR,
                                   BOOL, BOOL);
#endif


VOID PASCAL ListBoxRefresh(lbi, hDC, lprcUpdate)
  LISTBOX  *lbi;
  HDC      hDC;
  LPRECT   lprcUpdate;
{
  char     buf[MAXBUFSIZE];
#ifdef MEWEL_TEXT
  char     szBlank[MAXBUFSIZE];
  COLOR    attr;
#endif
  COLOR    a;
  HWND     hLB;
  LIST     *p;
  WINDOW   *lbw;
  INT      nVisStrings;
  INT      row, col;
  INT      iHeight, iWidth, iLeftCol;
  BOOL     bIsExtsel;
  BOOL     bIsMultisel;
  BOOL     bIsMulticol;
  DWORD    dwFlags;
  INT      idxItem;
  DRAWITEMSTRUCT dis;
  LPHDC    lphDC;
#if defined(MEWEL_GUI) || defined(MOTIF)
  HFONT    hFont, hOldFont;
  INT      cySpacing;
  INT      cyDiff = 0;
#endif


  /*
    Get the window handle and a pointer to the window structure
  */
  hLB = lbi->hListBox;
  lbw = lbi->wListBox;

  /*
    Don't do anything if drawing is disabled or if the listbox is hidden.
  */
  dwFlags = lbw->flags;
#ifndef ZAPP
  /* zapp - commented out because it was not redrawing properly */
  if ((dwFlags & LBS_NOREDRAW) || !IsWindowVisible(hLB))
    return;
#endif

  /*
    See if the listbox is a multiple-selection or multicolumn listbox
  */
  bIsExtsel   = (BOOL) ((dwFlags & LBS_EXTENDEDSEL) != 0L);
  bIsMultisel = (BOOL) ((dwFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0L);
  bIsMulticol = (BOOL) ((dwFlags & LBS_MULTICOLUMN) != 0L);

  /*
    Refresh the scrollbars. Do it before we calculate the drawing area
    in case the appearence or disappearence of a scrollbar affects the
    size of the drawing area.
    (4/12/93) If the scrollbar vanished or appeared, redraw the
    non-client area.
  */
  _ListBoxSetScrollBars(lbw, lbi);
  if (lbw->ulStyle & WIN_UPDATE_NCAREA)
  {
    SendMessage(hLB, WM_NCPAINT, 0, 0L);
    lbw->ulStyle &= ~WIN_UPDATE_NCAREA;
  }



#ifdef MEWEL_TEXT
  /*
    Get the color, height, width and starting column
  */
  if ((attr = lbw->attr) == SYSTEM_COLOR)
    attr = WinGetClassBrush(hLB);
#endif

#if defined(MEWEL_GUI) || defined(MOTIF)
  /*
    See if the top of the clipping rect falls right in the middle of a 
    line of text. If so, then we need to move the clipping rect up a little
    so the entire line of text will be written properly. Otherwise,
    the graphics engines will not write a partial line of text.
  */
  /*
    Get the font to draw with
  */
  if ((hFont = (HFONT) SendMessage(hLB, WM_GETFONT, 0, 0L)) != NULL)
    hOldFont = SelectObject(hDC, hFont);
  lphDC = _GetDC(hDC);
#if defined(META) || defined(GX)
  cyDiff = (lprcUpdate->top - CY_LISTBOXOFFSET) % lbi->tmHeightAndSpace;
  if (cyDiff)
  {
    lprcUpdate->top -= cyDiff;
    lphDC->rClipping.top -= cyDiff;
  }
  cyDiff = (lprcUpdate->bottom - CY_LISTBOXOFFSET) % lbi->tmHeightAndSpace;
  if (cyDiff)
  {
    lprcUpdate->bottom += cyDiff;
    lphDC->rClipping.bottom += cyDiff;
  }
#endif


  if (bIsMulticol)
  {
    GetClientRect(hLB, lprcUpdate);
    lprcUpdate->top += CY_LISTBOXOFFSET;
    cyDiff = 0;
  }

  row      = (lprcUpdate->top    - CY_LISTBOXOFFSET) / lbi->tmHeightAndSpace;
  /* iHeight is the row at which we should stop updating */
  iHeight  = (lprcUpdate->bottom - CY_LISTBOXOFFSET) / lbi->tmHeightAndSpace;

  if (cyDiff)
    iHeight++;

  /*
    We want to center the text within the total height. cySpacing has
    the extra vertical spacing which should be applied.
  */
  if ((cySpacing = (lbi->tmHeightAndSpace - lbi->tmHeight) >> 1) < 0)
    cySpacing = 0;

#else
  (void) lprcUpdate;
  row      = 0;
  iHeight  = lbi->nVisibleStrings;
#endif

  col      = 0;
  nVisStrings = lbi->nVisibleStrings;
  iWidth   = bIsMulticol ? WIN_CLIENT_WIDTH(lbw) : 1;
  iLeftCol = lbi->iLeftCol;

#ifdef MEWEL_TEXT
  /*
    Possibly start string to the left of the listbox if the left column
    is > 0.
  */
  if (lbi->wHorizExtent)
    col = -iLeftCol;
#endif

  /*
    For a multicolumn listbox, it's easy to erase the whole client
    area. Don't use WinClear() because we want to use this DC.
  */
#ifdef MEWEL_TEXT
  SendMessage(hLB, WM_ERASEBKGND, (WPARAM) hDC, 0L);
#endif

  /*
    For an owner-drawn listbox, set up the DC and other stuff
  */
#ifdef OWNERDRAWN
  if (IS_OWNERDRAWN(dwFlags))
    hDC = _ListBoxSetupDIS(lbw, &dis, hDC, &lphDC);
#endif

#if defined(MEWEL_GUI) || defined(MOTIF)
#if 11195
  if (!IS_OWNERDRAWN(dwFlags))
#endif
  SetBkMode(hDC, TRANSPARENT);
#endif


  /*
    For a multicolumn listbox, we need to highlight the entire column,
    not just the text in the column.
  */
#ifdef MEWEL_TEXT
  if (bIsMulticol)
  {
    memset(szBlank, ' ', lbi->iColWidth);
    szBlank[lbi->iColWidth] = '\0';
  }
#endif

  if (lbi->nStrings > 0)
  {
#if defined(MEWEL_GUI) || defined(MOTIF)
    int  cxWidth = (bIsMulticol) ? lbi->iColWidth : WIN_CLIENT_WIDTH(lbw);
#endif

    /*
      Go through the entries starting at the first one in the window
    */
    idxItem = (bIsMulticol) ? iLeftCol*nVisStrings : lbi->iTopLine + row;
    for (p = ListGetNth(lbi->strList, idxItem);  p;  p = p->next, idxItem++)
    {
      BOOL bHasFocus;
      BOOL bSelected;
#ifdef MEWEL_TEXT
      BOOL bMultiSelWithFocus;
#endif

      if (bIsMultisel)
      {
        /*
          For a multisel listbox, highlite if the item is selected or if it
          has the focus.
        */
        bSelected = (BOOL) ((*(LPSTR) p->data != '\0') ||
                            (bIsExtsel && lbi->iCurrSel == idxItem));
      }
      else
      {
        /*
          For a single-sel listbox, highlite if the item is the current one
        */
        bSelected = (lbi->iCurrSel == idxItem);
      }

      bHasFocus = (bSelected && InternalSysParams.hWndFocus == hLB);


#ifdef MEWEL_TEXT
      /*
        Determine the color attribute to write the string with. Default
        to the normal color, and if it's selected, reverse the attributes.
      */
      a = attr;
      if (bSelected)
      {
        if (!IS_OWNERDRAWN(dwFlags))
          a = MAKE_HIGHLITE(attr);
        HIGHLITE_ON();
      }

      /*
        Set a flag if we are dealing with the current focus item within
        a multiple selection listbox. For this item, we will draw arrows
        on both sides of the string.
      */
      bMultiSelWithFocus = (BOOL) (bIsMultisel &&
                                   lbi->iCurrSel == idxItem &&
                                   InternalSysParams.hWndFocus == hLB);
#endif


      /*
        If we have owner-drawn items, let the app take care of the
        drawing.
      */
      if (IS_OWNERDRAWN(dwFlags))
      {
        _ListBoxDrawOwnerDrawnItem(lbw, row, col, a, bIsMultisel,
                       bSelected, bHasFocus, p, &dis, idxItem, TRUE, FALSE);
        goto advance;
      }


      /*
        Copy the listbox item into the output buffer. For a multiple
        selection entry which has the focus, copy the little arrow into
        the first column.
      */
#if defined(MEWEL_GUI) || defined(MOTIF)
      lstrncpy((LPSTR) buf+0, (LPSTR) p->data + 1 + SZITEMDATA,
                                        sizeof(buf)-0);
#else
      if (bIsMultisel)
        buf[0] = (BYTE) 
         ((bMultiSelWithFocus) ? WinGetSysChar(SYSCHAR_MULTISELFOCUS) : ' ');
      lstrncpy((LPSTR) buf+bIsMultisel, (LPSTR) p->data + 1 + SZITEMDATA,
                                        sizeof(buf)-1);
#endif


      /*
        Output the item.
      */
#if defined(MEWEL_GUI) || defined(MOTIF)
      ListBoxDrawItem(hDC,lbi, row,col,cxWidth,cySpacing, buf, bSelected,bHasFocus);
#else
      /*
        For a non-multicolumn listbox, erase the line. If this line has
        the selected item, then erase it in reverse video.
      */
      if (bIsMulticol)
        WinPuts(hLB, row, col, szBlank, a);
      else
        WinEraseEOL(hLB, row, 0, a);
      WinPuts(hLB, row, col, buf, a);
#ifdef COMPUSERVE_MEWEL
      if (bIsMultisel)
        WinPutc(hLB, row, WIN_CLIENT_WIDTH(lbw) -1,
         bMultiSelWithFocus ? WinGetSysChar(SYSCHAR_MULTISELFOCUS_RIGHT) : ' ',
                a);
#endif
#endif

      /*
        Move the the next row and through the columns
      */
  advance:
      row++;
      if (row >= iHeight || row >= lbi->nVisibleStrings)
      {
        if (!bIsMulticol || (col += lbi->iColWidth) >= iWidth)
        {
          HIGHLITE_OFF();
          break;
        }
        else
        {
          /*
            Move to the next column of a multicolumn listbox
          */
#if defined(MEWEL_GUI) || defined(MOTIF)
          row = (lprcUpdate->top + CY_LISTBOXOFFSET) / lbi->tmHeightAndSpace;
#else
          row = 0;
#endif
          iLeftCol++;
        }
      }

      HIGHLITE_OFF();
    }
  }

#ifdef MEWEL_TEXT
  /*
    Erase any unfilled listbox rows to the specified background color.
  */
  do
  {
    for (  ;  row <= iHeight;  row++)
    {
      if (!bIsMulticol)
        WinEraseEOL(hLB, row, 0, attr);
      else
        WinEraseEOL(hLB, row, col, attr);
    }

    /*
      Move to the next column of a multicolumn listbox
    */
    row = 0;
    col += lbi->iColWidth;
  } while (bIsMulticol && (col < iWidth));
#endif

#if defined(MEWEL_GUI) || defined(MOTIF)
  if (hFont)
    SelectObject(hDC, hOldFont);
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : _ListBoxMaybeRefresh()                                        */
/*                                                                          */
/* Purpose  : Performs fast updating of a listbox. This is used when the    */
/*            selection is changed from one item to another within the      */
/*            same visible window. This function removed the highlight from */
/*            the old item and highlights the new item.                     */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL _ListBoxMaybeRefresh(lbw, lbi, oldCurrSel, oldTopLine)
  WINDOW  *lbw;
  LISTBOX *lbi;
  int     oldCurrSel;
  int     oldTopLine;
{
  int     lbheight     = lbi->nVisibleStrings;
  int     lbwidth      = WIN_CLIENT_WIDTH(lbw);
  int     iOldDistance = max(oldCurrSel - oldTopLine, 0);
  int     iCurrSel     = lbi->iCurrSel;
  int     iOldLeftCol  = lbi->iLeftCol;
  BOOL    bExtSel;
  BOOL    bMultiSel;
  BOOL    bMultiCol;
  BOOL    bSelected;
  BOOL    bHighlight;
  HWND    hLB          = lbi->hListBox;
#if defined(MEWEL_TEXT)
  char    szBlank[MAXBUFSIZE];
#endif
  char    szNearBuf[MAXBUFSIZE];  /* for medium model!!! ugh! */
  BOOL    bVisible;
  DWORD   dwFlags = lbw->flags;

#ifdef OWNERDRAWN
  DRAWITEMSTRUCT dis;
  HDC   hDC = 0;
  LPHDC lphDC;
  HFONT hFont = 0, hOldFont = 0;
#endif

#if defined(MEWEL_GUI) || defined(MOTIF)
  int   cxWidth, cySpacing;
#endif


#if defined(MOTIF)
  /*
    In MOTIF, this routine should only draw for an owner-drawn combo
  */
  if (!IS_OWNERDRAWN(lbw->flags))
    return;
#endif


  bExtSel   = (BOOL) ((dwFlags & LBS_EXTENDEDSEL) != 0L);
  bMultiSel = (BOOL) ((dwFlags & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0L);
  bMultiCol = (BOOL) ((dwFlags & LBS_MULTICOLUMN) != 0L);
#if defined(MEWEL_GUI) || defined(MOTIF)
  cxWidth = (bMultiCol) ? lbi->iColWidth : lbwidth;
#endif


  /*
    Avoid a posible divide-by-zero error
  */
  if (lbheight == 0)
    return;

#if defined(MEWEL_GUI) || defined(MOTIF)
  /*
    We want to center the text within the total height. cySpacing has
    the extra vertical spacing which should be applied.
  */
  if ((cySpacing = (lbi->tmHeightAndSpace - lbi->tmHeight) >> 1) < 0)
    cySpacing = 0;
#endif


  bVisible = IsWindowVisible(lbw->win_id);

#ifdef WAGNER
  if (iOldDistance >= lbheight || oldCurrSel < 0)
    iOldDistance = lbheight - 1;
#endif

  /*
    See if the (possibly new) selected line is off the screen
  */
  if (!bMultiCol && iCurrSel != -1 &&
      (iCurrSel < lbi->iTopLine || iCurrSel >= lbi->iTopLine+lbheight))
  {
#ifdef WAGNER
    lbi->iTopLine = max(0, min(max(iCurrSel-iOldDistance, 0), lbi->nStrings-lbheight));
#else
    lbi->iTopLine = max(iCurrSel - iOldDistance, 0);
#endif
  }


  /*
    For a multicolumn listbox, we need to make sure that the column which
    has the currently selected item is visible on the screen. So, adjust
    the iLeftCol variable if it's currently off the screen.

    6/21/93 (maa)
      Added '+1' to the calculation. If the current column is cut off in
    the middle by the edge of the listbox, then scroll & refresh.
  */
  if (bMultiCol)
  {
    int thisCol = iCurrSel / lbheight;
    if ((thisCol - lbi->iLeftCol + 1) * lbi->iColWidth >= lbwidth ||
        (thisCol < lbi->iLeftCol && lbi->iLeftCol > 0))
      lbi->iLeftCol = thisCol;
  }


  /*
    Now that the fields in the lbi structure have been updated, we see
    if we should bother to reflect the changes on the screen.
  */
  if (!bVisible)
    return;

  /*
    If the listbox has a non-null update region, then this means that
    we are going to be refreshing the whole thing anyway. So, forget
    about the incremental redrawing which takes place right here.

    We also test to see if the update rect is equal to the entire
    listbox. Otherwise, we need to do the refreshing now.
  */
  if (EqualRect(&lbw->rUpdate, &lbw->rClient))
    return;

  /*
    Refresh only if we've changed the selection or topline
    (4/12/93) If the listbox does not have the focus, we should
    remove the focus indicator.
  */
  if (iCurrSel != oldCurrSel || (bMultiSel && lbi->nStrings) ||
      lbi->iTopLine != oldTopLine || bForceMaybeRefresh)
  {
    if (lbi->iTopLine != oldTopLine || lbi->iLeftCol != iOldLeftCol)
    {
      /*
        Cause a total repaint to be done
      */
      ListboxInvalidateRect(hLB);
      UpdateWindow(hLB);
      return;
    }

    {
    /*
      Fast processing to unhilite the old selection and hilite the new one
    */
    LIST  *p;
    int   row, col;
    COLOR attr;

    /*
      Get a DC for the entire listbox. Set the font to the custom
      listbox font, and set the writing mode to TRANBSPARENT.
      If we have an owner-drawn listbox, set it up for drawing.
    */
    hDC = GetDC(hLB);
    _PrepareWMCtlColor(hLB, CTLCOLOR_LISTBOX, hDC);
    if ((hFont = (HFONT) SendMessage(hLB, WM_GETFONT, 0, 0L)) != NULL)
      hOldFont = SelectObject(hDC, hFont);
#if 11195
    if (!IS_OWNERDRAWN(lbw->flags))
#endif
      SetBkMode(hDC, TRANSPARENT);
    if (IS_OWNERDRAWN(lbw->flags))
      _ListBoxSetupDIS(lbw, &dis, hDC, &lphDC);

    /*
      If we are setting a selection for the first time, make
      oldCurrSel a valid number.
    */
    if (oldCurrSel == -1)
      oldCurrSel = lbi->iTopLine;

#if defined(MEWEL_TEXT)
    /*
      We need a blank line for highlighting/unhighlighting the entries
    */
    if (bMultiCol)
    {
      memset(szBlank, ' ', lbi->iColWidth);
      szBlank[lbi->iColWidth] = '\0';
    }
#endif


    /*
      Draw two strings, the unlighted old selection (bHighlight == 0)
      and the highlighted new selection (bHighlight == 1).
    */
    for (bHighlight = 0;  bHighlight < 2;  bHighlight++)
    {
      /*
        Get a pointer to the listbox item to (un)highlight
      */
      INT iSel = (bHighlight) ? iCurrSel : oldCurrSel;

      if (bHighlight)
      {
        if (iSel < 0)
          break;

        /*
          Show the selection if the listbox (or the edit in a combo) 
          has the focus
        */
#if !110593
        /*
          11/5/93 (maa)
            This code seems to cause problems with apps which like to
            control listboxes which do not have the focus set
            (ie - Paper Clip, USA Info Systems)
        */
        if (InternalSysParams.hWndFocus != hLB && 
            (lbw->parent->idClass != COMBO_CLASS || 
             InternalSysParams.hWndFocus != lbw->sibling->win_id))
          break;
#endif
      }

      if ((p = ListGetNth(lbi->strList, iSel)) == NULL)
        break;

      /*
        In a multisel listbox, keep the old selection highlighted

        An item stays selected if we are losing focus on the listbox
        (iCurrSel == iOldSel)
      */
      bSelected = (BOOL) (bHighlight && (!bMultiSel || bExtSel) ||
                          *(LPSTR)p->data != '\0' ||
                         IS_OWNERDRAWN(dwFlags) && iCurrSel == oldCurrSel);


#if defined(MEWEL_TEXT)
      /*
        Get the BIOS color attribute
      */
      if ((attr = lbw->attr) == SYSTEM_COLOR)
        attr = WinGetClassBrush(lbw->win_id);
      if (bSelected)
      {
        if (!IS_OWNERDRAWN(lbw->flags))
          attr = MAKE_HIGHLITE(attr);
        HIGHLITE_ON();
      }
#endif

      if (bMultiCol)
      {
        /*
          Draw the new item in a multicolumn listbox if we are highlighting
          or if we have scrolled the columns.
        */
        if (bHighlight || iOldLeftCol == lbi->iLeftCol)
        {
          row = iSel % lbheight;
          col = (iSel / lbheight - lbi->iLeftCol) * lbi->iColWidth;
          if (bMultiSel)
            szNearBuf[0] = bHighlight?WinGetSysChar(SYSCHAR_MULTISELFOCUS):' ';
          lstrncpy(szNearBuf+bMultiSel,p->data+1+SZITEMDATA,sizeof(szNearBuf)-1);
#if defined(MEWEL_GUI) || defined(MOTIF)
          ListBoxDrawItem(hDC, lbi, row, col, cxWidth, cySpacing, 
                          szNearBuf, bSelected, FALSE);
#else
          WinPuts(hLB, row, col, szBlank, attr);
          WinPuts(hLB, row, col, szNearBuf, attr);
#endif
        }
      }
      else
      {
#if defined(MEWEL_GUI) || defined(MOTIF)
        col = 0;
#else
        col = -lbi->iLeftCol;
#endif

        /*
          Take care of an owner-drawn item
        */
        if (IS_OWNERDRAWN(dwFlags))
        {
          _ListBoxDrawOwnerDrawnItem(lbw, iSel-lbi->iTopLine, col,
                                     attr, bMultiSel, bSelected,
                                 bHighlight, p, &dis, iSel, FALSE, FALSE);
          continue;
        }

        /*
          Copy the listbox item's string into a formatting buffer
        */
#if defined(MEWEL_GUI) || defined(MOTIF)
        lstrncpy(szNearBuf, p->data+1 + SZITEMDATA, sizeof(szNearBuf)-1);
#else
        if (bMultiSel)
          szNearBuf[0] = bHighlight?WinGetSysChar(SYSCHAR_MULTISELFOCUS):' ';
        lstrncpy(szNearBuf+bMultiSel,p->data+1+SZITEMDATA,sizeof(szNearBuf)-1);
#endif

        /*
          Get the y coordinate to write at
        */
        row = iSel - lbi->iTopLine;

        /*
          Output the string
        */
#if defined(MEWEL_GUI) || defined(MOTIF)
        ListBoxDrawItem(hDC,lbi,row,0,cxWidth,cySpacing,szNearBuf,bSelected,bHighlight);

#else
        WinEraseEOL(hLB, row, 0, attr);

#if defined(COMPUSERVE_MEWEL)
        WinPuts(hLB, row, bMultiSel+col, szNearBuf, attr);
        if (bMultiSel)
          WinPutc(hLB, row, lbwidth-1,
            bHighlight?WinGetSysChar(SYSCHAR_MULTISELFOCUS_RIGHT):' ', attr);
#else
        WinPuts(hLB, row, col, szNearBuf, attr);
#endif
#endif
      }
    } /* for bHighlight */


    /*
      Restore the system font to the DC and delete the DC
    */
    if (hFont)
      SelectObject(hDC, hOldFont);
    if (hDC)
      ReleaseDC(hLB, hDC);

    HIGHLITE_OFF();
    /*
      Refresh the scroll bars
    */
    _ListBoxSetScrollBars(lbw, lbi);
    }
  }
}


#if defined(MEWEL_GUI) || defined(MOTIF)
static VOID PASCAL ListBoxDrawItem(
  HDC  hDC,
  LISTBOX *lbi,
  INT  row, INT col,
  INT  cxWidth,
  INT  cySpacing,
  LPSTR lpBuf,
  BOOL bSelected,
  BOOL bHasFocus)
{
  RECT r;
  INT  y;
  COLORREF clrText;

  y = (row * lbi->tmHeightAndSpace) + CY_LISTBOXOFFSET;
  SetRect((LPRECT) &r, col, y, col + cxWidth, y + lbi->tmHeightAndSpace);

  /*
    The bg an fg colors might have been set with a WM_CTLCOLOR. So, only
    use different fg and bg colors if we are highlighting.
  */
  if (bSelected)
  {
    /*
      Save the old text color, cause we need to restore it later
    */
    clrText = GetTextColor(hDC);
    SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
  }

  if (bSelected)
  {
    FillRect(hDC, &r, SysBrush[COLOR_HIGHLIGHT]);
  }
  else
  {
    FillRect(hDC, &r, _GetDC(hDC)->hBrush);
  }

  /*
    If we have a horizontally scrolling extent set up on the listbox, then
    start at the character which is at the left column. A good way to set
    up the clipping would be to start the string to the left of the listbox.
  */
  if (lbi->wHorizExtent)
    col -= lbi->iLeftCol;

  /*
    Move the origin over to the right a little so that it looks better
  */
  col += (SysGDIInfo.tmAveCharWidth >> 1);
  y   += cySpacing;

  /*
    Output the listbox string
  */
  if (lbi->nTabStops)
    TabbedTextOut(hDC, col, y, lpBuf, lstrlen(lpBuf),
                  lbi->nTabStops, lbi->pTabStops, 0);
  else
    TextOut(hDC, col, y, lpBuf, lstrlen(lpBuf));

  /*
    Restore the old text color
  */
  if (bSelected)
    SetTextColor(hDC, clrText);

  /*
    Draw the focus indicator

    Do not draw the focus rect if the edit portion of a combo has the focus.
  */
  if (bHasFocus && InternalSysParams.hWndFocus == lbi->wListBox->win_id)
    DrawFocusRect(hDC, (LPRECT) &r);
}
#endif



static VOID PASCAL ListBoxSelectExtended(lbi, iNewSel)
  LISTBOX *lbi;
  INT     iNewSel;
{
  LIST *p;
  int  n;

  /* 
    Need to check all, since MEWEL seems to miss mouse interrupts
    when mouse is moved quickly.
  */
  for (n = 0, p = lbi->strList;  p;  p = p->next, n++)
  {
    BOOL bRefresh = FALSE;

    if ((* (LPSTR) p->data))  /* The item is selected */
    {
      /* If not between current selection and original, clear it */
      if ((iNewSel <= xSelecting && n < iNewSel) || 
          (iNewSel >= xSelecting && n > iNewSel))
        bRefresh = TRUE;
    }
    else   /* not selected */
    {
      /* If between current selection and original, set it */
      if ((n >= iNewSel && n <= xSelecting) || 
          (n <= iNewSel && n >= xSelecting))
        bRefresh = TRUE + TRUE;
    }

    if (bRefresh)
    {
      ListBoxSelect(lbi, n, bRefresh - TRUE);
      _ListBoxMaybeRefresh(lbi->wListBox, lbi, n, lbi->iTopLine);
    }
  } /* end for */

  /*
    Select the current item
  */
  ListBoxSelect(lbi, iNewSel, TRUE);
}


/*
  These functions are used for architectures like SPARC and AIX, where
  we cannot access a double word unless it's on a DWORD boundary.
*/
#if defined(WORD_ALIGNED)
static VOID  PASCAL LB_SET_ITEMDATA(pData, dw)
  LPSTR pData;
  DWORD dw;
{
  memcpy(pData+1, &dw, sizeof(DWORD));
}

static DWORD PASCAL LB_GET_ITEMDATA(pData)
  LPSTR pData;
{
  DWORD dw;
  memcpy(&dw, pData+1, sizeof(DWORD));
  return dw;
}
#endif


/*
  CheckExtendedSelections()
    Given an extended selection listbox, 
      1) Sees if anything in the listbox is selected
      2) Sees if any selected item is currently visible
*/
static BOOL PASCAL CheckExtendedSelections(
  WINDOW  *lbw,
  LISTBOX *lbi,
  BOOL    *pbSelVisible,
  BOOL    bState,
  LPRECT  lpRect)
{
  LIST *p;
  int  i, y;
  int  cxWidth;
  BOOL bSelected = FALSE;
  RECT rectItem;

  if (pbSelVisible)
    *pbSelVisible = FALSE;

  SetRect(lpRect, 0, 0, 0, 0);

  y = CY_LISTBOXOFFSET;
  cxWidth = WIN_CLIENT_WIDTH(lbw);

  for (i = 0, p = lbi->strList;  p;  p = p->next, i++)
  {
    /*
      If something selected, set the return value to TRUE.
    */
    if (* (LPSTR) p->data != bState)
    {
      bSelected = TRUE;
      /*
        See if the selected item is visible.
      */
      if (i >= lbi->iTopLine && i < lbi->iTopLine + lbi->nVisibleStrings)
      {
        if (pbSelVisible)
          *pbSelVisible = TRUE;
        SetRect(&rectItem, 0, y, cxWidth, y + lbi->tmHeightAndSpace);
        UnionRect(lpRect, lpRect, &rectItem);
        break;
      }
    }
    y += lbi->tmHeightAndSpace;
  }

  return bSelected;
}

