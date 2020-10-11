/*===========================================================================*/
/*                                                                           */
/* File    : WSTDPROC.C                                                      */
/*                                                                           */
/* Purpose : StdWindowWinProc()                                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#endif


LRESULT FAR PASCAL StdWindowWinProc(HWND hWnd, UINT message, 
                                    WPARAM wParam, LPARAM lParam)
{
  WINDOW *w = WID_TO_WIN(hWnd);
  INT    mouserow, mousecol, iHitCode;
  INT    iClass;
  RECT   r;
  DWORD  ulFlags;
  COLOR  attr;


  if (!w)
    return TRUE;

  iClass = _WinGetLowestClass(w->idClass);
  ulFlags = w->flags;


  switch (message)
  {
    case WM_SETCURSOR   :
    {
      DWORD dwHwndFlags;

      /*
        wParam is the handle of the window which has the cursor.
        LOWORD(lParam) is the hit test code
        HIWORD(lParam) is the mouse message
      */
      if (TEST_PROGRAM_STATE(STATE_NO_WMSETCURSOR))
        return TRUE;

      /*
        Get the style bits of the window which has the cursor.
      */
      dwHwndFlags = WinGetFlags((HWND) wParam);

      /*
        See if the cursor is on the window's resizing borders. If so, change
        the cursor shape to one of the arrow cursors.
      */
      if (LOWORD(lParam) >= HTLEFT && LOWORD(lParam) <= HTBOTTOMRIGHT &&
          (dwHwndFlags & WS_THICKFRAME) && !(ulFlags & WS_MINIMIZE))
      {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
        static LPCSTR HTtoCursor[] =
        {
          /* HTLEFT       */  IDC_SIZEWE,
          /* HTRIGHT      */  IDC_SIZEWE,
          /* HTTOP        */  IDC_SIZENS,
          /* HTTOPLEFT    */  IDC_SIZENWSE,
          /* HTTOPRIGHT   */  IDC_SIZENESW,
          /* HTBOTTOM     */  IDC_SIZENS,
          /* HTBOTTOMLEFT */  IDC_SIZENESW,
          /* HTBOTTOMRIGHT*/  IDC_SIZENWSE
         };
#if defined(MOTIF)
        if (w->dwXflags & WSX_USEMEWELSHELL)
#endif
        SetCursor(LoadCursor(NULL, HTtoCursor[LOWORD(lParam) - HTLEFT]));
#elif defined(MEWEL_TEXT)
        SetCursor(achSysCursor[LOWORD(lParam) - HTLEFT + 1]);
#endif
        return TRUE;
      }
      /*
        If we have a child window, then pass the WM_SETCURSOR message
        onto the parent window so that the parent can control how the
        cursor looks.
      */
      else if ((ulFlags & WS_CHILD) && w->parent != NULL)
      {
        if (SendMessage(w->parent->win_id,WM_SETCURSOR,wParam,lParam) == TRUE)
          return TRUE;
      }
      else if (LOWORD(lParam) == HTCLIENT)
      {
        HCURSOR hCursor = ClassIDToClassStruct(w->idClass)->hCursor;
        if (hCursor)
          SetCursor(hCursor);
        else
          goto setdefcursor;
      }
      else
      {
setdefcursor:
#if defined(MEWEL_GUI) || defined(XWINDOWS)
        SetCursor(LoadCursor(NULL, IDC_ARROW));
#else
        SetCursor(NORMAL_CURSOR);
#endif
      }
      return TRUE;
    }



#if defined(MEWEL_GUI)
    case WM_BORDER :
      GUIDrawBorder(hWnd, wParam);
      return TRUE;
#endif


    case WM_NCPAINT:
      /*
        WM_NCPAINT is sent when the window's non-client area has to
        be redrawn. This includes the borders, scrollbars, shadows,
        and menu bars.
      */
      if (!IsWindowVisible(hWnd))
        return TRUE;
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      if (IsIconic(hWnd))
        return TRUE;
#endif
      if (HAS_BORDER(ulFlags) || HAS_CAPTION(ulFlags) ||
          WinHasScrollbars(hWnd, SB_BOTH))
