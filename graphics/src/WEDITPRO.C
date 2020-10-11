/*===========================================================================*/
/*                                                                           */
/* File    : WEDITPRO.C                                                      */
/*                                                                           */
/* Purpose : Window proc for edit controls                                   */
/*                                                                           */
/* History :                                                                 */
/* $Log:   E:/vcs/mewel/weditpro.c_v  $                                      */
/*                                                                           */
/*     Rev 1.9   30 Nov 1993 17:02:06   Adler                                */
/*  Changed RectContainsPoint to PtInRect.                                   */
/* Removed the DLGC_WANTTABS style in the return value to WM_GETDLGCODE. This*/
/*  means that multi-line edit controls will not accept tabs as the default  */
/*  behavior.                                                                */
/*  Moved VK_BACK processing from the WM_KEYDOWN case to the WM_CHAR case.   */
/*  EM_REPLACESEL will now work even if the edit control is read-only.       */
/*                                                                           */
/*     Rev 1.8   25 Oct 1993 13:48:36   Adler                                */
/*  Added new arg for PrepareWMCtlColor()                                    */
/*                                                                           */
/*     Rev 1.7   24 Sep 1993 15:11:10   Adler                                */
/*  Added EM_SETREADONLY message.                                            */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NO_STD_INCLUDES

#if !defined(aix)
#include <ctype.h>
#endif
#include "winedit.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif


static INT  PASCAL _Edit_WMGETTEXT(BUFFER *, LPSTR, UINT);
static VOID PASCAL _Edit_WMSETTEXT(BUFFER *, LPSTR);
static INT  PASCAL _Edit_WMGETTEXTLEN(BUFFER *);

#if defined(USE_NATIVE_GUI)
#define INVALIDATERECT(hWnd, r, bErase)
#else
#define INVALIDATERECT(hWnd, r, bErase)  InvalidateRect(hWnd, r, bErase)
#endif


