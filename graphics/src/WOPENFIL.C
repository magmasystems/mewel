/*===========================================================================*/
/*                                                                           */
/* File    : WOPENFIL.C                                                      */
/*                                                                           */
/* Purpose : Implements the MS Windows OpenFile() function.                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_DOSFILEMGR

#ifdef VAXC
#include <file.h>
#define O_BINARY 0
#include <types.h>
#include <stat.h>
#include <errno.h>
#endif /* VAXC */


#include "wprivate.h"
#include "window.h"

#if defined(UNIX)
#include <errno.h>
#endif


void FAR PASCAL abspath(BYTE *, BYTE *, int);
int FAR PASCAL _bytedel(BYTE *, int);
#if !defined(__ZTC__) && !defined(__WATCOMC__) && !defined(aix) && !defined(sunos) && !defined(solaris) && !defined(usl)
extern char *getcwd(char *, int);
#endif


int FAR PASCAL OpenFile(pszFile, pOFS, wMode)
  PCSTR     pszFile;
  POFSTRUCT pOFS;
  UINT      wMode;
{
  int       fd = -1;

  /*
    First, search for the file in the current directory, the directory
    where the app was run from, and along the PATH.
  */
  if (_DosSearchPath((PSTR) "PATH", (PSTR) pszFile, pOFS->szPathName))
    pszFile = pOFS->szPathName;

  if (wMode & OF_DELETE)
  {
    fd = unlink((char *) pszFile);
  }
  else if (wMode & OF_EXIST)
  {
    fd = (access((char *) pszFile, 0) == 0) ? TRUE : -1;
  }
  else if (wMode & OF_PARSE)
  {
    fd = 0;
  }
  else if (wMode & (OF_READWRITE | OF_WRITE | OF_CREATE))
  {
    /* do not truncate when OF_REOPEN is specified */
    int mode = (wMode & OF_READWRITE) ? O_RDWR : 
	(wMode & OF_REOPEN) ? O_WRONLY : (O_WRONLY | O_TRUNC);

    if ((fd = open((char *) pszFile, mode | O_BINARY)) < 0 && (wMode & OF_CREATE))
    {
      if (!(wMode&OF_PROMPT) || 
            MessageBox(NULLHWND, (PSTR) "Create the file?", (PSTR) "Open",
                       (wMode & OF_CANCEL) ? MB_OKCANCEL : MB_OK) != IDCANCEL)
        fd = open((char *) pszFile, mode|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
    }
  }
  else /* OF_READ */
  {
    fd = open((char *) pszFile, O_RDONLY | O_BINARY);
  }

  if (fd != -1)
  {
    char buf[MAXPATH];
    abspath((BYTE *) pszFile, (BYTE *) buf, sizeof(buf));
    strcpy((char *) pOFS->szPathName, (char *) buf);
    pOFS->nErrCode = 0;
  }
  else
    pOFS->nErrCode = errno;

  return fd;
}



/****************************************************************************/
/*  ABSPATH.C                                                               */
/*    abspath(BYTE *path, BYTE *absbuf, int abslen)                         */
/*                                                                          */
/*  Takes the path name specified in 'path' and returns its canonical form  */
/* into absbuf[];                                                           */
/*                                                                          */
/* Copyright (C) 1989  Marc Adler / Magma Systems    All Rights Reserved    */
/*                                                                          */
/****************************************************************************/
void FAR PASCAL abspath(path, absbuf, abslen)
  BYTE *path;
  BYTE *absbuf;
  int  abslen;
{
  BYTE *szAbsBuf;
  int  drive, currdrive, numdrives, c;


  absbuf[0] = '\0';


  /*
     Get the specified or default drive.
  */
#ifdef DOS
#ifdef __TURBOC__
  currdrive = getdisk() + 1;
#else
  _dos_getdrive((unsigned *) &currdrive);
#endif
#endif
#ifdef OS2
{
  ULONG  ulDriveMap;
  DosQCurDisk((PUSHORT) &currdrive, (PULONG) &ulDriveMap);
}
#endif /* OS2 */


  /*
    Prepend the drive specifier to the absolute path
  */

#ifndef UNIX
  if (path[1] == ':')
  {
    drive = toupper(*path);
    path += DRIVESPEC_LENGTH;
  }
  else
  {
    drive = 'A' + currdrive - 1;
  }
  absbuf[0] = (BYTE) drive;  absbuf[1] = ':';  absbuf[2] = '\0';
#endif /* UNIX */

   
  /*
     If the file specifier didn't start with '\', the prepend the
     current directory to the path.
  */
  if (*path != CH_SLASH)
  {
#ifdef DOS
#ifdef __TURBOC__
    setdisk(toupper(drive) - 'A');
#else
    _dos_setdrive(toupper(drive) - 'A' + 1, (unsigned *) &numdrives);
#endif
#endif
#ifdef OS2
    DosSelectDisk(toupper(drive) - 'A' + 1);
#endif /* OS2 */

    getcwd((char *) absbuf + DRIVESPEC_LENGTH, abslen - DRIVESPEC_LENGTH);
#if defined(TEST) && defined(UNIX)
    {
    BYTE *s;
    if (DRIVESPEC_LENGTH)
      _bytedel(absbuf, DRIVESPEC_LENGTH);
    for (s = absbuf;  *s;  s++)
      if (*s == '\\')
        *s = '/';
    }
#endif

#ifdef DOS
#ifdef __TURBOC__
    setdisk(currdrive - 'A');
#else
    _dos_setdrive(currdrive - 'A' + 1, (unsigned *) &numdrives);
#endif
#endif
#ifdef OS2
    DosSelectDisk(currdrive - 'A' + 1);
#endif /* OS2 */

#ifndef UNIX
    _bytedel(absbuf + 2, 2);   /* get rid of the extra drive specifier */
#endif
    if (absbuf[strlen((char *) absbuf) - 1] != CH_SLASH)
      strcat((char *) absbuf, STR_SLASH);
  }
  strcat((char *) absbuf, (char *) path);

  szAbsBuf = absbuf + DRIVESPEC_LENGTH;
  while ((c = *szAbsBuf++) != '\0')
    if (c == CH_SLASH && *szAbsBuf == '.')
    {
      if (szAbsBuf[1] == '.')    /* \.. */
      {
        BYTE *start, *end;
        end = szAbsBuf + 2;      /* point to slash after the .. */

        for (start = szAbsBuf-2;  *start != ':' && *start != CH_SLASH;  start--) ;
        if (*start == ':')
          /* we have c:\..\xxx */
          _bytedel(start+1, 3);
        else
          /* we have c:\foo\..\baz */
          /*            |     |    */
          _bytedel(start+1, (end - start));

        szAbsBuf = absbuf + DRIVESPEC_LENGTH;
      }
      else if (szAbsBuf[1] == CH_SLASH || szAbsBuf[1] == '\0')   /* \.\  or \. */
      {
        _bytedel(szAbsBuf-1, 2);
        szAbsBuf = absbuf + DRIVESPEC_LENGTH;
      }
    }
}


int FAR PASCAL _bytedel(s, n)
  BYTE *s;
  int  n;
{
  BYTE *end = s + n;
  while ((*s++ = *end++) != '\0')
    ;
  return TRUE;
}


#ifdef TEST
main()
{
  BYTE path[80], absbuf[80];
  int  fd1, fd2;
  OFSTRUCT of1, of2;

#if 0
  while (gets(path))
  {
    abspath(path, absbuf, sizeof(absbuf));
    printf("Absolute path is [%s]\n", absbuf);
  }
#endif

  fd1 = OpenFile("mewldemo", &of1, OF_READ);
  fd2 = OpenFile("xxxYYY", &of2, OF_READWRITE);
}


BYTE *FAR PASCAL _DosSearchPath(szEnvVar, szFile, szRetName)
  BYTE *szEnvVar;
  BYTE *szFile;
  BYTE *szRetName;
{
  extern char *getenv();
  char szPath[256], *pDir, *pSemi, *pEnd;
  char szName[MAXPATH];

  /*
    First, search the current directory for the file
  */
  if (access((char *) szFile, 0) == 0)
    return (PSTR) strcpy((char *) szRetName, (char *) szFile);


  /*
    Check the directory where the file was executed from (in DOS 3.0 or above)
  */
#ifdef DOS
  if (_osmajor >= 3)
  {
    GetModuleFileName(0, szPath, sizeof(szPath)-1);
    if ((pEnd = (PSTR) strrchr((char *) szPath, '\\')) != NULL)
    {
      strcpy((char *) pEnd+1, (char *) szFile);
      if (access((char *) szPath, 0) == 0)
         return (PSTR) strcpy((char *) szRetName, (char *) szPath);
      }
  }
#endif /* DOS*/


  /*
    Get the path specified in the environment var
  */
  lstrupr((LPSTR) szEnvVar);
  if ((pDir = (PSTR) getenv((char *) szEnvVar)) == NULL)
    return NULL;

  strcpy((char *) szPath, (char *) pDir);

  for (;;)
  {
    /*
      Is there a semi-colon. If so, cut it off before copying.
    */
    if ((pSemi = (PSTR) strchr((char *) pDir, ';')) != NULL)
      *pSemi = '\0';
    /*
      Copy the path, a backslash, and the filename
    */
    strcpy((char *) szName, (char *) pDir);
    pEnd = szName + strlen((char *) szName);
    if (pEnd[-1] != '\\')    /* append the final backslash */
      *pEnd++ = '\\';
    strcpy((char *) pEnd, (char *) szFile);

    /*
      Check for existence
    */
    if (access((char *) szName, 0) == 0)
      return (PSTR) strcpy((char *) szRetName, (char *) szName);

    /*
      Not there. Try to move on to the next path.
    */
    if (pSemi == NULL)
      return NULL;
    else
      pDir = pSemi + 1;   /* go past the semicolon */
  }
}

#endif /* TEST */

