/*===========================================================================*/
/*                                                                           */
/* File    : WPRNTDLG.C                                                      */
/*                                                                           */
/* Purpose : Implements the common dialog for Printing                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "commdlg.h"
#include "dlgs.h"

#define CDERR_INITIALIZATION   1
#define CDERR_NOTEMPLATE       2
#define CDERR_LOADRESFAILURE   3
extern VOID PASCAL SetCommDlgError(INT);

#if 0
typedef struct tagPD
{
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    HDC     hDC;
} PRINTDLG;

#define PD_RETURNDC                  0x00000100
#define PD_RETURNIC                  0x00000200

typedef struct tagDEVNAMES
{
    UINT wDriverOffset;
    UINT wDeviceOffset;
    UINT wOutputOffset;
    UINT wDefault;
} DEVNAMES;
typedef DEVNAMES FAR* LPDEVNAMES;

#define DN_DEFAULTPRN      0x0001
#endif



typedef struct tagInternalPD
{
  PRINTDLG     pd;
} INTERNALPRINTDLG, *LPINTERNALPRINTDLG;

static LPINTERNALPRINTDLG lpInternalPD = NULL;

static INT  FAR PASCAL PrintDlgProc(HWND, UINT, WPARAM, LPARAM);


BOOL WINAPI PrintDlg(lpPrDlg)
  PRINTDLG FAR *lpPrDlg;
{
  INTERNALPRINTDLG   myInternalPD;
  LPINTERNALPRINTDLG lpSaveInternalPD;

  HWND hDlg = NULLHWND;
  INT  rc = FALSE;
  FARPROC lpDlgProc;
  DWORD Flags;


  (void) hDlg;

  if (lpPrDlg == NULL)
  {
    SetCommDlgError(CDERR_INITIALIZATION);
    return FALSE;
  }

  lpSaveInternalPD = lpInternalPD;
  lpInternalPD = &myInternalPD;
  memset(lpInternalPD, 0, sizeof(myInternalPD));

  /*
    Set the MEWEL instance to the fd of the open resource file
  */
  if (lpPrDlg->hInstance <= 0)
    lpPrDlg->hInstance = MewelCurrOpenResourceFile;

  /*
    Copy the passed PRINTDLG structure over to an internal memory location
    so that we can modify it without affecting the caller.
  */
  lmemcpy((LPSTR) &lpInternalPD->pd, (LPSTR) lpPrDlg, sizeof(PRINTDLG));

  /*
    Make an instance thunk for the dialog proc.
  */
  lpDlgProc = MakeProcInstance(PrintDlgProc, lpPrDlg->hInstance);


  /*
    See if we are loading a custom dialog box template
  */
  Flags = lpPrDlg->Flags;
  if (Flags & (PD_ENABLEPRINTTEMPLATE | PD_ENABLESETUPTEMPLATE))
  {
    LPCSTR lpName =((Flags & PD_ENABLESETUPTEMPLATE) && (Flags & PD_PRINTSETUP))
      ? lpPrDlg->lpSetupTemplateName : lpPrDlg->lpPrintTemplateName;

    /*
      Make sure we have a valid template name
    */
    if (!lpName)
    {
      SetCommDlgError(CDERR_NOTEMPLATE);
      goto bye;
    }

    rc = DialogBoxParam(lpPrDlg->hInstance, (LPSTR) lpName,
                        lpPrDlg->hwndOwner, lpDlgProc, (LONG) lpPrDlg);
  }
  else if (Flags & (PD_ENABLEPRINTTEMPLATEHANDLE|PD_ENABLESETUPTEMPLATEHANDLE))
  {
    HGLOBAL hT =((Flags & PD_ENABLESETUPTEMPLATEHANDLE) && (Flags & PD_PRINTSETUP))
                    ? lpPrDlg->hSetupTemplate : lpPrDlg->hPrintTemplate;
    rc = DialogBoxIndirectParam(lpPrDlg->hInstance, hT,
                        lpPrDlg->hwndOwner, lpDlgProc, (LONG) lpPrDlg);
  }
  else
  {
    LPCSTR lpName = (Flags & PD_PRINTSETUP) ? "PRINTSETUPORD" : "PRINTDLGORD";
    rc = DialogBoxParam(lpPrDlg->hInstance, (LPSTR) lpName,
                        lpPrDlg->hwndOwner, lpDlgProc, (LONG) lpPrDlg);
  }


#ifndef IMS
  /*
    Copy the modified PRINTDLG structure back to the passed structure
  */
  if (rc)
    lmemcpy((LPSTR) lpPrDlg, (LPSTR) &lpInternalPD->pd, sizeof(PRINTDLG));
#endif


bye:
  lpInternalPD = lpSaveInternalPD;
  return (BOOL) rc;
}



