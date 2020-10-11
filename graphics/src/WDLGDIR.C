/*===========================================================================*/
/*                                                                           */
/* File    : DLGDIR.C                                                        */
/*                                                                           */
/* Purpose : DlgDirList() and DlgDirSelect()                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCLUDE_COMBOBOX

#include "wprivate.h"
#include "window.h"

#if defined(__GNUC__)
#define __TURBOC__
#endif


static BOOL FAR PASCAL _DlgDirSelect(LPSTR, HWND);
static BOOL FAR PASCAL _DlgDirList(HWND, LPSTR, HWND, int, UINT);


BOOL FAR PASCAL DlgDirSelectComboBoxEx(hDlg, lpszPath, cbPath, idCombo)
  HWND  hDlg;
  LPSTR lpszPath;
  int   cbPath;
  int   idCombo;
{
  (void) cbPath;
  return DlgDirSelectComboBox(hDlg, lpszPath, idCombo);
}

BOOL FAR PASCAL DlgDirSelectEx(hDlg, lpszPath, cbPath, idList)
  HWND  hDlg;
  LPSTR lpszPath;
  int   cbPath;
  int   idList;
{
  (void) cbPath;
  return DlgDirSelect(hDlg, lpszPath, idList);
}


BOOL FAR PASCAL DlgDirListComboBox(hDlg, szPath, idListBox, idStaticPath, wFiletype)
  HWND  hDlg;
  LPSTR szPath;
  int   idListBox;
  int   idStaticPath;
  UINT  wFiletype;
{
  HWND     hCombo;
  WINDOW   *w;
  COMBOBOX *pCombo;

  hCombo = GetDlgItem(hDlg, idListBox);
  if ((w = WID_TO_WIN(hCombo)) == (WINDOW *) NULL)
    return FALSE;
  pCombo = (COMBOBOX *) w->pPrivate;
  return _DlgDirList(hDlg, szPath, pCombo->hListBox, idStaticPath, wFiletype);
}

BOOL FAR PASCAL DlgDirList(hDlg, szPath, idListBox, idStaticPath, wFiletype)
  HWND hDlg;
  LPSTR szPath;
  int  idListBox;
  int  idStaticPath;
  UINT wFiletype;
{
  HWND hListBox = GetDlgItem(hDlg, idListBox);
  return _DlgDirList(hDlg, szPath, hListBox, idStaticPath, wFiletype);
}


static
BOOL FAR PASCAL _DlgDirList(hDlg, szPath, hListBox, idStaticPath, wFiletype)
  HWND hDlg;
  LPSTR szPath;
  HWND hListBox;
  int  idStaticPath;
  UINT wFiletype;
{
  int  rc;
  unsigned drives;
  char szFile[MAXPATH];
  char szCurrDir[MAXPATH];
  BYTE currdrv, olddrv;
  LPSTR szOrigPath = szPath;
  LPSTR pSlash;

  szFile[0] = '\0';
#if defined(VAXC)
  getcwd(szCurrDir, MAXPATH,0);  /* Get it in DEC/Shell (UNIX) format */
#else
  getcwd(szCurrDir, MAXPATH);
#endif
  olddrv = currdrv = *szCurrDir;

#if defined(DOS) || defined(OS2)
  /*
    4/1/91 (maa)
    I just found out that MS Windows will merrily accept a forward slash!
    So, we will go transform all forward slashes to backward slashes.
  */
  for (pSlash = szPath;  *pSlash;  pSlash++)
    if (*pSlash == '/')
      *pSlash = CH_SLASH;
#endif


  /*
    First, we must parse out the drive and subdirectory, and change
    to that drive and path. We place a '\0' after the path to
    separate it from the filename.
  */
#if !defined(UNIX) && !defined(VAXC)
  if (szPath[1] == ':')
  {
    currdrv = (char) toupper(*szPath);
  /*
    WE SHOULD CHECK FOR CRITICAL ERRORS HERE
  */
#ifdef DOS
#ifdef __TURBOC__
    setdisk(currdrv - 'A');
#else
    _dos_setdrive(currdrv - 'A' + 1, &drives);
#endif
#endif /* DOS */
#ifdef OS2
    DosSelectDisk(toupper(currdrv) - 'A' + 1);
#endif /* OS2 */

    /*
      Getting the current directory on an invalid drive will set off the
      critical error handler.
    */
#if defined(VAXC)
    getcwd(szCurrDir, MAXPATH,0);  /* Get it in DEC/Shell (UNIX) format */
#else
    getcwd(szCurrDir, MAXPATH);
#endif

    if (!CheckCriticalError(olddrv))
      return FALSE;

    szPath += 2;

    if (!CheckCriticalError(olddrv))
      return FALSE;
  }