#if defined(MOTIF)
        if (w->dwXflags & WSX_USEMEWELSHELL)
#endif
        WinDrawBorder(hWnd, 
                      (w->ulStyle & WIN_ACTIVE_BORDER) != 0 ||
                      InternalSysParams.hWndFocus == hWnd   ||
                      GetActiveWindow() == hWnd);
#if defined(MEWEL_TEXT)
      if (ulFlags & WS_SHADOW)
        WinDrawShadow(hWnd);
#endif
      if (w->hMenu)
        InternalDrawMenuBar(w->hMenu);
      break;


    case WM_PAINT     :
    case WM_PAINTICON :  /* not used in Windows 3.1 */
    {
      PAINTSTRUCT ps;
      if (!IsWindowVisible(hWnd))
        return TRUE;

      /*
        Start the paint processing. This will possibly result in a
        WM_ERASEBACKGROUND message being sent, where in that case,
        ps.fErase will be set to true.
      */
      BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
      /*
        Draw a bitmap icon if a window is minimized
      */
      if (message == WM_PAINTICON || (ulFlags & WS_MINIMIZE))
      {
        HICON hIcon;
        LPEXTWNDCLASS pCl = ClassIDToClassStruct(w->idClass);
#if defined(XWINDOWS)
        if (w->dwXflags & WSX_USEMEWELSHELL)
#endif
        if ((hIcon = w->hIcon) != NULL || (hIcon = pCl->hIcon) != NULL)
          DrawIcon(ps.hdc, 0, 0, hIcon);
      }
#endif

      /*
        We might need to generate the WM_ERASEBKGND message ourself if 
        BeginPaint() didn't do it.
      */
      if (ps.fErase == FALSE)
        SendMessage(hWnd, 
                    (ulFlags & WS_MINIMIZE) ? WM_ICONERASEBKGND : WM_ERASEBKGND,
                    ps.hdc, 0L);

      /*
        Finish up the painting
      */
      EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
      break;
    }
      

    case WM_ERASEBKGND     :
    case WM_ICONERASEBKGND :
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      GUIEraseBackground(hWnd, (HDC) wParam);
#else
      /*
        If wParam != 0, then it is a handle to a display context. We
          can extract the color attribute from there, and we can get
          the rectangle to erase from the window's rUpdate rectangle.
          This was sent by BeginPaint().
      */
      if ((attr = w->attr) == SYSTEM_COLOR)
        attr = WinGetClassBrush(hWnd);
      if (attr == 0)  /* don't allow same bk and fg, cause mouse disappears */
        attr = 7;
      /*
        If an HDC was passed, then there is a valid update rectangle
        in the window's rUpdate structure. We get this, clip it to
        the client area, and use this new rectangle as the one to
        fill.
      */
      r = w->rUpdate;                   /* rFill is in client coordinates */
      WinClientRectToScreen(hWnd, &r);  /* Now in screen coord */
      IntersectRect(&r, &r, &w->rClient);
      WinFillRect(hWnd, (HDC) 0, &r, w->fillchar, attr);
#endif
      return TRUE;


    case WM_ACTIVATE :
      /*
        If we are activating a window, then also set the input focus to
        that window.
      */
      if (wParam)
      {
        SetFocus(hWnd);
        /*
          If the user explicitly sent a WM_ACTIVATE message to us, we must
          set the active window ourselves.
        */
        if (wParam == 1 &&    /* ...not done through a mouse click */
            (GetActiveWindow() != hWnd || !IsChild(GetActiveWindow(), hWnd)))
        {
           /*
             Search the ancestors for a window to activate. Set 'w' to
             the newly active window.
           */
           HWND hWndRoot = _WinGetRootWindow(hWnd);
           if (hWndRoot != NULL)
           {
             InternalSysParams.hWndActive = hWndRoot;
             w = WID_TO_WIN(hWndRoot);  /* for the benefit of WM_NCACTIVATE */
             goto lbl_ncactivate;       /* fall through & redraw caption */
           }
        }
      }
      break;


    /*
      WM_NCACTIVE is sent when a window's non-client area should be shown
      in the active or inactive state.
    */
    case WM_NCACTIVATE :
