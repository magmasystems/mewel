/*===========================================================================*/
/*                                                                           */
/* File    : WSPY.C                                                          */
/*                                                                           */
/* Purpose : Message spying routine for debugging purposes                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static VOID PrintMsg(HWND, UINT, WPARAM, LPARAM);


typedef struct tagMsgInfo
{
  PSTR pMsgName;
  UINT wMsg;
} MSGINFO, *PMSGINFO, FAR *LPMSGINFO;

MSGINFO MsgInfo[] =
{
  "WM_USER",			WM_USER,
  "WM_ACTIVATE",		WM_ACTIVATE,
  "WM_ACTIVATEAPP",		WM_ACTIVATEAPP,
  "WM_ASKCBFORMATNAME",         WM_ASKCBFORMATNAME,
  "WM_CANCELMODE",		WM_CANCELMODE,
  "WM_CHAR",			WM_CHAR,
  "WM_CHARTOITEM",              WM_CHARTOITEM,
  "WM_CHANGECBCHAIN",           WM_CHANGECBCHAIN,
  "WM_CHILDACTIVATE",           WM_CHILDACTIVATE,
  "WM_CLEAR",			WM_CLEAR,
  "WM_CLOSE",			WM_CLOSE,
  "WM_COPY",			WM_COPY,
  "WM_COMPACTING",              WM_COMPACTING,
  "WM_COMPAREITEM",		WM_COMPAREITEM,
  "WM_COMMNOTIFY",		WM_COMMNOTIFY,
  "WM_COMMAND",			WM_COMMAND,
  "WM_CREATE",			WM_CREATE,
  "WM_CTLCOLOR",		WM_CTLCOLOR,
  "WM_CUT",			WM_CUT,
  "WM_DESTROYCLIPBOARD",        WM_DESTROYCLIPBOARD,
  "WM_DESTROY",			WM_DESTROY,
  "WM_DEVMODECHANGE",           WM_DEVMODECHANGE,
  "WM_DELETEITEM",              WM_DELETEITEM,
  "WM_DEADCHAR",		WM_DEADCHAR,
  "WM_DRAWCLIPBOARD",           WM_DRAWCLIPBOARD,
  "WM_DRAWITEM",                WM_DRAWITEM,
  "WM_DROPFILES",		WM_DROPFILES,
  "WM_ENDSESSION",		WM_ENDSESSION,
  "WM_ENABLE",			WM_ENABLE,
  "WM_ENTERIDLE",		WM_ENTERIDLE,
  "WM_ERASEBKGND",		WM_ERASEBKGND,
  "WM_FONTCHANGE",		WM_FONTCHANGE,
  "WM_GETMINMAXINFO",           WM_GETMINMAXINFO,
  "WM_GETDLGCODE",		WM_GETDLGCODE,
  "WM_GETFONT",			WM_GETFONT,
  "WM_GETTEXTLENGTH",           WM_GETTEXTLENGTH,
  "WM_GETTEXT",			WM_GETTEXT,
  "WM_HSCROLL",			WM_HSCROLL,
  "WM_HSCROLLCLIPBOARD",        WM_HSCROLLCLIPBOARD,
  "WM_ICONERASEBKGND",          WM_ICONERASEBKGND,
  "WM_INITMENUPOPUP",           WM_INITMENUPOPUP,
  "WM_INITDIALOG",		WM_INITDIALOG,
  "WM_INITMENU",		WM_INITMENU,
  "WM_KEYUP",			WM_KEYUP,
  "WM_KEYDOWN",			WM_KEYDOWN,
  "WM_KILLFOCUS",		WM_KILLFOCUS,
  "WM_MDIRESTORE",		WM_MDIRESTORE,
  "WM_MDICREATE",		WM_MDICREATE,
  "WM_MDISETMENU",		WM_MDISETMENU,
  "WM_MDIGETACTIVE",            WM_MDIGETACTIVE,
  "WM_MDICASCADE",		WM_MDICASCADE,
  "WM_MDITILE",			WM_MDITILE,
  "WM_MDIACTIVATE",		WM_MDIACTIVATE,
  "WM_MDIDESTROY",		WM_MDIDESTROY,
  "WM_MDIICONARRANGE",          WM_MDIICONARRANGE,
  "WM_MDINEXT",			WM_MDINEXT,
  "WM_MDIMAXIMIZE",		WM_MDIMAXIMIZE,
  "WM_MEASUREITEM",             WM_MEASUREITEM,
  "WM_MENUCHAR",		WM_MENUCHAR,
  "WM_MENUSELECT",		WM_MENUSELECT,
  "WM_MOVE",			WM_MOVE,
  "WM_MOUSEMOVE",		WM_MOUSEMOVE,
  "WM_MOUSEACTIVATE",           WM_MOUSEACTIVATE,
  "WM_NCPAINT",			WM_NCPAINT,
  "WM_NCDESTROY",		WM_NCDESTROY,
  "WM_NCCREATE",		WM_NCCREATE,
  "WM_NCCALCSIZE",		WM_NCCALCSIZE,
  "WM_NCACTIVATE",		WM_NCACTIVATE,
  "WM_NCHITTEST",		WM_NCHITTEST,
  "WM_NEXTDLGCTL",		WM_NEXTDLGCTL,
  "WM_PALETTEISCHANGING",       WM_PALETTEISCHANGING,
  "WM_PAINT",			WM_PAINT,
  "WM_PAINTICON",               WM_PAINTICON,
  "WM_PASTE",			WM_PASTE,
  "WM_PALETTECHANGED",          WM_PALETTECHANGED,
  "WM_PARENTNOTIFY",            WM_PARENTNOTIFY,
  "WM_PAINTCLIPBOARD",          WM_PAINTCLIPBOARD,
  "WM_PENWINFIRST",		WM_PENWINFIRST,
  "WM_PENWINLAST",		WM_PENWINLAST,
  "WM_POWER",			WM_POWER,
  "WM_QUERYENDSESSION",         WM_QUERYENDSESSION,
  "WM_QUERYNEWPALETTE",         WM_QUERYNEWPALETTE,
  "WM_QUIT",			WM_QUIT,
  "WM_QUERYOPEN",		WM_QUERYOPEN,
  "WM_QUEUESYNC",               WM_QUEUESYNC,
  "WM_QUERYDRAGICON",           WM_QUERYDRAGICON,
  "WM_RENDERALLFORMATS",        WM_RENDERALLFORMATS,
  "WM_RENDERFORMAT",		WM_RENDERFORMAT,
  "WM_SETTEXT",			WM_SETTEXT,
  "WM_SETFONT",                 WM_SETFONT,
  "WM_SETCURSOR",		WM_SETCURSOR,
  "WM_SETREDRAW",		WM_SETREDRAW,
  "WM_SETFOCUS",		WM_SETFOCUS,
  "WM_SHOWWINDOW",		WM_SHOWWINDOW,
  "WM_SIZE",			WM_SIZE,
  "WM_SIZECLIPBOARD",           WM_SIZECLIPBOARD,
  "WM_SPOOLERSTATUS",           WM_SPOOLERSTATUS,
  "WM_SYSTEMERROR",		WM_SYSTEMERROR,
  "WM_SYSDEADCHAR",		WM_SYSDEADCHAR,
  "WM_SYSKEYUP",		WM_SYSKEYUP,
  "WM_SYSCOMMAND",              WM_SYSCOMMAND,
  "WM_SYSKEYDOWN",		WM_SYSKEYDOWN,
  "WM_SYSCOLORCHANGE",          WM_SYSCOLORCHANGE,
  "WM_SYSCHAR",			WM_SYSCHAR,
  "WM_TIMER",			WM_TIMER,
  "WM_TIMECHANGE",		WM_TIMECHANGE,
  "WM_UNDO",			WM_UNDO,
  "WM_USER",			WM_USER,
  "WM_VKEYTOITEM",              WM_VKEYTOITEM,
  "WM_VSCROLL",			WM_VSCROLL,
  "WM_VSCROLLCLIPBOARD",        WM_VSCROLLCLIPBOARD,
  "WM_WINDOWPOSCHANGED",        WM_WINDOWPOSCHANGED,
  "WM_WININICHANGE",		WM_WININICHANGE,
  "WM_WINDOWPOSCHANGING",       WM_WINDOWPOSCHANGING,
  "WM_MOUSEREPEAT",             WM_MOUSEREPEAT,
  "WM_MOUSEMOVE",		WM_MOUSEMOVE,
  "WM_LBUTTONDOWN",		WM_LBUTTONDOWN,
  "WM_LBUTTONUP",		WM_LBUTTONUP,
  "WM_LBUTTONDBLCLK",           WM_LBUTTONDBLCLK,
  "WM_RBUTTONDOWN",		WM_RBUTTONDOWN,
  "WM_RBUTTONUP",		WM_RBUTTONUP,
  "WM_RBUTTONDBLCLK",           WM_RBUTTONDBLCLK,
  "WM_MBUTTONDOWN",		WM_MBUTTONDOWN,
  "WM_MBUTTONUP",		WM_MBUTTONUP,
  "WM_MBUTTONDBLCLK",           WM_MBUTTONDBLCLK,
  "WM_NCMOUSEMOVE",		WM_NCMOUSEMOVE,
  "WM_NCLBUTTONDOWN",           WM_NCLBUTTONDOWN,
  "WM_NCLBUTTONUP",		WM_NCLBUTTONUP,
  "WM_NCLBUTTONDBLCLK",         WM_NCLBUTTONDBLCLK,
  "WM_NCRBUTTONDOWN",           WM_NCRBUTTONDOWN,
  "WM_NCRBUTTONUP",		WM_NCRBUTTONUP,
  "WM_NCRBUTTONDBLCLK",         WM_NCRBUTTONDBLCLK,
  "WM_NCMBUTTONDOWN",           WM_NCMBUTTONDOWN,
  "WM_NCMBUTTONUP",		WM_NCMBUTTONUP,
  "WM_NCMBUTTONDBLCLK",         WM_NCMBUTTONDBLCLK,
  "WM_ALT",                     WM_ALT,
  "WM_BORDER",                  WM_BORDER,
  "WM_GETID",                   WM_GETID,
  "WM_GETFLAGS",		WM_GETFLAGS,
  "WM_HELP",                    WM_HELP,
  "WM_SETFLAGS",		WM_SETFLAGS,
  "WM_SETID",                   WM_SETID,
  "WM_SHADOW",                  WM_SHADOW,
  "WM_VALIDATE",                WM_VALIDATE,
  "FN_ERRVALIDATE",             FN_ERRVALIDATE,
};


static BOOL bIsSorted = FALSE;

#if defined(__WATCOMC__)
static int       strCompare(const void     *s1, const void     *s2)
#else
static int CDECL strCompare(const void FAR *s1, const void FAR *s2)
#endif
{
  return ((LPMSGINFO) s1)->wMsg - ((LPMSGINFO) s2)->wMsg;
}


static VOID PrintMsg(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  MSGINFO  tmpMsg;
  PMSGINFO pMsg;
  PSTR     pStr;
  char     szBuf[80];
  char     szTmp[32];

  if (!bIsSorted)
  {
    bIsSorted++;
    qsort(MsgInfo, sizeof(MsgInfo)/sizeof(MsgInfo[0]), 
          sizeof(MsgInfo[0]), strCompare);
  }

  if (message >= WM_USER)
  {
    sprintf(szTmp, "WM_USER+%d", message - WM_USER);
    pStr = szTmp;
  }
  else
  {
    tmpMsg.wMsg = message;
    pMsg = (PMSGINFO) bsearch(&tmpMsg, MsgInfo, sizeof(MsgInfo)/sizeof(MsgInfo[0]), 
                              sizeof(MsgInfo[0]), strCompare);
    pStr = (pMsg != NULL) ? pMsg->pMsgName : NULL;
  }

  if (pStr)
  {
    sprintf(szBuf, "%04d %-32s 0x%04x 0x%08lx [HI=%u LO=%u]\n",
            hWnd, pStr, wParam, lParam, HIWORD(lParam), LOWORD(lParam));
    OutputDebugString(szBuf);
  }
}


typedef struct tagCallWndProcHookData
{
  USHORT hlParam;
  USHORT llParam;
  WPARAM wParam;
  UINT   wMsg;
  HWND   hWnd;
} CALLWNDPROCHOOKDATA, FAR *LPCALLWNDPROCHOOKDATA;

int FAR PASCAL SpyHookProc(nCode, wDummy, lpHookData)
  int  nCode;
  UINT wDummy;
  LPCALLWNDPROCHOOKDATA lpHookData;
{
  (void) nCode;
  (void) wDummy;

  PrintMsg(lpHookData->hWnd, lpHookData->wMsg, lpHookData->wParam,
           MAKELONG(lpHookData->llParam, lpHookData->hlParam));

  return TRUE;
}


VOID FAR PASCAL WinMessageSpy(hInstance, bInstall)
  HANDLE hInstance;
  BOOL   bInstall;
{
  static FARPROC lpfnHook = 0;

  (void) hInstance;

  if (bInstall)
  {
    lpfnHook = MakeProcInstance(SpyHookProc, hInstance);
    SetWindowsHook(WH_CALLWNDPROC, lpfnHook);
  }
  else
  {
    UnhookWindowsHook(WH_CALLWNDPROC, lpfnHook);
    FreeProcInstance(lpfnHook);
  }
}

