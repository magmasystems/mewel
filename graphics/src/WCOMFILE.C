/*===========================================================================*/
/*                                                                           */
/* File    : WCOMFILE.C                                                      */
/*                                                                           */
/* Purpose : Implements the standard open and save dialog boxes              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOGDI
#define NOCOMM
#define NOGRAPHICS
#define NOOBJECTS
#define NOKERNEL

#ifdef __cplusplus
extern "C" {
#endif

#include "wprivate.h"
#include "window.h"
#include "commdlg.h"

/*
  Take the IDs from COMMDLG.H to keep track with the user's RC file
*/
#define ID_FILEEDIT       COMMFILE_EDIT
#define ID_FILELIST       COMMFILE_FILES
#define ID_FILTERCOMBO    COMMFILE_FILTERS
#define ID_PATH           COMMFILE_PATH
#define ID_DIRLIST        COMMFILE_DIRS
#define ID_DRIVECOMBO     COMMFILE_DRIVES
#define ID_READONLYCB     COMMFILE_READONLY

#define CDERR_INITIALIZATION   1
#define CDERR_NOTEMPLATE       2
#define CDERR_LOADRESFAILURE   3

#if defined(UNIX)
#define MASK_FILESONLY    0x8000
#else
#define MASK_FILESONLY    0x0000
#endif

#define MSG(x) apcCommFileStr[(x) - INTL_COMMFILE_FIRST]
static char* apcCommFileStr[INTL_COMMFILE_COUNT] = {
  "File not found. Create a new one?",  /* INTL_CREATEPROMPT    */
  "File already exists. Overwrite?"     /* INTL_OVERWRITEPROMPT */
};

#ifdef INTERNATIONAL_MEWEL
static void GetIntlCommFileStrings(void)
{
  extern int MewelCurrOpenResourceFile;
  static BOOL fStringsRead = FALSE;

  if (MewelCurrOpenResourceFile != -1 && !fStringsRead)
  {
    int i;

    for (i = 0;  i < INTL_COMMFILE_COUNT;  i++)
    {
      char acBuffer[80];
      int iLength;

      iLength = LoadString(MewelCurrOpenResourceFile,
                           INTL_COMMFILE_FIRST + i, acBuffer, 80);
      if (iLength > 0) apcCommFileStr[i] = lstrsave(acBuffer);
    }

    fStringsRead = TRUE;
  }
}
#else
#define GetIntlCommFileStrings()
#endif /* INTERNATIONAL_MEWEL */

#define OFN_SAVEDIALOG  1
#define OFN_OPENDIALOG  2

typedef struct tagInternalOFN
{
  OPENFILENAME ofn;
  UINT         fIntFlags;
  char         szStartingDir[MAXPATH];
  PLIST        listFilterSpecs;
  HHOOK        hHook;
} INTERNALOPENFILENAME, *LPINTERNALOPENFILENAME;

/*
  Declaring this as a static removes any chance of reentrancy here!
*/
static LPINTERNALOPENFILENAME lpInternalOfn = NULL;
static INT                    iCommDlgError;
static PSTR                   pszEverything = "*.*";


static BOOL FAR PASCAL _GetOpenFileName(LPOPENFILENAME, INT);
extern BOOL FAR PASCAL _XGetOpenFileName(LPOPENFILENAME, INT);
extern VOID     PASCAL SetCommDlgError(INT);
static INT  FAR PASCAL OpenDlgProc(HWND, UINT, WPARAM, LPARAM);
static INT      PASCAL DlgAddCorrectExtension(PSTR, PSTR, BOOL);
static INT      PASCAL GetCurrFilter(HWND, LPSTR);
extern INT  FAR PASCAL SpecHasWildcards(LPSTR);


DWORD FAR PASCAL CommDlgExtendedError(void)
{
  return (DWORD) iCommDlgError;
}

VOID PASCAL SetCommDlgError(iError)
  INT iError;
{
  iCommDlgError = iError;
}


#if !defined(UNIX) && !defined(VAXC)

