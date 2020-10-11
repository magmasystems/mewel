/*===========================================================================*/
/*                                                                           */
/* File    : WINDLG.C                                                        */
/*                                                                           */
/* Purpose : Implements the dialog box class.                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOKERNEL

#include "wprivate.h"
#include "window.h"

typedef struct iteminfo
{
  HWND   hItemWnd;
  int    idItem;
  char   letter;
} ITEMINFO;

typedef struct dialog
{
  DIALOGPROC *dlgFunc;
  INT     result;
  DWORD   flags;
#define DLG_MODAL     0x0001
#define DLG_ENDED     0x0002
  HWND    hParent;
  HWND    hWndFocus;
  int     idClass;

/*
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     4
#define DWL_USER        8
*/
  DWORD   dwMsgResult;
  DWORD   dwUser;
} DIALOG;

DWORD _DlgBoxlParam = 0L;       /* for DialogBoxParam() */
BOOL  bInDialogCreate = FALSE;  /* for _CreateWindow()  */
BOOL  bInModalDialogCreate = FALSE;  /* for _CreateWindow()  */

/*
  3/26/91 (maa)
  Implemented a stack of currently viewed modal dialogs. This way,
  WinDrawAllWindows() can redraw modal dialog boxes correctly
*/
#define MAXDLGSTACK   16
HWND FARDATA _HDlgStack[MAXDLGSTACK];
int  DlgStackSP = 0;

/*
  The reason code is sent along with the WM_VALIDATE message. The LOWORD
  is the handle of the window which will be getting the focus. The HIWORD
  is the reason code, which can be :

    VR_NEXTDLGCTRL   focus was lost cause of WM_NEXTDLGCTRL call
    VR_LBUTTONDOWN   focus was lost cause of WM_LBUTTONDOWN call
    VR_RBUTTONDOWN   focus was lost cause of WM_RBUTTONDOWN call
    other            has the VK_xxx code for a WM_CHAR message
*/
DWORD _dwValidateReason = 0L;

#define IS_CHAR_MSG(m) ((m)==WM_CHAR || (m)==WM_KEYDOWN || (m)==WM_SYSKEYDOWN)

#ifdef __cplusplus
extern "C" {
#endif
extern HWND  FAR PASCAL _DlgGetFirstControlOfGroup(HWND);
extern HWND  FAR PASCAL _DlgGetLastControlOfGroup(HWND);
extern INT   FAR PASCAL _CheckRadioButton(HWND,int,int,int, BOOL, BOOL);
extern BOOL  FAR PASCAL _IsButtonClass(HWND, UINT);

static DIALOG* PASCAL _DlgHwndToStruct(HWND);
static HWND  PASCAL IsDlgAccel(HWND, int);
static DWORD PASCAL _DlgDMDefID(HWND, WPARAM, UINT);
static HWND  PASCAL _InitModelessDialog(HWND, LONG, FARPROC);
static BOOL  PASCAL IsDialogKey(HWND, UINT, UINT, LONG);
static VOID  PASCAL DialogBoxShow(HWND);
static VOID  PASCAL DlgSetInitialFocus(HWND);
static HWND  PASCAL DlgQueryInitialFocus(HWND);
static HWND  PASCAL CheckComboChild(HWND);
#ifdef __cplusplus
}
#endif


/*===========================================================================*/
/*                                                                           */
/* Purpose : _DlgHwndToStruct()                                              */
/*                                                                           */
/*===========================================================================*/
static DIALOG *PASCAL _DlgHwndToStruct(hDlg)
  HWND hDlg;
{
  /*
    Get a pointer to the dialog box structure
  */
  WINDOW *w;
  return ((w = WID_TO_WIN(hDlg)) == NULL) ? (DIALOG *) NULL
                                          : (DIALOG *) w->pPrivate;
}

/*===========================================================================*/
/*                                                                           */
/* File    : DialogCreate()                                                  */
/*                                                                           */
/* Purpose : Function to create a dialog box                                 */
/*                                                                           */
/* Returns : The handle of the dialog box, or NULL if not successful.        */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL DialogCreate(hParent, row1,col1,row2,col2, title,attr,
                                  dlgflags,dlgfunc,id,pszClass,hInst)
  HWND   hParent;
  int    row1, col1, row2, col2;
  LPSTR  title;
  COLOR  attr;
  DWORD  dlgflags;
  FARPROC dlgfunc;
  int    id;
  LPSTR  pszClass;
  HANDLE hInst;
{
  HWND hWnd;
  WINDOW *w;
  DIALOG *d;

#if defined(USE_SYSCOLOR)
  if (attr == SYSTEM_COLOR)
  {
    attr = WinQuerySysColor(NULLHWND, SYSCLR_DLGBOX);
    dlgflags |= WS_SYSCOLOR;
  }
#endif

  if (!hParent)
    hParent = _HwndDesktop;

  /*
    If no custom class is being used, then use the standard MEWEL dialog class
  */
  if (!pszClass || pszClass[0] == '\0')
    pszClass = "DIALOG";

  /*
    Force the dialog box to be a popup if it's not a child window.
  */
  if (!(dlgflags & WS_CHILD))
    dlgflags |= WS_POPUP;

  /*
    Create a normal window
  */
  bInDialogCreate++;
  hWnd = _CreateWindow(pszClass,
                       title,
#if defined(MOTIF)
                       dlgflags,
#else
                       dlgflags & ~WS_SYSMENU,
#endif
                       col1, row1, col2-col1, row2-row1,
                       attr,
                       id,
                       hParent,
                       (HMENU) NULL,
                       hInst,
                       (LPSTR) NULL);
  bInDialogCreate--;

  if (hWnd == NULL || (w = WID_TO_WIN(hWnd)) == NULL)
    return NULLHWND;

  /*
    Set a flag which marks this window as a dialog box.
  */
  w->ulStyle |= WIN_IS_DLG;
  
  /*
    Now that the WIN_IS_DLG bit has been set, it is safe to create a
    system menu for the dialog. WinCreateSysMenu() has special logic
    which handles a dialog box system menu.
  */
  if (dlgflags & WS_SYSMENU)
    WinCreateSysMenu(hWnd);

  /*
    Fill in the dialog structure with the flags, user-defined proc,
    and parent window.
  */
  if ((d = (DIALOG *) w->pPrivate) == NULL)
  {
    /*
      w->pPrivate can be NULL if we had a dialog box without a dialog
      proc (like some modeless dialogs are). If so, then allocate
      the pPrivate memory right here.
    */
    if ((d = (DIALOG *) (w->pPrivate = emalloc(sizeof(DIALOG)))) == NULL)
    {
      DestroyWindow(hWnd);
      return NULLHWND;
    }
  }
  d->dlgFunc   = (DIALOGPROC *) dlgfunc;
  d->flags     = 0L;
  d->hParent   = hParent;
  d->idClass   = w->idClass;
  d->hWndFocus = hWnd;

  /*
    KLUDGE!!! We should use the font which was passed in via LoadDialog.
  */
  if (dlgflags & DS_SETFONT)
  {
    SendMessage(hWnd, WM_SETFONT, GetStockObject(SYSTEM_FONT), 0L);
  }

  return hWnd;
}


/*===========================================================================*/
/*                                                                           */
/* File    : DialogBox()                                                     */
/*                                                                           */
/* Purpose : Main driver for the dialog box                                  */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL CreateDialog(hModule, idDialog, hParent, dlgfunc)
  HMODULE hModule;
  LPCSTR  idDialog;
  HWND    hParent;
  FARPROC dlgfunc;
{
  return CreateDialogParam(hModule, idDialog, hParent, dlgfunc, 0L);
}

HWND FAR PASCAL CreateDialogParam(hModule, idDialog, hParent, dlgfunc, lParam)
  HMODULE hModule;
  LPCSTR  idDialog;
  HWND    hParent;
  FARPROC dlgfunc;
  LONG lParam;
{
  HWND hDlg;
  if ((hDlg = LoadDialog(hModule, idDialog, hParent, dlgfunc)) == NULL)
    return NULL;
  return _InitModelessDialog(hDlg, lParam, dlgfunc);
}

HWND FAR PASCAL CreateDialogIndirect(hInst, lpDlg, hParent, dlgfunc)
  HANDLE hInst;
  CONST VOID FAR *lpDlg;
  HWND   hParent;
  FARPROC dlgfunc;
{
  HWND hDlg;
  if ((hDlg = _CreateDialogIndirect(hInst, lpDlg, hParent, dlgfunc)) == NULL)
    return NULL;
  return _InitModelessDialog(hDlg, _DlgBoxlParam, dlgfunc);
}

HWND FAR PASCAL CreateDialogIndirectParam(hInst, lpDlg, hParent, dlgfunc, lParam)
  HANDLE hInst;
  CONST VOID FAR *lpDlg;
  HWND   hParent;
  FARPROC dlgfunc;
  LONG   lParam;
{
  HWND hDlg;
  if ((hDlg = _CreateDialogIndirect(hInst, lpDlg, hParent, dlgfunc)) == NULL)
    return NULL;
  return _InitModelessDialog(hDlg, lParam, dlgfunc);
}

