/*===========================================================================*/
/*                                                                           */
/* File    : WINCOMBO.C                                                      */
/*                                                                           */
/* Purpose : Implements the combo-box class.                                 */
/*                                                                           */
/* History :                                                                 */
/*    5/18/93 (maa) - Added WS_POPUP to ListBoxCreate().                     */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_LISTBOX
#define INCLUDE_COMBOBOX
#define INCLUDE_SCROLLBAR

#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#endif

/*
  The CBN_INTERNALSELCHANGE message is sent by the combobox proc to itself
  when the app sets the current selection with messages like CB_SETSEL,
  CB_FINDSTRING, etc. The difference between this message and CBN_SELCHANGE
  is that this new internal message *should not* send a CBN_SELCHANGE message
  to the parent. This is so that the dialog proc does not think that the
  user has manually scrolled through the combobox.
*/
#define CBN_INTERNALSELCHANGE  99


static BOOL PASCAL _OwnerDrawnComboDraw(HWND, HWND, int);
static UINT PASCAL MapCBtoLB(UINT);
static VOID PASCAL ComboNotifyParent(WINDOW *, INT);
static COMBOBOX* PASCAL HwndToPCombo(HWND);
static LRESULT PASCAL ComboEditKbdHandler(HWND, UINT, WPARAM, LPARAM);

extern INT FAR PASCAL WinAdjustPopupWindow(HWND, LPVOID);


/*
  Televoice says that this variable is not required...
*/
#ifndef TELEVOICE
#define USE_DONTPULLDOWNLB
#endif

#ifdef USE_DONTPULLDOWNLB
/*
  This little variable is set to TRUE only when the listbox of a dropdown
  listbox is visible and we click the mouse on the combobox icon. The
  listbox should be hidden and should *not* be popped up again when the
  WM_MOUSEDOWN message is sent from DispatchMessage() to the combobox.
*/
static BOOL bDontPulldownListbox = FALSE;
#endif

static BOOL bDrawingOwnerDrawnStatic = FALSE;

/*
  This variable is set to TRUE when the listbox of a dropdown list combobox
  is dropped down and the selection changes, which is involved through the
  call to SendMessage with WM_LBUTTONDOWN to set the static variable 
  bSelecting, has to be ignored. (Michael Rossman)
 */
static BOOL bIgnoreSelChange = FALSE;

/*
  This variable is set to TRUE while the listbox portion is being painted.
  This prevents someone from releasing the mouse button while the paint
  operation is happening. Otherwise, if a user releases the mouse button
  while the listbox is painting, the listbox will go away.
*/
static BOOL bIgnoreMouseUp = FALSE;


/*
  Platform-independent combo icon dimensions
*/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#define SS_BORDER       WS_BORDER
#else
#define SS_BORDER       0L
#endif

#if defined(MOTIF)
#define EditWinProc     _XEditWndProc
#define ListBoxWinProc  _XListBoxWndProc
#define StaticWinProc   _XTextWndProc
#endif



LRESULT FAR PASCAL ComboBoxWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  static BOOL bAListboxIsCurrentlyDroppedDown = 0;

  WINDOW   *w, *wListBox;
  COMBOBOX *pCombo;
  HWND     hListBox;
  HWND     hEdit;
  int      iSel;
  LONG     rc;
  char     buf[MAXBUFSIZE];
  BOOL     bIsDropdown;
  UINT     message2;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return CB_ERR;
  if ((pCombo = (COMBOBOX *) w->pPrivate) == NULL)
    pCombo = (COMBOBOX *) (w->pPrivate = emalloc(sizeof(COMBOBOX)));
  hListBox = pCombo->hListBox;
  wListBox = WID_TO_WIN(hListBox);
  hEdit    = pCombo->hEdit;

  bIsDropdown = (BOOL) ((w->flags & CBS_DROPDOWN) != 0);


  switch (message)
  {
    case WM_NCPAINT :
      /*
        Since the non-client area of the combo is the same as the
        client area, we defer all painting to the WM_PAINT message.
      */
      return TRUE;

    case WM_PAINT  :
      /*
        Output the icon, then draw the children
      */
#if defined(MOTIF)
      {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      }
#elif defined(MEWEL_GUI)
      {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);
      if (bIsDropdown && !bAListboxIsCurrentlyDroppedDown)
        DrawSysTransparentBitmap(hWnd, ps.hdc, SYSBMPT_COMBO);
      EndPaint(hWnd, &ps);
      }
#elif defined(MEWEL_TEXT)
      WinClear(hWnd);
      if (bIsDropdown)
      {
        COLOR attr = w->attr;
        if (attr == SYSTEM_COLOR)
          attr = WinQuerySysColor(hWnd,SYSCLR_BTNFACE);  /* COLOR_BTNFACE */

        /*
          Gray the dropdown arrow when a combobox is disabled
        */
        if (w->flags & WS_DISABLED)
          attr = WinQuerySysColor(NULLHWND, SYSCLR_DISABLEDBORDER);

        WinPutc(hWnd, 0, RECT_WIDTH(w->rect)-1, pCombo->chComboBoxIcon, attr);
      }