LRESULT FAR PASCAL EditWinProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{
  int    rc = TRUE;
  MWCOORD  row, col;
  int    oldCurrline;
  RECT   rClient;
  BUFFER *b;
  WINDOW *wEdit;
  WINDOW *wParent;
  DWORD  dwOldModify;
  DWORD  dwFlags;
  EPOS   idxBOL, iLine;
  LPSTR  szLine, szNL;
  BOOL   bIsReadonly;
  HDC    hDC = 0;
  BOOL   bSetCursor = FALSE;

  static BOOL bSelecting = FALSE;


  /*
    Get a pointer to the buffer structure
  */
  if ((b = EditHwndToBuffer(hEdit)) == NULL)
  {
    if (msg == WM_NCCALCSIZE)  /* sent before buffer is created ... */
    {
      WinSetClientRect(hEdit, (LPRECT) lParam);
    }
    return FALSE;
  }
  dwOldModify = b->dwModify;

  /*
    And get a pointer to the window structure
  */
  wEdit = WID_TO_WIN(hEdit);
  dwFlags = wEdit->flags;
  b->szText = (LPSTR) EDITORLock(b->hText);

  /*
    See if the edit control is read-only
  */
  bIsReadonly = (BOOL) ((dwFlags & ES_READONLY) != 0L);


  /*
    Client-area mouse messages should be converted back into screen coords
    for this winproc.
  */
  if (IS_WM_MOUSE_MSG(msg))
    lParam = _UnWindowizeMouse(hEdit, lParam);


  switch (msg)
  {
#if 0
    case WM_NCHITTEST :
      rc = HTCLIENT;
      goto bye;
#endif

    case WM_SETFONT   :
      StdWindowWinProc(hEdit, msg, wParam, lParam);
      EditSetFont(hEdit);
      goto bye;

    case WM_SIZE      :
      GetClientRect(hEdit, &rClient);
      b->nLinesInWindow = rClient.bottom / (b->yFontHeight + b->yFontLeading);
      if (b->nLinesInWindow <= 0)
        b->nLinesInWindow = 1;
      goto bye;

#ifdef MEWEL_TEXT
    case WM_BORDER     :
      /*
        For a single-line edit field, draw square brackets around the
        edges. For a framed edit control, let WinDrawBorder() handle
        everything.
      */
      rClient = wEdit->rect;
      if (RECT_HEIGHT(rClient) == 1)   /* single line? */
      {
        COLOR attr = wEdit->attr;
        if ((dwFlags & WS_DISABLED) ||
            ((wParent = wEdit->parent) != NULL &&
              wParent->idClass == COMBO_CLASS  &&
              (wParent->flags & WS_DISABLED)))
          attr = WinQuerySysColor(NULLHWND, SYSCLR_DISABLEDBORDER);

        WinPutc(hEdit, 0, -1,
                WinGetSysChar(SYSCHAR_EDIT_LBORDER), attr);
        WinPutc(hEdit, 0, RECT_WIDTH(rClient)-2,
                WinGetSysChar(SYSCHAR_EDIT_RBORDER), attr);
      }
      else
        rc = FALSE; /* tell WinDrawBorder() that we didn't handle the border */
      goto bye;
#endif


    case WM_NCPAINT    :
      _PrepareWMCtlColor(hEdit, CTLCOLOR_EDIT, 0);
      WinDrawBorder(hEdit, InternalSysParams.hWndFocus == hEdit);
      goto bye;


    case WM_PAINT      :
    {
      RECT rUpdate;
      PAINTSTRUCT ps;

      hDC = BeginPaint(hEdit, (LPPAINTSTRUCT) &ps);
      _PrepareWMCtlColor(hEdit, CTLCOLOR_EDIT, hDC);

      /*
        Make sure that the window shows the cursor position (except
        if we are scrolling).
        5/13/93 (maa)
          Added the DestroyCaret and STATE_NO_SYNCCURSOR flags to try
          to eliminate redundant caret processing.
      */
      if (hEdit == InternalSysParams.hWndFocus)
        DestroyCaret();
      b->fFlags |= STATE_NO_SYNCCURSOR;
      EditSetCursor(hEdit);
      b->fFlags &= ~STATE_NO_SYNCCURSOR;

      /*
        Did EditSetCursor add to the update area?
      */
      if (GetUpdateRect(hEdit, &rUpdate, FALSE))
        UnionRect(&ps.rcPaint, &ps.rcPaint, &rUpdate);
      EditShow(hEdit, hDC, &ps.rcPaint);
      EndPaint(hEdit, (LPPAINTSTRUCT) &ps);
      goto bye;
    }

    case WM_SYNCCURSOR :
      /*
        Refresh the caret. If the edit's update rectangle is not empty,
        then don't refresh cause the caret will be updated anyway as
        part of the WM_PAINT procedure.
      */
      if (IsRectEmpty(&wEdit->rUpdate))
      {
#if !defined(USE_NATIVE_GUI)
        _EditPositionCursor(hEdit, b);
#endif
        b->fFlags &= ~STATE_SYNCCURSOR_POSTED;
      }
      goto bye;


    case WM_DESTROY    :
      /*
        When an edit control is destroyed, free the text buffer. We do not
        have to free the BUFFER structure pointer to by wEdit->pPrivate,
        since it will be freed in WinDelete().
      */
      EDITORUnlock(b->hText);
      EDITORFree(b->hText);
      return TRUE;


    case WM_SETFOCUS   :
      /*
        Position the caret
      */
      b->fFlags |= (STATE_INSERT | STATE_HASFOCUS | STATE_HASCURSOR);
      EditSetCursor(hEdit);

      /*
        If a single-line edit control is part of a dialog box, then select
        the entire contents when the focus is set to it.
      */
      wParent = wEdit->parent;
      if (wParent && (IS_DIALOG(wParent) || wParent->idClass==COMBO_CLASS) &&
          (SendMessage(hEdit, WM_GETDLGCODE, 0, 0L) & DLGC_HASSETSEL) &&
          !(dwFlags & ES_MULTILINE))
        SendMessage(hEdit, EM_SETSEL, 0, MAKELONG(0, 0x7FFF));
      /* fall through... */

    case WM_KILLFOCUS  :
      /*
        Notify the parent that we gained or relinquished the focus
      */
      _EditNotifyParent(hEdit, (msg==WM_SETFOCUS) ? EN_SETFOCUS : EN_KILLFOCUS);

      /*
        The only way we can tell that a single-line, non-bordered edit control
        has the focus is to display it in reverse video.
      */
      if (!(dwFlags & WS_BORDER))
      {
        if (!(dwFlags & ES_MULTILINE))
          INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
      }
      else
      {
        /*
          We have a bordered edit field, so show the focus state by
          drawing the border with a double or single line.
          We cause a WM_NCPAINT message to be sent to the edit control.
        */
        InvalidateNCArea(hEdit);
        if (msg == WM_SETFOCUS)
          EditSyncCursor(hEdit, b);
      }

      if (msg == WM_KILLFOCUS)
      {
        b->fFlags &= ~(STATE_HASFOCUS | STATE_HASCURSOR);
        bSelecting = FALSE;
        if (b->fFlags & STATE_SOMETHING_SELECTED)
        {
          /*
            We wanna turn off the highlighting, but leave the selection
            flag intact.
            (4/23/90 maa) Why do we want to do this? MS Windows removes
            the selection state when we leave an edit field.
            (6/6/90 maa)  If we are in an edit field with selected text,
            and we invoke a menu to choose a CUT/COPY/PASTE command, we
            don't want to clear the selection.
          */
          if (!wParam || !IS_MENU(WID_TO_WIN(wParam)))
            EditClearSelect(hEdit);
#ifdef TELEVOICE
          /*
            Televoice is using color to show which edit control has the focus.
            So, when an edit control loses focus, it needs to repaint itself.
          */
          else
            INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
#endif
        }
      }
      break;


    case WM_GETDLGCODE :
      if (dwFlags & ES_MULTILINE)
        rc = DLGC_HASSETSEL    | /* DLGC_WANTCHARS | */ DLGC_WANTARROWS;
      else
        rc = DLGC_HASSETSEL /* |    DLGC_WANTCHARS |    DLGC_WANTARROWS */;
      goto bye;


    case WM_GETID :
      rc = b->idCtrl;
      goto bye;

    case WM_SETID :
      rc = wEdit->idCtrl = b->idCtrl = wParam;
      goto bye;

    case WM_SETFLAGS :
      EDITORUnlock(b->hText);
      return b->style = wEdit->flags = lParam;


    case WM_GETTEXT :
      rc = _Edit_WMGETTEXT(b, (LPSTR) lParam, (UINT) wParam);
      goto bye;

    case WM_GETTEXTLENGTH :
      rc = _Edit_WMGETTEXTLEN(b);
      goto bye;


    case WM_SETTEXT_STATIC :  /* rsm */
        /* Sets redraw flag to REDRAW_STATIC, used to display word-wrapped
         * static text in a multiline edit control. Makes use of last
         * character slot when calculating word-wrapping.
         */
    case WM_SETTEXT :
      /* lParam is the FAR pointer to the text */
      if ((void *) lParam == NULL)
        lParam = (DWORD) (LPSTR) "";
      rc = lstrlen((LPSTR) lParam);
      if ((UINT) rc >= b->nTotalAllocated)
        _EditReallocate(b, (UINT) ((rc + 16) & 0xFFF0));  /* round up to 16 */

      _Edit_WMSETTEXT(b, (LPSTR) lParam);

      /* Clear out any shlegum from before... */
      lmemset(b->szText+b->nTotalBytes, '\0', b->nTotalAllocated-b->nTotalBytes);
      b->lastlnum = max(_EditIndexToLine(hEdit, b->nTotalBytes, NULL), 1);
      EditBeginningOfBuffer(hEdit);
      b->fFlags &= ~(STATE_DIRTY | STATE_SOMETHING_SELECTED);
      b->dwModify = 0L;
      _EditReformatAll(hEdit);
      INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);

      /*
        4/28/93 (maa)
        It seems that the EN_CHANGE notification needs to be sent to the
        dialog box if we do a SetDlgItemText on an edit control.
      */
      _EditNotifyParent(hEdit, EN_CHANGE);

      break;


    case WM_KEYDOWN:
    {
#if defined(USE_WINDOWS_COMPAT_KEYS)
      BOOL bCtrl  = (BOOL) (GetKeyState(VK_CONTROL) != 0);
      BOOL bShift = (BOOL) (GetKeyState(VK_SHIFT)   != 0);
      BOOL bAlt   = (BOOL) (GetKeyState(VK_MENU)    != 0);
#endif

      /*
        If text is selected and the user preeses an arrow key, unhighlight
        the selected text. This is what MS Windows does.
      */
      if (_EditIsDirectionKey(wParam))
        EditClearSelect(hEdit);

      switch (wParam)
      {
        case VK_UP :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt && !bCtrl)
#endif
          rc = EditUp(hEdit);
          break;

        case VK_DOWN :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt && !bCtrl)
#endif
          rc = EditDown(hEdit);
          break;

        case VK_LEFT :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if      (bCtrl)
            rc = EditPrevWord(hEdit);
          else if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt)
