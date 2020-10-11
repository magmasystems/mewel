/*===========================================================================*/
/*                                                                           */
/* File    : DLGOPEN.C                                                       */
/*                                                                           */
/* Purpose : Impelements a pre-defined dialog box used for getting a         */
/*           file name from the user.                                        */
/*                                                                           */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#define ID_EDIT       20
#define ID_LISTBOX    30
#define ID_PATH       40

static char szFileName[MAXPATH];
static char szExtension[13];

#define ATTRDIRLIST     0x4010
#define CBEXTMAX        6
#define cbRootNameMax   (80 - CBEXTMAX - 1)

extern FAR PASCAL SpecHasWildcards(LPSTR);

static int FAR PASCAL DlgFnOpen(HWND, UINT, WPARAM, LPARAM);
static int FAR PASCAL DlgAddCorrectExtension(PSTR, BOOL);

typedef struct dlgTemplate
{
  BYTE   szClass[1];
  LPCSTR szTitle;
  DWORD  dwStyle;
  int    x, y, width, height;
  UINT   idCtrl;
} DTEMPLATE;

/*
  A dialog box definition for creating dialog boxes on-the-fly
*/
static DTEMPLATE DlgOpenTemplate[] =
{
  { CT_EDIT,    "File &name:",  WS_BORDER | WS_TABSTOP | WS_GROUP,
      3, 1,  35, 3,  ID_EDIT    },
  { CT_LISTBOX, "&Files",       LBS_STANDARD | WS_TABSTOP,
      3, 5,  38, 11, ID_LISTBOX },
  { CT_STATIC,  NULL,           SS_TEXT,
     48, 6,  11, 1,  ID_PATH    },
  { CT_BUTTON,  "&OK",          WS_BORDER | WS_TABSTOP | WS_GROUP | BS_DEFPUSHBUTTON,
     48, 10,  8, 3,  IDOK       },
  { CT_BUTTON,  "&CANCEL",      WS_BORDER | WS_TABSTOP | WS_GROUP | BS_PUSHBUTTON,
     48, 13,  8, 3,  IDCANCEL   },
};


INT FAR PASCAL DlgOpenFile(hParent, szExt, szReturnedFname)
  HWND hParent;
  LPSTR szExt;
  LPSTR szReturnedFname;
{
  DTEMPLATE *pTemplate;
  int       nTemplate;
  HWND      hDlg;
  int       rc;

  if ((hDlg = DialogCreate(hParent, 5,2,23,71, "Open File",
                            0xFF, 
                            WS_CAPTION | WS_BORDER | WS_SYSMENU,
                            (FARPROC) DlgFnOpen, 0, (PSTR)NULL, 0)) == NULLHWND)
    return FALSE;


  pTemplate = DlgOpenTemplate;
  for (nTemplate = 5;  nTemplate-- > 0;  pTemplate++)
  {
    HWND hCtrl;
    hCtrl = _CreateWindow((LPCSTR) pTemplate->szClass,
                          pTemplate->szTitle,
                          pTemplate->dwStyle | WS_CHILD,
                          pTemplate->x,      /* x,y, cx,cy */
                          pTemplate->y,
                          pTemplate->width,
                          pTemplate->height,
                          0xFF,              /* color   */
                          pTemplate->idCtrl, /* id      */
                          hDlg,              /* hParent */
                          (HMENU) NULLHWND,  /* hMenu   */
                          0,                 /* hInst   */
                          (DWORD) NULL);     /* lpCreateParams */
    if (hCtrl == NULLHWND)
      return FALSE;
  }
      
  if (szExt)
    lstrncpy(szExtension, szExt, sizeof(szExtension));
  else
    strcpy((char *) szExtension, "*.*");

  rc = _DialogBox(hDlg);

  if (rc)
    lstrcpy(szReturnedFname, szFileName);
  else
    szReturnedFname[0] = '\0';

  return rc;
}