INT FAR PASCAL DialogBoxIndirectParam(hInst, hTemplate, hParent, dlgfunc, lParam)
  HANDLE hInst;
  GLOBALHANDLE hTemplate;
  HWND   hParent;
  FARPROC dlgfunc;
  LONG   lParam;
{
  HWND hDlg;
  LPSTR lpTemplate;

  _DlgBoxlParam = lParam;

  if ((lpTemplate = GlobalLock(hTemplate)) == NULL)
    return -1;

  bInModalDialogCreate++;
  hDlg = CreateDialogIndirect(hInst, lpTemplate, hParent, dlgfunc);
  bInModalDialogCreate--;

  GlobalUnlock(hTemplate);
  if (hDlg == NULL)
    return -1;
  return _DialogBox(hDlg);
}


static HWND PASCAL _InitModelessDialog(hDlg, lParam, dlgfunc)
  HWND hDlg;
  LONG lParam;
  FARPROC dlgfunc;
{
  int  rc = TRUE;
  DIALOG *d;

  /*
    Windows will not send a WM_INITDIALOG message to a dialog box which
    has a NULL dialog proc. Instead, the app must explicitly send
    the dialog box proc the WM_INITDIALOG message. This happens in the
    case where the modeless dialog box has its own CLASS. 
  */
  if (dlgfunc != NULL)
    rc = (int)SendMessage(hDlg,WM_INITDIALOG,DlgQueryInitialFocus(hDlg),lParam);

  /*
    See if the WM_INITDIALOG destroyed the dialog
  */
  if (!IsWindow(hDlg))
    return (HWND) -1;

  /*
    If the dialog box returns TRUE from WM_INITDIALOG, then the
    focus is set automatically to the first focusable control.
  */
  if (rc || !IsChild(hDlg, InternalSysParams.hWndFocus))
    DlgSetInitialFocus(hDlg);
  else
    /*
      We must set the hWndFocus member anyway.
    */
    if ((d = _DlgHwndToStruct(hDlg)) != NULL)
      d->hWndFocus = InternalSysParams.hWndFocus;

  /*
    1/18/93 (maa)
    I do not know if modeless dialog boxes should be automatically shown
    if the WS_VISIBLE flag is not on them initially..... So let's put
    in the test for IsWindowVisible.
  */
  if (IsWindowVisible(hDlg))
    DialogBoxShow(hDlg);
  return hDlg;
}



int FAR PASCAL DialogBoxParam(hInst, lpszID, hParent, dlgfunc, lParam)
  HANDLE hInst;
  LPCSTR lpszID;
  HWND   hParent;
  FARPROC dlgfunc;
  LONG   lParam;
{
  _DlgBoxlParam = lParam;
  return DialogBox(hInst, lpszID, hParent, dlgfunc);
}


int FAR PASCAL _DialogBoxParam(hDlg, lParam)
  HWND hDlg;
  LONG lParam;
{
  _DlgBoxlParam = lParam;
  return _DialogBox(hDlg);
}

int FAR PASCAL DialogBox(hInst, lpszID, hParent, dlgfunc)
  HANDLE hInst;
  LPCSTR lpszID;
  HWND   hParent;
  FARPROC dlgfunc;
{
  HWND hDlg;

  bInModalDialogCreate++;
  hDlg = LoadDialog(hInst, lpszID, hParent, dlgfunc);
  bInModalDialogCreate--;

  if (hDlg == NULL)
    return -1;
  return _DialogBox(hDlg);
}

int FAR PASCAL DialogBoxIndirect(hInst, hTemplate, hParent, dlgfunc)
  HANDLE hInst;
  GLOBALHANDLE hTemplate;
  HWND   hParent;
  FARPROC dlgfunc;
{
  HWND hDlg;
  LPSTR lpTemplate;

  if ((lpTemplate = GlobalLock(hTemplate)) == NULL)
    return -1;

  bInModalDialogCreate++;
  hDlg = CreateDialogIndirect(hInst, (LPSTR) lpTemplate, hParent, dlgfunc);
  bInModalDialogCreate--;

  if (hDlg == (HWND) -1)
    return -1;
  GlobalUnlock(hTemplate);
  return _DialogBox(hDlg);
}