#endif
          rc = EditLeft(hEdit);
          break;

        case VK_RIGHT :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if      (bCtrl)
            rc = EditNextWord(hEdit);
          else if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt)
#endif
          rc = EditRight(hEdit);
          break;

#if !defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_CTRL_RIGHT:
          rc = EditNextWord(hEdit);
          break;
        case VK_CTRL_LEFT  :
          rc = EditPrevWord(hEdit);
          break;
#endif

        case VK_NEXT   :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt && !bCtrl)
#endif
          rc = EditPageDown(hEdit);
          break;

        case VK_PRIOR  :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt && !bCtrl)
#endif
          rc = EditPageUp(hEdit);
          break;

        case VK_HOME :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bCtrl)
            rc = EditBeginningOfBuffer(hEdit);
          else if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt)
#endif
          rc = EditBeginningOfLine(hEdit);
          break;

        case VK_END :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bCtrl)
            rc = EditEndOfBuffer(hEdit);
          else if (bShift)
            rc = EditSelect(hEdit, wParam);
          else if (!bAlt)
#endif
          rc = EditEndOfLine(hEdit);
          break;

#if !defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_CTRL_END  :
          rc = EditEndOfBuffer(hEdit);
          break;
        case VK_CTRL_HOME :
          rc = EditBeginningOfBuffer(hEdit);
          break;
#endif

#if defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_INSERT    :
          if (bCtrl)
          {
do_copy:
            if (!bIsReadonly)
            {
              rc = EditCopySelection(hEdit, FALSE, TRUE);
              _EditNotifyParent(hEdit, EN_CHANGE);
            }
          }
          else if (bShift)
          {
do_paste:
            if (!bIsReadonly)
            {
              rc = EditPaste(hEdit);
              _EditNotifyParent(hEdit, EN_CHANGE);
            }
          }
          else if (!bAlt)
          {
            if (!bIsReadonly)
              rc = EditToggleInsert(hEdit);
          }
          break;