static INT FAR PASCAL PrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPINTERNALPRINTDLG lpIntPD;
  LPPRINTDLG         lpPD;
  DWORD              Flags;
  DWORD              NewFlags;

  (void) lParam;


  lpIntPD = lpInternalPD;
  lpPD    = &lpIntPD->pd;
  Flags   = lpPD->Flags;

  switch (message)
  {
    case WM_INITDIALOG :
      /*
        Check the RETURNDEFAULT flag
      */
      if (Flags & PD_RETURNDEFAULT)
      {
        /*
          Fill in the hDevMode and hDevNames structures for the default
          printer. Do not show the dialog box.
          If there is no default printer and the PD_NOWARNING flag is
          set, then do not warn the user about no default printer.
        */
        (void) lpPD->hDevMode;
        (void) lpPD->hDevNames;
        EndDialog(hDlg, TRUE);
        return TRUE;
      }

      /*
        Set up the hook stuff. lParam of the WM_INITDIALOG message contains
        the custom hook data.
      */
      if ((Flags & PD_ENABLEPRINTHOOK) && lpPD->lpfnPrintHook)
        SetWindowsHook(WH_COMMDLG, (FARPROC) lpPD->lpfnPrintHook);
      else if ((Flags & PD_ENABLESETUPHOOK) && lpPD->lpfnSetupHook)
        SetWindowsHook(WH_COMMDLG, (FARPROC) lpPD->lpfnSetupHook);

      /*
        Do some flags stuff
      */

      /*
        Process the Print Range groupbox
      */
      if      ((Flags & (PD_SELECTION | PD_NOSELECTION)) == PD_SELECTION)
      {
        CheckRadioButton(hDlg, rad1, rad3, rad2);
      }
      else if ((Flags & (PD_PAGENUMS  | PD_NOPAGENUMS))  == PD_PAGENUMS)
      {
        CheckRadioButton(hDlg, rad1, rad3, rad3);
        if (lpPD->nFromPage != 0xFFFF)
          SetDlgItemInt(hDlg, edt1, lpPD->nFromPage, FALSE);
        if (lpPD->nToPage != 0xFFFF)
          SetDlgItemInt(hDlg, edt2, lpPD->nToPage, FALSE);

        /*
          NOTE:
          What do we do with the nMinPage and nMaxPage fields???
        */
      }
      else /* if ((Flags & PD_ALLPAGES) || !(Flags & (PD_SELECTION|PD_PAGENUMS))) */
        CheckRadioButton(hDlg, rad1, rad3, rad1);

      if (Flags & PD_NOPAGENUMS)
      {
        EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
        EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
        EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
      }
      else if (Flags & PD_NOSELECTION)
      {
        EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
      }


      /*
        Process the Copies edit field
      */
      if (lpPD->hDevMode == NULL)
        SetDlgItemInt(hDlg, edt3, lpPD->nCopies, FALSE);
#ifdef NOTYET
      else
        SetDlgItemInt(hDlg, edt3, lpPD->nCopies, lpDM->dmCopies);
#endif

      /*
        Process the Collate checkbox
      */
      CheckDlgButton(hDlg, chx2, (Flags & PD_COLLATE) != 0);

      /*
        Process the Print to File checkbox
      */
      if (Flags & PD_HIDEPRINTTOFILE)
        ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);
      else if (Flags & PD_DISABLEPRINTTOFILE)
        EnableWindow(GetDlgItem(hDlg, chx1), FALSE);
      else
        CheckDlgButton(hDlg, chx1, (Flags & PD_PRINTTOFILE) != 0);

      /*
        Hide or show the HELP pushbutton
      */
      if (!(Flags & PD_SHOWHELP))
        ShowWindow(GetDlgItem(hDlg, 1038), SW_HIDE);

      /*
        If there is a hook set, use it for the WM_INITDLG message
      */
    if (InternalSysParams.pHooks[WH_COMMDLG])
        if ((*(COMMDLGHOOKPROC *) InternalSysParams.pHooks[WH_COMMDLG]->lpfnHook)
              (hDlg, message, wParam, lParam))
          return FALSE;

      return TRUE;


    case WM_COMMAND :
      switch (wParam)
      {
        case IDOK :
          NewFlags = 0L;

          if (IsDlgButtonChecked(hDlg, rad2))
          {
            NewFlags |= PD_SELECTION;
          }
          else if (IsDlgButtonChecked(hDlg, rad3))
          {
            NewFlags |= PD_PAGENUMS;
            lpPD->nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
            lpPD->nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
            /*
              NOTE:
              What do we do with the nMinPage and nMaxPage fields???
            */
          }
          else if (IsDlgButtonChecked(hDlg, rad1))
          {
            NewFlags |= PD_ALLPAGES;
          }

          lpPD->nCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);

          if (IsDlgButtonChecked(hDlg, chx1))
            NewFlags |= PD_PRINTTOFILE;
          if (IsDlgButtonChecked(hDlg, chx2))
            NewFlags |= PD_COLLATE;

          /*
            Possibly create a DC or IC and return it.
          */
/*#ifdef NOTYET*/
          {
          LPCSTR lpDriver;
          LPCSTR lpDevice;
          LPCSTR lpOutput;

          lpDriver = "HPLaser";
          lpDevice = "HPLaser";
          lpDevice = "LPT1";

          if (Flags & PD_RETURNDC)
            lpPD->hDC = CreateDC(lpDriver, lpDevice, lpOutput, NULL);
          else if (Flags & PD_RETURNIC)
            lpPD->hDC = CreateIC(lpDriver, lpDevice, lpOutput, NULL);
          }
/*#endif*/

          /*
            Set the new flags into the Flags member
          */
          lpPD->Flags &= ~(PD_ALLPAGES | PD_COLLATE | PD_SELECTION |
                           PD_PRINTTOFILE | PD_PAGENUMS);
          lpPD->Flags |= NewFlags;

          /*
            To do :
              Create the hDevMode and hDevNames structures
          */
          (void) lpPD->hDevMode;
          (void) lpPD->hDevNames;

          SetWindowsHook(WH_COMMDLG, (FARPROC) NULL);
          EndDialog(hDlg, TRUE);
          break;

        case IDCANCEL :
          SetWindowsHook(WH_COMMDLG, (FARPROC) NULL);
          EndDialog(hDlg, FALSE);
          break;

        case pshHelp :
          break;

        case psh1    :  /* setup */
          break;

      } /* switch (wParam) */
      return TRUE;


    case WM_KEYDOWN    :
      if (wParam == VK_F1 && (Flags & PD_SHOWHELP))
      {
        SendMessage(hDlg, WM_COMMAND, pshHelp, 0L);
        return TRUE;
      }
      break;
  }

  return FALSE;
}