static int ChangeDriveAndDir(char* pcDir)
{
  int iError;

  if (!pcDir || !pcDir[0])
    return 0;

  /*
    If it's not only a drive and a colon, then try to change the directory
  */
  if (pcDir[1] != ':' || pcDir[2])
  {
    /*
      Consider a trailing '\', which is only valid for the root
      directory (MSDOS = QDOS = Quick and Dirty OS...)
    */
    char* pcLast = pcDir + strlen(pcDir) - 1;
    if (*pcLast == '\\' && strcmp(pcDir, "\\") && strcmp(pcDir + 1, ":\\"))
    {
      *pcLast = 0;
      iError = chdir(pcDir);
      *pcLast = '\\';
    }
    else
      iError = chdir(pcDir);

    /*
      Return on error
    */
    if (iError)
    {
      CLR_INT24_ERR();
      return iError;
    }
  }

  /*
    If there is a drive letter, change also the default drive
  */
  if (pcDir[1] == ':')
  {
#if defined(__DPMI32__)
    unsigned nDrives;
    _dos_setdrive(pcDir[0] & 0x1F, &nDrives);
    _dos_getdrive(&nDrives);
    if (nDrives != (pcDir[0] & 0x1F))
    {
      CLR_INT24_ERR();
      return -1;
    }
#else
    union REGS r;
    r.h.ah = 0x0E;
    r.h.dl = (pcDir[0] - 1) & 0x1F;
    intdos(&r, &r);
    if (r.x.cflag)
    {
      CLR_INT24_ERR();
      return -1;
    }
#endif
  }
  return 0;
}

/*
  ChangeDriveAndDir() calls chdir(), so put the #define here <g>
*/
#define chdir ChangeDriveAndDir

/*
  As there can be gaps between drive letters (CD-ROMs or Network),
  CB_SETCURSEL(cDrive-'A') doesn't always work
*/
static void SelectDriveInCombo(HWND hwndDlg, char cDrive)
{
  char acDriveString[6];

  wsprintf(acDriveString, "[-%c-]", toupper(cDrive));
  SendDlgItemMessage(hwndDlg, ID_DRIVECOMBO, CB_SELECTSTRING,
                     (WPARAM)-1, (LONG)(LPSTR)acDriveString);
}

#endif /* !defined(UNIX) && !defined(VAXC) */


BOOL FAR PASCAL GetOpenFileName(lpOfn)
  LPOPENFILENAME lpOfn;
{
#if defined(MOTIF)
  if (GetProfileInt("motif", "UseFileSelectionWidget", 0))
    return _XGetOpenFileName(lpOfn, OFN_OPENDIALOG);
  else
#endif /* MOTIF */
    return _GetOpenFileName(lpOfn, OFN_OPENDIALOG);
}

BOOL FAR PASCAL GetSaveFileName(lpOfn)
  LPOPENFILENAME lpOfn;
{
#if defined(MOTIF)
  if (GetProfileInt("motif", "UseFileSelectionWidget", 0))
    return _XGetOpenFileName(lpOfn, OFN_SAVEDIALOG);
  else
#endif
    return _GetOpenFileName(lpOfn, OFN_SAVEDIALOG);
}


static BOOL FAR PASCAL _GetOpenFileName(lpOfn, iMode)
  LPOPENFILENAME lpOfn;
  INT            iMode;
{
  HWND hDlg = NULLHWND;
  INT  rc;
  FARPROC lpOpenDlgProc;
  LPINTERNALOPENFILENAME lpSaveInternalOfn;
  INTERNALOPENFILENAME myInternalOfn;

  (void) hDlg;

  if (lpOfn == NULL)
  {
    SetCommDlgError(CDERR_INITIALIZATION);
    return FALSE;
  }

  lpSaveInternalOfn = lpInternalOfn;
  lpInternalOfn = &myInternalOfn;
  memset(lpInternalOfn, 0, sizeof(myInternalOfn));

  /*
    Don't display previous Int24 errors caused by the application
  */
  CLR_INT24_ERR();

  /*
    Set the MEWEL instance to the fd of the open resource file
  */
  if (lpOfn->hInstance <= 0)
    lpOfn->hInstance = MewelCurrOpenResourceFile;

  lmemcpy((LPSTR) &lpInternalOfn->ofn, (LPSTR) lpOfn, sizeof(OPENFILENAME));
  lpInternalOfn->fIntFlags = iMode;

  lpOpenDlgProc = MakeProcInstance(OpenDlgProc, lpOfn->hInstance);

  /*
    Invoke the dialog box
  */
  if (lpOfn->Flags & OFN_ENABLETEMPLATE)
  {
    if (!lpOfn->lpTemplateName)
    {
      SetCommDlgError(CDERR_NOTEMPLATE);
      lpInternalOfn = lpSaveInternalOfn;
      return FALSE;
    }

    rc = DialogBoxParam(lpOfn->hInstance, (LPSTR) lpOfn->lpTemplateName,
                     lpOfn->hwndOwner, lpOpenDlgProc, (LONG) lpOfn);
  }
  else
  {
    rc = DialogBoxParam(lpOfn->hInstance, (LPSTR) "CommFileDlg",
                     lpOfn->hwndOwner, lpOpenDlgProc, (LONG) lpOfn);
  }

  /*
    Check for rc == -1, which means we couldn't find the dialog
  */
  if (rc == -1)
  {
    SetCommDlgError(CDERR_LOADRESFAILURE);
    lpInternalOfn = lpSaveInternalOfn;
    return FALSE;
  }

  /*
    Copy the return values...
  */
  if (rc)
    lmemcpy((LPSTR) lpOfn, (LPSTR) &lpInternalOfn->ofn, sizeof(OPENFILENAME));

  lpInternalOfn = lpSaveInternalOfn;
  return (BOOL) rc;
}