#else
        case VK_INSERT    :
          if (!bIsReadonly)
            rc = EditToggleInsert(hEdit);
          break;
        case VK_SH_INS    :
do_paste:
          if (!bIsReadonly)
          {
            rc = EditPaste(hEdit);
            _EditNotifyParent(hEdit, EN_CHANGE);
          }
          break;
        case VK_CTRL_INS  :
do_copy:
          if (!bIsReadonly)
          {
            rc = EditCopySelection(hEdit, FALSE, TRUE);
            _EditNotifyParent(hEdit, EN_CHANGE);
          }
          break;
#endif


        case VK_DELETE    :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (bShift)
          {
do_cut:
            if (!bIsReadonly)
            {
              if (b->fFlags & STATE_SOMETHING_SELECTED)
              {
                rc = EditCopySelection(hEdit, TRUE, TRUE);
                _EditNotifyParent(hEdit, EN_CHANGE);
              }
            }
          }
          else if (!bAlt && !bCtrl)
#endif
          if (!bIsReadonly)
          {
            if (b->fFlags & STATE_SOMETHING_SELECTED)
              rc = EditCopySelection(hEdit, TRUE, FALSE);
            else
              rc = EditDeleteChar(hEdit);
            _EditNotifyParent(hEdit, EN_CHANGE);
          }
          break;

#if !defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_SH_DEL    :
do_cut:
          if (!bIsReadonly)
          {
            if (b->fFlags & STATE_SOMETHING_SELECTED)
            {
              rc = EditCopySelection(hEdit, TRUE, TRUE);
              _EditNotifyParent(hEdit, EN_CHANGE);
            }
          }
          break;
#endif

#if !defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_SH_LEFT   :
        case VK_SH_RIGHT  :
        case VK_SH_UP     :
        case VK_SH_DOWN   :
        case VK_SH_HOME   :
        case VK_SH_END    :
          if (!bIsReadonly)
            rc = EditSelect(hEdit, wParam);
          break;
#endif

        case VK_ESCAPE    :
          _EditNotifyParent(hEdit, IDCANCEL);
          break;

        case VK_TAB       :
        case VK_RETURN    :
#ifdef INTERNATIONAL_MEWEL
        case 0x15         :