#endif

      /*
        If the combo box has an edit control (not a static control), then we
        must see if the user entered a brand new value in the edit field.
        If so, then this value is the string which must be displayed. It is
        not yet in the listbox portion of the combobox, so we should not
        access the listbox for this value. We must just repaint the edit
        control.
      */
      if ((w->flags & CBS_DROPDOWNLIST) == CBS_DROPDOWN)
      {
        iSel = LB_ERR;
      }
      else
      {
        iSel = (int) ListBoxWinProc(hListBox, LB_GETCURSEL, 0, 0L);
        if (iSel == LB_ERR)
          SetWindowText(hEdit, NULL);
        /*
          Only set the edit text if the redraw flag is on.
        */
        else if (!(wListBox->flags & (LBS_NOREDRAW | LBS_OWNERDRAWFIXED)))
        {
          ListBoxWinProc(hListBox, LB_GETTEXT, iSel, (DWORD) (LPSTR) buf);
          SetWindowText(hEdit, buf);
        }
      }

      /*
        Draw the edit control
      */
      InternalInvalidateWindow(hEdit, TRUE);
      UpdateWindow(hEdit);
      if (iSel != LB_ERR)
        _OwnerDrawnComboDraw(hListBox, hEdit, iSel);

      /*
        Draw the listbox
      */
      InternalInvalidateWindow(hListBox, TRUE);
      if (!(wListBox->flags & LBS_NOREDRAW) && !TEST_WS_HIDDEN(wListBox))
        UpdateWindow(hListBox);

      w->rUpdate = RectEmpty;
      return TRUE;

    case WM_GETDLGCODE :
      return DLGC_WANTARROWS;

    case WM_NCLBUTTONDOWN :
    case WM_LBUTTONDOWN   :
    {
      /*
        See if we clicked on the icon. If we did, pull down the listbox.
      */
      if (message == WM_LBUTTONDOWN)
        lParam = _UnWindowizeMouse(hWnd, lParam);

      if (bIsDropdown)
      {
#if defined(MOTIF)
        if (1)
#elif defined(MEWEL_GUI) || defined(XWINDOWS)
        RECT r;
        POINT pt;

        pt = MAKEPOINT(lParam);
        SetRect(&r,
                w->rect.right-SM_CXCOMBOICON, w->rect.top,
                w->rect.right, w->rect.top+SM_CYCOMBOICON);
        if (PtInRect(&r, pt))
#else
        int mouserow = HIWORD(lParam);
        int mousecol = LOWORD(lParam);
        if (mouserow == w->rect.top && mousecol == w->rect.right-1)
#endif

#ifdef USE_DONTPULLDOWNLB
          if (bDontPulldownListbox == FALSE)
#endif
            SendMessage(hWnd, CB_SHOWDROPDOWN, TEST_WS_HIDDEN(wListBox), 0L);
      }
#ifdef USE_DONTPULLDOWNLB
      bDontPulldownListbox = FALSE;
#endif
      break;
    }


    case WM_CHAR       :
    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
      /*
        Pressing ALT-DOWN or F4 on a dropdown listbox toggles the
        dropdown state.
      */
