/*===========================================================================*/
/*                                                                           */
/* File    : WCOMFILE.C                                                      */
/*                                                                           */
/* Purpose : Implements the standard open and save dialog boxes              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOGDI
#define NOCOMM
#define NOGRAPHICS
#define NOOBJECTS
#define NOKERNEL

#include "wprivate.h"
#include "window.h"
#include "commdlg.h"

/* #define WM_FINDREPLACE  (0xC000 + 157) */

#define ID_FINDTEXT     100
#define ID_REPLTEXT     101
#define ID_WHOLEWORDCB  102
#define ID_DOWNRB       103
#define ID_UPRB         104
#define ID_MATCHCASECB  105
#define ID_REPLACE      106
#define ID_REPLACEALL   107
#define ID_UPDOWNGB     108


#define CDERR_INITIALIZATION   1
#define CDERR_NOTEMPLATE       2
#define CDERR_LOADRESFAILURE   3


#define OFR_FINDDIALOG  1
#define OFR_REPLDIALOG  2

typedef struct tagInternalOFR
{
  LPFINDREPLACE  lpofr;
  UINT         fIntFlags;
  HHOOK        hHook;
} INTERNALFINDREPLACE, FAR *LPINTERNALFINDREPLACE;

/*
  Declaring this as a static removes any chance of reentrancy here!
*/
static INTERNALFINDREPLACE  InternalOfr;
static INT                  iCommDlgError;
static UINT                 uFindReplaceMsg;

static HWND FAR PASCAL _FindReplace(LPFINDREPLACE, INT);
extern VOID     PASCAL SetCommDlgError(INT);
static INT  FAR PASCAL FindDlgProc(HWND, UINT, WPARAM, LPARAM);


#if 0
/*
  These are defined in wcomfile.c
*/
DWORD FAR PASCAL CommDlgExtendedError(void)
{
  return (DWORD) iCommDlgError;
}

static VOID PASCAL SetCommDlgError(iError)
  INT iError;
{
  iCommDlgError = iError;
}
#endif



HWND FAR PASCAL FindText(lpOfr)
  LPFINDREPLACE lpOfr;
{
  return _FindReplace(lpOfr, OFR_FINDDIALOG);
}

HWND FAR PASCAL ReplaceText(lpOfr)
  LPFINDREPLACE lpOfr;
{
  return _FindReplace(lpOfr, OFR_REPLDIALOG);
}


static HWND FAR PASCAL _FindReplace(lpOfr, iMode)
  LPFINDREPLACE lpOfr;
  INT            iMode;
{
  HWND hDlg = NULLHWND;
  FARPROC lpFindProc;

  if (lpOfr == NULL)
  {
    SetCommDlgError(CDERR_INITIALIZATION);
    return FALSE;
  }

  InternalOfr.lpofr = lpOfr;
  InternalOfr.fIntFlags = iMode;

  lpFindProc = MakeProcInstance(FindDlgProc, lpOfr->hInstance);

  if (lpOfr->Flags & FR_ENABLETEMPLATE)
  {
    if (!lpOfr->lpTemplateName)
    {
      SetCommDlgError(CDERR_NOTEMPLATE);
      return FALSE;
    }

    hDlg = CreateDialog(lpOfr->hInstance, (LPSTR) lpOfr->lpTemplateName,
                        lpOfr->hwndOwner, lpFindProc);
  }
  else
  {
#if 0
    hDlg = CreateDialogIndirect(lpOfr->hInstance, (LPSTR) &FileDlgTemplate,
                                lpOfr->hwndOwner, lpFindDlgProc);
#else
    hDlg = CreateDialog(lpOfr->hInstance, 
                     (iMode == OFR_FINDDIALOG) ? (LPSTR) "FindText"
                                               : (LPSTR) "ReplaceText",
                     lpOfr->hwndOwner, lpFindProc);

#endif
  }

#if 0
  if (!hDlg)
  {
    SetCommDlgError(CDERR_LOADRESFAILURE);
    return FALSE;
  }
  _DialogBoxParam(hDlg, (DWORD) (LPSTR) &InternalOfr);
#endif

  if ( uFindReplaceMsg == 0 )
    uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
  return hDlg;
}