#endif
          goto do_char;
      }

      if (wParam & 0xFF00)  /* do this only if WM_CHAR will not follow */
        bSetCursor = TRUE;
      break;
    }


    case WM_CHAR   :
      /*
        Windows handles the BACKSPACE key at WM_CHAR time, not WM_KEYDOWN time.
      */
      if (wParam == VK_BACK)
      {
        if (!bIsReadonly)
        {
          rc = EditBackspace(hEdit);
          _EditNotifyParent(hEdit, EN_CHANGE);
        }
      }

      else if (wParam == ('C' & 0x1F))
        goto do_copy;
      else if (wParam == ('V' & 0x1F))
        goto do_paste;
      else if (wParam == ('X' & 0x1F))
        goto do_cut;

      /*
        Allow "foreign" chars (including paragraph sign) to be entered.
      */
      else if (wParam >= ' ' && wParam < 255)
      {
        BYTE s[2];

        /*
          A user pressing <ENTER> on a single-line edit control will
          cause the dialog box to be accepted.
        */
do_char:
        if (wParam == VK_RETURN && (b->style & ES_MULTILINE) == FALSE)
        {
          _EditNotifyParent(hEdit, IDOK);
          rc = TRUE;
          goto bye;
        }

        s[0] = (BYTE) wParam;  s[1] = '\0';
        if (b->style & ES_LOWERCASE)
          s[0] = (BYTE) tolower(s[0]);
        else if (b->style & ES_UPPERCASE)
          s[0] = (BYTE) lang_upper(s[0]);

        if (!bIsReadonly)
        {
          rc = EditInsertChar(hEdit, (LPSTR) s);
          _EditNotifyParent(hEdit, EN_CHANGE);
        }
      }
      bSetCursor = TRUE;
      break;


    case WM_PASTE      :
      if (!bIsReadonly)
      {
        rc = EditPaste(hEdit);
        _EditNotifyParent(hEdit, EN_CHANGE);
        bSetCursor = TRUE;
      }
      break;

    case WM_CLEAR      :
      if (!bIsReadonly && (b->fFlags & STATE_SOMETHING_SELECTED))
      {
        rc = EditCopySelection(hEdit, TRUE, FALSE);
        _EditNotifyParent(hEdit, EN_CHANGE);
        bSetCursor = TRUE;
      }
      break;

    case WM_CUT        :
      if (!bIsReadonly)
      {
        rc = EditCopySelection(hEdit, TRUE, TRUE);
        _EditNotifyParent(hEdit, EN_CHANGE);
        bSetCursor = TRUE;
      }
      break;

    case WM_COPY       :
      rc = EditCopySelection(hEdit, FALSE, TRUE);
      break;

    case WM_VSCROLL    :
    {
      switch (wParam)
      {
      	case SB_LINEUP   :
          rc = EditUp(hEdit);
          break;
      	case SB_LINEDOWN :
          rc = EditDown(hEdit);
          break;
      	case SB_PAGEUP   :
          rc = EditPageUp(hEdit);
          break;
      	case SB_PAGEDOWN :
          rc = EditPageDown(hEdit);
          break;
        case SB_THUMBTRACK :
        {
          HWND hVSB, hHSB;
          int  minpos, maxpos, currpos;

          WinGetScrollbars(hEdit, &hHSB, &hVSB);
          GetScrollRange(hVSB, SB_CTL, &minpos, &maxpos);
          currpos = LOWORD(lParam);
          SetScrollPos(hVSB, SB_CTL, max(min(currpos, maxpos), minpos), TRUE);
          b->currlinenum = GetScrollPos(hVSB, SB_CTL);
          _EditGetLine(hEdit, b->currlinenum, &b->iCurrPosition);

#ifdef MEWEL_TEXT
          break;
#else
          /*
             Return now to keep from trying to update the diplay as we move,
             screen update's couldn't keep up.
          */
          rc = TRUE;
          goto bye;
#endif
        }
      }
      INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
      _EditNotifyParent(hEdit, EN_VSCROLL);
      break;
    }

    case WM_HSCROLL    :
    {
      UINT oldScroll = b->iHscroll;
      INT  iWidth    = wEdit->rClient.right - wEdit->rClient.left;

      switch (wParam)
      {
      	case SB_LINEUP   :
          if (b->iHscroll > 0)
            b->iHscroll--;
          break;
      	case SB_LINEDOWN :
          b->iHscroll++;
          break;
      	case SB_PAGEUP   :
          if (b->iHscroll > 0)
          {
            if (((int) (b->iHscroll -= iWidth)) < 0)
              b->iHscroll = 0;
          }
          break;
      	case SB_PAGEDOWN :
          b->iHscroll += iWidth;
          break;
        case SB_THUMBTRACK :
        {
          HWND hVSB, hHSB;
          int  minpos, maxpos, currpos;

          WinGetScrollbars(hEdit, &hHSB, &hVSB);
          GetScrollRange(hHSB, SB_CTL, &minpos, &maxpos);
          currpos = LOWORD(lParam);
          SetScrollPos(hHSB, SB_CTL, max(min(currpos, maxpos), minpos), TRUE);
          b->iHscroll = GetScrollPos(hHSB, SB_CTL);
          break;
        }
      }

      /*
        If we changed the scroll position, then cause the edit control
        to be refreshed and notify the parent. We set the STATE_SCROLLING
        flag so that the WM_PAINT processing does not try to re-adjust the
        'iHscroll' value so that the cursor is in the window.
      */
      if (oldScroll != b->iHscroll)
      {
        INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
        b->fFlags |= STATE_SCROLLING;
        _EditNotifyParent(hEdit, EN_HSCROLL);
      }
      break;
    }


    case WM_LBUTTONDOWN:
      /*
       If we pressed the left button down, we want to begin marking a
       region. We capture all mouse msgs and anchor the starting point.
      */
      WinGetClient(hEdit, &rClient);
      if (!PtInRect(&rClient, MAKEPOINT(lParam)))
        goto call_dwp;

      /*
        Make row and col client-based
      */
      col = LOWORD(lParam) - rClient.left;
      row = HIWORD(lParam) - rClient.top;

      /*
        Transform the mouse point into a character-based line/col position
      */
      EditMouseToChar(hEdit, b, col, row, &col, &row);

      /*
        If there was an area previously selected, remove the selection
        so we can start over.
      */
      if (b->fFlags & STATE_SOMETHING_SELECTED)
      {
        EditClearSelect(hEdit);
        EditSetCursor(hEdit);
        INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
        UpdateWindow(hEdit);
      }

      /*
        Capture the mouse
      */
      SetCapture(hEdit);
      bSelecting = TRUE;

      /*
        Update the current position and the selection range.
      */
      EditGoto(hEdit, b->topline + row, b->iHscroll + col);
      EditSelect(hEdit, 0);
      bSetCursor = TRUE;
      break;


    case WM_NCLBUTTONUP:
    case WM_LBUTTONUP  :
      /*
        The button was released. If the starting click point is the
        same as the ending click point, then we don't have a selection.
      */
      if (bSelecting)
      {
        if (b->iCurrPosition == b->iposStartMark)
          EditClearSelect(hEdit);
      }
      bSelecting = FALSE;
      ReleaseCapture();
      break;


    case WM_MOUSEMOVE  :
      /*
        We are only interested in mouse movement if we are selecting
      */
      if (bSelecting)
      {
        EPOS oldPosition;

        /*
          Get the mouse position in client-area coordinates
        */
        WinGetClient(hEdit, &rClient);
        col = LOWORD(lParam) - rClient.left;
        row = HIWORD(lParam) - rClient.top;

        /*
          Check the mouse against the window boundaries. Return if the
          mouse is outside of the window.
          NOTE: We could implement scrolling if we wanted to.
        */
        if (row < 0 || col < 0 || LOWORD(lParam) >= rClient.right ||
                                  HIWORD(lParam) >= rClient.bottom)
        {
          rc = FALSE;
          goto bye;
        }

        /*
          Save the old current position and old current line
        */
        oldPosition = b->iCurrPosition;
        oldCurrline = b->currlinenum;

        /*
          Transform the mouse point into a character-based line/col position
        */
        EditMouseToChar(hEdit, b, col, row, &col, &row);

        /*
          Change the current position
        */
        EditGoto(hEdit, b->topline+row, b->iHscroll+col);

        /*
          If we did not change the current position, then don't bother
          updating the selection range
        */
        if (b->iCurrPosition == oldPosition)
          break;

        /*
          Update the selection range
        */
        EditSelect(hEdit, 0);

        /*
          Make part of the edit window dirty so that it has to be repainted
        */
        if (oldCurrline == b->currlinenum)
          InvalidateLine(hEdit, REDRAW_LINE, 0);
        else
          INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
      }
      else
      {
        rc = FALSE;
        goto bye;
      }
      break;



    case EM_CANUNDO   :
    case EM_UNDO      :
      rc = FALSE;
      goto bye;


    case EM_GETHANDLE :
      /*
        MEWEL extension
        lParam can be a pointer to an integer which the edit buffer
        size will be returned into.
      */
      if (lParam)
        * (LPUINT) lParam = b->nTotalAllocated;
      rc = b->hText;
      goto bye;


    case EM_GETLINE :
    {
      /* wParam is the 0-based index of the line to retrieve   */
      /* lParam is a FAR ptr to the buffer to receive the line */
      LPSTR szLine;
      LPSTR szDest;
      EPOS iPos;

      if ((szLine = _EditGetLine(hEdit, wParam+1, &iPos)) == NULL)
      {
        rc = 0;
        goto bye;
      }

      /*
        Copy everything up until the newline into the user-defined buffer
      */
      for (szDest = (LPSTR) lParam;  *szLine && *szLine != CH_NEWLINE;
           *szDest++ = *szLine++) ;
      *szDest = '\0';
      rc = (szDest - (LPSTR) lParam);  /* the # of chars copied... */
      goto bye;
    }


    case EM_GETLINECOUNT :
      rc = b->lastlnum;
      goto bye;


    case EM_GETSEL       :
      /*
        Returns the current selection. If something is selected, returns
        the starting position in the LOWORD and the starting position of
        the unselected text in the HIWORD. If nothing is selected, returns
        the current position in both the LOWORD and the HIWORD.
      */
      EDITORUnlock(b->hText);
      if (b->fFlags & STATE_SOMETHING_SELECTED)
      {
        EPOS startMark = b->iposStartMark;
        EPOS endMark   = b->iposEndMark+1;
        /*
          Make sure that, in case we have selected from right to left,
          the mark closest to the beginning of the buffer is in LOWORD.
        */
        if (startMark > endMark)
        {
          startMark ^= endMark, endMark ^= startMark, startMark ^= endMark;
        }
        return MAKELONG(startMark, endMark);
      }
      else
        return MAKELONG(b->iCurrPosition, b->iCurrPosition);


    /*
      These two messages are MEWEL-specific. They are used to get and set
      the edit control's insert mode.
    */
    case EM_GETSTATE :
      rc = b->fFlags;
      goto bye;
    case EM_SETSTATE :
      b->fFlags = wParam;
      break;  /* so we can refresh */


    case EM_LINEFROMCHAR :
      rc = _EditIndexToLine(hEdit, wParam, NULL)-1;
      goto bye;


    case EM_LINEINDEX    :
      _EditGetLine(hEdit, (wParam==(WPARAM) -1) ? b->currlinenum : wParam+1, (UINT*)&rc);
      goto bye;


    case EM_LINELENGTH   :
      /*
        wParam is the 0-based index in the edit buffer. It was most likely
        retreived by a prior call to EM_LINEINDEX
      */
      wParam = _EditIndexToLine(hEdit, wParam, NULL);
      if ((szLine =  (LPSTR) _EditGetLine(hEdit, wParam, (UINT *) &rc)) != NULL)
      {
        /* Determine the length of the line */
        szNL = (LPSTR) lmemchr(szLine, CH_NEWLINE, b->nTotalBytes - rc);
        rc = (szNL == NULL) ? lstrlen(szLine) : szNL - szLine;
        goto bye;
      }
      else
      {
        rc = 0;
        goto bye;
      }


    case EM_LINESCROLL   :
      /* LOWORD(lParam) is # of lines to scroll, HIWORD(lP) is # of cols */
      rc = EditScroll(hEdit, LOWORD(lParam), HIWORD(lParam));
      break;

    case EM_REPLACESEL   :
      /*
        Replace the selcted text with the text pointed to by lParam. If
        there is no selected text, the new text is inserted at the
        cursor position. This command is only valid in non-readonly edits.

        11/12/93 (maa)
          It seems as if MS Windows will do a replacement on an edit
        control with the ES_READONLY style. (Ref: Ingenio Software)
      */