static INT FAR PASCAL OpenDlgProc(hDlg, message, wParam, lParam)
  HWND   hDlg;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  char  szFileName[MAXPATH];  /* used also as near buffer for near/far mism */
  char  szFilter[16];
  LPINTERNALOPENFILENAME lpIntOfn;
  LPOPENFILENAME lpOfn;
  HWND  hwndFilterCombo;
  HWND  hEdit;
  LPSTR pSlash, pDot;
  BYTE  chDrive;
  INT   cchDir;
  INT   iSel;
  BOOL  bDontAddDefExt = FALSE;

  lpIntOfn = lpInternalOfn;
  lpOfn = &lpIntOfn->ofn;

  hEdit = GetDlgItem(hDlg, ID_FILEEDIT);
  hwndFilterCombo = GetDlgItem(hDlg, ID_FILTERCOMBO);

  switch (message)
  {
    case WM_INITDIALOG :
      GetIntlCommFileStrings();

      /*
        Record what the current directory is. If the user passed in a
        filename which has an entire file spec, then use the path
        in the filespec.
      */
      getcwd(szFileName, sizeof(szFileName));
      lstrcpy(lpIntOfn->szStartingDir, szFileName);  /* near/far crap */

#if !defined(UNIX) && !defined(VAXC)
      chDrive = (BYTE) toupper(lpIntOfn->szStartingDir[0]);
#endif

      if (lpIntOfn->ofn.lpstrFile != NULL)
      {
        lstrcpy(szFileName, lpIntOfn->ofn.lpstrFile);
#if !defined(UNIX) && !defined(VAXC)
        /*
          If the file spec was not null and included a drive letter, 
          use that drive
        */
        if (szFileName[0] && szFileName[1] == ':')
          chDrive = (BYTE) toupper(szFileName[0]);
#endif
      }

      if (lpIntOfn->ofn.lpstrFile != NULL &&
          (pSlash = strrchr(szFileName, CH_SLASH)) != NULL)
      {
        if (pSlash == szFileName)  /* if '\foo.txt' */
        {
          lpIntOfn->szStartingDir[0] = CH_SLASH;
          lpIntOfn->szStartingDir[1] = '\0';
        }
        else
        {
          /*
            Copy the directory part into szStartingDir and leave the
            filename in lpstrFile.
          */
          /*
            Make pSlash a ptr into lpstrFile instead of szFileName.
            (near/far crap)
          */
          pSlash = lpIntOfn->ofn.lpstrFile + (pSlash - (LPSTR) szFileName);
          *pSlash = '\0';  /* get rid of the last slash */
          lstrcpy(lpIntOfn->szStartingDir, lpIntOfn->ofn.lpstrFile);
          lstrcpy(lpIntOfn->ofn.lpstrFile, pSlash+1);
        }

        /*
          If the user did not specify an initial starting directory, then
          change to 'szStartingDir'.
        */
        if (!lpOfn->lpstrInitialDir)
        {
#if !defined(UNIX) && !defined(VAXC)
          if (strlen(lpIntOfn->szStartingDir)==2 && lpIntOfn->szStartingDir[1]==':')
            strcat(lpIntOfn->szStartingDir, STR_SLASH);
#endif
          lstrcpy(szFileName, lpIntOfn->szStartingDir);  /* near/far crap */
          chdir(szFileName);
        }
      }


      /*
        Process the pairs of filter descriptions and file specs
      */
      if (lpOfn->lpstrFilter)
      {
        LPCSTR lpDesc;

        for (lpDesc = lpOfn->lpstrFilter;  *lpDesc;  )
        {
          SendMessage(hwndFilterCombo, CB_ADDSTRING, (WPARAM) -1, (LONG) lpDesc);
          lpDesc += lstrlen((LPSTR) lpDesc) + 1;
          ListAdd(&lpIntOfn->listFilterSpecs, 
                  ListCreate(lstrsave((LPSTR) lpDesc)));
          lpDesc += lstrlen((LPSTR) lpDesc) + 1;
        }
      }

      /*
        Process the custom filter (later)
      */
      (void) lpOfn->lpstrCustomFilter;
      (void) lpOfn->nMaxCustFilter;

      /*
        Process the initial filter setting. If it's non-zero, use the
        appropriate filter. If it's zero and a custom filter was specified,
        use the custom filter.
      */
      iSel = 0;
      if (lpOfn->nFilterIndex)
        iSel = (int) lpOfn->nFilterIndex-1;
      else if (lpOfn->lpstrCustomFilter)
        iSel = 1000;
      SendMessage(hwndFilterCombo, CB_SETCURSEL, (WPARAM) iSel, 0L);

      /*
        See if the user wants to change the caption of the dialog box
      */
      if (lpOfn->lpstrTitle)
      {
        lstrcpy(szFileName, (LPSTR) lpOfn->lpstrTitle);  /* near/far crap */
        SetWindowText(hDlg, szFileName);
      }
      else if (lpIntOfn->fIntFlags & OFN_SAVEDIALOG)
        SetWindowText(hDlg, "Save File");
      else
        SetWindowText(hDlg, "Open File");

      /*
        Set up the hook stuff. lParam of the WM_INITDIALOG message contains
        the custom hook data.
      */
      if ((lpOfn->Flags & OFN_ENABLEHOOK) && lpOfn->lpfnHook)
        lpIntOfn->hHook = SetWindowsHookEx(WH_COMMDLG, (FARPROC) lpOfn->lpfnHook, 0, 0);
      else
        lpIntOfn->hHook = NULL;

      /*
        Do some flags stuff
      */
      if (!(lpOfn->Flags & OFN_SHOWHELP))
        ShowWindow(GetDlgItem(hDlg, IDHELP), SW_HIDE);
      if (lpOfn->Flags & OFN_READONLY)
        CheckDlgButton(hDlg, ID_READONLYCB, TRUE);
      if (lpOfn->Flags & OFN_HIDEREADONLY)
        ShowWindow(GetDlgItem(hDlg, ID_READONLYCB), SW_HIDE);


      /*
        See if there is an initial directory the user want to consider
      */
      if (lpOfn->lpstrInitialDir)
      {
        lstrcpy(szFileName, (LPSTR) lpOfn->lpstrInitialDir);  /* near/far crap */
        chdir(szFileName);
#if !defined(UNIX) && !defined(VAXC)
        if (szFileName[1] == ':')
          chDrive = (BYTE) toupper(szFileName[0]);
#endif
      }

      /*
        Set the initial value of the file edit control
      */
      if (lpIntOfn->ofn.lpstrFile && lpIntOfn->ofn.lpstrFile[0])
        lstrcpy(szFileName+2, lpIntOfn->ofn.lpstrFile);  /* near/far crap */
      else
        GetCurrFilter(hwndFilterCombo, szFileName+2);
      SetDlgItemText(hDlg, ID_FILEEDIT, szFileName+2);
      SendMessage(hEdit, EM_SETSEL, 0, -1L);
      SetFocus(hEdit);

      /*
        Perform the directory and file listings
        If we are in a SAVE dialog, then the file listbox should have
        a listing of files as defined by the current filter spec.

        If we are in a OPEN dialog, and the initial file name does
        not have a wildcard spec, then use the selected filter.
      */
#if !defined(UNIX) && !defined(VAXC)
      if (lpIntOfn->fIntFlags & OFN_SAVEDIALOG)
      {
        GetCurrFilter(hwndFilterCombo, szFileName);
      }
      else if (!SpecHasWildcards(szFileName))
      {
        GetCurrFilter(hwndFilterCombo, szFileName);
      }
      else
      {
        szFileName[0] = chDrive;
        szFileName[1] = ':';
      }
      DlgDirList(hDlg, szFileName,    ID_FILELIST, 0,       MASK_FILESONLY);
#else
      DlgDirList(hDlg, szFileName+2,  ID_FILELIST, 0,       MASK_FILESONLY);
#endif

      DlgDirList(hDlg, pszEverything, ID_DIRLIST,  ID_PATH, 0x8010);
#if !defined(UNIX) && !defined(VAXC)
      DlgDirListComboBox(hDlg, pszEverything, ID_DRIVECOMBO, 0, 0xC000);
      SelectDriveInCombo(hDlg, chDrive);
#endif

      if (InternalSysParams.pHooks[WH_COMMDLG])
        if ((*(COMMDLGHOOKPROC *) InternalSysParams.pHooks[WH_COMMDLG]->lpfnHook)
              (hDlg,message,wParam,lParam))
          return FALSE;
      return FALSE;


    case WM_CHARTOITEM :
      /*
        As the directory strings begin with a bracket "[X...]",
        pressing the key <X> won't set the caret to the correct item

        Note: Users must add the style LBS_WANTKEYBOARDINPUT to the
              listbox COMMFILE_DIRS in CommFileDlg
      */
      if (wParam > 32 && wParam < 256)
      {
        char acSearch[3];

        /*
          It's a printable character, convert it to a string "[?"
        */
        acSearch[0] = '[';
        acSearch[1] = (char) wParam;
        acSearch[2] = 0;

        /*
          Set the caret to the next item which begins with the desired string
        */
        SendMessage((HWND) lParam, LB_SELECTSTRING, HIWORD(lParam),
                    (LONG) (LPSTR) acSearch);

        /*
          Workaround for YALMA (Yet Another Little Mewel Anomaly...)
        */
        InvalidateRect((HWND) lParam, NULL, TRUE);

        /*
          return "No further action"
        */
        return -2;
      }

      /*
        It was no printable char, return "Default action"
      */
      return -1;



    case WM_COMMAND :
      if (wParam == ID_FILEEDIT)
      {
        /* Ignore edit notification messages */
        break;
      }

      switch (wParam)
      {
        case ID_DIRLIST  :
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE :
              break;   /* do nothing */

            case LBN_DBLCLK :
process_dir:
              /*
                Get the directory name into szFileName[] and change
                to the new directory.
              */
              if (DlgDirSelect(hDlg, szFileName, ID_DIRLIST))
              {
                cchDir  = strlen(szFileName);
                if (szFileName[cchDir-1] == CH_SLASH)
                  szFileName[cchDir-1] = '\0';
                chdir(szFileName);
              }

display_dir:
              /*
                Fill the file listbox with the files matching the current 
                filter spec (or *.*), and fill in the file edit field too.
              */
              GetCurrFilter(hwndFilterCombo, szFileName);
              DlgDirList(hDlg, szFileName, ID_FILELIST, 0, MASK_FILESONLY);
              SetDlgItemText(hDlg, ID_FILEEDIT, szFileName);
              SendDlgItemMessage(hDlg, ID_FILEEDIT,
                                 EM_SETSEL, 0, MAKELONG(0, -1));
              DlgDirList(hDlg, pszEverything, ID_DIRLIST,  ID_PATH, 0x8010);
              break;
          }
          break;


        case ID_FILELIST :
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE :
              /* 
                Make sure we have a selection CFN/TeleVoice
              */
              if (SendMessage(GetDlgItem(hDlg,ID_FILELIST),LB_GETCURSEL,0,0L)==LB_ERR)
                break;

              /*
                A single-click in the file list simply puts a copy of
                the file name in the file-edit field
              */
              DlgDirSelect(hDlg, szFileName, ID_FILELIST);
              SetDlgItemText(hDlg, ID_FILEEDIT, szFileName);
              SendMessage(hEdit, EM_SETSEL, 0, -1L);
              break;
    
            case LBN_DBLCLK :
             /* 
              * make sure we have a selection CFN/TeleVoice
              */
              if (SendMessage(GetDlgItem(hDlg,ID_FILELIST),LB_GETCURSEL,0,0L)==LB_ERR)
                break;

              /*
                A double-click in the file list is the same thing
                as choosing a file.
              */
              getcwd(szFileName, sizeof(szFileName));
              cchDir = strlen(szFileName);
              if (szFileName[cchDir-1] != CH_SLASH)
                szFileName[cchDir++] = CH_SLASH;
              /*
                szFileName now is "\dir1\dir2\"
              */
              DlgDirSelect(hDlg, szFileName+cchDir, ID_FILELIST);
              /*
                Do not add the default extension if the user clicked
                on a listbox item.
              */
              bDontAddDefExt = TRUE;
              goto good_file;
          }
          break;


        case ID_DRIVECOMBO :
          switch (HIWORD(lParam))
          {
            case CBN_SELCHANGE :
              if (SendMessage(LOWORD(lParam), CB_GETDROPPEDSTATE, 0, 0L))
                break;
            case CBN_SELENDOK  :
              /*
                Form a filespec of <drive>:<defext>
              */
#if !defined(UNIX) && !defined(VAXC)
              DlgDirSelectComboBox(hDlg, szFileName, ID_DRIVECOMBO);
#endif
              strcat(szFileName, pszEverything);
              DlgDirList(hDlg, szFileName, ID_DIRLIST, ID_PATH, 0x8010);
              GetCurrFilter(hwndFilterCombo, szFileName);
              DlgDirList(hDlg, szFileName, ID_FILELIST, 0, MASK_FILESONLY);
              break;
          }
          break;


        case ID_FILTERCOMBO :
          switch (HIWORD(lParam))
          {
            case CBN_SELCHANGE :
              if (SendMessage(hwndFilterCombo, CB_GETDROPPEDSTATE, 0, 0L))
                break;
            case CBN_SELENDOK  :
              /*
                If we changed the current filter, then refresh the file
                listbox and put the new spec in the edit field.
              */
              GetCurrFilter(hwndFilterCombo, szFileName);
              DlgDirList(hDlg, szFileName, ID_FILELIST, 0, MASK_FILESONLY);
              SetDlgItemText(hDlg, ID_FILEEDIT, szFileName);
              break;
          }
          break;


        case ID_FILEEDIT :
        case IDOK        :
          /*
            If the user pressed ENTER while a directory item was highlighted,
            then process the entry in the directory listbox.
          */
          if (wParam == IDOK && GetFocus() == GetDlgItem(hDlg, ID_DIRLIST))
            goto process_dir;

          /*
            Get the current filter spec and fill in the nFilterIndex member. 
            The filter index starts at 1. 
          */
          iSel = GetCurrFilter(hwndFilterCombo, szFilter);
          lpOfn->nFilterIndex = (iSel == CB_ERR) ? 0 : iSel+1;

          /*
            Get the filename which the user entered
          */
          cchDir=GetDlgItemText(hDlg,ID_FILEEDIT,szFileName,sizeof(szFileName));

          /*
            If no filename is entered, then break
          */
          cchDir--;
          for (cchDir--;  cchDir >= 0 && szFileName[cchDir] == ' ';  cchDir--)
            ;
          if (cchDir < 0)
            break;

          /*
            Is it a drive letter or a directory?
          */
          if (chdir(szFileName) == 0)
          {
#if !defined(UNIX) && !defined(VAXC)
            if (szFileName[1] == ':') 
              SelectDriveInCombo(hDlg, szFileName[0]);
#endif
            goto display_dir;
          }

          /*
            If we do not have wildcards in the filename, and the filename
            is not the name of an actual file (such as \x\foo.txt), then
            prepend the current directory to the filename. Otherwise, just
            use the wildcard.
          */