#endif /* VAXC || UNIX */


  /* Isolate the drive and directory */
  if (*szPath)
  {
    /*
      If we have a directory name is szPath without a drive specifier,
      copy the current drive to szCurrDir.
    */
    if (*szPath == CH_SLASH)
#if defined(UNIX) || defined(VAXC)
      szCurrDir[0] = '\0';
#else
      sprintf((char *) szCurrDir, "%c:", currdrv);
#endif
    /*
      If the last character of the path isn't a slash, then add one.
      We don't have c:\
    */
    else
     if (szCurrDir[strlen((char *) szCurrDir)-1] != CH_SLASH)
      strcat((char *) szCurrDir, STR_SLASH);

    lstrcat(szCurrDir, szPath);
  }
  /*
    At this point, szCurrDir is "drive:\directory\filespec"
  */

  /*
    Find the right-most slash character.
  */
  pSlash = (PSTR) strrchr((char *) szCurrDir, CH_SLASH);

  /* Remove any wildcards */
  if (SpecHasWildcards((PSTR) szCurrDir))
  {
    if (pSlash != NULL)
    {
      /*
         c:\dbl\*.* ==> c:\dbl
         c:\*.*     ==> c:\
      */
      lstrcpy(szFile, pSlash+1);
      if (*(pSlash-1) == ':')
        *(pSlash+1) = '\0';
      else
        *pSlash = '\0';
      pSlash = NULL;   /* for the chdir test below... */
    }
  }
  else
  {
    /*
      Make sure that the directory name doesn't end in a slash. However,
      if we have something like A:\, then leave it alone.
    */
    if (pSlash != NULL && *(pSlash+1) == '\0' && pSlash[-1] != ':')
      *pSlash = '\0';
    strcpy((char *) szFile, "*.*");
  }

  if (chdir(szCurrDir) < 0)
  {
    /*
      The change-directory could have failed because we have
        [drive]\path\filename
      Strip off the filename portion and try changing directories again.
    */
#if 0
    /*
      2/12/92 (maa)
        Took this out cause DlgDirList should just return FALSE if the
      filename was an actual filename, and not a wildcard.
    */
    if (pSlash)
    {
      strcpy((char *) szFile, (char *) pSlash+1);
      *pSlash = '\0';
      if (chdir(szCurrDir) < 0)
        return FALSE;
    }
    else
#endif
      return FALSE;
  }

#if defined(VAXC)
  getcwd(szCurrDir, MAXPATH,0);  /* Get it in DEC/Shell (UNIX) format */
#else
  getcwd(szCurrDir, MAXPATH);
#endif

  /*
    Second, if the static control exists, we copy the drive and
    subdirectory to the control
  */
  if (idStaticPath)
    SendDlgItemMessage(hDlg, idStaticPath, WM_SETTEXT, 0, (DWORD)(LPSTR) szCurrDir);

  /*
    Third, fill the listbox with the files in the current directory
    which match the filespec.
  */
  if (hListBox)
  {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0L);

    rc = (int) SendMessage(hListBox, LB_DIR, wFiletype, (DWORD)(LPSTR)szFile);
    if (rc == LB_ERR)
      return FALSE;
  }

  /*
    As per Windows, we must remove the drive and/or directory from szPath
  */
  lstrcpy(szOrigPath, szFile);

  return TRUE;
}


BOOL FAR PASCAL DlgDirSelectComboBox(hDlg, szFname, idListBox)
  HWND  hDlg;
  LPSTR szFname;
  int   idListBox;
{
  HWND     hCombo;
  WINDOW   *w;
  COMBOBOX *pCombo;

  hCombo = GetDlgItem(hDlg, idListBox);
  if ((w = WID_TO_WIN(hCombo)) == (WINDOW *) NULL)
    return FALSE;
  pCombo = (COMBOBOX *) w->pPrivate;
  return _DlgDirSelect(szFname, pCombo->hListBox);
}

BOOL FAR PASCAL DlgDirSelect(hDlg, szFname, idListBox)
  HWND  hDlg;
  LPSTR szFname;
  int   idListBox;
{
  return _DlgDirSelect(szFname, GetDlgItem(hDlg, idListBox));
}


static
BOOL FAR PASCAL _DlgDirSelect(szFname, hListBox)
  LPSTR szFname;
  HWND hListBox;
{
  BOOL isDir = FALSE;
  int  iSel;

  /*
    Get the highlighted file name
  */
  iSel = (int) SendMessage(hListBox, LB_GETCURSEL, 0, 0L);
  SendMessage(hListBox, LB_GETTEXT, iSel, (DWORD)(LPSTR) szFname);


  if (*szFname == '[')
  {
    /* We have a drive or directory */
    bytedel(szFname, 1, 0);
    if (*szFname == '-')
    {
      /* We have a drive */
      bytedel(szFname, 1, 0);
      szFname[1] = ':';
      szFname[2] = '\0';
      isDir = TRUE;
    }
    else
    {
      /* append a backslash */
      *lstrchr(szFname, ']') = CH_SLASH;
      isDir = TRUE;
    }
  }

  return isDir;
}


/*
  SpecHasWildcards - returns TRUE if the file spec has ? or *
*/
INT FAR PASCAL SpecHasWildcards(sz)
  LPSTR sz;
{
  return (lstrchr(sz, '*') != NULL || lstrchr(sz, '?') != NULL);
}


INT FAR PASCAL CheckCriticalError(olddrv)
  INT olddrv;
{
  unsigned drives;

#ifdef DOS

  if (IS_INT24_ERR())
  {
    MessageBox(NULLHWND, Int24ErrMsg, (PSTR) NULL, MB_OK);
    CLR_INT24_ERR();
    if (olddrv != 0)
#ifdef DOS
#ifdef __TURBOC__
      setdisk(toupper(olddrv) - 'A');
#else
      _dos_setdrive(toupper(olddrv) - 'A' + 1, &drives);
#endif
#endif /* DOS */
#ifdef OS2
      DosSelectDisk(toupper(olddrv) - 'A' + 1);
#endif /* OS2 */

    return FALSE;
  }
  else
    return TRUE;

#else
  return TRUE;
#endif /* DOS */
}