#if !111293
      if (!bIsReadonly)
#endif
      {
        rc = EditCopySelection(hEdit, TRUE, FALSE);
        rc = EditInsertBlock(hEdit, (LPSTR) lParam);
      }
      break;

    case EM_SETHANDLE    :
      if (wParam != b->hText)
      {
        /*
          The SDK docs say that the app itself is responsible for
          freeing the handle.
        */
        EDITORUnlock(b->hText);
        b->hText = wParam;
      }
      b->iCurrPosition = 0;
      b->iposStartMark = b->iposEndMark = 0;
      b->iHscroll      = 0;
      b->currlinenum   = b->topline = 1;
      b->nTotalAllocated = wParam = (UINT) EDITORSize(b->hText);
      SendMessage(hEdit, WM_SETTEXT, 0, (LONG) EDITORLock(b->hText));
      EDITORUnlock(b->hText);
      break;

    case EM_SETMODIFY    :
      if (wParam)
        b->dwModify = 1L;
      else
        b->dwModify = 0L;
      rc = TRUE;
      goto bye;


    case EM_SETPASSWORDCHAR :
      b->chPassword = (BYTE) wParam;
      rc = TRUE;
      goto bye;


    case EM_GETMODIFY :
      rc = (b->dwModify != 0L);
      goto bye;


    case EM_LIMITTEXT :
      b->nMaxLimit = wParam;
      b->fFlags |= STATE_LIMITSET;
      rc = TRUE;
      goto bye;


    case EM_SETSEL       :
      if (LOWORD(lParam) > HIWORD(lParam))
      {
        rc = TRUE;
        goto bye;
      }

      /*
        If lParam is 0xFFFFFFFF or 0xFFFF0000, then mark the entire
        control.
      */
      if (HIWORD(lParam) == (UINT) -1 && ((int) LOWORD(lParam)) <= 0)
      {
        b->iposStartMark = 0;
        b->iposEndMark   = b->nTotalBytes - 1;
      }
      else if (LOWORD(lParam) == HIWORD(lParam))
      {
        /*
          If we are setting the selection where the start == the end, then
          just position the cursor without setting the selection state on.
          11/13/92 (maa)
            changed "- 1" to "- 0" so we can set the current position to
            the end of the line.
        */
        EPOS wPos;

        /*
          If thee is something selected, then forget about the selection.
        */
        if (b->fFlags & STATE_SOMETHING_SELECTED)
          EditClearSelect(hEdit);

        wPos = min(b->nTotalBytes, LOWORD(lParam));
        if (wPos <= b->nTotalBytes - 0)
        {
          iLine = _EditIndexToLine(hEdit, wPos, &idxBOL);
          EditGoto(hEdit, iLine, wPos - idxBOL);
      	  bSetCursor = TRUE;
        }
        break;
      }
      else
      {
        if (b->nTotalBytes == 0)
          b->iposStartMark = 0;
        else
          b->iposStartMark = min(LOWORD(lParam), b->nTotalBytes-1);
        if (HIWORD(lParam))
        {
          if (b->nTotalBytes == 0)
            b->iposEndMark = 0;
          else
            b->iposEndMark = min(HIWORD(lParam)-1, b->nTotalBytes-1);
        }
        else
          b->iposEndMark = 0;

        /*
          7/14/90 (maa) seems that the char at HIWORD(lP) should *not*
          be selected.
        */
      }

      /*
        7/14/90 (maa) - seems that Windows sets the position of the
        cursor to the end marker. However, if the ending mark position
        is at the EOF, then set the cursor to one after the last char.
      */
      iLine = _EditIndexToLine(hEdit, b->iposEndMark, &idxBOL);
      EditGoto(hEdit, iLine,
               (b->iposEndMark == b->nTotalBytes - 1) ? b->nTotalBytes
                                                      : b->iposEndMark-idxBOL);
      b->fFlags |= STATE_SOMETHING_SELECTED;
      INVALIDATERECT(hEdit, (LPRECT) NULL, FALSE);
      break;


    case EM_FMTLINES   :
    {
      UINT oflg = b->fFlags;

      if (wParam)
         b->fFlags |= STATE_EM_FMTLINES;
      else
         b->fFlags &= ~STATE_EM_FMTLINES;

      if (b->fFlags == oflg)
        rc = 0;
      else
        rc = (lstrchr(b->szText, CH_WORDWRAP) != NULL);
      goto bye;
    }

    case EM_SETREADONLY:
      if (wParam)
        wEdit->flags |=  ES_READONLY;
      else
        wEdit->flags &= ~ES_READONLY;
      goto bye;

    case WM_COMMAND    :
    case WM_SYSCOMMAND :
      /*
        Edit controls shouldn't get these, but if they do, pass it
        onto the parent. (Such as when we press <CTRL F6> in an
        edit control.
      */
      EDITORUnlock(b->hText);
      return SendMessage(GetParent(hEdit), msg, wParam, lParam);

    default :