lbl_ncactivate:
      /*
        Set or clear the window's WIN_ACTIVE_BORDER style, depending
        on if we are activating or decactivating.
      */
      if (wParam)
      {
        if (iClass == NORMAL_CLASS || (ulFlags & WS_POPUP))
          w->ulStyle |= WIN_ACTIVE_BORDER;
      }
      else
        w->ulStyle &= ~WIN_ACTIVE_BORDER;
      goto invalidate_ncarea;


    case WM_MOUSEACTIVATE :
    {
      WINDOW *wParent;

      if (!IsWindowEnabled(hWnd) || (w->ulStyle & LBS_IN_COMBOBOX))
        return MA_NOACTIVATE;

      wParent = w->parent;

#if 91692
      /*
        In Windows, we should set the active window to a top-level
        window, a popup window, or an MDI document window.

        Note - the code after the #else breaks in zApp when a pane
        window is the child of an MDI doc window. The WM_MDIACTIVATE
        will never get sent because the pane (a subclass of NORMAL_CLASS)
        gobbles up the activation message.
      */
      if (wParent == (WINDOW *) NULL || wParent == InternalSysParams.wDesktop ||
          (ulFlags & WS_POPUP) || IS_MDIDOC(w))
#else
      if (iClass == NORMAL_CLASS || (ulFlags & WS_POPUP))
#endif
      { /* gets the activation status. */
        SetActiveWindow(hWnd);
        return MA_ACTIVATEANDEAT;
      }
      if (wParent != (WINDOW *) NULL && wParent != InternalSysParams.wDesktop)
      {
        /*
          If the titlebar window intercepted the WM_MOUSEACTIVATE and
          returned MA_ACTIVATEANDEAT, we still must set it active.
        */
        LONG rc = SendMessage(wParent->win_id,WM_MOUSEACTIVATE,wParam,lParam);
        if (rc == MA_ACTIVATEANDEAT && 
            (_WinGetLowestClass(wParent->idClass) == NORMAL_CLASS || 
            (wParent->flags & WS_POPUP)))
          SetActiveWindow(wParent->win_id);
        return rc;
      }
      return MA_ACTIVATE;
    }


    case WM_SETFOCUS :
    case WM_KILLFOCUS:
     /*
       Don't redraw the border for dialog boxes
     */
     if (IS_DIALOG(w))
       break;

     /*
       Don't redraw the border if setting focus to our menus.
     */
     if (message == WM_KILLFOCUS && (w->hMenu || w->hSysMenu))
       if (wParam == w->hMenu || wParam == w->hSysMenu)
         break;

     /*
       Redraw the window's borders to show that it has the focus.
     */
#if defined(MEWEL_GUI)
     /*
       Don't refresh the entire non-client area if the active window
       is getting the focus (ie - when we kill the focus from an
       active window's menubar or pulldown menu).
     */
     if (GetActiveWindow() == hWnd)
       break;
#endif
     goto invalidate_ncarea;


    case WM_SETTEXT :
      /*
        Release the memory associated with the new text, and allocate memory
        for the new text. Translate any tildes in the title.
      */
      if (w->title)
        MYFREE_FAR(w->title);
      if ((w->title = lstrsave((LPSTR) lParam)) != NULL)
      {
        /*
          We do not want to translate the text of a static control which
          is part of a dropdown-list combobox. We also do not want to
          translate a static control which has SS_NOPREFIX.
        */
        if (w->parent->idClass != COMBO_CLASS &&
            !(IsStaticClass(hWnd) && (ulFlags & SS_NOPREFIX)))
          _TranslatePrefix(w->title);
      }

#if defined(MOTIF)
      XMEWELOnWMSETTEXT(hWnd, w->title);
#endif

      /*
        Update a static text control right away, since an app might
        be constantly updating a status message.
        For a control window, we can probably get away with redrawing 
        the client area.
      */
#if !defined(USE_NATIVE_GUI)
      if (iClass != NORMAL_CLASS)
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
      if ((iClass == TEXT_CLASS   || 
           iClass == STATIC_CLASS && (ulFlags & SS_TEXT)) &&
           w->parent->idClass != COMBO_CLASS
         )
        UpdateWindow(hWnd);