/* Dialog function for Open File */
static int FAR PASCAL DlgFnOpen(hDlg, msg, wParam, lParam)
  HWND   hDlg;
  UINT   msg;
  WPARAM wParam;
  LPARAM lParam;
{
  char rgch[256];
  int  cchDir;
  PSTR pchFile;
  BOOL fWild;
  HWND hEdit = GetDlgItem(hDlg, ID_EDIT);

  switch (msg)
  {
  case WM_INITDIALOG:
    /* Set edit field with default search spec */
    SetDlgItemText(hDlg, ID_EDIT, szExtension);
    /* Don't let user type more than cbRootNameMax bytes in edit ctl. */
    SendMessage(hEdit, EM_LIMITTEXT, cbRootNameMax, 0L);
    SendMessage(hEdit, EM_SETSEL, 0, -1L);
    /*
      fill list box with filenames that match spec, and fill static field
      with path name 
    */
    if (!DlgDirList(hDlg, szExtension, ID_LISTBOX, ID_PATH, ATTRDIRLIST))
      EndDialog(hDlg, 0);
    SetFocus(hEdit);
    break;

  case WM_COMMAND:
    if (LOWORD(lParam) == hEdit)
    {
      /* Ignore edit notification messages */
      break;
    }

    switch (wParam)
    {
    case ID_EDIT:
    case IDOK:
      /* 
        Get contents of edit field and add the search spec if it 
        does not contain one.
      */
      GetDlgItemText(hDlg, ID_EDIT, (LPSTR) szFileName, cbRootNameMax);

      /* 
        Append appropriate extension to user's entry
      */
      DlgAddCorrectExtension(szFileName, TRUE);

      /*
        Try to open directory.  If successful, fill listbox with contents
        of new directory.
      */
      if (SpecHasWildcards(szFileName))
      {
        if (DlgDirList(hDlg, szFileName, ID_LISTBOX, ID_PATH, ATTRDIRLIST))
        {
          SetDlgItemText(hDlg, ID_EDIT, szFileName);
          SendMessage(hEdit, EM_LIMITTEXT, cbRootNameMax, 0L);
          SendMessage(hEdit, EM_SETSEL, 0, -1L);
          strncpy((char *) szExtension, (char *) szFileName, sizeof(szExtension));
          break;
        }
        else
          EndDialog(hDlg, 0);
      }

      DlgAddCorrectExtension(szFileName, FALSE);
      /*
        If no directory list and filename contained search spec, beep
      */
      if (SpecHasWildcards(szFileName))
      {
        MessageBeep(0);
        break;
      }
      /*
        Looks like we have a possibly valid file here. Upper-case it and
        tell the user that we have a file name. The user is then responsible
        for opening it.
      */
#if !defined(UNIX)
      lstrupr((LPSTR) szFileName);
#endif
      EndDialog(hDlg, 1);
      break;

    case IDCANCEL:
      EndDialog(hDlg, 0);
      break;

    case ID_LISTBOX:
      switch (HIWORD(lParam))
      {
        /* Single click case */
        case LBN_SELCHANGE :
          GetDlgItemText(hDlg, ID_EDIT, (LPSTR) rgch, cbRootNameMax);

          /*
           * Get selection, which may be either a prefix to a new search path
           * or a filename. DlgDirSelect parses selection, and appends a
           * backslash if selection is a prefix 
           */
          if (DlgDirSelect(hDlg, szFileName, ID_LISTBOX))
          {
            cchDir  = strlen((char *) szFileName);  /* get the highlited entry */
            pchFile = rgch + strlen((char *) rgch);

            /*
             * Now see if there are any wild characters (* or ?) in edit field.
             * If so, append to prefix. If edit field contains no wild cards
             * append default search spec.
             */
            fWild = (BOOL) (*pchFile == '*' || *pchFile == ':');
            while (pchFile > rgch)
            {
              pchFile--;

              if (*pchFile == '*' || *pchFile == '?')
                fWild = TRUE;
              else if (*pchFile == CH_SLASH || *pchFile == ':')
              {
                pchFile++;
                break;
              }
            }
            strcpy((char *) szFileName + cchDir, 
                   (fWild) ? (char *) pchFile : (char *) szExtension);
          }

set_edit:
          /* Set edit field to entire file/path name. */
          SetDlgItemText(hDlg, ID_EDIT, szFileName);
          SendMessage(hEdit, EM_LIMITTEXT, cbRootNameMax, 0L);
          SendMessage(hEdit, EM_SETSEL, 0, -1L);
          break;

        case LBN_DBLCLK :
          /*
           * Basically the same as ok.  If new selection is directory, open it
           * and list it.  Otherwise, open file. 
           */
          if (DlgDirList(hDlg, szFileName, ID_LISTBOX, ID_PATH, ATTRDIRLIST))
            goto set_edit;
          EndDialog(hDlg, 1);
          break;
      }
      break;

    default:
      return FALSE;
    }

  default:
    return FALSE;
  }

  return TRUE;
}


/* 
   Given filename or partial filename or search spec or partial
   search spec, add appropriate extension.
*/
static int FAR PASCAL DlgAddCorrectExtension(PSTR szEdit, BOOL fSearching)
{
  PSTR pchLast;
  int  chLast;

  chLast = *(pchLast = szEdit + strlen((char *) szEdit) - 1);

  /*
    If we have "A:" or "..", add "\<ext>"
  */
  if (chLast == ':' || !strcmp((char *) szEdit, ".."))
  {
add_slash:
    *++pchLast = CH_SLASH;
    strcpy((char *) pchLast + 1, (char *) szExtension);
  }
  /*
    If we have "xxxx\" add "<ext>"
  */
  else if (chLast == CH_SLASH)
    strcpy((char *) pchLast + 1, (char *) szExtension);
  else
  {
    if (strrchr((char *) szEdit, '.'))
      return FALSE;
    if (fSearching)
      goto add_slash;
    else
      strcpy((char *) pchLast + 1, (char *) szExtension+1);
  }
  return TRUE;
}

