/*===========================================================================*/
/*                                                                           */
/* File    : WINCREAT.C                                                      */
/*                                                                           */
/* Purpose : CreateWindow()                                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static DWORD _CurrExtStyle = 0L;


HWND FAR PASCAL _CreateWindow(szClass, szWindow, dwStyle,
                         x, y, width, height,
                         attr,  /* <- different from MS-WIndows */
                         id,    /* <- different from MS-WIndows */
                         hParent, hMenu, hInst,
                         lpParam)
  LPCSTR  szClass;
  LPCSTR  szWindow;
  DWORD   dwStyle;
  int     x, y;
  int     width, height;
  COLOR   attr;  /* <- different from MS-Windows */
  UINT    id;    /* <- different from MS-Windows */
  HWND    hParent;
  HMENU   hMenu;
  HINSTANCE hInst;
  VOID FAR *lpParam;
{
  HWND     hWnd;
  HWND     hOwner;
  int      idClass;
  UINT     x2;
  UINT     y2;
  int      iUserClass = 0;
  LPEXTWNDCLASS pCl;
  WINDOW   *w;
  RECT     rClient;
  BOOL     bIsTopLevel;

  if ((idClass = ClassNameToClassID(szClass)) < 0)
    return NULLHWND;

#if defined(USE_SYSCOLOR)
  if (attr == SYSTEM_COLOR)
    dwStyle |= WS_SYSCOLOR;
#endif

  if (szWindow == NULL)
    szWindow = (PSTR) "";

  if ((hOwner = hParent) == NULLHWND || !(dwStyle & WS_CHILD))
  {
    hParent = _HwndDesktop;
  }

  /*
    See if the window is a top-level window.
    For strict Windows compatibility :
      1) A top level window must have a caption
      2) A non-top-level window cannot have a menu

     The test 'hParent == _HwndDesktop && !(dwStyle & WS_CHILD)' prevents
     icon titles from being considered top-level windows.
  */
  bIsTopLevel = (hParent == _HwndDesktop && !(dwStyle & WS_CHILD) ||
             (dwStyle & (WS_OVERLAPPED | WS_POPUP | WS_CHILD)) != WS_CHILD);
                  
  if (bIsTopLevel)
  {
    dwStyle |= WS_CLIPSIBLINGS;
    if (TestWindowsCompatibility(WC_MAXCOMPATIBILITY) == WC_MAXCOMPATIBILITY)
      /* 
        Force a caption on overlapped windows, but not if we are creating
        the desktop window.
      */
      if (!(dwStyle & WS_POPUP) && _HwndDesktop)
        dwStyle |= WS_CAPTION;
  }


  /*
    MS Windows compatibility. If we create a child window with (0,0,0,0),
    then use the parent's client area.
  */
  if (hParent)
    WinGetClient(hParent, &rClient);
  else
    rClient = SysGDIInfo.rectScreen;
    
  if ((dwStyle & WS_CHILD) && !x && !y && !width && !height && hParent)
    x = width = CW_USEDEFAULT;
  else if (IS_CW_USEDEFAULT(x) && IS_CW_USEDEFAULT(y)) /* for AFX */
    width = CW_USEDEFAULT;

  /*
    MS Windows compatibility. Map CW_USEDEFAULT into valid numbers.
  */
  if (IS_CW_USEDEFAULT(x))
  {
#if defined(MOTIF) && 1
    if (hParent == _HwndDesktop)
    {
      x = rand() % (VideoInfo.width  >> 2);
      y = rand() % (VideoInfo.length >> 2);
    }
    else
#endif
    {
    x = rClient.left;
    y = rClient.top;  
    }
  }
  else
  /*
    Not CW_USEDEFAULT. If it's a child window, xlate client to screen coords.
  */
  if ((dwStyle & WS_CHILD) && hParent)
  {
    x += rClient.left;
    y += rClient.top;
  }

  if (IS_CW_USEDEFAULT(width))
  {
#if defined(MOTIF) && 1
    /*
      Make a Motif top-level window about 2/3 the size of the screen.
    */
    if (hParent == _HwndDesktop)
    {
      width  = min(((VideoInfo.width*3)/4), (VideoInfo.width-x));
      height = min(((VideoInfo.length*2)/3),    (VideoInfo.length-y));
    }
    else
#endif
    {
    width  = RECT_WIDTH(rClient);
    height = RECT_HEIGHT(rClient);
    }
  }

  x2 = x + width;
  y2 = y + height;


  /*
    Handle the tilde characters, but only if we don't have a static
    control with SS_NOPREFIX on.
  */
  if (!((dwStyle & SS_NOPREFIX) && 
       (idClass == TEXT_CLASS || idClass == STATIC_CLASS)))
    _TranslatePrefix((LPSTR) szWindow);

/*
  Windows sends a WM_GETMINMAXINFO message sometime during the creation
    process.
  SendMessage(hWnd, WM_GETMINMAXINFO, 0, (DWORD) (LPSTR) rMinMax);
*/

start:
  switch (idClass)
  {
    case NORMAL_CLASS :
    case DIALOG_CLASS :
      hWnd = WinCreate(hParent, y,x,y2,x2, (LPSTR) szWindow, 
                       attr, dwStyle, idClass, id);

      /*
        Set the bit indicating that a dialog box is really a dialog box.
        We need to do this *before* any messages are sent to DefDlgProc
        in order to distinguish the dlgbox from a window trying to act like
        a dialog box.
      */
      if (idClass == DIALOG_CLASS && hWnd)
        WID_TO_WIN(hWnd)->ulStyle |= WIN_IS_DLG;

#if defined(MOTIF)
      if (!_XCreateNormalWindow(hWnd, iUserClass, szWindow, dwStyle, 
                                x, y, width, height,
                                hParent, hMenu, lpParam, hInst))
        return hWnd;
#endif

      /*
        If a menu handle was passed in, then set the window's menu.
        (If we have max compatibility on, then this can't be done to
        child windows.)
      */
      if (bIsTopLevel || 
          TestWindowsCompatibility(WC_MAXCOMPATIBILITY) != WC_MAXCOMPATIBILITY)
      {
#if !defined(MOTIF)
        if (hMenu)
          SetMenu(hWnd, hMenu);
#endif
      }

      break;

    case EDIT_CLASS :
      hWnd = EditCreate(hParent, y,x,y2,x2, (LPSTR)szWindow, attr, dwStyle, id);
#if defined(MOTIF)
      _XCreateEdit(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                   hParent, id, lpParam);
#endif
      break;

    case BUTTON_CLASS     :
    {
      DWORD dw = dwStyle & 0x0F;

      if (dw == BS_CHECKBOX || dw == BS_AUTOCHECKBOX)
      {
        idClass = CHECKBOX_CLASS;
      }
      else if (dw == BS_3STATE || dw == BS_AUTO3STATE)
      {
        idClass = CHECKBOX_CLASS;
      }
      else if (dw == BS_RADIOBUTTON || dw == BS_AUTORADIOBUTTON)
      {
        idClass = RADIOBUTTON_CLASS;
      }
      else if (dw == BS_GROUPBOX)
      {
        idClass = FRAME_CLASS;
        dwStyle = (dwStyle & ~0x0FL);
      }
      else
      {
        idClass = PUSHBUTTON_CLASS;
      }
      goto start;
    }

    case PUSHBUTTON_CLASS :
      hWnd = PushButtonCreate(hParent, y,x,y2,x2, (LPSTR)szWindow, 
                              attr,dwStyle,id);
#if defined(MOTIF)
      _XCreateButton(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                     hParent, id, lpParam);
#endif
      break;
    case CHECKBOX_CLASS   :
      hWnd = CheckBoxCreate(hParent, y,x, height, width, (LPSTR) szWindow,
                            attr,dwStyle,id);
#if defined(MOTIF)
      _XCreateButton(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                     hParent, id, lpParam);
#endif
      break;
    case RADIOBUTTON_CLASS:
      hWnd = RadioButtonCreate(hParent, y,x, height, width, (LPSTR)szWindow,
                               attr,dwStyle,id);
#if defined(MOTIF)
      _XCreateButton(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                     hParent, id, lpParam);
#endif
      break;

    case LISTBOX_CLASS :
#if defined(MOTIF)
      if (!(XSysParams.ulOptions & XOPT_OWNERDRAWNLB))
        dwStyle &= ~(LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE);
#endif
      hWnd = ListBoxCreate(hParent, y,x,y2,x2, (LPSTR)szWindow,attr,dwStyle,id);
#if defined(MOTIF)
      _XCreateListBox(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                   hParent, id, lpParam);
#endif
      break;
    case COMBO_CLASS   :
      hWnd = ComboBoxCreate(hParent, y,x,y2,x2, (LPSTR)szWindow,attr,dwStyle,id);
      break;

    case SCROLLBAR_CLASS :
      /*
        NOTE - we should really pass coordinates to the scrollbar create proc
      */
      hWnd = ScrollBarCreate(hParent, y,x,y2,x2, attr, dwStyle | SBS_CTL, id);
#if defined(MOTIF)
      _XCreateScrollbar(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                   hParent, id, lpParam);
#endif
      break;

    case STATIC_CLASS :
      if ((dwStyle & (SS_FRAME | SS_BOX)) || (dwStyle & SS_ICON) == SS_ICON)
        goto stcreate;
      else /* if (dwStyle & SS_TEXT) */
      {
        dwStyle |= SS_TEXT;
        goto stcreate;
      }

    case FRAME_CLASS  :
      dwStyle |= SS_FRAME;
      goto stcreate;
    case BOX_CLASS    :
      dwStyle |= SS_BOX;
      goto stcreate;
    case ICON_CLASS   :
      dwStyle |= SS_ICON;
      goto stcreate;
    case TEXT_CLASS   :
      dwStyle |= SS_TEXT;
stcreate:
      hWnd = StaticCreate(hParent, y,x,y2,x2, (LPSTR)szWindow,attr,dwStyle,id,hInst);
#if defined(MOTIF)
      if (idClass == FRAME_CLASS)
      _XCreateGroupBox(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                   hParent, id, lpParam);
      else
      _XCreateText(hWnd, idClass, szWindow, dwStyle, x, y, width, height,
                   hParent, id, lpParam);
#endif
      break;

    default :
    {
      int  iBaseClass ;

      /*
        Create a window of the base-class type
      */
      if ((iBaseClass = ClassIDToClassStruct(idClass)->idBaseClass) >= 0)
      {
        if (iUserClass == 0)     /* Record the original class id */
          iUserClass = idClass;
        idClass = iBaseClass;
        goto start;
      }
      else
        return NULLHWND;
    }
  }

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return NULLHWND;

  /*
    Set the extended style for Win 3.0 compatibility.
  */
  w->dwExtStyle = _CurrExtStyle;

  /*
    Set the window instance
  */
  w->hInstance = hInst;

  /*
    Set the owner
  */
  w->hWndOwner = hOwner;

  /*
    Set up some of the stuff in the class structure, such as the wndproc,
    extra bytes, and menu. If the class has CS_OWNDC specified, then give
    the window its own DC.
  */
  if (iUserClass)
  {
    pCl = ClassIDToClassStruct(iUserClass);
    w->idClass = iUserClass;
    WinSetWinProc(hWnd, pCl->lpfnWndProc);
    if (pCl->cbWndExtra)
      SetWindowExtra(hWnd, pCl->cbWndExtra);
    if (pCl->style & CS_OWNDC)
      w->hDC = GetDC(hWnd);

    /*
      Do not give a menu to a child window if the 'max Windows compatible'
      state is set.
    */
    if (bIsTopLevel || 
        TestWindowsCompatibility(WC_MAXCOMPATIBILITY) != WC_MAXCOMPATIBILITY)
    {
      /*
        If a menu wasn't specified, and the class has a menu, then load
        the class menu in.
      */
#if !defined(MOTIF)
      if (!hMenu && pCl->lpszMenuName)
      {
        /*
          See if there is a menu specified in the window class
        */
        hMenu = LoadMenu(hInst, pCl->lpszMenuName);
        SetMenu(hWnd, hMenu);
      }
#endif
    }
  }

  /*
    Send the WM_NCCREATE and WM_CREATE messages to the window. If the return
    code from WM_NCCREATE is FALSE, then don't create the window. Also, if
    the app returns -1 from WM_CREATE, abort the window creation.
  */
  if (hWnd)
  {
    CREATESTRUCT cs;
    BOOL bPartOfCombo;

    bPartOfCombo = (BOOL) (w->parent &&
                   _WinGetLowestClass(w->parent->idClass) == COMBO_CLASS);

    /*
      Windows does not send WM_CREATE to children of combos, and MFC 2.5
      knows this!
    */
    if (!bPartOfCombo)
    {
      cs.lpCreateParams = lpParam;
      cs.hInstance  = hInst;
      cs.hMenu      = (dwStyle & WS_CHILD) ? id : hMenu;
      cs.hwndParent = hParent;
      cs.cx         = width;
      cs.cy         = height;
      cs.x          = x;
      cs.y          = y;
      cs.style      = dwStyle;
      cs.lpszName   = szWindow;
      cs.lpszClass  = szClass;
      cs.dwExStyle  = 0L;
      if (SendMessage(hWnd, WM_NCCREATE, 0, (DWORD)(LPCREATESTRUCT) &cs))
      {
        if (SendMessage(hWnd, WM_CREATE, 0, (DWORD)(LPCREATESTRUCT) &cs) == -1)
        {
          DestroyWindow(hWnd);
          return NULLHWND;
        }
      }
      else
      {
        DestroyWindow(hWnd);
        return NULLHWND;
      }
    }
  }

  /*
    Make sure handle is still valid! It is common to call destroy
    window in a WM_CREATE handler for superclassed windows. (AJP)
  */
  if (WID_TO_WIN(hWnd) == (WINDOW *) NULL)
    return NULLHWND;

  /*
    For Win 3.0 compatibility, we should issue the WM_PARENTNOTIFY
    message
  */
  if ((dwStyle & WS_CHILD) && !(_CurrExtStyle & WS_EX_NOPARENTNOTIFY))
  {
    WINDOW *wParent = w->parent;
    /*
      Controls of dialog boxes automatically get the WS_EX_NOPARENTNOTIFY
      style.
    */
    if (IS_DIALOG(wParent))
      w->dwExtStyle |= WS_EX_NOPARENTNOTIFY;
    else
      WinParentNotify(hWnd, WM_CREATE, 0);
  }


  /*
    For an initially created window, we have to send a WM_SIZE message
    to the window when it is first shown. (MS Windows compatibility)
    The WIN_SEND_WMSIZE message is turned off in SetWindowPos().
  */
  w->ulStyle |= WIN_SEND_WMSIZE;

  if (idClass == NORMAL_CLASS)
  {
    if (dwStyle & WS_MINIMIZE)
    {
      w->flags &= ~WS_MINIMIZE;  /* so WinMinimize will iconize, not restore */
      ShowWindow(hWnd, SW_MINIMIZE);
    }
    else if (dwStyle & WS_MAXIMIZE)
    {
      w->flags &= ~WS_MAXIMIZE;  /* so WinZoom will zoom, not restore */
      ShowWindow(hWnd, SW_MAXIMIZE);
    }
  }

  /*
    For an initially created window, we have to send a WM_SIZE message
    to the window when it is first shown. (MS Windows compatibility)
    Also, show the window if the WS_VISIBLE option was set.
  */
  if (dwStyle & WS_VISIBLE)
    ShowWindow(hWnd, (dwStyle & WS_POPUP) ? SW_SHOW : SW_SHOWNOACTIVATE);

  return hWnd;
}