/****************************************************************************/
/*                                                                          */
/* Function : _DialogBox()                                                  */
/*                                                                          */
/* Purpose  : Main driver for modal dialog boxes.                           */
/*                                                                          */
/* Returns  : The result code passed back by EndDialog().                   */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL _DialogBox(hDlg)
  HWND   hDlg;
{
  WINDOW *wDlg;
  DIALOG *d;
  MSG    msg;
  RECT   r;
  RECT   rOrig, rSaved;
  HWND   hOldFocus;
  HWND   hOldCapture;
  HWND   hOldActive;
  HWND   hWndOwner;
  HWND   hParent;
  INT    rc;
  HANDLE hScreen = NULL;
  DWORD  ulFlags;
  HWND	 hOldComboBox = NULLHWND;		/* CFN */
  HWND	 hOldModal = InternalSysParams.hWndSysModal;


  /*
    Get a pointer to the dialog box structure, and set the dialog box
    flags to indicate that we have a MODAL dialog box.
  */
  if ((wDlg = WID_TO_WIN(hDlg)) == NULL)
    return FALSE;
  ulFlags = wDlg->flags;
  d = (DIALOG *) wDlg->pPrivate;
  d->flags    |= DLG_MODAL;

  /*
    System modal dialog boxes (like message boxes) must be registered
    with the kernel. System modal dialogs can also be nested.
  */
  if (ulFlags & DS_SYSMODAL)
    InternalSysParams.hWndSysModal = hDlg;

#if defined(MOTIF)
#if 0
  /*
    We know we have a modal dialog box. Tell it to Motif.
  */
  XtVaSetValues(wDlg->widgetShell,
                XmNdialogStyle, 
                  (ulFlags & DS_SYSMODAL) ? XmDIALOG_SYSTEM_MODAL
                                          : XmDIALOG_APPLICATION_MODAL,
                NULL);
#endif

  if (XSysParams.ulOptions & XOPT_DLGGRAB)
    XtAddGrab(wDlg->widgetShell,
              (Boolean) ((XSysParams.ulOptions & XOPT_DLGEXCLUSIVEGRAB) != 0),
              FALSE);

#endif


  /*
    Create a system menu for the dialog box.
  */
  if ((ulFlags & WS_SYSMENU) && wDlg->hSysMenu == NULLHWND)
    WinCreateSysMenu(hDlg);

  /*
    Save the old captured window and the old focus
  */
  hOldFocus   = InternalSysParams.hWndFocus;
  hOldCapture = InternalSysParams.hWndCapture;

  /*
    Let the old focus window know that we are changing focus.
  */
  if (hOldFocus)
  {
    if (WID_TO_WIN(hOldFocus)->ulStyle & LBS_IN_COMBOBOX)       /* CFN */
    {
      hOldComboBox = GetParent(hOldFocus);                      /* CFN */
      if (GetParent(hDlg) == hOldFocus)                         /* CFN */
        SetParent(hDlg, hOldComboBox);                          /* CFN */
    }
    SendMessage(hOldFocus, WM_KILLFOCUS, hDlg, 0L);
  }

  hOldActive = GetActiveWindow();
  hParent = d->hParent;

  /*
    In case the parent of the dialog box is a child window, we want to
    disable the top-level window which is the parent of that child.
  */
  hWndOwner = _WinGetRootWindow(hParent);

#if !defined(MOTIF)
  /*
    Motif will do this anyway, so don't do it here. Doing it here results
    in a total repaint of the parent window.
  */
  EnableWindow(hWndOwner, FALSE);
#endif

  /*
    Turn on the dialog box's "hidden" flag so that the dialog box controls
    are not drawn during the WM_INITDIALOG processing.
  */
  SET_WS_HIDDEN(wDlg);
  wDlg->flags &= ~WS_VISIBLE;

  /*
    Add another member to the dialog box stack
  */
  if (DlgStackSP < MAXDLGSTACK)
    _HDlgStack[DlgStackSP++] = hDlg;

  /*
    Record the original window rectangle. We will later test it to
    see if the dialog box was moved.
  */
  GetWindowRect(hDlg, &r);
  rOrig = r;

  /*
    Save the screen under the dialog box, as well as the old cursor coords.
    This screen will be restored when the dialog box ends.
    We only do this if the dialog box can't be moved or resized, or if the
    NO_SAVEBITS state has been set by the app.
  */
  if (!(wDlg->flags & (WS_THICKFRAME | WS_VISIBLE)) &&
      !HAS_CAPTION(wDlg->flags) &&
      !TEST_PROGRAM_STATE(STATE_NO_SAVEBITS) && !VID_IN_GRAPHICS_MODE())
  {
    /*
      Adjust the rectangle to encompass the shadowed area....
    */
    _WinAdjustRectForShadow(wDlg, (LPRECT) &r);

    /*
      Before we save the screen, we should make sure that all of the windows
      have been properly updated. Otherwise, the first _PeekMsg() below
      can cause the invalid windows to be refreshed, thereby making the
      saved area totally wrong.
    */
    if (TEST_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST))
      RefreshInvalidWindows(_HwndDesktop);

    hScreen = _WinSaveRect(NULLHWND, &r);
    rSaved = r;
  }


  /*
    Send a WM_INITDIALOG message to the app's dialog function.
  */
  rc = (INT) SendMessage(hDlg, WM_INITDIALOG, DlgQueryInitialFocus(hDlg),
                         _DlgBoxlParam);

  /*
    Set the focus to the first (non-static) item in the dialog box.
  */
  if (rc && wDlg->children)
  {
    /*
      We took out the SetFocus() below since we don't want the WM_KILLFOCUS
      message being sent, as it is if we call SetFocus(). The problem with
      WM_KILLFOCUS is that some underlying controls (ie - FRAME, SCROLLBAR)
      will overwrite our dialog box when they unhighlite themselves.
    */
    DlgSetInitialFocus(hDlg);
  }

  d->hWndFocus = InternalSysParams.hWndFocus;
  _DlgBoxlParam = 0L;

  /*
    Display the dialog box.
  */
  if (!(d->flags & DLG_ENDED))
  {
    CLR_WS_HIDDEN(wDlg);
    wDlg->flags |= WS_VISIBLE;
    DialogBoxShow(hDlg);

#if defined(TELEVOICE)
    /*
      From Fred Needham:

      The line indicated is a 'kludge' by Televoice to treat 
      all modals as system-modal.  My problem was bringing up a modal 
      dialog box while several modaless boxes were active.  The 
      mouse would still activate the modaless boxes but not send 
      any message to the modaless box(es).  Without this kludge 
      the display got very confusing.  If there is a better 
      solution I would appreciate hearing about it.

      Adler's note :
        If we have a modal dialog box, and we have a modeless dialog on
      top of it, all mouse messages (even if we click on the modeless
      dialog) will go to the underlying modal dialog. This is because
      MEWEL sends all mouse messages to the System Modal dialog.
    */
    InternalSysParams.hWndSysModal = hDlg;
#endif
  }
  

  /*
    Main dialog box message loop....
  */
  while (!(d->flags & DLG_ENDED))
  {
#if defined(MOTIF)
    if (!XtIsManaged(wDlg->widget))
      break;
    while (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (!XtIsManaged(wDlg->widget) || (d->flags & DLG_ENDED))
        break;
    }
#else
    /*
      Give background events a chance to be processed....
    */
    while (!_PeekMessage(&msg))
    {
      if (!(wDlg->flags & DS_NOIDLEMSG))
        SendMessage(d->hParent, WM_ENTERIDLE, MSGF_DIALOGBOX, (DWORD) hDlg);
    }
    _WinGetMessage(&msg);
#endif


    if (msg.message == WM_QUIT)
      break;

    /*
      We need to see if there is a hook proc installed. This is
      especially important for the help code generated by Case:W.
    */
    if (InternalSysParams.pHooks[WH_MSGFILTER])
      if ((* (MYHOOKPROC *) InternalSysParams.pHooks[WH_MSGFILTER]->lpfnHook)
          (MSGF_DIALOGBOX, 0, (DWORD) (LPSTR) &msg))
        continue;

    if (InternalSysParams.pHooks[WH_COMMDLG])
      if ((* (COMMDLGHOOKPROC *) InternalSysParams.pHooks[WH_COMMDLG]->lpfnHook)
          (msg.hwnd, msg.message, msg.wParam, msg.lParam))
        continue;


    /*
      See if this is a message which is meant for the dialog box.
    */
    if (IsDialogMessage(hDlg, &msg))
        continue;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  /*
    3/29/93 (fred needham @ televoice)
    Send the last dialog control a WM_KILLFOCUS message.
  */
  if (wDlg->flags & WS_VISIBLE)
    SendMessage(d->hWndFocus, WM_KILLFOCUS, hOldFocus, 0L);

  /*
    Get rid of the dialog box and record the result
  */
  rc = d->result;

  /*
    Before the dialog is destroyed, fetch the final screen position to
    see if it was moved. If it wasn't, then we may be able to refresh
    the underlying screen from the saved screen buffer.
  */
  GetWindowRect(hDlg, &r);

  /*
    Call _WinDestroy() instead of WinDestroy() so that the underlying parent
    windows will not get redrawn (since we restore the previous screen anyway).
  */
  _WinDestroy(hDlg);
  DlgStackSP--;

  /*
    Re-Enable the parent window if this was modal
  */
#if !defined(MOTIF)
  EnableWindow(hWndOwner, TRUE);
#endif
  WinUpdateVisMap();

  /*
    Restore the underlying screen, the old cursor position, and free the
    screen buffer.
  */
  if (hScreen && EqualRect((LPRECT) &r, (LPRECT) &rOrig))
  {
    WinRestoreRect(NULLHWND, &rSaved, hScreen);
  }
  else
  {
    UnionRect(&rOrig, &rOrig, &r);
#if defined(MEWEL_TEXT)
    if (ulFlags & WS_SHADOW)
    {
      rOrig.bottom++;  rOrig.right += 2;
    }
#endif
    WinGenInvalidRects(_HwndDesktop, (LPRECT) &rOrig);
    RefreshInvalidWindows(_HwndDesktop);
  }

  /*
    Free the buffer which saved the underlying screen.
  */
  if (hScreen)
    GlobalFree(hScreen);

  /*
    Restore the old activation status. Set the current active window
    to NULL in order to insure that _WinActive() doesn't send deactivation
    messages to the just-destroyed dialog box.
  */
  InternalSysParams.hWndActive = NULLHWND;
  if (hOldActive)
    _WinActivate(hOldActive, FALSE);

  /*
    Restore the old focus window and capture window.
  */
  if (hOldFocus && hOldFocus != hOldActive)
  {
    if (hOldComboBox)   /* CFN */
      SendMessage(hOldComboBox, CB_SHOWDROPDOWN, TRUE, 0L);
    else
      SetFocus(hOldFocus);
  }

  SetCapture(hOldCapture);
  InternalSysParams.hWndSysModal = hOldModal;
  return rc;
}


/****************************************************************************/
/*                                                                          */
/* Function : DefDlgProc()                                                  */
/*                                                                          */
/* Purpose  : Window proc for dialog boxes.                                 */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LRESULT FAR PASCAL DefDlgProc(hDlg, message, wParam, lParam)
  HWND   hDlg;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  WINDOW *w;
  DIALOG *d;
  HWND   hb;
  int    id, idClass;
  LONG   rc = FALSE;
  UINT   dlgCode = 0;
  BOOL   bAlt;


  /*
    Get a pointer to the WINDOW structure
  */
  w = WID_TO_WIN(hDlg);

  /*
    Get a pointer to the dialog structure
  */
  if ((d = _DlgHwndToStruct(hDlg)) == NULL)
  {
    if ((d = (DIALOG *) (w->pPrivate = emalloc(sizeof(DIALOG)))) == NULL)
      return FALSE;
  }


#ifdef ZAPP
  if (d->idClass == DIALOG_CLASS)
  {
#endif

  /*
    Try calling the dialog proc first. If it returns a non-FALSE, then
    pass back the return code.
  */
  if (d->dlgFunc && (rc = (*d->dlgFunc)(hDlg,message,wParam,lParam)) != FALSE)
    return rc;

#ifdef ZAPP
  }
  else /* !DIALOG_CLASS */
  {
    /*
      See if the control captures all messages
    */
    if (IS_CHAR_MSG(message) && 
         InternalSysParams.hWndFocus && InternalSysParams.hWndFocus != hDlg)
    {
      UINT dlgCode = (UINT) SendMessage(InternalSysParams.hWndFocus, 
                                        WM_GETDLGCODE, 0, 0L);
      if (dlgCode & DLGC_WANTMESSAGE)
        return rc;
      if ((dlgCode & DLGC_WANTCHARS) && message == WM_CHAR)
        return rc;
    }
  }