#if defined(USE_WINDOWS_COMPAT_KEYS)
      if (((wParam == VK_DOWN && GetKeyState(VK_MENU))
#else
      if ((wParam == VK_ALT_DOWN
#endif
          || wParam == VK_F4) && bIsDropdown)
      {
        SendMessage(hWnd, CB_SHOWDROPDOWN, TEST_WS_HIDDEN(wListBox), 0L);
        ReleaseCapture();  /* The listbox captured the mouse, so if we */
                           /* pick up the mouse, the selection will move */
      }

      if (InternalSysParams.hWndFocus == hEdit)
      {
        ComboEditKbdHandler(hEdit, message, wParam, lParam);
      }
      break;


    case CB_SETEDITSEL  :
      message = EM_SETSEL;     goto send1;
    case CB_GETEDITSEL  :
      message = EM_GETSEL;     goto send1;
    case CB_LIMITTEXT   :
      message = EM_LIMITTEXT;  goto send1;
    case WM_SETTEXT     :
    case WM_GETTEXT     :
    case WM_GETTEXTLENGTH :
    case EM_GETSTATE    :
    case EM_SETSTATE    :
send1:
      if ((w->flags & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
        return EditWinProc(hEdit, message, wParam, lParam);
      else   /* we have a static field */
        return StaticWinProc(hEdit, message, wParam, lParam);

    case CB_ADDSTRING   :
    case CB_DELETESTRING:
    case CB_DIR         :
    case CB_FINDSTRING  :
    case CB_FINDSTRINGEXACT :
    case CB_GETCOUNT    :
    case CB_GETCURSEL   :
    case CB_GETLBTEXT   :
    case CB_GETLBTEXTLEN:
    case CB_INSERTSTRING:
    case CB_RESETCONTENT:
    case CB_SELECTSTRING:
    case CB_SETCURSEL   :
    case CB_SETITEMDATA :
    case CB_GETITEMDATA :
    case WM_SETREDRAW   :
      message2 = MapCBtoLB(message);
      rc = ListBoxWinProc(hListBox, message2, wParam, lParam);
      /*
        If the selection of the listbox has changed, then refresh
        the static or edit field in the dropdown(list) combo. 
        Unfortunately, the listbox routines do not do this because
        they think that the listbox has no parent.
      */
#if 92993
      if (
#else
      if (bIsDropdown && TEST_WS_HIDDEN(wListBox) &&
#endif
          (message == CB_SETCURSEL  || message == CB_SETITEMDATA ||
           message == CB_FINDSTRING || message == CB_SELECTSTRING ||
           message == CB_FINDSTRINGEXACT))
        SendMessage(hWnd, WM_COMMAND, w->idCtrl,
                          MAKELONG(hListBox, CBN_INTERNALSELCHANGE));

      /*
        If we set the redraw flag back on in the combobox, then make
        sure that the edit/static control is updated.
      */
      else if (message == WM_SETREDRAW && wParam)
      {
        InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
      }
      return rc;


    case CB_SHOWDROPDOWN:
      /*
        Show or hide the listbox.
        We use a simple trick here... we attach or detach the listbox from
        the combo box....
      */
      if (bIsDropdown)
      {
        HANDLE hVSB;
        WINDOW *wVSB;

        if (wParam)  /* wParam != 0 means show the listbox */
        {
          char szBuf[80];
          /*
            Attach the listbox to the combobox and draw the listbox.
            Set the focus to the listbox and simulate a mouse-down
            message on the first item of the listbox.
          */
          int  idx, yTop;

          /*
            Already visible?
          */
          if (!TEST_WS_HIDDEN(wListBox))
            return TRUE;

          /*
            Move the popup listbox to the proper place right under the
            combobox.
          */
          yTop = w->rect.top + SM_CYCOMBOICON;
#if defined(MOTIF)
          if (1)
          {
          Position destX, destY;

          /*
            Get the new position in screen coordinates
          */
          XtTranslateCoords(w->widget, w->rect.left, w->rect.top, &destX, &destY);
          destX = destX - w->rect.left;
          destY = destY - w->rect.top + SM_CYCOMBOICON;

          XtTranslateCoords(WID_TO_WIN(hEdit)->widget, 0, SM_CYCOMBOICON,
                            &destX, &destY);

#if 0
          destX = w->rWindowRoot.left;
          destY = w->rWindowRoot.top + SM_CYCOMBOICON;
#endif

          XtVaSetValues(wListBox->widgetFrame,
                        XtNx, destX,
                        XtNy, destY,
                        NULL);
          _XGetRealCoords(wListBox);
          }
#elif defined(XWINDOWS)
          /*
            Motif always glues the listbox to the enclosing form widget
          */
          WinMove(hListBox, SM_CYCOMBOICON, 0);
#else
          WinMove(hListBox, yTop, w->rect.left);
#endif
          pCombo->yOrigBottom = wListBox->rect.bottom;
          pCombo->yAdjustedBottom = yTop;

          wListBox->ulStyle |= WIN_NOCLIP_TO_PARENT;

          /*
            First notify the parent as per Win 3
          */
          ComboNotifyParent(w, CBN_DROPDOWN);

          /*
            Change the extent of the combo box so that it takes up
            the listbox area too.
          */
          w->rect.bottom = pCombo->yOrigBottom;
#if defined(XWINDOWS)
          XMEWELResizeWindow(hWnd, RECT_WIDTH(w->rect), RECT_HEIGHT(w->rect));
#endif
          _WinSetClientRect(hWnd);

          /*
            Re-attach the listbox to the window tree and show it.
          */
          SetParent(hListBox, hWnd);
          wListBox->flags &= ~WS_CLIPSIBLINGS;
          /* Do the scrollbar too */
          if ((hVSB = wListBox->hSB[SB_VERT]) != NULL)
          {
            /*
              Show the scrollbar only if the range is > 1.
            */
            int minPos, maxPos;
            GetScrollRange(hVSB, SB_VERT, &minPos, &maxPos);

            if (minPos != maxPos)
            {
              wVSB = WID_TO_WIN(hVSB);
              wVSB->flags &= ~WS_CLIPSIBLINGS;
              CLR_WS_HIDDEN(wVSB);
              wVSB->flags |= WS_VISIBLE;
              _WinSetClientRect(hListBox);
            }
          }

          if (WinAdjustPopupWindow(hListBox, NULL))
          {
            /*
              We must align the bottom of the listbox with the top
              of the combobox control.
            */
            int iDiff;
            if ((iDiff = wListBox->rect.bottom - w->rect.top) > 0)
              WinMove(hListBox, wListBox->rect.top - iDiff, wListBox->rect.left);
          }

#if defined(XWINDOWS)
          XtPopup(wListBox->widgetFrame, XtGrabNone);
#if 0
          _XmGrabPointer(wListBox->widgetFrame, TRUE, ButtonPressMask,
                         GrabModeAsync, GrabModeAsync, None,
                         XmGetMenuCursor(XSysParams.display), CurrentTime);
          _XmGrabKeyboard(w->widget, FALSE, GrabModeAsync, GrabModeAsync,
                          CurrentTime);
          _XmAddGrab(wListBox->widgetFrame, TRUE, TRUE);
#endif
          XDefineCursor(XSysParams.display, 
                        XtWindowOfObject(wListBox->widgetFrame),
                        XmGetMenuCursor(XSysParams.display));
#endif

          /*
             Don't destroy selection already in the edit/static field
             Not sure how to handle DlgDirList (ComboDirList) yet
             (ie. fullpath in the static/edit box
           */

          if ((idx = (int) SendMessage(hListBox,LB_GETCURSEL,0,0L)) == LB_ERR)
            idx = -1;

          if ((w->flags & CBS_DROPDOWNLIST) == CBS_DROPDOWN)
          {
            SendMessage(hEdit, WM_GETTEXT,sizeof(szBuf),(DWORD)(LPSTR)szBuf);
            idx = (int) SendMessage(hListBox, LB_FINDSTRING, (WPARAM) -1,
                                    (DWORD) (LPSTR) szBuf);
          }
          /*
             Need to do the WM_LBUTTONDOWN to set the static variable
             bSelecting. We then set the selection back again.
           */
          bIgnoreSelChange = TRUE;
          wListBox->flags &= ~LBS_NOTIFY;
          SendMessage(hListBox, WM_LBUTTONDOWN, 0, MAKELONG(0, 0));
          SetCapture(hWnd);
          bIgnoreSelChange = FALSE;
          wListBox->flags |= LBS_NOTIFY;
          SendMessage(hListBox, LB_SETCURSEL, idx, 0L);
      	  if (idx == -1 && (w->flags & CBS_DROPDOWNLIST) == CBS_DROPDOWN)
          {
            SendMessage(hEdit, WM_SETTEXT,0, (DWORD) (LPSTR) szBuf);
          }
          bAListboxIsCurrentlyDroppedDown++;

          bIgnoreMouseUp = TRUE;
          ShowWindow(hListBox, SW_SHOWNA);
          _WinUpdateVisMap(wListBox);
          SetFocus(hListBox);
          RefreshInvalidWindows(_HwndDesktop);
          SetCapture(hListBox);
        }
        else /* (wParam == 0) */
        {
          /*
            Already hidden?
          */
          if (TEST_WS_HIDDEN(wListBox))
            return TRUE;

          /*
            We need to send the listbox a WM_LBUTTONUP message to
            counteract the effect of sending it the WM_LBUTTONDOWN
            message, as we did above. This tells the listbox to release
            the capture and to reset some of its internal states.
            Do not use SendMessage here, as we don't want this message
            to go through ComboListBoxWndProc().
          */
          ListBoxWinProc(hListBox, WM_LBUTTONUP, 0, 0L);

          /*
            Set the extent of the combobox back to a single-line
          */
          w->rect.bottom = pCombo->yAdjustedBottom;
#if defined(XWINDOWS)
          XMEWELResizeWindow(hWnd, RECT_WIDTH(w->rect), RECT_HEIGHT(w->rect));
#endif
          _WinSetClientRect(hWnd);

          /*
            Detach the listbox from the combo box, and hide the listbox.
            The combo box will be redrawn sans the listbox.
          */
          SetParent(hListBox, NULLHWND);
          SET_WS_HIDDEN(wListBox);
          wListBox->flags &= ~WS_VISIBLE;
#if defined(MOTIF)
          XtPopdown(wListBox->widgetFrame);
#if 0
          XtUngrabPointer(wListBox->widgetFrame, CurrentTime);
          XtUngrabKeyboard(w->widget, CurrentTime);
          _XmRemoveGrab(wListBox->widgetFrame);
#endif
#endif
          if ((hVSB = wListBox->hSB[SB_VERT]) != NULL)
          {
            wVSB = WID_TO_WIN(hVSB);
            SET_WS_HIDDEN(wVSB);
            wVSB->flags &= ~WS_VISIBLE;
          }
          WinUpdateVisMap();

          WinGenInvalidRects(_HwndDesktop, &wListBox->rect);

          ComboNotifyParent(w, CBN_CLOSEUP);
          bAListboxIsCurrentlyDroppedDown--;

          /*
            Reset the focus back to the combo box
          */
          SetFocus(hEdit);
        }
      }
      return TRUE;


    case CB_GETDROPPEDSTATE :
      /*
        Return TRUE if listbox portion of a combo is dropped down or visible.
      */
      return (!bIsDropdown || !TEST_WS_HIDDEN(wListBox));



    case WM_COMMAND  :
      if (LOWORD(lParam) == hListBox || LOWORD(lParam) == hEdit)
      {
        switch (HIWORD(lParam))
        {
          case CBN_INTERNALSELCHANGE :
          case CBN_SELCHANGE :
          case CBN_DBLCLK    :
            if (bIgnoreSelChange)
              goto no_send_selchg;
            iSel = (int) ListBoxWinProc(hListBox,LB_GETCURSEL,0,0L);
            if (!_OwnerDrawnComboDraw(hListBox, hEdit, iSel))
            {
              if (iSel == LB_ERR)
                SetWindowText(hEdit, NULL);
              else
              {
                ListBoxWinProc(hListBox, LB_GETTEXT, iSel, (DWORD) (LPSTR) buf);
                SetWindowText(hEdit, buf);
              }
            }

            if (HIWORD(lParam) == CBN_DBLCLK)
            {
              SendMessage(hWnd, CB_SHOWDROPDOWN, FALSE, 0L);
              ComboNotifyParent(w, CBN_SELENDOK);
              SetFocus(hEdit);
            }
            break;

          /*
            Transform edit and listbox notification messages into combobox
            notification messages.
          */
          case EN_UPDATE :
            lParam = MAKELONG(hWnd, CBN_EDITUPDATE);
            break;
          case EN_CHANGE :
            lParam = MAKELONG(hWnd, CBN_EDITCHANGE);
            break;
          case LBN_SETFOCUS:
          case EN_SETFOCUS :
            lParam = MAKELONG(hWnd, CBN_SETFOCUS);
            break;
          case LBN_KILLFOCUS :
          case EN_KILLFOCUS  :
            lParam = MAKELONG(hWnd, CBN_KILLFOCUS);
            break;

        }
send_selchg:
        if (HIWORD(lParam) != CBN_INTERNALSELCHANGE)
          ComboNotifyParent(w, (INT) HIWORD(lParam));
      }

      /*
        Send all combobox notification messages onto the parent
      */
      else if (LOWORD(lParam) == hWnd)
      {
        goto send_selchg;
      }

no_send_selchg:
      break;

   case WM_SETFOCUS  :
     ComboNotifyParent(w, CBN_SETFOCUS);
     /*
       If we set the focus here, pass the baton onto the edit control.
       Do this only if the listbox portion does not have the focus.
       (The listbox could have gotten the focus if the app dropped
       down the listbox in response to the CBN_SETFOCUS message.)
     */
     if (InternalSysParams.hWndFocus != hListBox)
       SetFocus(hEdit);
     break;

   case WM_KILLFOCUS :
      /*
        If we lose the focus to a non-child, kill the dropdown list.
      */
      if (wParam != hListBox && wParam != hEdit)
      {
        if (bIsDropdown)
          SendMessage(hWnd, CB_SHOWDROPDOWN, FALSE, 0L);
        ComboNotifyParent(w, CBN_KILLFOCUS);
      }
      return StdWindowWinProc(hEdit, message, wParam, lParam);

    case WM_MOUSEACTIVATE :
      WinToTop(hWnd);
      WinToTop(hListBox);
      /*
        (maa) 10/26/90  If we return MA_ACTIVATE, then the dialog box
        caption will be grayed and the current active window will
        remain as NULLHWND
      */
      goto call_dwp;


#ifdef OWNERDRAWN
    case WM_DRAWITEM    :
      /*
        A little code to make sure that when we are drawing the static
        part of an owner-drawn combobox, we do not overwrite the right-hand
        border character.
      */
      if (bDrawingOwnerDrawnStatic)
      {
        LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT) lParam;
        lpDis->rcItem.right -= IGetSystemMetrics(SM_CXBORDER);
      }
      /* fall through... */
    case WM_COMPAREITEM :
    case WM_DELETEITEM  :
    case WM_MEASUREITEM :
      return SendMessage(GetParent(hWnd), message, wParam, lParam);
#endif

    case WM_CTLCOLOR    :
      /*
        Change the pointer to the edit/listbox control to a pointer
        to this combo control. Apps only know about the combo box,
        not its parts.
      */
      lParam = MAKELONG(hWnd, HIWORD(lParam));
      return SendMessage(GetParent(hWnd), message, wParam, lParam);

    case WM_DESTROY :
      /*
        When a dropdown combo box is destroyed, then we must manually
        destroy the listbox, since it is unhooked from the parent,
        and therefore, not processed automatically by DestroyWindow()
      */
      if (bIsDropdown && wListBox != NULL && wListBox->parent == NULL)
      {
        _WinDestroy(hListBox);

#if 0
        /*  
          Commented out 9/14/93 Fred Needham / TeleVoice
          If the parent is NULL then there is nothing to update
        */
        WinUpdateVisMap();
#endif
      }
      break;


    case WM_SETFONT    :
    {
      /*
        WM_SETFONT should set the font of the listbox and the edit/static
        field.
      */
      WINDOW *wEdit = WID_TO_WIN(hEdit);
      wListBox->hFont = wEdit->hFont = (HFONT) wParam;
      goto call_dwp;
    }


#if defined(MEWEL_GUI) || defined(XWINDOWS)
    case WM_ERASEBKGND :
      return TRUE;
#endif

    case WM_ENABLE     :
      /*
        Allow both the edit control and the button to gray
      */
      StdWindowWinProc(hEdit, message, wParam, lParam);
      goto call_dwp;

    case WM_HELP       :
    case WM_NCCALCSIZE :
    case WM_SETCURSOR  :
    case WM_NCHITTEST  :
    case WM_SYSCOMMAND :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);


    case WM_SIZE       :
      /*
        Don't do anything if the new size is same as the current size.
      */
      if ((MWCOORD) LOWORD(lParam) == RECT_WIDTH(w->rect) &&
          (MWCOORD) HIWORD(lParam) == RECT_HEIGHT(w->rect))
        break;


      /*
        In case of a WM_SIZE message, we have to resize the child windows
        of the combobox.
      */
      SetWindowPos(hEdit, NULL, 0, 0, 
                   LOWORD(lParam) - ((w->flags & CBS_DROPDOWN) ?
                                         SM_CXCOMBOICON : 0), 
                   SM_CYCOMBOICON,
                   SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

      SetWindowPos(hListBox, NULL, 0, 0,
                   LOWORD(lParam),
                   (w->flags & CBS_DROPDOWN) ?
                     pCombo->yOrigBottom - pCombo->yAdjustedBottom :
                     HIWORD(lParam) - SM_CYCOMBOICON,
                   SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

      /* we can't set the listbox size to the WM_SIZE height, because
         this height is the height of the *visible* combobox
         and therefore too small. The drawback of this method is that
         the listbox height can't be modified dynamically.
      */
      if (w->flags & CBS_DROPDOWN)
        SetWindowPos(hWnd, NULL, 0, 0,
                     LOWORD(lParam),
                     SM_CYCOMBOICON,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
      break;


    default :
      return StdWindowWinProc(hEdit, message, wParam, lParam);
  }

  return TRUE;
}



/*****************************************************************************/
/*                                                                           */
/*  ComboListBoxWndProc()                                                    */
/*    This is a front-end for the default listbox winproc, and is used       */
/* for dropdown listboxes that are attached to a combobox. It does two       */
/* things:                                                                   */
/*   1) If the ESCAPE key is pressed inside the listbox, the listbox is      */
/* hidden and the focus set to the edit control.                             */
/*   2) If the listbox loses the focus, it is hidden.                        */
/*                                                                           */
/* Note: A small problem exists here.... if we are using a dropdown-list     */
/* combobox (a static control instead of an edit control), and the user      */
/* presses ESCAPE in the listbox, the focus will be set to the static        */
/* control, and the keyboard will seem disabled.                             */
/*                                                                           */
/*****************************************************************************/
LRESULT FAR PASCAL ComboListBoxWndProc(hWnd, message, wParam, lParam)
  HWND hWnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND     hCombo;
  COMBOBOX *pCombo;
  WINDOW   *wLB, *wCombo;

  if ((wLB = WID_TO_WIN(hWnd)) != NULL && (hCombo = wLB->hWndOwner) != NULL)
  {
    wCombo = WID_TO_WIN(hCombo);
    pCombo = (COMBOBOX *) wCombo->pPrivate;

    if ((wCombo->flags & CBS_DROPDOWN))
    {
      if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
      {
        BOOL bDirectionKey = (BOOL)
#if defined(USE_WINDOWS_COMPAT_KEYS)
           (wParam == VK_TAB && !GetKeyState(VK_CONTROL) && !GetKeyState(VK_MENU) || wParam == VK_RETURN);
#else
           (wParam == VK_TAB || wParam == VK_SH_TAB || wParam == VK_RETURN);
#endif

        /*
          If we hit a certain key, then we must set the focus back to the
          edit portion of the combobox.
        */
        if (bDirectionKey || 
            wParam == VK_ESCAPE || wParam == VK_F4 || 
#if defined(USE_WINDOWS_COMPAT_KEYS)
            (wParam == VK_DOWN && GetKeyState(VK_MENU)))
#else
            wParam == VK_ALT_DOWN)
#endif
        {
          if (wParam != VK_ESCAPE)
            SendMessage(hCombo, WM_COMMAND, 0, MAKELONG(hCombo,CBN_SELCHANGE));
          SetFocus(pCombo->hEdit);
          /*
            Send direction keys onto the dialog box manager.
          */
          if (bDirectionKey)
            SendMessage(hCombo, WM_KEYDOWN, wParam, lParam);
        }
      }

      else if (message == WM_LBUTTONDOWN)
      {
        /*
          If we actually clicked the button in the listbox portion
          (as opposed to the phony WM_LBUTTONDOWn message sent at
          the top of this file), then pay attention to the 
          mouse up message.
        */
        bIgnoreMouseUp = FALSE;
      }
      else if (message == WM_MOUSEMOVE && IsMouseLeftButtonDown())
      {
        /*
          If we dragged the mouse in the listbox, then pay attention
          to the mouse up message
        */
        LONG lP2 = _UnWindowizeMouse(hWnd, lParam);
        if (PtInRect(&wLB->rClient, MAKEPOINT(lP2)))
          bIgnoreMouseUp = FALSE;
      }

      else if (message == WM_LBUTTONUP || message == WM_NCLBUTTONUP)
      {
        if (bIgnoreMouseUp)
          return TRUE;
        if (message == WM_LBUTTONUP)
          lParam = _UnWindowizeMouse(hWnd, lParam);
        else /* (message == WM_NCLBUTTONUP) */
          message = WM_LBUTTONUP;

        /*
          If we released the mouse within the listbox, then it's the same
          thing as double-clicking on the item...
        */
        if (PtInRect(&wLB->rClient, MAKEPOINT(lParam)))
        {
         /*
           Windows comboboxes don't send CBN_SELCHANGE if the user is
           scrolling through the listbox portion of a combobox with
           the mouse. We let the combobox know when the mouse button is released.
         */
          SendMessage(hCombo,WM_COMMAND,0,MAKELONG(hCombo, CBN_SELCHANGE));
          SendMessage(hCombo,WM_COMMAND,0,MAKELONG(hWnd,   CBN_DBLCLK));
          SetFocus(pCombo->hEdit);
#ifdef USE_DONTPULLDOWNLB
          bDontPulldownListbox--;  /* use -- here!!! Trick to make it -1 */
          /*
            Don't know why this -1 stuff is here. 
          */
          if (bDontPulldownListbox == (BOOL) -1)
            bDontPulldownListbox = FALSE;
#endif
        }
        /*
          If we released the button on the combo-box icon, then don't
          release the capture from the listbox.
        */
        else
#if defined(MEWEL_GUI) || defined(XWINDOWS)
        if (HIWORD(lParam) >= wCombo->rect.top && 
            HIWORD(lParam) <  wCombo->rect.top + SM_CYCOMBOICON &&
            LOWORD(lParam) >= wCombo->rect.right - SM_CXCOMBOICON &&
            LOWORD(lParam) <  wCombo->rect.right)
#else
        if (HIWORD(lParam) == wCombo->rect.top && 
            LOWORD(lParam) == wCombo->rect.right-1)
#endif
        {
          ReleaseCapture();
          return TRUE;
        }
      }
      else if (message == WM_KILLFOCUS && wParam != hWnd)
      {
        SendMessage(hCombo, CB_SHOWDROPDOWN, FALSE, 0L);
        if (wParam == hCombo || wParam == pCombo->hEdit)
        {
#ifdef USE_DONTPULLDOWNLB
          bDontPulldownListbox++;  /* use ++ here!!! */
#endif
          ComboNotifyParent(wCombo, CBN_SELENDOK);
        }
        else
        {
          ComboNotifyParent(wCombo, CBN_KILLFOCUS);
          ComboNotifyParent(wCombo, CBN_SELENDCANCEL);
        }
      }
      else if (message == WM_GETDLGCODE)
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
    }
  }

#if defined(MOTIF)
  return ListBoxWinProc(hWnd, message, wParam, lParam);
#else
  return DefWindowProc(hWnd, message, wParam, lParam);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  ComboEditWndProc()                                                       */
/*    This is a front-end for the default edit winproc, and is used for the  */
/* edit fields  that are attached to a combobox. It does one thing  :        */
/*   1) If the UP or DOWN arrow keys are pressed, then they are passed to    */
/* the listbox proc.                                                         */
/*                                                                           */
/*****************************************************************************/
LRESULT FAR PASCAL ComboEditWndProc(hWnd, message, wParam, lParam)
  HWND hWnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND     hCombo;
  WINDOW   *wCombo;
  COMBOBOX *pCombo;

  /*
    Get some combo box info
  */
  hCombo = GetParent(hWnd);
  wCombo = WID_TO_WIN(hCombo);
  pCombo = (COMBOBOX *) wCombo->pPrivate;


  if (message == WM_SYSKEYDOWN ||
      message == WM_KEYDOWN    ||
      message == WM_CHAR)
    /*
      If we hit a character in the edit field, then let the combobox
      proc handle it. We need to send it to the combobox window
      proc in case there is subclassing on the combo.
    */
    return SendMessage(hCombo, message, wParam, lParam);

  /*
    If we get a WM_KILLFOCUS, then we must also make sure
    to generate a CBN_KILLFOCUS for the behalf of the parent.
  */
  if (message == WM_KILLFOCUS && wParam != hWnd)
  {
    if (wParam != wCombo->win_id && wParam != pCombo->hListBox)
      ComboNotifyParent(wCombo, CBN_KILLFOCUS);
  }


#ifdef OWNERDRAWN
   /*
     If we get a paint message for an edit field which is part of an
     owner-drawn listbox, then let the regular edit paint routine get
     called, and then call DrawOwnerDrawnStatic to paint the contents
     of the edit control.
   */
   if (message == WM_PAINT)
   {
#if defined(MOTIF)
     EditWinProc(hWnd, message, wParam, lParam);
#else
     DefWindowProc(hWnd, message, wParam, lParam);
#endif
     DrawOwnerDrawnStatic(hWnd);
     return TRUE;
   }

   if (message == WM_DRAWITEM)
      return SendMessage(GetParent(hWnd), message, wParam, lParam);
#endif

call_dwp:
#if defined(MOTIF)
  return EditWinProc(hWnd, message, wParam, lParam);
#else
  return DefWindowProc(hWnd, message, wParam, lParam);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  ComboStaticWndProc()                                                     */
/*    This is a front-end for the default text winproc, and is used for the  */
/* text fields  that are attached to a combobox. It does a few things:       */
/*   1) If the UP or DOWN arrow keys are pressed, then they are passed to    */
/* the listbox proc.                                                         */
/*                                                                           */
/*****************************************************************************/
LRESULT FAR PASCAL ComboStaticWndProc(hWnd, message, wParam, lParam)
  HWND hWnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
{
  switch (message)
  {
    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
    case WM_CHAR       :
    /*
      If we hit a character in the static field, then let the combobox
      proc handle it. We need to send it to the combobox window
      proc in case there is subclassing on the combo.
    */
      goto call_comboproc;

    case WM_NCHITTEST :
      /*
        We can set the focus to this static field
      */
      return HTCLIENT;

    case WM_GETDLGCODE :
      return DLGC_WANTARROWS;

    case WM_SETFOCUS   :
    case WM_KILLFOCUS  :
    {
      /*
        Highlight or unhighlight the static field
      */
#ifdef OWNERDRAWN
      HWND     hCombo;
      WINDOW   *wCombo;
      COMBOBOX *pCombo;

      hCombo  = GetParent(hWnd);
      wCombo  = WID_TO_WIN(hCombo);
      pCombo  = (COMBOBOX *) wCombo->pPrivate;

      if ((wCombo->flags & CBS_OWNERDRAWFIXED))
        _OwnerDrawnComboDraw(pCombo->hListBox, hWnd, 
                   (int) ListBoxWinProc(pCombo->hListBox,LB_GETCURSEL,0,0L));
      else
#endif
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);

      /*
        If we are setting the focus to a different control, send a 
        CBN_KILLFOCUS message to the combobox's parent.
      */
      if (message == WM_KILLFOCUS && wParam != hWnd  &&
          wCombo->parent && wParam != wCombo->win_id && 
          wParam != pCombo->hListBox)
        ComboNotifyParent(wCombo, CBN_KILLFOCUS);

      return TRUE;
    }

#ifdef OWNERDRAWN
    case WM_DRAWITEM    :
    case WM_COMPAREITEM :
    case WM_DELETEITEM  :
    case WM_MEASUREITEM :
      goto call_comboproc;
#endif

    default         :
call_dwp:
#if defined(MOTIF)
      return StaticWinProc(hWnd, message, wParam, lParam);
#else
      return DefWindowProc(hWnd, message, wParam, lParam);
#endif
  }

call_comboproc:
  return SendMessage(GetParent(hWnd), message, wParam, lParam);
}



/****************************************************************************/
/*                                                                          */
/* Function : ComboBoxCreate()                                              */
/*                                                                          */
/* Purpose  : Low-level combobox creation routine                           */
/*                                                                          */
/* Returns  : The handle of the combobox if successful, NULLHWND if not.    */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL ComboBoxCreate(hParent,row1,col1,row2,col2,title,attr,flags,id)
  HWND  hParent;
  int   row1, row2, col1, col2;
  LPSTR title;
  COLOR attr;
  DWORD flags;
  int   id;
{
  HWND     hCombo, hLB;
  WINDOW   *w, *wLB;
  COMBOBOX *pCombo;
  DWORD    fFlags;
  int      rowBottom;
  BYTE     chClass;

  /*
    Take away potentially harmful flags
  */
  flags &= ~(WS_VSCROLL | WS_HSCROLL | WS_BORDER);

  /*
    If the combobox is a dropdown combo, then create the combo box
    with a bottom extent equal to the height of the edit/static control.
  */
  rowBottom = (flags & CBS_DROPDOWN) ? row1+SM_CYCOMBOICON : row2;

  /*
    Create the combo
  */
  hCombo = WinCreate(hParent, row1,col1,rowBottom,col2, NULL,
                     attr, flags | WS_CHILD, COMBO_CLASS, id);
#if defined(MOTIF)
  _XCreateComboBox(hCombo, COMBO_CLASS, NULL, flags | WS_CHILD,
                   col1, row1, col2-col1, rowBottom-row1, 
                   hParent, id, NULL);
#endif

  if (!hCombo || (w = WID_TO_WIN(hCombo)) == (WINDOW *) NULL)
    return (HWND) CB_ERR;

  /*
    Fill in the internal combo structure
  */
  pCombo = (COMBOBOX *) w->pPrivate;
  pCombo->chComboBoxIcon  = WinGetSysChar(SYSCHAR_COMBO_ICON);
  pCombo->yOrigBottom     = row2;
  pCombo->yAdjustedBottom = rowBottom;

  /*
    Create the edit field or static text field. Subtract 1 from col2 so
    we have room for the icon.
  */
  if ((flags & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
  {
    chClass = CT_STATIC;

    pCombo->hEdit =
      _CreateWindow((LPCSTR) &chClass,
                    title,
                    SS_TEXT | WS_CHILD | SS_BORDER,
                    0, 0, 
                    (col2 - col1) - SM_CXCOMBOICON,
#if defined(MOTIF)
                    SM_CYCOMBOICON - 2*IGetSystemMetrics(SM_CXBORDER),
#else
                    SM_CYCOMBOICON,
#endif
                    attr, id,
                    hCombo,
                    NULL,
                    NULL,
                    NULL);
    if (pCombo->hEdit)
      WinSetWinProc(pCombo->hEdit, (WINPROC *) ComboStaticWndProc);
  }
  else
  {
    BOOL bHasIcon = !((flags & CBS_SIMPLE) == CBS_SIMPLE);
    fFlags = (flags & CBS_AUTOHSCROLL) ? ES_AUTOHSCROLL : 0L;
    chClass = CT_EDIT;
    pCombo->hEdit =
      _CreateWindow((LPCSTR) &chClass,
                    title,
                    fFlags | WS_CHILD | WS_BORDER,
                    0, 0,
                    (col2-col1) - (bHasIcon ? SM_CXCOMBOICON : 0),
                    SM_CYCOMBOICON,
                    attr, id,
                    hCombo,
                    NULL,
                    NULL,
                    NULL);
    if (pCombo->hEdit)
      WinSetWinProc(pCombo->hEdit, ComboEditWndProc);
  }
  if (!pCombo->hEdit)
    return (HWND) CB_ERR;

  /*
    Create the listbox control
  */
  fFlags = (flags & CBS_SORT) ? LBS_STANDARD : LBS_NOTIFY;
  fFlags |= (WS_VSCROLL | WS_BORDER);
  if (flags & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
    fFlags |= LBS_OWNERDRAWFIXED;
  if (flags & CBS_HASSTRINGS)
    fFlags |= LBS_HASSTRINGS;


  chClass = CT_LISTBOX;

  hLB = pCombo->hListBox =
   _CreateWindow((LPCSTR) &chClass,
                 NULL, 
                 fFlags | WS_CHILD | ((flags & CBS_DROPDOWN) ? WS_POPUP : 0),
                 0, SM_CYCOMBOICON,
                 (col2-col1),
                 (row2-row1) - SM_CYCOMBOICON,
                 attr, id,
                 hCombo,
                 NULL,
                 NULL,
                 NULL);
  if (!hLB)
    return (HWND) CB_ERR;

  /*
    Send the WM_CREATE so that the ListBoxWinProc can do the initial
    scrollbar processing.
    (Note : _CreateWindow will not send the WM_CREATE message to
     a child of a combo, so we have to do it ourselves.)
  */
  SendMessage(hLB, WM_CREATE, 0, 0L);

  /*
    We need some kind of indicator that this listbox belongs to a combobox,
    cause when the listbox is hidden, we cut it's parent link to the listbox.
  */
  wLB = WID_TO_WIN(hLB);
#if !80994
  if (flags & CBS_DROPDOWN)
#endif
    wLB->ulStyle |= LBS_IN_COMBOBOX;

  /*
    Make the listbox hidden initially if we don't have CBS_SIMPLE
  */
  if (flags & CBS_DROPDOWN)
  {
    SetParent(hLB, NULLHWND);
    WinSetWinProc(hLB, ComboListBoxWndProc);
    _WinShowWindow(wLB, FALSE);
#if defined(MOTIF) && 0
    XtUnmanageChild(wLB->widget);
    XtUnmapWidget(wLB->widget);
#endif
  }
  else
  {
    _WinShowWindow(wLB, TRUE);
#if defined(MOTIF) && 0
    XtManageChild(wLB->widget);
#endif
  }

  return hCombo;
}


static UINT CBtoLBMsg[][2] =
{
  CB_ADDSTRING,      LB_ADDSTRING,
  CB_DELETESTRING,   LB_DELETESTRING,
  CB_DIR,            LB_DIR,
  CB_FINDSTRING,     LB_FINDSTRING,
  CB_FINDSTRINGEXACT,LB_FINDSTRINGEXACT,
  CB_GETCOUNT,       LB_GETCOUNT,
  CB_GETCURSEL,      LB_GETCURSEL,
  CB_GETLBTEXT,      LB_GETTEXT,
  CB_GETLBTEXTLEN,   LB_GETTEXTLEN,
  CB_INSERTSTRING,   LB_INSERTSTRING,
  CB_RESETCONTENT,   LB_RESETCONTENT,
  CB_SELECTSTRING,   LB_SELECTSTRING,
  CB_SETCURSEL,      LB_SETCURSEL,
  CB_SETITEMDATA,    LB_SETITEMDATA,
  CB_GETITEMDATA,    LB_GETITEMDATA,
  WM_SETREDRAW,      WM_SETREDRAW,
  0, 0
};
static UINT PASCAL MapCBtoLB(message)
  UINT message;
{
  int i;
  for (i = 0;  CBtoLBMsg[i][0] != 0 && CBtoLBMsg[i][0] != message;  i++)
    ;
  return (CBtoLBMsg[i][0] == 0) ? message : CBtoLBMsg[i][1];
}

static VOID PASCAL ComboNotifyParent(wCB, notifyCode)
  WINDOW  *wCB;
  INT     notifyCode;
{
  if (wCB->parent)
    SendMessage(wCB->parent->win_id, WM_COMMAND, wCB->idCtrl,
                                     MAKELONG(wCB->win_id, notifyCode));
}

/*
  HwndToPCombo(hWnd)
    Given the handle of a combobox's listbox, static, or edit control,
    returns a pointer to the internal COMBOBOX structure.
*/
static COMBOBOX* PASCAL HwndToPCombo(hWnd)
  HWND hWnd;
{
  WINDOW *wCombo = WID_TO_WIN(GetParent(hWnd));
  return (COMBOBOX *) wCombo->pPrivate;
}


/****************************************************************************/
/*                                                                          */
/*    OWNERDRAWN COMBOBOX STUFF                                             */
/*                                                                          */
/****************************************************************************/
#ifdef OWNERDRAWN
static BOOL PASCAL _OwnerDrawnComboDraw(hListBox, hEdit, iSel)
  HWND hListBox;
  HWND hEdit;
  int  iSel;
{
  DWORD dwFlags = WinGetFlags(hListBox);

  if (IS_OWNERDRAWN(dwFlags))
  {
    DRAWITEMSTRUCT dis;
    HDC    hDC;
    HFONT  hFont, hOldFont;
    LPHDC  lphDC;
    WINDOW *wEdit;
    WINDOW *wLB;
    LIST   *pItem;
    LISTBOX *lbi;

    if (iSel != LB_ERR)
    {
      hDC = GetDC(hEdit);
      if ((hFont = (HFONT) SendMessage(hListBox, WM_GETFONT, 0, 0L)) != NULL)
        hOldFont = SelectObject(hDC, hFont);

      wEdit = WID_TO_WIN(hEdit);
      wLB   = WID_TO_WIN(hListBox);

      _ListBoxSetupDIS(wEdit, &dis, hDC, &lphDC);
      dis.hwndItem = wEdit->hWndOwner;

      lbi = (LISTBOX *) wLB->pPrivate;
      pItem = ListGetNth(lbi->strList, iSel);
      bDrawingOwnerDrawnStatic++;
      _ListBoxDrawOwnerDrawnItem(wLB, 0, 1, 0x00, FALSE,
                                 FALSE, (InternalSysParams.hWndFocus == hEdit), 
                                 pItem, &dis, iSel, TRUE, TRUE);
      bDrawingOwnerDrawnStatic--;
      if (hFont)
        SelectObject(hDC, hOldFont);
      ReleaseDC(hEdit, hDC);
    }
    return TRUE;
  }
  else
    return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : DrawOwnerDrawnStatic()                                        */
/*                                                                          */
/* Purpose  : Renders the static (or edit) portion of an owner-drawn        */
/*            combobox.                                                     */
/*                                                                          */
/* Returns  : TRUE of the owner-drawn item was drawn, FALSE if not.         */
/*                                                                          */
/* Called by: StaticDraw.                                                   */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DrawOwnerDrawnStatic(hStatic)
  HWND hStatic;  /* static or edit control which is part of combo */
{
  HWND hCombo, hListBox;
  INT  iSel;

  /*
    Get the handle to the combo box
  */
  hCombo = GetParent(hStatic);

  /*
    Get a handle to the listbox portion of the combo.
  */
  hListBox = ((COMBOBOX *) (WID_TO_WIN(hCombo)->pPrivate))->hListBox;

  /*
    Let the owner-draw logic handle the drawing.
  */
  iSel = (int) ListBoxWinProc(hListBox, LB_GETCURSEL, 0, 0L);
  return _OwnerDrawnComboDraw(hListBox, hStatic, iSel);
}

#endif


static LRESULT PASCAL ComboEditKbdHandler(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  WINDOW   *w = WID_TO_WIN(hWnd);
  BOOL     bIsEdit;
  HWND     hCombo;
  WINDOW   *wCombo;
  COMBOBOX *pCombo;

  /*
    Get some combo box info
  */
  hCombo = GetParent(hWnd);
  wCombo = WID_TO_WIN(hCombo);
  pCombo = (COMBOBOX *) wCombo->pPrivate;

  /*
    If we get an ALT-DOWN or F4 key, then we pass it onto to combo box.
  */
  if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && 
#if defined(USE_WINDOWS_COMPAT_KEYS)
      ((wParam == VK_DOWN && GetKeyState(VK_MENU))
#else
      (wParam == VK_ALT_DOWN
#endif
       || wParam == VK_F4))
    return SendMessage(hCombo, message, wParam, lParam);

  /*
    Is this window an edit or a static control?
  */
  bIsEdit = (BOOL) (_WinGetLowestClass(w->idClass) == EDIT_CLASS);


  /*
    If we press TAB while in the combo listbox, ignore it.
  */
  if (bIsEdit && (wParam == VK_TAB || wParam == VK_SH_TAB))
    return TRUE;

  /*
    If we press a vertical direction key in the edit or static control, 
    then pass it onto the listbox portion of the combobox. For a static
    control, we also want to pass ASCII characters and the HOME/END
    key onto the listbox.
  */
  if (message == WM_KEYDOWN &&
     (wParam==VK_UP || wParam==VK_DOWN || wParam==VK_PRIOR || wParam==VK_NEXT ||
     (!bIsEdit && (wParam==VK_HOME || wParam==VK_END || wParam>' ' && wParam<127))
     ))
  {
    HWND   hLB = pCombo->hListBox;
    WINDOW *wLB = WID_TO_WIN(hLB);

#ifdef TELEVOICE
    /*
      A TELEVOICE change that sets the selectstring from the edit box.
    */
    {
    char szBuf[80];
    EditWinProc(hWnd, WM_GETTEXT, sizeof(szBuf), (DWORD)(LPSTR) szBuf);
    SendMessage(hLB, LB_SELECTSTRING, (WPARAM) -1, (DWORD)(LPSTR) szBuf);
    }
#endif

    ListBoxWinProc(hLB, message, wParam, lParam);

    /*
      If the listbox is hidden, then we disconnected it from the combo
      box. We have to manually send the CBN_SELCHANGE message.
    */
    if (!wLB->parent)
      SendMessage(hCombo,WM_COMMAND,wLB->idCtrl,MAKELONG(hLB,CBN_SELCHANGE));

    /*
      Highlight the edit text if the edit control is not owner-drawn
    */
    if (!IS_OWNERDRAWN(WinGetFlags(hCombo)))
      EditWinProc(hWnd, EM_SETSEL, 0, MAKELONG(0, 0x7FFF)); 

    return TRUE;
  }

  /*
    Pass the keystroke onto the MEWEL edit or static window proc.
  */
  if (bIsEdit)
    return EditWinProc(hWnd, message, wParam, lParam);
  else
    return StaticWinProc(hWnd, message, wParam, lParam);
}