call_dwp:
      EDITORUnlock(b->hText);
      return StdWindowWinProc(hEdit, msg, wParam, lParam);
  }

  /*
    If the edit control is about to display new text, let the parent know.
  */
  if (b->dwModify != dwOldModify)
    _EditNotifyParent(hEdit, EN_UPDATE);

  if (msg == WM_KILLFOCUS)
#if defined(USE_NATIVE_GUI)
    ;
#else
    DestroyCaret();
#endif
  /*
    11/5/92 (maa)  try to speed up things
  */
  else if (bSetCursor)
    EditSetCursor(hEdit);

bye:
  EDITORUnlock(b->hText);
  return rc;
}


static INT PASCAL _Edit_WMGETTEXT(b, lpDest, iCnt)
  BUFFER *b;
  LPSTR lpDest;
  UINT  iCnt;
{
  INT   ch;
  LPSTR lpSrc  = b->szText;
  LPSTR lpDestStart = lpDest;

  if (iCnt > 0)
  {
    /*
      Copy each character in the edit buffer over to lpDest. Check
      for the special case of a NEWLINE (\n) character
    */
    while (iCnt-- && (ch = *lpDest++ = *lpSrc++) != '\0')
    {
      /*
        If we have a \r\n hard carriage return, then do not transform it.
      */
      if (ch == CH_WORDWRAP && *lpSrc == CH_NEWLINE)
      {
        *lpDest++ = '\n';
        iCnt--;
        continue;
      }

      /*
        Replace the \n\r combination with a blank
      */
      if (ch == CH_NEWLINE)         /* \n = wordwrapped line */
      {
        if (iCnt && (b->fFlags & STATE_EM_FMTLINES)) /* returns \r\r\n */
        {
          lpDest[-2] = '\r';
          lpDest[-1] = '\r';
          *lpDest++  = '\n';
          iCnt--;
        }
        else if (*lpSrc == CH_WORDWRAP)  /* \n\r */
        {
          lpDest[-1] = ' ';
          lpSrc++;  /* span the \r */
          iCnt++;
        }
        else if (iCnt)  /* a solitary \n */
        {
          lpDest[-1] = '\r';
          *lpDest++  = '\n';
          iCnt--;
        }
      } /* end if (CH_NEWLINE) */
    } /* while */
    *--lpDest = '\0';
  }
  return (lpDest - lpDestStart);
}