static INT FAR PASCAL FindDlgProc(hDlg, message, wParam, lParam)
  HWND   hDlg;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  LPINTERNALFINDREPLACE lpIntOfr;
  LPFINDREPLACE lpOfr;

  (void) lParam;

  lpIntOfr = &InternalOfr;
  lpOfr = lpIntOfr->lpofr;

  switch (message)
  {
    case WM_INITDIALOG :
      /*
        Set the initial contents of the edit controls
      */
      if (lpOfr->lpstrFindWhat && lpOfr->lpstrFindWhat[0])
        SetDlgItemText(hDlg, ID_FINDTEXT, lpOfr->lpstrFindWhat);
      if (lpIntOfr->fIntFlags & OFR_REPLDIALOG)
        if (lpOfr->lpstrReplaceWith && lpOfr->lpstrReplaceWith[0])
          SetDlgItemText(hDlg, ID_REPLTEXT, lpOfr->lpstrReplaceWith);

      /*
        Set up the hook stuff. lParam of the WM_INITDIALOG message contains
        the custom hook data.
      */
      if ((lpOfr->Flags & FR_ENABLEHOOK) && lpOfr->lpfnHook)
        lpIntOfr->hHook = SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC) lpOfr->lpfnHook, 0, 0);

      /*
        Do some flags stuff
      */
      if (!(lpOfr->Flags & FR_SHOWHELP))
        ShowWindow(GetDlgItem(hDlg, IDHELP), SW_HIDE);

      if (lpOfr->Flags & FR_HIDEWHOLEWORD)
        ShowWindow(GetDlgItem(hDlg, ID_WHOLEWORDCB), SW_HIDE);
      if (lpOfr->Flags & FR_NOWHOLEWORD)
        EnableWindow(GetDlgItem(hDlg, ID_WHOLEWORDCB), FALSE);
      if (lpOfr->Flags & FR_WHOLEWORD)
        CheckDlgButton(hDlg, ID_WHOLEWORDCB, TRUE);

      CheckRadioButton(hDlg, ID_DOWNRB, ID_UPRB, ID_DOWNRB + 
        ((lpOfr->Flags & FR_DOWN) ? 0 : 1));
      if (lpOfr->Flags & FR_NOUPDOWN)
      {
        EnableWindow(GetDlgItem(hDlg, ID_DOWNRB), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_UPRB), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_UPDOWNGB), FALSE);
      }
      if (lpOfr->Flags & FR_HIDEUPDOWN)
      {
        ShowWindow(GetDlgItem(hDlg, ID_DOWNRB), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, ID_UPRB), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, ID_UPDOWNGB), SW_HIDE);
      }

      if (lpOfr->Flags & FR_HIDEMATCHCASE)
        ShowWindow(GetDlgItem(hDlg, ID_MATCHCASECB), SW_HIDE);
      if (lpOfr->Flags & FR_NOMATCHCASE)
        EnableWindow(GetDlgItem(hDlg, ID_MATCHCASECB), FALSE);
      if (lpOfr->Flags & FR_MATCHCASE)
        CheckDlgButton(hDlg, ID_MATCHCASECB, TRUE);

      return TRUE;


    case WM_COMMAND :
      switch (wParam)
      {
        case IDOK         :
        case ID_REPLACE   :
        case ID_REPLACEALL:
          /*
            Get the find and replace text
          */
          GetDlgItemText(hDlg, ID_FINDTEXT, lpOfr->lpstrFindWhat,
                                            lpOfr->wFindWhatLen);
          if (lpIntOfr->fIntFlags & OFR_REPLDIALOG)
            GetDlgItemText(hDlg, ID_REPLTEXT, lpOfr->lpstrReplaceWith,
                                              lpOfr->wReplaceWithLen);

          /*
            Test the whole-word indicator
          */
          if (IsDlgButtonChecked(hDlg, ID_WHOLEWORDCB))
            lpOfr->Flags |=  FR_WHOLEWORD;
          else
            lpOfr->Flags &= ~FR_WHOLEWORD;

          /*
            Test the case indicator
          */
          if (IsDlgButtonChecked(hDlg, ID_MATCHCASECB))
            lpOfr->Flags |=  FR_MATCHCASE;
          else
            lpOfr->Flags &= ~FR_MATCHCASE;

          /*
            Test the up/down indicator
           * replace always does find next!
          */
          lpOfr->Flags |=  FR_DOWN;
          if (lpIntOfr->fIntFlags & OFR_FINDDIALOG)
            if (!IsDlgButtonChecked(hDlg, ID_DOWNRB))
              lpOfr->Flags &= ~FR_DOWN;

          /*
            Set the operation flags
          */
          lpOfr->Flags &= ~( FR_DIALOGTERM | FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
          if (wParam == IDOK)
            lpOfr->Flags |= FR_FINDNEXT;
          else if (wParam == ID_REPLACE)
            lpOfr->Flags |= FR_REPLACE;
          else
            lpOfr->Flags |= FR_REPLACEALL;

          /*
            Notify the app
          */
          SendMessage(lpOfr->hwndOwner, uFindReplaceMsg, 0, (LONG) lpOfr);
          break;


        case IDCANCEL :
          lpOfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
          lpOfr->Flags |= FR_DIALOGTERM;
          SendMessage(lpOfr->hwndOwner, uFindReplaceMsg, 0, (LONG) lpOfr);
          DestroyWindow(hDlg);
          break;

        case IDHELP   :
#if 32693
          SendMessage(lpOfr->hwndOwner, FindAtom(HELPMSGSTRING), hDlg, 0L);
#else
          MessageBox(hDlg, "Help not implemented yet.", "Save File", MB_OK);
#endif
          break;

      } /* switch (wParam) */
      return TRUE;

    case WM_CLOSE:
      return PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
  }

  return FALSE;
}