#if 12194
          if (szFileName[0] != CH_SLASH 
#if defined(DOS) || defined(OS2)
              && szFileName[1] != ':'
#endif
              )
#else
          if (!SpecHasWildcards(szFileName) && access(szFileName, 0) != 0)
#endif
          {
            getcwd(szFileName, sizeof(szFileName));
            cchDir = strlen(szFileName);
            if (szFileName[cchDir-1] != CH_SLASH)  /* not "C:\" */
              szFileName[cchDir++] = CH_SLASH;
            /*
              szFileName now is "\current directory\"
            */
          }
          else
            cchDir = 0;
          GetDlgItemText(hDlg, ID_FILEEDIT, 
                         szFileName+cchDir, sizeof(szFileName) - cchDir);

          /*
            szFileName now is "\dir1\dir2\*.txt" or "\dir1\dir2\foo.txt"
          */


          /* 
            Append appropriate extension to user's entry. But do it only
            if the file does not exist.
          */
          if (!bDontAddDefExt)
          {
            if (access(szFileName, 0) != 0)
              DlgAddCorrectExtension(szFileName, szFilter, TRUE);
          }

          /*
            Try to open directory.  If successful, fill listbox with contents
            of new directory.
          */
          if (SpecHasWildcards(szFileName))
          {
            if (DlgDirList(hDlg, szFileName, ID_FILELIST, 0, MASK_FILESONLY))
            {
              DlgDirList(hDlg, pszEverything, ID_DIRLIST,  ID_PATH, 0x8010);
              SetDlgItemText(hDlg, ID_FILEEDIT, szFileName);
              SendMessage(hEdit, EM_SETSEL, 0, -1L);
              SetFocus(GetDlgItem(hDlg, ID_FILEEDIT));
              break;
            }
          }

          if (!bDontAddDefExt)
          {
            if (access(szFileName, 0) != 0)
              DlgAddCorrectExtension(szFileName, szFilter, FALSE);
          }

