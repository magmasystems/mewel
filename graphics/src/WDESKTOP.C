/*===========================================================================*/
/*                                                                           */
/* File    : WDESKTOP.C                                                      */
/*                                                                           */
/* Purpose : Desktop creation and winproc                                    */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


HWND WINAPI GetDesktopWindow(void)
{
  return _HwndDesktop;
}

HWND WINAPI GetDesktopHwnd(void)
{
  return _HwndDesktop;
}



HWND FAR PASCAL WinCreateDesktopWindow(void)
{
  _HwndDesktop = _CreateWindow((PSTR) "Normal", /* class */
                               NULL,            /* title */
                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS, /* dwStyle */
                               0,0, VideoInfo.width, VideoInfo.length,
                               SYSTEM_COLOR,
                               0,              /* id */
                               NULLHWND,       /* hParent */
                               NULLHWND,       /* hMenu */
                               0,              /* hInst */
                               0L);            /* lpParam */
  
  InternalSysParams.wDesktop = WID_TO_WIN(_HwndDesktop);
  InternalSysParams.wDesktop->fillchar = WinGetSysChar(SYSCHAR_DESKTOP_BACKGROUND);
  InternalSysParams.wDesktop->flags |= WS_VISIBLE;
  InternalSysParams.WindowList = (WINDOW *) NULL;  /* remove it from the window list */
  WinSetWinProc(_HwndDesktop, _DesktopWndProc);
  return _HwndDesktop;
}


LRESULT FAR PASCAL _DesktopWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  (void) lParam;

  switch (message)
  {
    case WM_SYSCOLORCHANGE:
      /*
        Cause the desktop to be refreshed if we change the background color
      */
      if (wParam == SYSCLR_BACKGROUND)
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
      break;

    case WM_DEVMODECHANGE :
      /*
        Resize the desktop so that it takes up the entire screen
      */
      WinSetSize(hWnd, VideoInfo.length, VideoInfo.width);
      break;

    case WM_NCHITTEST     :
      /*
        Ignore mouse-clicks on the desktop.
      */
      return HTNOWHERE;

    case WM_SETCURSOR     :
#ifdef MEWEL_GUI
      SetCursor(LoadCursor(NULL, IDC_ARROW));
#else
      SetCursor(NORMAL_CURSOR);
#endif
      return FALSE;

    case WM_ERASEBKGND    :
#if defined(MEWEL_GUI)
      _GetDC((HDC) wParam)->hBrush = SysBrush[COLOR_BACKGROUND];
#endif
      goto call_dwp;

    case WM_CTLCOLOR      :
      /*
        In case a message box or a top-level dialog box sends this to
        the desktop window, pass it on to the "DefWindowProc"
      */
    case WM_PAINT         :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);
  }
  return TRUE;
}