#endif /* ZAPP */


  /*
    Big message case
  */
  switch (message)
  {
    case WM_ACTIVATE   :
      if (wParam)
      {
        /*
          Gaining activation? Set the focus to the saved control.
        */
        InternalSysParams.hWndActive = hDlg;
        SetFocus(d->hWndFocus);
      }
      else
      {
        /*
          Losing the activation? Save the handle of the focus control.
        */
        if (IsChild(hDlg, InternalSysParams.hWndFocus))
        {
          d->hWndFocus = InternalSysParams.hWndFocus;
          /*
            If the focus was set to the listbox portion of a combo, then
            set it to the combobox itself.
          */
          if (WID_TO_WIN(d->hWndFocus)->ulStyle & LBS_IN_COMBOBOX)
            d->hWndFocus = GetParent(d->hWndFocus);
        }
      }
      return TRUE;


    case WM_NCACTIVATE :
      /*
        The top-level modal dialog box should not be deactivated
      */
      if (!wParam && (d->flags&DLG_MODAL) && _HDlgStack[DlgStackSP-1] == hDlg)
        break;
      goto call_stdproc;


    case WM_SETFOCUS  :
      if (!(d->flags & DLG_MODAL))
        BringWindowToTop(hDlg);
      /*
        Set the focus back to the window in the dialog which currently has
        the focus. If no window currently has the focus, set it to the
        first control.
      */
      if (IsChild(hDlg, (HWND) wParam))
        d->hWndFocus = (HWND) wParam;
      if ((hb = d->hWndFocus) == NULL)
        hb = DlgGetFirstTabItem(hDlg);
      if (hb)
        SetFocus(d->hWndFocus = hb);
      break;


    case WM_QUIT :
      /*
        PostQuitMessage() should end the dialog
      */
      if (!(d->flags & DLG_ENDED))
        EndDialog(hDlg, 0);
      break;

    /*
      We should intercept the WM_CHAR events where the TAB and BACKTAB
      keys were pressed, and change the focus to the next/prev control.
      In additon, we may want to recognize certain special "dialog exit"
      keys, like ENTER and ESC.
    */
    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case WM_CHAR       :
#endif
      /*
        Send the WM_GETDLGCODE message to the control window.
      */
      if (InternalSysParams.hWndFocus && InternalSysParams.hWndFocus != hDlg)
        dlgCode = (UINT) SendMessage(InternalSysParams.hWndFocus,WM_GETDLGCODE,0,0L);

      if (dlgCode & (DLGC_WANTALLKEYS | DLGC_WANTCHARS))
        break;

#if defined(DOS) || defined(OS2)
      bAlt = (BOOL) ((lParam & 0x20000000L) != 0L);
      if (bAlt && wParam == ' ' && GetSystemMenu(hDlg, FALSE))
#else
#if defined(USE_WINDOWS_COMPAT_KEYS)
      if ((wParam == VK_F1 && GetKeyState(VK_CONTROL)) && GetSystemMenu(hDlg, FALSE))
#else
      if (wParam == VK_CTRL_F1 && GetSystemMenu(hDlg, FALSE))
#endif
#endif
      {
        WinActivateSysMenu(hDlg);
        return TRUE;
      }

      switch (wParam)
      {
        case VK_RETURN:
          /*
          When the user presses ENTER, we do one of the following :
            1) If a pushbutton has the focus, send a WM_COMMAND with
               wParam equal to the id of that button.
            2) We should pass the newline on to a multiline edit class.
            3) If the dlg box has a default button, send a WM_COMMAND
               with wParam equal to the id of the default button
            4) Send a WM_COMMAND with wParam equal to IDOK
          */
do_CR:
          if ((hb = InternalSysParams.hWndFocus) != NULLHWND)
            w = WID_TO_WIN(hb);

          if (hb)
          {
            idClass = _WinGetLowestClass(w->idClass);
            /*
              Give listboxes and comboboxes a shot at generating their
              WM_COMMAND/LBN_DBLCLK messages.
            */
            if (idClass == LISTBOX_CLASS || idClass == COMBO_CLASS)
              SendMessage(hb, WM_KEYDOWN, VK_RETURN, 0L);
            /*
              Don't do anything for a multi-line edit field
            */
            else if (idClass == EDIT_CLASS && (w->flags & ES_MULTILINE))
              return FALSE;
          }

          /*
            If we hit ENTER on a pushbutton, send the pushbutton's ID.
            Otherwise, fetch the id of the default pushbutton. If
            there is no default pushbutton, send IDOK.
          */
          if (hb && _IsButtonClass(hb, PUSHBUTTON_CLASS))
            id = w->idCtrl;
          else if ((hb = DlgGetDefButton(hDlg, &id)) == NULLHWND)
            id = IDOK;
          if (!IsWindowEnabled(hb))
          {
            MessageBeep(0);
            return FALSE;
          }

send_WMCOMMAND:
          SendMessage(hDlg, WM_COMMAND, id, 
                      MAKELONG(GetDlgItem(hDlg, id), BN_CLICKED));
          return TRUE;

        case VK_ESCAPE :
          /* 
            When the user presses the ESCAPE key anywhere, send a
            WM_COMMAND/IDCANCEL message to the user's dlg box function.
          */
          id = IDCANCEL;
          goto send_WMCOMMAND;

        case VK_TAB     :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
        case VK_SH_TAB :
#endif
          /*
            If we get a tab or backtab, advance to the next/prev control.
          */
          if (dlgCode & DLGC_WANTTAB)
            break;
do_dialogtab:
#if defined(USE_WINDOWS_COMPAT_KEYS)
          if (wParam == VK_TAB && (GetKeyState(VK_CONTROL) || GetKeyState(VK_MENU)))
            break;
          id = GW_HWNDNEXT;
          if (wParam == VK_LEFT || wParam == VK_UP ||
              (wParam == VK_TAB && GetKeyState(VK_SHIFT)))
            id = GW_HWNDPREV;
          return DialogTab(hDlg, id, wParam);
#else
          return DialogTab(hDlg,
            (wParam == VK_DOWN || wParam == VK_RIGHT || wParam == VK_TAB)
                                   ? GW_HWNDNEXT : GW_HWNDPREV, wParam);
#endif


        case VK_LEFT   :
        case VK_RIGHT  :
        case VK_DOWN   :
        case VK_UP     :
          if (dlgCode & DLGC_WANTARROWS)
            break;
          if (dlgCode & DLGC_BUTTON)
            goto do_dialogtab;

          /*
            Advance if we are in a listbox and we press the LEFT/RIGHT arrow 
            keys, or if we are in a single line edit class (which is not part 
            of a combo box) and we press the UP or DOWN arrow keys.
          */
          if ((hb=InternalSysParams.hWndFocus)!=NULL && (w=WID_TO_WIN(hb))!=NULL)
          {
            idClass = _WinGetLowestClass(w->idClass);
            if (idClass == LISTBOX_CLASS && !(w->flags & LBS_MULTICOLUMN) &&
                (wParam == VK_LEFT || wParam == VK_RIGHT))
              goto do_dialogtab;
            if (idClass == EDIT_CLASS && (wParam == VK_UP || wParam == VK_DOWN)
                                      && !(w->flags & ES_MULTILINE)
                                      && w->parent->idClass != COMBO_CLASS)
              goto do_dialogtab;
          }
          break;


        default :
          /*
            See if the key is a dialog accelerator (ie - ALT+letter)
          */
          if ((hb = IsDlgAccel(hDlg, wParam)) != NULLHWND)
          {
#ifndef MOVED_ON_042293  /* TELEVOICE */
            /* span the static text & go to next */
            if (IsStaticClass(hb))
              if ((hb = GetNextDlgTabItem(hDlg, hb, FALSE)) == NULLHWND)
                break;
#endif

            /*
              Make sure that we can set the focus to the new control
              (MEWEL-specific extension)
            */
            _dwValidateReason = MAKELONG(hb, wParam);
            if (!DlgSetFocus(hb))
              return TRUE;

            /*
              If an accelerator is associated with a pushbutton, then
              invoke that pushbutton. If it's with a checkbox or radiobutton,
              toggle it by simulating a keypress. If it's a listbox or
              edit control, do nothing ... just change the focus to that
              control.
            */
            if (_IsButtonClass(hb, PUSHBUTTON_CLASS))
              goto do_CR;
            else if (_IsButtonClass(hb, RADIOBUTTON_CLASS) ||
                     _IsButtonClass(hb, CHECKBOX_CLASS))
            {
              SendMessage(hb, WM_KEYDOWN, ' ', 0L);
            }
            return TRUE;
          }
          break;
      }
      break;


    case WM_CLOSE :
      hb = GetDlgItem(hDlg, IDCANCEL);
      if (hb & !IsWindowEnabled(hb))
      {
        MessageBeep(0);
      }
      else
      {
        /*
          Windows sends the WM_COMMAND/IDCANCEL when the user
          selects the Close option from the system menu.
        */
        PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
      }
      break;


    case WM_SYSCOMMAND :
      /*
        Do not process SC_NEXT/PREVWINDOW if we are a modal dialog box.
      */
      if ((wParam == SC_NEXTWINDOW || wParam == SC_PREVWINDOW) && 
          (d->flags & DLG_MODAL))
        return TRUE;
      goto call_stdproc;


    case WM_NEXTDLGCTL :
    {
      /*
        if lParam != 0, then use wParam as the window handle of the
        control to set the focus to.
        if lParam == 0, then look at wParam. If non-zero, get the
        previous control, else get the next control.
      */
      HWND hNewCtrl;

      if (lParam)
        hNewCtrl = (HWND) wParam;
      else
        hNewCtrl = GetNextDlgTabItem(hDlg, InternalSysParams.hWndFocus, wParam);
      if (hNewCtrl && hNewCtrl != InternalSysParams.hWndFocus)
      {
        _dwValidateReason = MAKELONG(hNewCtrl, VR_NEXTDLGCTRL);
        DlgSetFocus(hNewCtrl);
      }
      break;
    }

    case DM_GETDEFID :
    case DM_SETDEFID :
      return _DlgDMDefID(hDlg, wParam, message);


    default :
call_stdproc:
      rc = StdWindowWinProc(hDlg, message, wParam, lParam);
      break;
  }

  return rc;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsDlgAccel()                                                  */