#endif

invalidate_ncarea:
      InvalidateNCArea(hWnd);
      break;


    case WM_GETTEXT :
      if (w->title)
      {
        INT iLen;
        if (!wParam)
          return 0;
        lstrncpy((LPSTR) lParam, w->title, wParam);
        iLen = lstrlen(w->title);
        return iLen;
      }
      else
        return 0;

    case WM_GETTEXTLENGTH :
      return (w->title) ? lstrlen(w->title) : 0;

    /*
      MEWEL-specific messages used to set and query the control ID, and to
      set and query the window flags.
    */
    case WM_GETID :
      return w->idCtrl;
    case WM_SETID :
      return w->idCtrl = wParam;
    case WM_GETFLAGS :
      return WinGetFlags(hWnd);
    case WM_SETFLAGS :
      return w->flags = lParam;

    case WM_ENABLE   :
#if defined(MOTIF)
      if (w->widget)
        XtVaSetValues(w->widget, XmNsensitive, wParam ? TRUE : FALSE, NULL);
#endif
      /*
        If we have a control window, invalidate it so it can be refreshed
        in the "grayed" colors.
      */
      if (iClass >= BUTTON_CLASS && iClass <= COMBO_CLASS)
      {
#if defined(MEWEL_GUI)
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
#else
        /*
          For text mode, we want to redraw the entire contents of the
          field, not just the border.
        */
        InvalidateRect(hWnd, (LPRECT) NULL, TRUE);

        /*  
         * Allow another WM_CTLCOLOR message to be sent to get the gray color
         */
        if (w->ulStyle & WIN_DIDDLED_COLOR)	/* 6/30/93 CFN */
        {										/* 6/30/93 CFN */
          w->ulStyle &= ~WIN_DIDDLED_COLOR;     /* 6/30/93 CFN */
          w->attr = SYSTEM_COLOR;               /* 6/30/93 CFN */
        }										/* 6/30/93 CFN */
#endif
        goto invalidate_ncarea;
      }
      break;

    case WM_CLOSE :
      DestroyWindow(hWnd);
      return TRUE;

    case WM_HELP :
      /*
        Pass WM_HELP messages up to parent window
      */
      if (w->parent)
        PostMessage(w->parent->win_id, message, wParam, lParam);
      break;


    case WM_NCHITTEST   :
      return DoNCHitTest(hWnd, MAKEPOINT(lParam));


    case WM_NCLBUTTONDOWN :
      /*
        The coordinates are screen-based.
      */
      mouserow = HIWORD(lParam);
      mousecol = LOWORD(lParam);

      /*
        Since all mouse messages are directed to the system modal dialog
        box (ie - a message box), then we want to make sure that the
        mouse was clicked within the boundaries of the window.
      */
      if (hWnd == InternalSysParams.hWndSysModal && 
          !PtInRect(&w->rect, MAKEPOINT(lParam)))
         return FALSE;

      /*
        If we are using WM_NCmouse messages like we should be doing,
        then the hit-code will come along for free in the wParam value.
      */
      iHitCode = wParam;

#if defined(MEWEL_GUI) || defined(XWINDOWS)
      /*
        For a GUI icon, drag the icon to a new location
      */
      if (IsIconic(hWnd))
        return WinRubberband(hWnd, WM_MOVE, mouserow, mousecol, 0);