static VOID PASCAL _Edit_WMSETTEXT(b, lpSrc)
  BUFFER *b;
  LPSTR  lpSrc;
{
  INT   ch;
  LPSTR lpDest  = b->szText;
  UINT  iLen = 0;
  UINT  nMax;

  nMax = (b->nMaxLimit > 0) ? b->nMaxLimit : b->nTotalAllocated-1;

  /*
    Windows replaces a \r\n combination with a single \n
  */
  for (  ; iLen <= nMax && (ch = *lpDest++ = *lpSrc++) != '\0';  iLen++)
  {
    if (ch == '\r')
    {
      if (*lpSrc == '\n')  /* CR+LF is a 'hard' linefeed, convert to \n */
      {
        lpDest[-1] = '\n';
        lpSrc++;
      }
      else if (*lpSrc == '\r' && lpSrc[1] == '\n' && iLen < nMax)
      {                   /* CR+CR+LF is a 'soft' linefeed */
        lpDest[-1] = '\n';
        *lpDest++ = CH_WORDWRAP;
        lpSrc += 2;
        iLen++;
      }
      else    /* ignore a single CR (what else should we do?) */
      {
        iLen--;
        lpDest--;
      }
    }
  } /* for */

  if ((UINT) iLen > (UINT) nMax)
    lpDest[-1] = '\0';
  b->nTotalBytes = iLen;
}


static INT PASCAL _Edit_WMGETTEXTLEN(b)
  BUFFER *b;
{
  LPSTR pSrc = b->szText;
  INT   iCnt = 0;
  INT   ch;

  while ((ch = *pSrc++) != '\0')
  {
    if (ch == CH_NEWLINE)
    {
      if (*pSrc == CH_WORDWRAP)  /* \n\r */
      {
        /* \n\r - gets replaced by BLANK by GETTEXT */
        iCnt++;
        pSrc++; /* skip over the word wrap character */
      }
      else
      {
        iCnt += 2;  /* a single \n gets replaced by \r\n */
      }
    }
    else if (ch == CH_WORDWRAP)
    {
      if (*pSrc == CH_NEWLINE)  /* \r\n is a hard return */
      {
        iCnt += 2;
        pSrc++; /* skip over the '\n' character */
      }
      else if (b->fFlags & STATE_EM_FMTLINES)
      {
        /* returns \r\r\n */
        iCnt += 3;
      }
      else  /* just a single solitary \r */
        iCnt++;
    }
    else /* regular character */
      iCnt++;
  }

  /*
    Return the length
  */
  return iCnt;
}

