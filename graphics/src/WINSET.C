/*===========================================================================*/
/*                                                                           */
/* File    : WSETFLAG.C                                                      */
/*                                                                           */
/* Purpose : WinSetFlags() returns the value of the flags field              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MOUSE

#include "wprivate.h"
#include "window.h"


DWORD FAR PASCAL WinSetFlags(hWnd, fFlags)
  HWND hWnd;
  DWORD fFlags;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? (w->flags = fFlags) : 0L;
}

DWORD FAR PASCAL WinSetStyle(hWnd, fFlags)
  HWND hWnd;
  DWORD fFlags;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? (w->ulStyle = fFlags) : 0L;
}

/*===========================================================================*/
/*                                                                           */
/* File    : WGETFLAG.C                                                      */
/*                                                                           */
/* Purpose : WinGetFlags() returns the value of the flags field              */
/*                                                                           */
/*===========================================================================*/
DWORD FAR PASCAL WinGetFlags(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? w->flags : 0L;
}

DWORD FAR PASCAL WinGetStyle(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? w->ulStyle : 0L;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WSETATTR.C                                                      */
/*                                                                           */
/* Purpose : WinSetAttr()                                                    */
/*                                                                           */
/* History : 3/6/90 (maa) WinSetAttr now returns the old attribute. Also     */
/*                        added the fRedraw arg.                             */
/*                                                                           */
/*===========================================================================*/
COLOR FAR PASCAL WinSetAttr(HWND hWnd, COLOR attr, BOOL fRedraw)
{
  WINDOW *w = WID_TO_WIN(hWnd);
  COLOR  oldAttr;

  (void) fRedraw;

  if (w)
  {
    oldAttr = w->attr;
    w->attr = attr;
#if defined(USE_SYSCOLOR)
    if (attr == SYSTEM_COLOR)
      w->flags |=  WS_SYSCOLOR;
    else
      w->flags &= ~WS_SYSCOLOR;
#endif

    w->ulStyle |= WIN_UPDATE_NCAREA;
    InvalidateRect(hWnd, (LPRECT) NULL, TRUE);

    return oldAttr;
  }
  else
    return 0x0000;
}

COLOR FAR PASCAL WinGetAttr(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? w->attr : 0x0000;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WSETCAPT.C                                                      */
/*                                                                           */
/* Purpose : SetCapture(), ReleaseCapture()                                  */
/*                                                                           */
/*===========================================================================*/
#if defined(TELEVOICE) && defined(MEWEL_TEXT)
static void PASCAL SetReleaseCursor(void);
static void PASCAL SetCaptureCursor(void);
#endif

HWND FAR PASCAL SetCapture(hWnd)
  HWND hWnd;
{
#if defined(XWINDOWS)
  _XSetCapture(hWnd);
#endif
#if defined(TELEVOICE) && defined(MEWEL_TEXT)
  SetCaptureCursor();
#endif
  return (InternalSysParams.hWndCapture = hWnd);
}

INT FAR PASCAL ReleaseCapture(void)
{
#if defined(XWINDOWS)
  _XReleaseCapture();
#endif
#if defined(TELEVOICE) && defined(MEWEL_TEXT)
  SetReleaseCursor();
#endif
  return (int) (InternalSysParams.hWndCapture = NULLHWND);
}

HWND FAR PASCAL GetCapture(void)
{
  return InternalSysParams.hWndCapture;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WSETPROC.C                                                      */
/*                                                                           */
/* Purpose : WinSetWinProc()                                                 */
/*                                                                           */
/*===========================================================================*/
WINPROC *FAR PASCAL WinSetWinProc(hWnd, pfFunc)
  HWND   hWnd;
  WINPROC *pfFunc;
{
  WINPROC *oldFunc;
  WINDOW *w;
  
  if ((w = WID_TO_WIN(hWnd)) != NULL)
  {
    oldFunc = w->winproc;
    w->winproc = pfFunc;
    return oldFunc;
  }
  return NULL;
}

/*===========================================================================*/
/*                                                                           */
/* File    : WSETTEXT.C                                                      */
/*                                                                           */
/* Purpose : WinSetText(), WinGetText(), WinGetTextLength()                  */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL SetWindowText(hWnd, lpText)
  HWND  hWnd;
  LPCSTR lpText;
{
  if (lpText == NULL)
    lpText = (LPCSTR) "";
  return (int) SendMessage(hWnd, WM_SETTEXT, 0, (DWORD) lpText);
}

int FAR PASCAL GetWindowText(hWnd, text, maxlen)
  HWND  hWnd;
  LPSTR text;
  int   maxlen;
{
  return (int) SendMessage(hWnd, WM_GETTEXT, (WPARAM)maxlen, (DWORD)(LPSTR) text);
}

int FAR PASCAL GetWindowTextLength(hWnd)
  HWND hWnd;
{
  return (int) SendMessage(hWnd, WM_GETTEXTLENGTH, 0, (DWORD) 0L);
}


VOID FAR PASCAL _TranslatePrefix(szWindow)
  LPSTR szWindow;
{
  LPSTR pAmp;

#if !defined(MEWEL_32BITS)
  /*
    Some window titles can be handles or ordinals
  */
  if (FP_SEG(szWindow) == 0)
    return;
#endif

  /*
    Handle the highlite prefix correctly. Change '&' to '~'
  */
  if (szWindow != NULL && (pAmp = (LPSTR) lstrchr(szWindow, '&')) != NULL)
    if (*pAmp == '&' && pAmp[1] != '\0')
      /*
        &x => ~x
        && => &
      */
      if (pAmp[1] == '&')
        lmemcpy(pAmp, pAmp+1, lstrlen(pAmp));
      else
        *pAmp = HILITE_PREFIX;
}


#if !defined(MEWEL_GUI) && !defined(XWINDOWS)
static HCURSOR hCurrCursor = 0;

HCURSOR FAR PASCAL SetCursor(chMouse)
  HCURSOR chMouse;
{
  HCURSOR hLastCursor;

  if (hCurrCursor == (HCURSOR) chMouse)
    return hCurrCursor;

#ifdef DOS
  MouseSetCursorChar(chMouse);
#endif

  hLastCursor = hCurrCursor;
  hCurrCursor = (HCURSOR) chMouse;
  return hLastCursor;
}

#if defined(TELEVOICE)
/*
 * 4/27/93
 * whenever the mouse is captured change the cursor to normal (arrow)
 *  if the mouse moves do not restore it
 *  (more acurate would be to post a MOUSEMOVE event)
 */
static POINT   savePoint;
static HCURSOR hSaveCursor = 0;		/* move 4/27/93 */

static void PASCAL SetCaptureCursor(void)
{
	hSaveCursor = hCurrCursor;
	if(hCurrCursor == NORMAL_CURSOR)
		return;
	GetCursorPos(&savePoint);
	SetCursor(NORMAL_CURSOR);
}
static void PASCAL SetReleaseCursor(void)
{
	POINT pt;
	if(hSaveCursor == NORMAL_CURSOR)
		return;
	GetCursorPos(&pt);
	if( pt.x == savePoint.x 
	&&  pt.y == savePoint.y)
		SetCursor(hSaveCursor);	/* only setback if has not moved */
}

#endif /* TELEVOICE */
#endif /* MEWEL_GUI */