/*===========================================================================*/
/*                              CreateWindow                                 */
/*===========================================================================*/
HWND FAR PASCAL CreateWindow(lpClassName, lpWindowName, dwStyle,
                               X, Y, nWidth, nHeight,
                               hwndParent, hMenu, hInstance, lpParam)
  LPCSTR lpClassName;
  LPCSTR lpWindowName;
  DWORD dwStyle;
  int X;
  int Y;
  int nWidth;
  int nHeight;
  HWND hwndParent;
  HMENU hMenu;
  HANDLE hInstance;
  VOID FAR *lpParam;
{
  HMENU hmenu;
  UINT  usChildID;
  HWND  hwnd;

  if (dwStyle & WS_CHILD)
  {
    usChildID = (UINT) hMenu;
    hmenu = (HMENU) NULL;
  }
  else
  {
    usChildID = 0;
    hmenu = hMenu;
  }

  hwnd = _CreateWindow(
		      lpClassName,	/* Window class name */
		      lpWindowName,	/* Window caption */
		      dwStyle,	/* Window style */
		      X,	/* Initial x position */
		      Y,	/* Initial y position */
		      nWidth,	/* Initial x size */
		      nHeight,	/* Initial y size */
		      SYSTEM_COLOR,	/* Screen colors */
		      (UINT) usChildID,	/* Child ID */
		      (HWND) hwndParent,	/* Parent window handle */
		      (HMENU) hmenu,	/* Window menu handle */
		      (HANDLE) hInstance,	/* Program instance handle */
		      (LPSTR) lpParam	/* Create parameters */
                    );

  _CurrExtStyle = 0L;
  return hwnd;
}


HWND FAR PASCAL CreateWindowEx(dwExtStyle, lpClassName, lpWindowName, dwStyle,
                               X, Y, nWidth, nHeight,
                               hwndParent, hMenu, hInstance, lpParam)
  DWORD dwExtStyle;
  LPCSTR lpClassName;
  LPCSTR lpWindowName;
  DWORD dwStyle;
  int X;
  int Y;
  int nWidth;
  int nHeight;
  HWND hwndParent;
  HMENU hMenu;
  HANDLE hInstance;
  VOID FAR *lpParam;
{
  HWND hWnd;

  _CurrExtStyle = dwExtStyle;
  hWnd = CreateWindow(
	      lpClassName,
	      lpWindowName,
	      dwStyle,
	      X,
	      Y,
	      nWidth,
	      nHeight,
	      hwndParent,
	      hMenu,
	      hInstance,
	      lpParam);

  if (hWnd)
    WID_TO_WIN(hWnd)->dwExtStyle = dwExtStyle;
  return hWnd;
}
