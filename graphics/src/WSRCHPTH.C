/*===========================================================================*/
/*                                                                           */
/* File    : WSRCHPTH.C                                                      */
/*                                                                           */
/* Purpose : Implements the _DosSearchPath() function.                       */
/*           This function attempts to locate a file in the current          */
/*           directory, then the directory where the executable file         */
/*           resides, and finally searches path directories listed in        */
/*           the passed environment varibale (ie - PATH,INCLUDE,LIB).        */
/*                                                                           */
/*           It returns NULL if not successful. Otherwise, it copies the     */
/*           complete path name into szRetName and returns a pointer to      */
/*           szRetName.                                                      */
/*                                                                           */
/*           This function is used by the resource loader (WINRES.C) to      */
/*           find the RES file, the profile code (WPROFILE.C) to locate      */
/*           the profile file, and the OpenFile() function (WOPENFIL.C).     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


PSTR FAR PASCAL _DosSearchPath(szEnvVar, szFile, szRetName)
  PSTR szEnvVar;
  PSTR szFile;
  PSTR szRetName;
{
  char szPath[256];
  char szName[MAXPATH];
  PSTR pDir, pSemi, pEnd;

#if defined(WZV) && (defined(UNIX) || defined(VAXC))
  return strcpy(szRetName, szFile);
#endif


  /*
    First, search the current directory for the file
  */
  if (access(szFile, 0) == 0)
    return (PSTR) strcpy(szRetName, szFile);

#if defined(UNIX)
  /*
    For UNIX systems, try all lower case, then try all upper case
  */
  strcpy(szPath, szFile);
  strlwr(szPath);
  if (access(szPath, 0) == 0)
    return (PSTR) strcpy(szRetName, szPath);

  strcpy(szPath, szFile);
  lstrupr(szPath);
  if (access(szPath, 0) == 0)
    return (PSTR) strcpy(szRetName, szPath);
#endif


  /*
    Check the directory where the file was executed from (in DOS 3.0 or above).
    However, don't do this if the filename is a path specification.
  */
#if defined(DOS)
  if (_osmajor >= 3 && strchr(szFile, CH_SLASH) == NULL)
  {
    InternalGetModuleFileName(szPath, sizeof(szPath));
    if ((pEnd = (PSTR) strrchr(szPath, CH_SLASH)) != NULL)
    {
      strcpy(pEnd+1, szFile);
      if (access(szPath, 0) == 0)
        return (PSTR) strcpy(szRetName, szPath);
    }
  }
#endif /* DOS*/


  /*
    Get the path specified in the environment var
  */
  lstrupr((LPSTR) szEnvVar);
  if ((pDir = (PSTR) getenv(szEnvVar)) == NULL)
    return NULL;

  strcpy(szPath, pDir);
  pDir = szPath;

  for (;;)
  {
    /*
      Is there a semi-colon. If so, cut it off before copying.
    */
    if ((pSemi = (PSTR) strchr(pDir, ';')) != NULL)
      *pSemi = '\0';
    /*
      Copy the path, a backslash, and the filename
    */
    strcpy(szName, pDir);
    pEnd = szName + strlen(szName);
    if (pEnd[-1] != CH_SLASH)    /* append the final backslash */
      *pEnd++ = CH_SLASH;
    strcpy(pEnd, szFile);

    /*
      Check for existence
    */
    if (access(szName, 0) == 0)
      return (PSTR) strcpy(szRetName, szName);

    /*
      Not there. Try to move on to the next path.
    */
    if (pSemi == NULL)
      return NULL;
    else
      pDir = pSemi + 1;   /* go past the semicolon */
  }
}