/*                                                                          */
/* Purpose  : Checks to see if a key corresponds to the hotkey of a dlg     */
/*            control.                                                      */
/*                                                                          */
/* Returns  : The handle of the window whose hotkey was pressed, NULL if no.*/
/*                                                                          */
/****************************************************************************/
static HWND PASCAL IsDlgAccel(hDlg, key)
  HWND hDlg;
  int  key;
{
  HWND   hWnd;
  WINDOW *w, *wDlg;

  if (!hDlg)
    return NULLHWND;

  /*
    Make sure we have an ALT key pressed.
    For the UNIX/VMS version, we want to also use the CTRL key.
  */
#if !defined(UNIX) && !defined(VAXC)
  if (!(key & 0xFF00) || !(key & ALT_SHIFT))
    return NULLHWND;
#endif

  /*
     Given an ALT key, derive which letter was pressed.
  */
#ifdef INTERNATIONAL_MEWEL
  if (key & SPECIAL_CHAR)
    key = lang_upper((key >> 8) & 0xFF);
  else
#endif
  if ((key = AltKeytoLetter(key)) == 0)
    return NULLHWND;

  /*
    Go through all of the controls in the dialog box
  */
  wDlg = WID_TO_WIN(hDlg);
  for (w = wDlg->children;  w;  w = w->sibling)
  {
    int  letter;
    LPSTR s;

    hWnd = w->win_id;
    /*
      Find the "hot letter"
    */
    letter = (w->title && (s = lstrchr(w->title, HILITE_PREFIX)) != NULL)
                ? s[1] : 0;

    /*
      See if the letter key pressed matches the control's "hot letter"
    */
    if (key == (int) lang_upper(letter) && IsWindowEnabled(hWnd))
    {
#ifdef MOVED_ON_042293 /* TELEVOICE */
      /* span the static text & go to next */
      while (hWnd && IsStaticClass(hWnd))
        hWnd = GetWindow(hWnd, GW_HWNDNEXT);
#endif
      return hWnd;
    }
  }

  return NULLHWND;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsDialogKey()                                                 */
/*                                                                          */
/* Purpose  : See if the key is meant for the dialog manager                */
/*                                                                          */
/* Returns  : TRUE if the key should be processed by the dlg manager.       */
/*                                                                          */
/****************************************************************************/
static BOOL PASCAL IsDialogKey(hDlg, message, wParam, lParam)
  HWND hDlg;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
{
  if (IS_CHAR_MSG(message))
  {
    BOOL bAlt;

    /*
      6/14/93 (maa)
        See if the focus control wants all keyboard messages. If so, then
        pass the message onto the control.
    */
    if (InternalSysParams.hWndFocus && InternalSysParams.hWndFocus != hDlg)
    {
      UINT dlgCode = (UINT) SendMessage(InternalSysParams.hWndFocus, 
                                        WM_GETDLGCODE, 0, 0L);
      if (dlgCode & DLGC_WANTMESSAGE)
        return FALSE;
    }

    switch (wParam)
    {
      case VK_RETURN  :
      case VK_ESCAPE  :
      case VK_TAB     :
#if !defined(USE_WINDOWS_COMPAT_KEYS)
      case VK_SH_TAB :
#endif
      case VK_LEFT    :
      case VK_RIGHT   :
      case VK_DOWN    :
      case VK_UP      :
        return TRUE;
      default         :
#if defined(DOS) || defined(OS2)
        bAlt = (BOOL) ((lParam & 0x20000000L) != 0L);
        if (bAlt && wParam == ' ' && GetSystemMenu(hDlg, FALSE))
#else
#if defined(USE_WINDOWS_COMPAT_KEYS)
        if ((wParam == VK_F1 && GetKeyState(VK_CONTROL)) && GetSystemMenu(hDlg, FALSE))
#else
        if (wParam == VK_CTRL_F1 && GetSystemMenu(hDlg, FALSE))
#endif
#endif
          return TRUE;

        return IsDlgAccel(hDlg, wParam) != NULLHWND;
    }
  }
  return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsDialogMessage()                                             */
/*                                                                          */
/* Purpose  : Sees if a message is meant for the dlg box manager.           */
/*                                                                          */
/* Returns  : TRUE if the message was processed by the dlg manager.         */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsDialogMessage(hDlg, lpMsg)
  HWND  hDlg;
  LPMSG lpMsg;
{
  WINDOW *wDlg;
  DIALOG *d;
  BOOL   bIsTrueDlg;
  UINT   message;

  if ((message = lpMsg->message) == WM_CHAR)   /* Rossman 8/13/93 */
    return FALSE;

  if ((wDlg = WID_TO_WIN(hDlg)) == NULL)
    return FALSE;

  bIsTrueDlg = IS_DIALOG(wDlg);

  /*
    Get a pointer to the dialog structure
  */
  d = _DlgHwndToStruct(hDlg);

  if (lpMsg->hwnd != NULLHWND && 
       (lpMsg->hwnd == hDlg || (IsChild(hDlg, lpMsg->hwnd))))
  {
    /*
      For a modeless dialog box, we need to see if an ALT-key combo
      invokes a menubar item. Don't let DefDlgProc() handle the
      message if the keystroke is not a dialog box accelerator AND
      if TranslateAccelerator() returns TRUE.
    */
    if ((!bIsTrueDlg || !(d->flags & DLG_MODAL)) &&
         !IsDlgAccel(hDlg, lpMsg->wParam) && TranslateMessage(lpMsg))
      return FALSE;  /* meant for menubar, not the dialog box */

    /*
      Mouse messages which are meant for controls should go right
      to the controls instead of to the dialog box proc.
    */
    message = lpMsg->message;

    if (message == WM_CHAR)  /* Rossman 8/12/93 */
      return FALSE;

    if (IS_MOUSE_MSG(message) && (lpMsg->hwnd != hDlg || !bIsTrueDlg))
    {
      DispatchMessage(lpMsg);
      return TRUE;
    }
    else
    if (IS_CHAR_MSG(message) || message == WM_SETFOCUS || message == WM_QUIT)
    {
      /*
        This is still a confusing point. What exactly does Windows consider
        a dialog message for a regular window trying to emulate a dialog
        box?
      */
      if (lpMsg->hwnd != hDlg &&
          !IsDialogKey(hDlg, message, lpMsg->wParam, lpMsg->lParam))
      {
#if 31293 && !defined(ZAPP)
        /*
          3/12/93 (maa)
            We need to pass the key onto the dialog proc first. Otherwise,
            in a message box, hitting just the letter 'C' for Cancel will
            not work, as the OK pushbutton would capture all keystrokes.
        */
        if (IS_CHAR_MSG(message) &&
            SendMessage(hDlg,message,lpMsg->wParam, lpMsg->lParam) != FALSE)
          return TRUE;
#endif
        return FALSE;
      }

      return (BOOL) DefDlgProc(hDlg, message, lpMsg->wParam, lpMsg->lParam);
    }
  }

  return FALSE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : DialogBoxShow()                                                 */
/*                                                                           */
/*===========================================================================*/
static VOID PASCAL DialogBoxShow(hDlg)
  HWND hDlg;
{
  ShowWindow(hDlg, SW_SHOW);
  /*
    Now that the dialog box is visible, we should send a WM_SETFOCUS
    message to the control that has focus so it can set the cursor,
    change its shape, etc. Before we do, update the dialog box so the
    focus gets shown correctly.
  */
  RefreshInvalidWindows(_HwndDesktop);
  SendMessage(InternalSysParams.hWndFocus, WM_SETFOCUS, NULLHWND, 0L);
} 

/*===========================================================================*/
/*                                                                           */
/* Purpose : EndDialog()                                                     */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL EndDialog(hDlg, result)
  HWND hDlg;
  int  result;
{
  DIALOG *d;

  if ((d = _DlgHwndToStruct(hDlg)) == NULL)
    return FALSE;
  d->result = result;
  d->flags |= DLG_ENDED;
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : DlgGetFirstTabItem()                                            */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL DlgGetFirstTabItem(hDlg)
  HWND   hDlg;
{
  HWND   hWnd;
  WINDOW *w, *wDlg;

  if (!hDlg)
    return NULLHWND;

  wDlg = WID_TO_WIN(hDlg);
  for (w = wDlg->children;  w;  w = w->sibling)
  {
    hWnd = w->win_id;
    if (!IsStaticClass(hWnd) && IsWindowEnabled(hWnd))
    {
      /*
        If the first control in the dialog box is a radio-button group,
        then we must set the focus to the button which is checked.
      */
      if (_IsButtonClass(hWnd, RADIOBUTTON_CLASS))
      {
        HWND hRB, hLast;
        WINDOW *w2 = w;

        hLast  = _DlgGetLastControlOfGroup(hWnd);
        while (w2)
        {
          if (!_IsButtonClass(w2->win_id, RADIOBUTTON_CLASS))
            break;
          if (SendMessage(hRB = w2->win_id, BM_GETCHECK, 0, 0L))
            return hRB;
          if (hRB == hLast)        /* No buttons checked? Return hWnd, */
            break;                 /*  which should be the 1st button. */
          w2 = WID_TO_WIN(GetWindow(hRB, GW_HWNDNEXT));  /* Try the next window. */
        }
      }
      break;
    }
  }

  return w ? w->win_id : NULLHWND;
}

/*===========================================================================*/
/*                                                                           */
/* File    : DLGTAB.C                                                        */
/*                                                                           */
/* Purpose : This function returns the next/prev field of a dialog box       */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL DialogTab(hDlg, dir, key)
  HWND   hDlg;
  int    dir;
  int    key;
{
  HWND   hWnd;

#if defined(MOTIF)
  return TRUE;
#endif

  hWnd = InternalSysParams.hWndFocus;

  if (hWnd && !IsChild(hDlg, hWnd))
  {
    DIALOG *d;
    hWnd = ((d = _DlgHwndToStruct(hDlg)) != NULL) ? d->hWndFocus : NULL;
  }

  if (!hDlg || !hWnd)
    return FALSE;

  /*
    If the current focus is on one of the controls of a combo box, then
    set the start of the search to the combo box itself.
  */
  hWnd = CheckComboChild(hWnd);

#if defined(USE_WINDOWS_COMPAT_KEYS)
  if (key == VK_TAB && !GetKeyState(VK_CONTROL) && !GetKeyState(VK_MENU))
#else
  if (key == VK_TAB || key == VK_SH_TAB)
#endif
  {
#if defined(MOTIF)
    /*
      Motif handles the tabbing by itself
    */
    if (XSysParams.ulFlags & XFLAG_IGNORE_DLGTAB)
    {
      XSysParams.ulFlags &= ~XFLAG_IGNORE_DLGTAB;
      return TRUE;
    }
#endif

    if ((hWnd = GetNextDlgTabItem(hDlg, hWnd, (BOOL) (dir == GW_HWNDPREV))) != NULLHWND)
    {
      _dwValidateReason = MAKELONG(hWnd, key);
      DlgSetFocus(hWnd);
    }
  }
  else
  {
    /*
      One of the arrow keys.....
      We need to advance to the next/prev group item (usually a radio button).
    */
    if ((hWnd = GetNextDlgGroupItem(hDlg, hWnd, (BOOL) (dir == GW_HWNDPREV))) != NULLHWND &&
        !IsStaticClass(hWnd))
    {
      _dwValidateReason = MAKELONG(hWnd, key);

      if (!DlgSetFocus(hWnd))
        return TRUE;

      /*
        Only check the radiobutton if it's an autoradiobutton.
      */
      if (_IsButtonClass(hWnd, RADIOBUTTON_CLASS) &&
          (WinGetFlags(hWnd) & 0x0FL) == BS_AUTORADIOBUTTON)
      {
        HWND hFirst, hLast;
        hFirst = _DlgGetFirstControlOfGroup(hWnd);
        hLast  = _DlgGetLastControlOfGroup(hWnd);
        _CheckRadioButton(hDlg, WID_TO_WIN(hFirst)->idCtrl,
                                WID_TO_WIN(hLast)->idCtrl,
                                WID_TO_WIN(hWnd)->idCtrl, TRUE, TRUE);
      }
    }
  }

  return TRUE;
}


HWND FAR PASCAL GetNextDlgTabItem(HWND hDlg, HWND hWndCurrent, BOOL fPrevious)
{
  HWND   hWnd;
  UINT   wDir = (fPrevious) ? GW_HWNDPREV : GW_HWNDNEXT;
  BOOL   bSecondTimeAround = FALSE;

  hWndCurrent = CheckComboChild(hWndCurrent);

  hWnd = GetWindow(hWndCurrent, wDir);

  for (;;)
  {
    /*
      Find the next/prev window which is not hidden nor disabled, and has
      the WS_TABSTOP bit set.
    */
    while (hWnd && hWnd != hWndCurrent &&
          (!IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd) || 
           IsStaticClass(hWnd) ||
            !(WinGetFlags(hWnd) & WS_TABSTOP)))
      hWnd = GetWindow(hWnd, wDir);

    if (hWnd || bSecondTimeAround)
      break;

    /*
      At the end of the rope? Go back to the top/bottom.
    */
    hWnd = fPrevious ? GetWindow(hWndCurrent,GW_HWNDLAST) : GetTopWindow(hDlg);
    bSecondTimeAround++;
  }

  return (hWnd == hWndCurrent) ? NULLHWND : hWnd;
}


/*===========================================================================*/
/*                                                                           */
/* Routines to handle moving to the next/prev group item.                    */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL GetNextDlgGroupItem(hDlg, hCtrl, fPrev)
  HWND hDlg;
  HWND hCtrl;
  UINT fPrev;
{
  HWND   hWnd;
  HWND   hWndCurrent;

  (void) hDlg;

  if (!IsWindow(hCtrl))
    return NULLHWND;

  hCtrl = CheckComboChild(hCtrl);

  hWndCurrent = hWnd = hCtrl;

  if (!fPrev)
  {
    /*
      If we went past the last window or the next window starts a new group,
      then return the first control in the group.
    */
    for (;;)
    {
      hCtrl = hWnd;
      if ((hWnd = GetWindow(hCtrl, GW_HWNDNEXT)) == NULLHWND ||
                  (WinGetFlags(hWnd) & WS_GROUP))
        if ((hWnd = _DlgGetFirstControlOfGroup(hCtrl)) == NULLHWND)
          break;
      if (hCtrl == hWnd       || 
          hWnd == hWndCurrent ||
          !IsStaticClass(hWnd)&&IsWindowVisible(hWnd)&&IsWindowEnabled(hWnd))
        break;
    }
  }
  else
  {
    /*
      If we are at the start of a group, then return the last control in
      the group.
    */
    for (;;)
    {
      hCtrl = hWnd;
      if (WinGetFlags(hCtrl) & WS_GROUP)  /* at the start of the group ? */
        hWnd = _DlgGetLastControlOfGroup(hCtrl);
      else
      {
        /*
          If we are at the first control of the dialog box, then return the
          last child in the group.
        */
        if ((hWnd = GetWindow(hCtrl, GW_HWNDPREV)) == NULLHWND)
          if ((hWnd = _DlgGetLastControlOfGroup(hCtrl)) == NULLHWND)
            break;
      }
      if (hCtrl == hWnd || 
          !IsStaticClass(hWnd) && IsWindowVisible(hWnd) &&
          (IsWindowEnabled(hWnd) || hWnd == hWndCurrent))
        break;
    }
  }

  return (IsStaticClass(hWnd) || !IsWindowVisible(hWnd) || 
          !IsWindowEnabled(hWnd) || hWnd == hWndCurrent) ? NULLHWND : hWnd;
}


/****************************************************************************/
/*                                                                          */
/* Function : _DlgGetFirst/LastControlOfGroup()                             */
/*                                                                          */
/* Purpose  : Given the handle to a control, finds the first/last control   */
/*            which is in the same group as the control.                    */
/*                                                                          */
/* Returns  : The first/last control of the group.                          */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL _DlgGetFirstControlOfGroup(hCtrl)
  HWND hCtrl;
{
  HWND hPrev;

  for (hPrev = hCtrl;  hCtrl;  hCtrl = GetWindow(hCtrl, GW_HWNDPREV))
  {
    WINDOW *w = WID_TO_WIN(hCtrl);
    /*
      See if the control has the GROUP style on it. If we found a 
      groupbox, then return the first control which follows the
      groupbox. Otherwise, just return the control.

      Even though the control may not have the WS_GROUP style, we want
      to return if we encounter a non-button.
    */
    BOOL bIsButton = _IsButtonClass(hCtrl, RADIOBUTTON_CLASS) ||
                     _IsButtonClass(hCtrl, CHECKBOX_CLASS)    ||
                     _IsButtonClass(hCtrl, PUSHBUTTON_CLASS);

    if ((w->flags & WS_GROUP) || !bIsButton)
      return (bIsButton) ? hCtrl : hPrev;

    /*
      Save the current button handle
    */
    hPrev = hCtrl;
  }

  /*
    hCtrl is NULL. Return the last valid control.
  */
  return hPrev;
}

HWND FAR PASCAL _DlgGetLastControlOfGroup(hCtrl)
  HWND hCtrl;
{
  HWND hLastCtrl = hCtrl;

  for (hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
       hCtrl && !(WinGetFlags(hCtrl) & WS_GROUP);
       hCtrl = GetWindow(hCtrl, GW_HWNDNEXT))
  {
    /*
      Even though the control may not have the WS_GROUP style, we want
      to return if we encounter a non-radiobutton.
    */
    if (!_IsButtonClass(hCtrl, RADIOBUTTON_CLASS) &&
        !_IsButtonClass(hCtrl, CHECKBOX_CLASS)    &&
        !_IsButtonClass(hCtrl, PUSHBUTTON_CLASS))
      return hLastCtrl;
    hLastCtrl = hCtrl;
  }

  return hLastCtrl;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsStaticClass()                                               */
/*                                                                          */
/* Purpose  : Tests if focus can be set to a control. FALSE if the control  */
/*            is a static control or if it's a non-control scrollbar.       */
/*                                                                          */
/* Returns  : TRUE if the focus can be set to the control.                  */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsStaticClass(hWnd)
  HWND hWnd;
{
  return (BOOL)((SendMessage(hWnd, WM_GETDLGCODE, 0, 0L) & DLGC_STATIC) != 0);
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : IsDialogButtonChecked()                                         */
/*                                                                           */
/*===========================================================================*/
BOOL FAR PASCAL IsDlgButtonChecked(hDlg, id)
  HWND hDlg;
  int  id;
{
  HWND hItem;
  
  if ((hItem = GetDlgItem(hDlg, id)) == NULLHWND)
    return FALSE;
  return (BOOL) SendMessage(hItem, BM_GETCHECK, 0, 0L);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : CheckDlgButton()                                                */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL CheckDlgButton(hDlg, id, state)
  HWND hDlg;
  int  id;
  int  state;
{
  HWND hItem;
  
  if ((hItem = GetDlgItem(hDlg, id)) == NULLHWND)
    return FALSE;
  SendMessage(hItem, BM_SETCHECK, (WPARAM) state, 0L);
  return TRUE;
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : CheckRadioButton()                                              */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL CheckRadioButton(hDlg, idFirst, idLast, idItem)
  HWND hDlg;
  int  idFirst, idLast;
  int  idItem;
{
  return _CheckRadioButton(hDlg, idFirst, idLast, idItem, FALSE, TRUE);
}

INT FAR PASCAL _CheckRadioButton(HWND hDlg, int idFirst, int idLast, int idItem,
                                 BOOL bSend_WMCOMMAND, BOOL bUncheckOthers)
{
  HWND hItem;
  WINDOW *w;
  int    id;
  BOOL   bIsChecked;

  
  /*
    Sort the id's
  */
  if (idFirst > idLast)
  {
    int tmp = idFirst;
    idFirst = idLast;
    idLast = tmp;
  }

  for (id = idFirst;  id <= idLast;  id++)
  {
    if ((hItem = GetDlgItem(hDlg, id)) == NULLHWND)
      return FALSE;
    w = WID_TO_WIN(hItem);

    /* We should really insure that hItem points to a radio button */
    if (!_IsButtonClass(hItem, RADIOBUTTON_CLASS))
      continue;

    /*
      Redraw the radio button only if its state has changed
    */
    bIsChecked = (BOOL) SendMessage(hItem, BM_GETCHECK, 0, 0L);
    if (bUncheckOthers)
    {
      if (bIsChecked && idItem != id || !bIsChecked && idItem == id)
        SendMessage(hItem, BM_SETCHECK, (WPARAM) (id == idItem), 0L);
    }
    else
    {
      /*
        We are dealing with a BS_RADIOBUTTON, so we should toggle the state
        and inform the parent. We should not much with the state of any
        other radio button.
      */
      if (id == idItem)
        SendMessage(hItem, BM_SETCHECK, (WPARAM) !bIsChecked, 0L);
    }

    /*
      Set the TABSTOP to the checked radio button
    */
    if (id == idItem)
      w->flags |= WS_TABSTOP;
    else
      w->flags &= ~WS_TABSTOP;
  }

  /*
    Notify the dialog box proc
  */
  if (bSend_WMCOMMAND && idItem >= idFirst && idItem <= idLast)
    SendMessage(hDlg, WM_COMMAND, idItem,
	  	MAKELONG(GetDlgItem(hDlg, idItem), BN_CLICKED));

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetDlgItem(hDlg, id)                                          */
/*                                                                          */
/* Purpose  : Maps a dialog box control's identifier into the window handle.*/
/*                                                                          */
/* Returns  : The window handle of the matched window, NULL if no match.    */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL GetDlgItem(hDlg, id)
  HWND hDlg;
  int  id;
{
  WINDOW *w;
  WINDOW *wDlg;

  if (!hDlg)
    return NULLHWND;

  /*
    Do a fast search along the dialog box's children list. Do not recurse.
  */
  wDlg = WID_TO_WIN(hDlg);
  for (w = wDlg->children;  w;  w = w->sibling)
    if ((int) w->idCtrl == id)
      return w->win_id;
  return NULLHWND;
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : GetDialogInt()                                                  */
/*                                                                           */
/*===========================================================================*/
UINT FAR PASCAL GetDlgItemInt(HWND hDlg, int id, BOOL FAR *lpTranslated, 
                              BOOL bSigned)
{
  BYTE buf[64], *s;
  BYTE ch;
  INT  wVal;
  
  /*
    Get the text from the edit control
  */
  if (!GetDlgItemText(hDlg, id, (LPSTR) buf, sizeof(buf)))
  {
    if (lpTranslated)
      *lpTranslated = FALSE;
    return FALSE;
  }

  /*
    Span the initial run of blanks
  */
  for (s = buf;  (ch = *s) != '\0' && isspace(ch);  s++)
    ;
  if (ch == '\0' || (!isdigit(ch) && ch != '-'))
  {
    if (lpTranslated)
      *lpTranslated = FALSE;
    return FALSE;
  }

  /*
    Convert the number and check for the optional sign
  */
  wVal = atoi((char *) s);
  if (!bSigned && ch == '-')
    wVal = -wVal;

  if (lpTranslated)
    *lpTranslated = TRUE;
  return (UINT) wVal;
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : GetDialogText()                                                 */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL GetDlgItemText(hDlg, id, text, maxcount)
  HWND hDlg;
  int  id;
  LPSTR text;
  int  maxcount;
{
  return (int) SendDlgItemMessage(hDlg, id, WM_GETTEXT, maxcount, (DWORD)text);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : SendDlgItemMessage()                                            */
/*                                                                           */
/*===========================================================================*/
LONG FAR PASCAL SendDlgItemMessage(hDlg, idItem, msg, wParam, lParam)
  HWND   hDlg;
  int    idItem;
  UINT   msg;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND hItem;
  
  if ((hItem = GetDlgItem(hDlg, idItem)) == NULLHWND)
    return FALSE;
  return SendMessage(hItem, msg, wParam, lParam);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : SetDialogText()                                                 */
/*                                                                           */
/*===========================================================================*/
BOOL FAR PASCAL SetDlgItemText(hDlg, id, text)
  HWND hDlg;
  int  id;
  LPCSTR text;
{
  return (BOOL) 
      SendDlgItemMessage(hDlg, id, WM_SETTEXT, 0, (DWORD) (LPSTR) text);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : SetDialogInt()                                                  */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL SetDlgItemInt(HWND hDlg, int id, UINT wValue, BOOL bSigned)
{
  char buf[64];

  if (bSigned)
    sprintf((char *) buf, "%d", (int) wValue);
  else
    sprintf((char *) buf, "%u", wValue);
  SetDlgItemText(hDlg, id, buf);
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : SetDlgItemStyle()                                               */
/*                                                                           */
/*===========================================================================*/
BOOL FAR PASCAL SetDlgItemStyle(HWND hDlg, int id, DWORD dwStyle)
{
  HWND   hItem;
  WINDOW *w;

  if ((hItem = GetDlgItem(hDlg, id)) == NULLHWND)
    return FALSE;
  if ((w = WID_TO_WIN(hItem)) == NULL)
    return FALSE;
  w->flags |= dwStyle;
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Purpose : DlgGetDefButton()                                               */
/*    Returns the handle of the first default pushbutton of a dialog box     */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL DlgGetDefButton(HWND hDlg, int *id)
{
  WINDOW *w;
  HWND   hWnd;
  UINT   wStyle;

  if (!hDlg)
    return NULLHWND;

  for (hWnd = GetTopWindow(hDlg); hWnd; hWnd = GetWindow(hWnd,GW_HWNDNEXT))
  {
    w = WID_TO_WIN(hWnd);
    wStyle = (UINT) (w->flags & 0x0FL);
    if (_IsButtonClass(hWnd, PUSHBUTTON_CLASS) && wStyle == BS_DEFPUSHBUTTON)
    {
      if (id)
        *id = w->idCtrl;
      return hWnd;
    }
  }

  return NULLHWND;
}


static VOID PASCAL DlgSetInitialFocus(HWND hDlg)
{
  HWND   hFirstItem;
  DIALOG *d;

  hFirstItem = DlgQueryInitialFocus(hDlg);

  /*
    Try setting the focus to the first item with WS_TABSTOP
  */
  if (hFirstItem != NULL)
  {
    if ((d = _DlgHwndToStruct(hDlg)) != NULL)
      d->hWndFocus = hFirstItem;
  }
  else
    hFirstItem = hDlg;  /* Set the focus to the dialog box itself */


  /*
    (Televoice - 1/21/94)
    When CreateDialog() switches from an prior modeless dialog to the new one,
    It did not send an INACTIVATE to the prior dialog proc.  The old dialog
    did not keep track of the last control to have the focus. 
  */
  if (InternalSysParams.hWndActive)                                 /* CFN */
    SendMessage(InternalSysParams.hWndActive, WM_ACTIVATE, 0, 0L);  /* CFN */


  InternalSysParams.hWndFocus = hFirstItem;
#if 40293
  /*
    4/3/93 (maa)
      We need to send a WM_SETFOCUS message to the control so that it
    can set it's internal states correctly. 
  */
  SendMessage(hFirstItem, WM_SETFOCUS, 0, 0L);
#endif
}

static HWND PASCAL DlgQueryInitialFocus(HWND hDlg)
{
  HWND hFirstItem = NULL;

  /*
    Try setting the focus to the first item with WS_TABSTOP
  */
  if (hFirstItem == NULL)
    hFirstItem = DlgGetFirstTabItem(hDlg);

  return hFirstItem;
}


/****************************************************************************/
/*                                                                          */
/* Function : DlgSetFocus()                                                 */
/*                                                                          */
/* Purpose  : Part of new data validation stuff. Checks to see if the       */
/*            focus of the current dlg control can be changed.              */
/*                                                                          */
/* Returns  : TRUE if focus was changed, FALSE if not.                      */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DlgSetFocus(HWND hWndNew)
{
  return _DlgSetFocus(hWndNew, TRUE);
}


BOOL FAR PASCAL _DlgSetFocus(HWND hWndNew, BOOL bSetFocus)
{
  if (hWndNew == NULLHWND)
    return FALSE;

  if (InternalSysParams.hWndFocus == NULLHWND)
  {
    if (bSetFocus)
      SetFocus(hWndNew);
    return TRUE;
  }


  /*
    1) Send the WM_VALIDATE directly to the control. This will give
    subclassed controls (such as the formatted edit fields) the
    chance to perform their own internal validation, such as range
    checking.

    2) Send the WM_VALIDATE message (with wParam set to the control id)
    to the user-written dialog box proc. This gives the app a chance to
    perform any form-wide validation (such as checking the contents of
    one field against another).

    3) If both WM_VALIDATEs return TRUE, then we can safely set the
    focus to the new control.

    4) If either of the WM_VALIDATEs return FALSE, the send the
    user-written dialog box proc the WM_COMMAND/FN_ERRVALIDATE
    notification code.
  */
  if (SendMessage(InternalSysParams.hWndFocus, WM_VALIDATE, 0, _dwValidateReason) == TRUE &&
      SendMessage(GetParentOrDT(InternalSysParams.hWndFocus), WM_VALIDATE, 
                  WID_TO_WIN(InternalSysParams.hWndFocus)->idCtrl, _dwValidateReason) == TRUE)
  {
    if (bSetFocus)
      SetFocus(hWndNew);
    return TRUE;
  }
  else
  {
    /*
      WM_VALIDATE returned FALSE, so an error occured. Don't set the
      focus to the next control. Instead, issue an error notification
      message.
    */
    SendMessage(GetParentOrDT(InternalSysParams.hWndFocus),
                WM_COMMAND,
                WID_TO_WIN(InternalSysParams.hWndFocus)->idCtrl,
                MAKELONG(InternalSysParams.hWndFocus, FN_ERRVALIDATE));
    return FALSE;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : _DlgDMDefID()                                                 */
/*                                                                          */
/* Purpose  : Processes the DM_SETDEFID and DM_GETDEFID messages.           */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
static DWORD PASCAL _DlgDMDefID(hDlg, wParam, message)
  HWND hDlg;
  WPARAM wParam;   /* id of the pushbutton for the DM_SETDEFID message */
  UINT message;  /* DM_SETDEFID or DM_GETDEFID */
{
  WINDOW *w;

  /*
    Scan the dialog box's children looking for a oushbutton
  */
  for (w = WID_TO_WIN(hDlg)->children;  w;  w = w->sibling)
  {
    /*
      See if this child is a pushbutton
    */
    if (_IsButtonClass(w->win_id, PUSHBUTTON_CLASS))
    {
      /*
        If we are setting the default pushbutton, then set this one
        if the id matches, or else clear the "defaultness".
      */
      if (message == DM_SETDEFID)
      {
        if (w->idCtrl == (int) wParam)
          w->flags |= BS_DEFPUSHBUTTON;
        else
          w->flags &= ~BS_DEFPUSHBUTTON;
      }
      else /* message == DM_GETDEFID */
      {
        if (w->flags & BS_DEFPUSHBUTTON)
          return MAKELONG(w->idCtrl, DC_HASDEFID);
      }
    }
  }

  return 0L;
}


VOID FAR PASCAL MapDialogRect(hDlg, lpRect)
  HWND   hDlg;
  LPRECT lpRect;
{
  DWORD dwBase = GetDialogBaseUnits();
  (void) hDlg;
  lpRect->left   = (lpRect->left   * LOWORD(dwBase)) / ((MWCOORD) 4);
  lpRect->right  = (lpRect->right  * LOWORD(dwBase)) / ((MWCOORD) 4);
  lpRect->top    = (lpRect->top    * HIWORD(dwBase)) / ((MWCOORD) 8);
  lpRect->bottom = (lpRect->bottom * HIWORD(dwBase)) / ((MWCOORD) 8);
}


/****************************************************************************/
/*                                                                          */
/* Function : DlgGetSetWindowLong()                                         */
/*                                                                          */
/* Purpose  : Sets or retrieves one of the DWL_xxx members of the DIALOG    */
/*            structure. Called by Get/SetWindowLong().                     */
/*                                                                          */
/* Returns  : The previous value.                                           */
/*                                                                          */
/****************************************************************************/
LONG FAR PASCAL DlgGetSetWindowLong(HWND hWnd, int iLong, LONG llong, BOOL bSet)
{
  DIALOG *d;
  LONG   dwOldVal;

  if ((d = _DlgHwndToStruct(hWnd)) == NULL)
    return 0L;

  switch (iLong)
  {
    case DWL_MSGRESULT:
      dwOldVal = d->dwMsgResult;
      if (bSet)
        d->dwMsgResult = llong;
      break;

    case DWL_DLGPROC  :
      dwOldVal = (LONG) d->dlgFunc;
      if (bSet)
        d->dlgFunc = (DIALOGPROC *) llong;
      break;

    case DWL_USER     :
      dwOldVal = d->dwUser;
      if (bSet)
        d->dwUser = llong;
      break;
  }

  return dwOldVal;
}

BOOL FAR PASCAL _IsDialogModal(hWnd)
  HWND hWnd;
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != NULL && IS_DIALOG(w))
  {
    DIALOG *d = _DlgHwndToStruct(hWnd);
    return (d && (d->flags & DLG_MODAL));
  }
  return FALSE;
}


static HWND PASCAL CheckComboChild(hWnd)
  HWND hWnd;
{
  WINDOW *w;

  /*
    If the window is a child of a combobox, return the handle of 
    the combobox itself.
  */
  if ((w = WID_TO_WIN(hWnd)) != NULL)
    if ((w=w->parent) != NULL && _WinGetLowestClass(w->idClass) == COMBO_CLASS)
      hWnd = w->win_id;

  return hWnd;
}


#if defined(ZAPP)

LONG FAR PASCAL GetDialogBaseUnits(void)
{
  TEXTMETRIC tm;
  LONG       tmpX;
  INT        dx, dy;
  HDC        hDC;
  DWORD      tExtents;

  LPSTR aveChString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  static long _zBaseUnits = 0;

  if (_zBaseUnits)
    return _zBaseUnits;

  /*
    The following calculations are done only one time
  */
  hDC = GetDC(_HwndDesktop);

  /*
    Get the extent of the test string. Get the average character width
    and round it up.
  */
  tExtents = GetTextExtent(hDC, aveChString, 52);
  tmpX = ((DWORD) LOWORD(tExtents) / 52L);
  if (((DWORD) LOWORD(tExtents) % 52L) >= 26L)
    tmpX++;

  dx = LOWORD(tmpX);

  /*
    Get the height of the test string
  */
  GetTextMetrics(hDC, &tm);
  dy = (tm.tmHeight + tm.tmExternalLeading);
  
  /*
    Set the dialog units
  */
  _zBaseUnits = MAKELONG(dx, dy);

  ReleaseDC(_HwndDesktop, hDC);

  return _zBaseUnits;
}

#else
LONG FAR PASCAL GetDialogBaseUnits(void)
{
#ifdef MEWEL_GUI
  return MAKELONG(8, 16);
#else
  return MAKELONG(4, 8);
#endif
}
#endif