good_file:
          /*
            For a savefile dialog box, if the file exists already,
            perhaps prompt the user to overwrite.
          */
          if ((lpIntOfn->fIntFlags & OFN_SAVEDIALOG) &&
              (lpOfn->Flags & OFN_OVERWRITEPROMPT)   &&
              access(szFileName, 0) == 0)
          {
            char acCaption[80];

            GetWindowText(hDlg, acCaption, 80);
            if (MessageBox(hDlg, MSG(INTL_OVERWRITEPROMPT), acCaption,
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES)
              break;
          }
          else
          {
            /*
              If we are opening a file which does not exist, and the
              OFN_CREATEPROMPT flag is set, then we must prompt the user
              as to whether or not he wants to create the file. If not,
              then stay in the dialog box.
            */
            if (!(lpIntOfn->fIntFlags & OFN_SAVEDIALOG) &&
                (lpOfn->Flags & OFN_CREATEPROMPT)       &&
                access(szFileName, 0) != 0)
            {
              char acCaption[80];

              GetWindowText(hDlg, acCaption, 80);
              if (MessageBox(hDlg, MSG(INTL_CREATEPROMPT), acCaption,
                  MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES)
                break;
              lpOfn->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
            }
          }

          /*
            Copy the custom filter
          */
          (void) lpOfn->lpstrCustomFilter;
          (void) lpOfn->nMaxCustFilter;

          /*
            Copy the chosen file into the return buffer, with full path
          */
          if (lpOfn->lpstrFile)
            lstrncpy(lpOfn->lpstrFile, szFileName, (UINT) lpOfn->nMaxFile);

          /*
            Copy the chosen file into the return buffer, just the file name
          */
          if (lpOfn->lpstrFileTitle)
          {
            pSlash = strrchr(szFileName, CH_SLASH);
            if (pSlash)
              lstrncpy(lpOfn->lpstrFileTitle, pSlash+1, (UINT) lpOfn->nMaxFileTitle);
            else
              lstrncpy(lpOfn->lpstrFileTitle, szFileName, (UINT) lpOfn->nMaxFileTitle);
          }

          /*
            Fill in the nFileOffset and nFileExtension members
          */
          if ((pSlash = strrchr(szFileName, CH_SLASH)) != NULL)
            lpOfn->nFileOffset = pSlash - (LPSTR) szFileName + 1;
          else
            lpOfn->nFileOffset = 0;
          if ((pDot = strrchr(szFileName, '.')) != NULL)
            lpOfn->nFileExtension = pDot - (LPSTR) szFileName + 1;
          else
            lpOfn->nFileExtension = 0;

          /*
            Set a flag telling the app that the user chose a different
            extension than the default extension.
          */
          if (lpOfn->lpstrDefExt)
          {
            if (pDot && lstricmp((LPSTR) lpOfn->lpstrDefExt, pDot+1))
              lpOfn->Flags |= OFN_EXTENSIONDIFFERENT;
          }

          /*
            Set the read-only indicator
          */
          if (IsDlgButtonChecked(hDlg, ID_READONLYCB))
            lpOfn->Flags |=  OFN_READONLY;
          else
            lpOfn->Flags &= ~OFN_READONLY;

          /*
            See if we need to restore the starting directory
          */
          if (lpOfn->Flags & OFN_NOCHANGEDIR)
          {
            lstrcpy(szFileName, lpIntOfn->szStartingDir);  /* near/far crap */
            chdir(szFileName);
          }

          if (lpIntOfn->hHook)
            UnhookWindowsHookEx(lpIntOfn->hHook);

          /*
            Dismiss the dialog box
          */
          EndDialog(hDlg, TRUE);
          break;


        case IDCANCEL :
          if (lpIntOfn->hHook)
            UnhookWindowsHookEx(lpIntOfn->hHook);
          EndDialog(hDlg, FALSE);
          break;

        case IDHELP   :
#if 32693
          /*
            Send current window handle along with help message
            defined as not used in windows) so that the WinHelp
            enables/disables the correct window

            From commfile.dlg ...
              #define HELPMSGSTRING  "commdlg_help"
          */
          SendMessage(lpOfn->hwndOwner, FindAtom(HELPMSGSTRING), hDlg, 0L);
#else
          MessageBox(hDlg, "Help not implemented yet.", "Save File", MB_OK);
#endif
          break;

      } /* switch (wParam) */
      return TRUE;

    case WM_KEYDOWN    :
      if(wParam == VK_F1)
	  {
         SendMessage(hDlg, WM_COMMAND, IDHELP, 0L);
		 return TRUE;
	  }
	  break;
  }

  return FALSE;
}