#endif

      /*
        Check for a mouse-button down on the top row of a window.
        It could mean that we clicked on one of the system icons or
        that we want to drag the window.
      */
      if (iHitCode != HTCLIENT && iHitCode != HTERROR && iHitCode != HTNOWHERE)
      {
        /*
          MS Windows brings a window to the top if you click on its caption
          bar. However, it *does not* automatically bring it to the top if
          you click in its client area.
        */
        {
        HWND hWndTemp;
        WINDOW *wTemp;

        /*
          Do not call BringWindowToTop on a child control window. Instead,
          climb up the ancestor tree until we get to the first overlapped
          or popup window.
        */
        for (hWndTemp = hWnd;  hWndTemp;  hWndTemp = GetParent(hWndTemp))
        {
          wTemp = WID_TO_WIN(hWndTemp);
          if (IS_MDIDOC(wTemp) ||
              (wTemp->flags & (WS_OVERLAPPED|WS_POPUP|WS_CHILD)) != WS_CHILD)
            break;
        }
        if (hWndTemp)
          BringWindowToTop(hWndTemp);
        }

        /*
          System menu?
        */
        if (iHitCode == HTSYSMENU)
          return WinActivateSysMenu(hWnd);

        /*
          Maximize?
        */
        if (iHitCode == HTMAXBUTTON)
        {
#if defined(MEWEL_GUI)
          if (WinTrackSysIcon(hWnd, HTMAXBUTTON))
#endif
          return SendMessage(hWnd, WM_SYSCOMMAND, 
                             (ulFlags & (WS_MINIMIZE | WS_MAXIMIZE))
                             ? SC_RESTORE : SC_MAXIMIZE,
                             0L);
        }

        /*
          Minimize?
        */
        if (iHitCode == HTMINBUTTON)
        {
#if defined(MEWEL_GUI)
          if (WinTrackSysIcon(hWnd, HTMINBUTTON))
#endif
          return SendMessage(hWnd, WM_SYSCOMMAND, 
                             (ulFlags&WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE,
                             0L);
        }


        /*
          Drag to a new position?
          Only if the window has a caption bar and is not zoomed, or
          if the window is minimized.
        */
        if (iHitCode == HTCAPTION && !(ulFlags & WS_MAXIMIZE))
        {
          /*
            We don't want a mouse click on the titlebar to invoke
            the rubberbanding routine, especially on slow machines.
            So, let's test the mouse to see if the left button is still
            down.
          */
#if defined(XWINDOWS)
          if (XMEWELIsLeftButtonUp())
#else
          if (!IsMouseLeftButtonDown())
#endif
            return TRUE;
          return WinRubberband(hWnd, WM_MOVE, mouserow, mousecol, 0);
        }

        /*
          Did we press the mouse on any of the resize borders? If so, 
          drag the window to a new size.
        */
        if (iHitCode >= HTLEFT && iHitCode <= HTBOTTOMRIGHT &&
            (ulFlags & WS_THICKFRAME) &&
            !(ulFlags & (WS_MINIMIZE | WS_MAXIMIZE)))
          return WinRubberband(hWnd, WM_SIZE, mouserow, mousecol, iHitCode);
      }
      break;


    case WM_NCLBUTTONDBLCLK :
      /*
        If we are using WM_NCmouse messages like we should be doing,
        then the hit-code will come along for free in the wParam value.
      */
      iHitCode = wParam;

      /*
        If we double-clicked on a system menu, then post a
        WM_SYSCOMMAND/SC_CLOSE message
      */
      if (iHitCode == HTSYSMENU || iHitCode == HTCAPTION)
      {
        if (iHitCode == HTSYSMENU)
        {
          UINT wState;
          wParam = SC_CLOSE;
          /*
            We can only post the SC_CLOSE if the system menu has a
            "Close" item and if the Close item is not disabled.
          */
          wState = GetMenuState(w->hSysMenu, SC_CLOSE, MF_BYCOMMAND);
          if (wState == (UINT) -1 || (wState & MF_DISABLED))
            break;
        }
        else /* HTCAPTION */
        {
          if (IsZoomed(hWnd) || IsIconic(hWnd))
            wParam = SC_RESTORE;
          else if (ulFlags & WS_MAXIMIZEBOX)
            wParam = SC_MAXIMIZE;
          else
            break;
        }
        SendMessage(hWnd, WM_SYSCOMMAND, wParam, 0L);
      }
      break;


    case WM_SYSCOMMAND :
    {
      /*
        If this window is a control window, then pass the WM_SYSCOMMAND
        message up the parent tree.
      */
      if (iClass != NORMAL_CLASS && iClass != DIALOG_CLASS)
        return SendMessage(GetParent(hWnd), WM_SYSCOMMAND, wParam, lParam);

      switch (wParam)
      {
        case SC_CLOSE     :
        {
          LONG rc;

          /*
            If we are closing the only top-level, main window, then first
            ask the app if they want to terminate the session. If
            so, then close the window. If the close was successful,
            then post the WM_QUIT message.
          */
          HWND hOwner = w->hWndOwner;
          if ((hOwner == _HwndDesktop || hOwner == NULL) && !IS_DIALOG(w))
          {
            /*
              Search for another top-level window which is not an
              icon title. If one exists, just close the window and
              don't exit the session.
            */
            WINDOW *wTop;
            for (wTop = InternalSysParams.wDesktop->children;  wTop;  wTop = wTop->sibling)
            {
              if (wTop != w)
              {
                hOwner = wTop->hWndOwner;
                if (hOwner == _HwndDesktop || hOwner == NULL)
                {
                  rc = SendMessage(hWnd, WM_CLOSE, 0, 0L);
                  if (rc == FALSE && !IsWindow(hWnd))
                    rc = TRUE;
                  return rc;
                }
              }
            }

            if (SendMessage(hWnd, WM_QUERYENDSESSION, 0, 0L) == TRUE)
            {
              /*
                The window is considered closed if the WM_CLOSE message
                returns TRUE or if (as in MFC 2.5) the app closed the
                window itself and returned FALSE.
              */
              rc = SendMessage(hWnd, WM_CLOSE, 0, 0L);
              if (rc == TRUE || !IsWindow(hWnd))
              {
                /*
                  Post a WM_QUIT message to NULLHWND because if we
                  closed the only top-level window, then PostQuitMessage()
                  will do nothing!
                */
postquit:
                PostMessage(NULLHWND, WM_QUIT, 0, 0L);
                PostQuitMessage(0);
              }
            }
          }
          else
          {
            /*
              We are closing a window which is not the main window...
            */
            SendMessage(hWnd, WM_CLOSE, 0, 0L);
            if (InternalSysParams.wDesktop->children == NULL)  /* closing only window? */
              goto postquit;
            break;
          }
          break;
        }
        case SC_RESTORE   :
          ShowWindow(hWnd, SW_RESTORE);
          break;
        case SC_MAXIMIZE  :
          ShowWindow(hWnd, SW_MAXIMIZE);
          break;
        case SC_MINIMIZE  :
          ShowWindow(hWnd, SW_MINIMIZE);
          break;
        case SC_MOVE      :
        case SC_SIZE      :
          WinRubberband(hWnd, (wParam==SC_MOVE) ? WM_MOVE : WM_SIZE, 
                        0, w->rect.left, HTBOTTOMRIGHT);
          break;
        case SC_KEYMENU   :
          if (LOWORD(lParam) == ' ')
            return _WinFindandInvokeSysMenu(hWnd);
          else
          {
            INT altKey = LetterToAltKey(LOWORD(lParam));
            if (altKey)
              PostMessage(hWnd, WM_KEYDOWN, altKey, 0L);
          }
          break;
        case SC_NEXTWINDOW :
        case SC_PREVWINDOW :
        {
          /*
            This is sent when the (SHIFT)CTRL-F6 or (SHIFT)CTRL-TAB keys
            are pressed within a window. This is the signal to move to
            the next or previous top-level window.
          */
          BOOL bNext = (BOOL) (wParam == SC_NEXTWINDOW);
          HWND hRoot;

          /*
            If this window is a control window or something, search
            the ancestor tree until we find a "top-level" window or
            dialog box.
          */
          if ((hRoot = _WinGetRootWindow(hWnd)) == NULL)
            break;

          /*
            We must put this window at the bottom of the sibling list
            so we can cycle properly.
          */
          if (bNext)
            SetWindowPos(hRoot, (HWND) 1, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

          /*
            If we are cycling to the next window, get the first window in
            the parent's child list. If we are cycling to the previous
            window, get the last window.
          */
          hWnd = GetWindow(hRoot, (bNext) ? GW_HWNDFIRST : GW_HWNDLAST);

          /*
            Go down the sibling list in order to find the first window
            which we can activate.
          */
          while (hWnd != NULL)
          {
            /*
              Make sure that we activate a window which is visible,
              enabled, and not a static window.
            */
            if (IsWindowVisible(hWnd) && IsWindowEnabled(hWnd) &&
                !IsStaticClass(hWnd))
            {
              SetActiveWindow(hWnd);
              break;
            }
            hWnd = GetWindow(hWnd, (bNext) ? GW_HWNDNEXT : GW_HWNDPREV);
          }
          break;
        }
      }
      break;
    }

    case WM_SYSCOLORCHANGE  :
      /*
        For a color change, we just cause this window to be redrawn.
      */
      InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
      goto invalidate_ncarea;

    case WM_QUERYOPEN       :
    case WM_QUERYENDSESSION :
    case WM_NCCREATE        :
      return TRUE;

    case WM_VALIDATE        :  /* a window is valid by default */
      return TRUE;

    case WM_COMMAND         :
      if (HIWORD(lParam) == FN_ERRVALIDATE)
        MessageBeep(0);
      return FALSE;

    case WM_NCCALCSIZE      :
      WinSetClientRect(hWnd, (LPRECT) lParam);
      break;

    case WM_CTLCOLOR :
    {
      /*
        If the code indicates that we are dealing with a standard
        MEWEL control, then search the system color table for the
        proper color of the control. If we are dealing with a
        custom control, just return the white brush.
      */
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      HWND hCtrl = LOWORD(lParam);
      DWORD flCtrl = WinGetFlags(hCtrl);

      if (HIWORD(lParam) == CTLCOLOR_BTN)
      {
        INT idxFace;

        SetTextColor((HDC) wParam,
          GetSysColor(((flCtrl & WS_DISABLED) != 0L) ? COLOR_GRAYTEXT
                                                     : COLOR_BTNTEXT));
#if 0
        idxFace = ((flCtrl & BS_INVERTED) != 0L) ? COLOR_BTNHIGHLIGHT
                                                 : COLOR_BTNFACE;
#else
        iClass = _WinGetLowestClass(WID_TO_WIN(hCtrl)->idClass);
        if (iClass == CHECKBOX_CLASS || iClass == RADIOBUTTON_CLASS)
          idxFace = COLOR_WINDOW;
        else
          idxFace = COLOR_BTNFACE;
#endif
        SetBkColor((HDC) wParam, GetSysColor(idxFace));
        return SysBrush[idxFace];
      }
      else
      {
        SetTextColor((HDC) wParam,
          GetSysColor(((flCtrl & WS_DISABLED) != 0L) ? COLOR_GRAYTEXT
                                                     : COLOR_WINDOWTEXT));
        SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
        return SysBrush[COLOR_WINDOW];
      }
#else
      if (HIWORD(lParam) <= CTLCOLOR_STATIC)
      {
        COLOR clr;
        clr = WinGetClassBrush((HWND) LOWORD(lParam));
        SetTextColor((HDC) wParam, (DWORD) GET_FOREGROUND(clr));
        SetBkColor((HDC) wParam, (DWORD) GET_BACKGROUND(clr));
        return SysCreateSolidBrush(AttrToRGB(GET_BACKGROUND(clr)));
      }
      else
      {
        SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
        return GetStockObject(WHITE_BRUSH);
      }
#endif
    }

    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
      /*
        Take care of a situation we encounter in Case:W. If a menu is
        currently activated and the RETURN key is posted to the
        owner of the menu, then pass that key onto the menu itself.
        Case:W does this in the code which implements context
        sensitive help for menu items.
      */
      if (wParam == VK_RETURN && InternalSysParams.hWndFocus && 
          IS_MENU(WID_TO_WIN(InternalSysParams.hWndFocus)))
      {
        PostMessage(InternalSysParams.hWndFocus, message, VK_RETURN, lParam);
      }
      /*
        Return FALSE so that IsDialogMessage can pass on the keystroke.
      */
      return FALSE;


    case WM_VKEYTOITEM :
    case WM_CHARTOITEM :
      return -1;

    case WM_GETFONT    :
      return w->hFont;

    case WM_SETFONT    :
      w->hFont = (HFONT) wParam;
      if (LOWORD(lParam))
        InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
      return FALSE;

    default :
      return FALSE;
  }
  return TRUE;
}

