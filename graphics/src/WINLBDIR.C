/*===========================================================================*/
/*                                                                           */
/* File    : LBDIR.C                                                         */
/*                                                                           */
/* Function: ListBoxDir()                                                    */
/*                                                                           */
/* Purpose : Fills a listbox with directory information                      */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*                                                                           */
/* Memory notes : If your app doesn't need file list boxes, you can stub     */
/*                out this routine.                                          */
/*                                                                           */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCLUDE_LISTBOX

#include "wprivate.h"
#include "window.h"

#ifdef OS2
static FILEFINDBUF FileDTA;
static HDIR hDir;
#endif /* OS2 */

/*
  DJGPP contains Turbo C compatible dir functions
*/
#if defined(__GNUC__)
#define __TURBOC__
#endif

#ifdef __TURBOC__
/* File attribute constants */
#define _A_NORMAL       0x00 
#define _A_RDONLY       0x01 
#define _A_HIDDEN       0x02 
#define _A_SYSTEM       0x04 
#define _A_VOLID        0x08 
#define _A_SUBDIR       0x10 
#define _A_ARCH         0x20 
#endif

extern DWORD FAR PASCAL _DOSEnumDisks();


INT FAR PASCAL ListBoxDir(lbi, filespec, filetype)
  LISTBOX *lbi;
  LPSTR    filespec;
  unsigned filetype;
{
#if defined(DOS) || defined(OS2)

#ifdef __TURBOC__
  struct ffblk  fileinfo;
#else
  struct find_t fileinfo;
#endif
  HWND hListBox;
  char buf[16];
  char szFileSpec[MAXPATH], *pSlash;
  PSTR pszFileSpec, pSemi;
  unsigned mask;
  BOOL bWeStoppedIt = FALSE;
  INT  nItemsDone = 0;
#ifdef OS2
  unsigned searchcnt = 1;
#endif

  hListBox = lbi->hListBox;

  if (!(lbi->wListBox->flags & LBS_NOREDRAW))
  {
    ListBoxSetRedraw(hListBox, FALSE);
    bWeStoppedIt = TRUE;
  }
  lstrcpy(szFileSpec, filespec);

  /*
    See if we want to search *exclusively* for directories and/or
    drives, bypassing normal files. If so, go to the directory
    checking code.
  */
  if ((filetype & 0x8000) && (filetype & 0x4010))
    goto check_dirs;

  mask = filetype & (_A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM);

  pszFileSpec = szFileSpec;
  for (;;)
  {
    /*
      If we have another spec after this one, close off the first one
    */
    if ((pSemi = strchr(pszFileSpec, ';')) != NULL)
      *pSemi++ = '\0';

#ifdef OS2
    hDir = 1;
    if (DosFindFirst(pszFileSpec, (PHDIR) &hDir, (USHORT) 0x00, &FileDTA,
                   		   sizeof(FileDTA), (PUSHORT) &searchcnt,
                                   (ULONG) 0) == 0)
    {
      do
      {
        strcpy(buf, FileDTA.achName);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND, 
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      } while (DosFindNext(hDir, &FileDTA, sizeof(FileDTA), 
                                           (PUSHORT) &searchcnt) == 0);
      DosFindClose(hDir);
    }
#endif /* OS2 */

#ifdef DOS
#ifdef __TURBOC__
    if (findfirst(pszFileSpec, &fileinfo, mask) == 0)
    {
      do
      {
        /*
          Check for exclusive search requested
        */
        if (filetype & 0x8000)
          if (((filetype & 0x00FF) ^ fileinfo.ff_attrib) != 0)
            continue;

        strcpy(buf, fileinfo.ff_name);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      } while (findnext(&fileinfo) == 0);
    }
#else
    if (_dos_findfirst(pszFileSpec, mask, &fileinfo) == 0)
    {
      do
      {
        /*
          Check for exclusive search requested
        */
        if (filetype & 0x8000)
          if (((filetype & 0x00FF) ^ fileinfo.attrib) != 0)
            continue;

        strcpy((char *) buf, (char *) fileinfo.name);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      } while (_dos_findnext(&fileinfo) == 0);
    }
#endif
#endif /* DOS */

    /*
      Check the critical error flag
    */
    if (CheckCriticalError(0) == 0)
      return LB_ERR;


    /*
      See if we have another path name
    */
    if (pSemi)
      pszFileSpec = pSemi;
    else
      break;
     
  }  /* for (;;) */


  /*
    FILL IN THE SUBDIRECTORIES
  */
check_dirs:
  mask = filetype & _A_SUBDIR;

  /*
    4/1/91 (maa)
    We should be able to get away with strrchr(), but I just found
    out that MS Windows will merrily accept a forward slash too!
    So, we will go transform all forward slashes to backward slashes.
  */
  for (pSlash = szFileSpec;  *pSlash;  pSlash++)
    if (*pSlash == '/')
      *pSlash = '\\';
  pSlash = (PSTR) strrchr((char *) szFileSpec, '\\');
  strcpy((pSlash != NULL) ? (char *) pSlash+1 : (char *) szFileSpec, "*.*");

#ifdef OS2
  searchcnt = 1;
  hDir = 1;
  if (mask && 
      DosFindFirst(szFileSpec, (PHDIR) &hDir, (USHORT) mask, &FileDTA,
	   sizeof(FileDTA), (PUSHORT) &searchcnt, (ULONG) 0) == 0)
  {
    do
    {
      if ((FileDTA.attrFile & _A_SUBDIR) && 
           (FileDTA.achName[0] != '.' || FileDTA.achName[1]))
      {
        sprintf(buf, "[%s]", FileDTA.achName);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      }
    } while (DosFindNext(hDir, &FileDTA, sizeof(FileDTA), (PUSHORT) &searchcnt) == 0);
    DosFindClose(hDir);
  }
#endif /* OS2 */

#ifdef DOS
#ifdef __TURBOC__
  if (mask && findfirst(szFileSpec, &fileinfo, mask) == 0)
  {
    do
    {
      /*
        Check for exclusive search requested
      */
      if (filetype & 0x8000)
        if (((filetype & 0x00FF) ^ fileinfo.ff_attrib) != 0)
          continue;

      if ((fileinfo.ff_attrib & _A_SUBDIR) && 
           (fileinfo.ff_name[0] != '.' || fileinfo.ff_name[1]))
      {
        sprintf(buf, "[%s]", fileinfo.ff_name);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      }
    } while (findnext(&fileinfo) == 0);
  }
#else
  if (mask && _dos_findfirst(szFileSpec, mask, &fileinfo) == 0)
  {
    do
    {
      if ((fileinfo.attrib & _A_SUBDIR) &&
           (fileinfo.name[0] != '.' || fileinfo.name[1]))
      {
        sprintf((char *) buf, "[%s]", fileinfo.name);
        if (SendMessage(hListBox, LB_ADDSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      }
    } while (_dos_findnext(&fileinfo) == 0);
  }
#endif
#endif /* DOS */

  /*
    FILL IN THE DISK DRIVES
  */
  if (filetype & 0x4000)
  {
    int   i;
    DWORD ulDriveMap;

    ulDriveMap = _DOSEnumDisks();
    for (i = 'A';  i <= 'Z';  i++)
    {
      if (ulDriveMap & 0x01)
      {
        sprintf((char *) buf, "[-%c-]", i);
        if (SendMessage(hListBox, LB_INSERTSTRING, ATEND,
                        (DWORD) (LPSTR) buf) == LB_ERRSPACE)
          return LB_ERRSPACE;
        nItemsDone++;
      }
      ulDriveMap >>= 1;
    }
  }

  lbi->iCurrSel = -1;
  lbi->iTopLine = 0;
  if (bWeStoppedIt)
    ListBoxSetRedraw(hListBox, TRUE);

#endif /* DOS || OS2 */

  return nItemsDone;
}


DWORD FAR PASCAL _DOSEnumDisks(void)
{
  int nDrives = 0;
  unsigned long ulDriveMask = 0L;
  unsigned drvStart, maxDrives, iDrive;
#ifdef OS2
  unsigned long tmpMask;
#endif

#ifdef DOS

#if defined(MSC) || defined(__WATCOMC__)
  _dos_getdrive(&drvStart);
  _dos_setdrive(0, &maxDrives);
#endif
#ifdef __TURBOC__
  drvStart  = getdisk();
  maxDrives = setdisk(drvStart);
#endif

  /*
    12/8/92 (maa)
    Try using GetDriveType() for better verification
  */
  for (iDrive = 0;  iDrive < maxDrives;  iDrive++)
  {
    if (GetDriveType(iDrive))
    {
      ulDriveMask |= 1L << iDrive;
      nDrives++;
    }
  }


#if defined(MSC) || defined(__WATCOMC__)
  _dos_setdrive(drvStart, &maxDrives);
#endif
#ifdef __TURBOC__
  setdisk(drvStart);
#endif

#endif /* DOS */


#ifdef OS2
  DosQCurDisk((PUSHORT) &drvStart, (PULONG) &ulDriveMask);
  DosSelectDisk(drvStart);
  nDrives = 0;
  tmpMask = ulDriveMask;

  for (iDrive = 'A';  iDrive <= 'Z';  iDrive++)
  {
    if (tmpMask & 0x01)
      nDrives++;
    tmpMask >>= 1;
  }
#endif /* OS2 */

  return ulDriveMask;
}