/* 
   Given filename or partial filename or search spec or partial
   search spec, add appropriate extension.
*/
static INT PASCAL DlgAddCorrectExtension(PSTR szEdit, PSTR szDefExt, 
                                         BOOL fSearching)
{
  PSTR pchLast;
  int  chLast;

  if (!SpecHasWildcards(szEdit))
  {
    if (lpInternalOfn->ofn.lpstrDefExt)
    {
      PSTR pDot = strrchr(szEdit, '.');
      if (!pDot)
      {
        strcat(szEdit, ".");
        lstrcat(szEdit, (LPSTR) lpInternalOfn->ofn.lpstrDefExt);
      }
    }
    return FALSE;
  }

  /*
    We have a wildcard in the filespec

    Get a pointer to the last character in the filename
  */
  chLast = *(pchLast = szEdit + strlen((char *) szEdit) - 1);

  /*
    If we have "A:" or "..", add "\<ext>"
    So, A: becomes A:*.txt and .. becomes ..\*.txt

    If we have "xxxx\" add "<ext>"
    So, dos\ becomes dos\*.txt
  */
  if (chLast == ':' || !strcmp((char *) szEdit, "..") || chLast == CH_SLASH)
  {
add_slash:
    if (chLast != CH_SLASH)
      *++pchLast = CH_SLASH;
    strcpy(pchLast + 1, szDefExt);
  }
  else
  {
    /*
      Don't add an extension if we already have one.
    */
    if (strrchr((char *) szEdit, '.') || !strcmp((char *) szEdit, "*"))
      return FALSE;

    if (fSearching)
      goto add_slash;
    else
      strcpy(pchLast+1, szDefExt+1);
  }
  return TRUE;
}


static INT PASCAL GetCurrFilter(hWndFilterCombo, lpszBuf)
  HWND  hWndFilterCombo;
  LPSTR lpszBuf;
{
  INT iSel = (INT) SendMessage(hWndFilterCombo, CB_GETCURSEL, 0, 0L);
  if (iSel != CB_ERR)
  {
    PLIST pNth = ListGetNth(lpInternalOfn->listFilterSpecs, iSel);
    lstrcpy(lpszBuf, pNth->data);
  }
  else
  {
    lstrcpy(lpszBuf, pszEverything);
  }
  return iSel;
}


/****************************************************************************/
/*                                                                          */
/* Function :                                                               */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
#if 0
BOOL WINAPI ChooseColor(lpCC)
  CHOOSECOLOR FAR *lpCC;
{
  (void) lpCC;
  return FALSE;
}
#endif

#if defined(MEWEL_TEXT) && defined(UNIX)
BOOL WINAPI ChooseFont(lpCF)
  CHOOSEFONT FAR *lpCF;
{
  (void) lpCF;
  return FALSE;
}
#endif

#if 0
BOOL WINAPI PrintDlg(lpPD)
  PRINTDLG FAR *lpPD;
{
  (void) lpPD;
  return FALSE;
}
#endif

#ifdef _cplusplus
}
#endif

