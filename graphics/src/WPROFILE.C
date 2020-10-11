/*===========================================================================*/
/*                                                                           */
/* File    : WPROFILE.C                                                      */
/*                                                                           */
/* Purpose : Routines to handle the MEWEL.INI file                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"


#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif

#if 0
typedef unsigned char BOOL;
typedef unsigned char BYTE;

extern int FAR PASCAL WriteProfileString(PSTR szApp,PSTR szKey,PSTR szVal);
extern int FAR PASCAL WritePrivateProfileString(PSTR szApp,PSTR szKey,PSTR szVal,PSTR szFile);
extern int FAR PASCAL GetProfileString(PSTR szApp,PSTR szKey,PSTR szDefault,PSTR szRetStr,int nSize);
extern int FAR PASCAL GetPrivateProfileString(PSTR szApp,PSTR szKey,PSTR szDefault,PSTR szRetStr,int nSize,PSTR szFile);
extern int FAR PASCAL GetProfileInt(PSTR szApp,PSTR szKey,int nDefault);
extern int FAR PASCAL GetPrivateProfileInt(PSTR szApp,PSTR szKey,int nDefault,PSTR szFile);

extern int FAR PASCAL lstricmp(LPSTR, LPSTR);
extern BYTE FAR *FAR PASCAL lstrcpy(BYTE FAR *s1,BYTE FAR *s2);

#endif

static BOOL FAR PASCAL _ProfileFindApp(LPCSTR);
static int  FAR PASCAL _ProfileFindKey(LPCSTR, LPSTR, int);
static int  FAR PASCAL _ProfileEnumAppKeys(LPSTR, int);
static int  FAR PASCAL _FindFile(PSTR);
static int      PASCAL xtoi(LPSTR);


static FILE *fpProfile  = NULL;
static FILE *fpProfTemp = NULL;
static long ProfileCurrSeekPos;
static long _ulSeekPosOfLastAppLineWritten;
static char szCurrProfileFile[MAXPATH];

#if defined(ZAPP) && !defined(__DPMI16__)
extern char* zProfileFile;
#define PROFILE_FILE  (PSTR) zProfileFile
#define BAKFILE       (PSTR) "zapp.bak"
#define TEMPFILE      "zappini.~~~"
#elif defined(MOTIF)
#define PROFILE_FILE  (PSTR) "wmm.ini"
#define BAKFILE       (PSTR) "wmm.bak"
#define TEMPFILE      "wmmini.~~~"
#else
#define PROFILE_FILE  (PSTR) "mewel.ini"
#define BAKFILE       (PSTR) "mewel.bak"
#define TEMPFILE      "mewelini.~~~"
#endif

/*
  We will try an experiment here. Instead of fclose()ing the profile
  file between successive reads, we will keep it open until the
  user switches to a writing mode.
*/
#define _FCLOSE_(fp)

/*
  Another experiment. We can have a situation where a user tries to do
  a lot of GetProfileString()s on a non-existent MEWEL.INI file. This
  can take up a lot of time because of the path searching which 
  GetProfileString() does. We will use a boolean flag to record whether
  or not we have a MEWEL.INI file. This flag is always set to true after
  we do a WriteProfileString(), cause we know that we just created or
  appended to a MEWEL.INI file. If FindFile() does not find a MEWEL.INI,
  then it will set this to FALSE. When we call FindFile() subsequent
  times, we will first test this flag, and if a MEWEL.INI file still doesn't
  exist, we will return FALSE immediately without searching the disk.
*/
BOOL  bProfileExists = TRUE;


/****************************************************************************/
/*                                                                          */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int FAR PASCAL WritePrivateProfileString(szApp, szKey, szVal, szProfileFile)
  LPCSTR szApp,
         szKey,
         szVal,
         szProfileFile;
{
  char line[256];
  char szTmp[128];
  PSTR szOpenmode;
  PSTR pszTmp;

  /*
    Windows has some strange behavior .... it will accept a profile
    file name with double-quotes surrounding it! Let's remove
    the quotes!
  */
  if (szProfileFile[0] == '"')
  {
    ((LPSTR) szProfileFile)[lstrlen(szProfileFile)-1] = '\0';
    lstrcpy((LPSTR) szProfileFile, (LPSTR) szProfileFile+1);
  }

  /*
    If the file was open from our successive reads, then close it first.
  */
  if (fpProfile)
  {
    fclose(fpProfile);
    fpProfile = NULL;
  }

  strcpy((char *) szTmp, TEMPFILE);
#ifndef UNIX
  if (szProfileFile[1] == ':')      /* a different disk? */
  {
    PSTR pSlash;
    lstrcpy(szTmp, szProfileFile);
    if ((pSlash = (PSTR) strrchr((char *) szTmp, '\\')) != NULL)
      strcpy((char *) pSlash + 1, TEMPFILE);
  }
#endif

  if ((fpProfTemp = fopen((char *) szTmp, "w")) == NULL)
    return FALSE;

  /*
    If the profile file exists, open in r+ mode, else create and open in a+
  */
  lstrcpy(line, szProfileFile);
  if (_FindFile(line))
    szOpenmode = (PSTR) "r+";
  else
    szOpenmode = (PSTR) "a+";

  if ((fpProfile = fopen((char *) line, (char *) szOpenmode)) == NULL)
    fpProfile = fpProfTemp;

  fseek(fpProfile, 0L, 0);

  /*
    See if the [app] is already in the profile file
  */
  if (_ProfileFindApp(szApp))
  {
    if (szKey == NULL)
    {
      /*
        If the key is NULL, then the entire [app] section should be
        deleted.
      */
      int nBytesToErase = (int) (ftell(fpProfTemp) - _ulSeekPosOfLastAppLineWritten);
      fseek(fpProfTemp, _ulSeekPosOfLastAppLineWritten, 0);
      while ((pszTmp = fgets(line, sizeof(line), fpProfile)) != NULL)
      {
        if (line[0] == '[')  /* found the next app section */
        {
          fputs((char *) line, fpProfTemp);
          break;
        }
        else if (line[0] == ';')
          /*
            Windows preserves all comment lines when the app is deleted
          */
          fputs((char *) line, fpProfTemp);
      }
      /*
        If we hit the end of file when we delete an [app], then we must
        truncate the temporary file at _ulSeekPosOfLastAppLineWritten.
        Otherwise, the [app] is still in the temp file, albeit without any
        key/value pairs.
      */
      if (pszTmp == NULL)
      {
        unsigned char chEOF = ' ';
        fseek(fpProfTemp, _ulSeekPosOfLastAppLineWritten, 0);
        while (nBytesToErase-- > 0)
          fwrite(&chEOF, 1, 1, fpProfTemp);
      }
    }
    else
    {
      /*
        If so, See if the key is in the app section
      */
      _ProfileFindKey(szKey, NULL, 0);
      /*
        The key is not in the app section - append it to the end of the
        app section. If the value is NULL, then the key should be deleted.
      */
      if (szVal != NULL)
#if (MODEL_NAME == MODEL_MEDIUM)
        fprintf(fpProfTemp, "%Fs=%Fs\n", szKey, szVal);
#else
        fprintf(fpProfTemp, "%s=%s\n", szKey, szVal);
#endif

      fseek(fpProfile, ProfileCurrSeekPos, 0);
    }

    /*
      Dump the rest of the INI file out to the temp file.
    */
    while (fgets((char *) line, sizeof(line), fpProfile))
      fputs((char *) line, fpProfTemp);
  }
  else
  {
    /*
      If [app] is not in the profile, append the app and string to the file.
      Do this only if we are not deleting a non-existent app.
    */
    if (szKey && szVal)
#if (MODEL_NAME == MODEL_MEDIUM)
      fprintf(fpProfTemp, "\n[%Fs]\n%Fs=%Fs\n", szApp, szKey, szVal);
#else
      fprintf(fpProfTemp, "\n[%s]\n%s=%s\n", szApp, szKey, szVal);
#endif
  }

  fclose(fpProfile);
  fclose(fpProfTemp);

  /*
    Make the temp file the new profile file.
  */
  unlink((char *) BAKFILE);
#ifndef UNIX
  if (szProfileFile[1] == ':')
  {
    lstrcpy(line, szProfileFile);  /* medium model kludge */
    unlink((char *) line);
  }
  else
#endif
  {
    lstrcpy(line, szProfileFile);  /* medium model kludge */
    rename((char *) line, (char *) BAKFILE);
  }

  lstrcpy(line, szProfileFile);  /* medium model kludge */
  rename((char *) szTmp, (char *) line);

  fpProfile = NULL;
  fpProfTemp = NULL;  /* signal that we are not in write mode */

  /*
    We are certain that a profile file exists, so set a flag saying so.
  */
  bProfileExists = TRUE;

  return TRUE;
}


int FAR PASCAL WriteProfileString(szApp, szKey, szVal)
  LPCSTR szApp,
         szKey,
         szVal;
{
  return WritePrivateProfileString(szApp, szKey, szVal, PROFILE_FILE);
}

/****************************************************************************/
/*                                                                          */
/* GetProfileString()                                                       */
/*                                                                          */
/*   Find the key associated with an application, and return the key value  */
/* as a string.                                                             */
/*                                                                          */
/****************************************************************************/

int FAR PASCAL GetPrivateProfileString(szApp, szKey, szDefault, szRetStr, nSize, szProfileFile)
  LPCSTR szApp;
  LPCSTR szKey;
  LPCSTR szDefault;
  LPSTR  szRetStr;
  int    nSize;
  LPCSTR szProfileFile;
{
  int  n;
  char fname[MAXPATH];

  /*
    Windows has some strange behavior .... it will accept a profile
    file name with double-quotes surrounding it! Let's remove
    the quotes!
  */
  if (szProfileFile[0] == '"')
  {
    ((LPSTR) szProfileFile)[lstrlen(szProfileFile)-1] = '\0';
    lstrcpy((LPSTR) szProfileFile, (LPSTR) szProfileFile+1);
  }

  /*
    Try to locate the MEWEL.INI file
  */
  lstrcpy(fname, szProfileFile);

  /*
    New addition for private profiles....
    If we are maintaining the open profile file, but the profile file
    which we want to read from is different than the last-opened
    profile file, then close the last-opened profile file.
  */
  if (fpProfile != NULL && strcmp((char *) fname, (char *) szCurrProfileFile) != 0)
  {
    fclose(fpProfile);
    fpProfile = NULL;
  }

  /*
    Find and open the MEWEL.INI file (if it is not opened yet).
  */
  if (fpProfile == NULL)
  {
    if (!_FindFile(fname) || (fpProfile = fopen((char *) fname, "r")) == NULL)
    {
copy_default:
      if (szDefault != NULL)
      {
        lstrncpy(szRetStr, szDefault, nSize-1);
        szRetStr[nSize-1] = '\0';
      }
      else
        szRetStr[0] = '\0';
      return lstrlen(szRetStr);
    }
  }
  else
    fseek(fpProfile, 0L, 0);

  /*
    Try to find the app name.
  */
  if (!_ProfileFindApp(szApp))
  {
    /*
      The app or key wasn't found. Return the default string.
    */
    _FCLOSE_(fpProfile);
    goto copy_default;
  }

  /*
    If the user specified NULL as the key to look for, return a buffer
    which contains all of the keys in the app.
  */
  if (szKey == NULL)
  {
    nSize = _ProfileEnumAppKeys(szRetStr, nSize);
    _FCLOSE_(fpProfile);
    return nSize;
  }

  /*
    Try to find the key. If found, the value is returned.
  */ 
  if ((n = _ProfileFindKey(szKey, szRetStr, nSize)) == 0)
  {
    _FCLOSE_(fpProfile);
    goto copy_default;
  }

  _FCLOSE_(fpProfile);
  return n;
}


int FAR PASCAL GetProfileString(szApp, szKey, szDefault, szRetStr, nSize)
  LPCSTR szApp;
  LPCSTR szKey;
  LPCSTR szDefault;
  LPSTR  szRetStr;
  int    nSize;
{
  return GetPrivateProfileString(szApp,szKey,szDefault,szRetStr,nSize,PROFILE_FILE);
}


/****************************************************************************/
/*                                                                          */
/* GetProfileInt()                                                          */
/*                                                                          */
/*  Find an integer key value associated with an application.               */
/*                                                                          */
/****************************************************************************/

UINT FAR PASCAL GetPrivateProfileInt(szApp, szKey, nDefault, szProfileFile)
  LPCSTR szApp;
  LPCSTR szKey;
  int    nDefault;
  LPCSTR szProfileFile;
{
  char szBuf[32], szDefault[32];

  sprintf((char *) szDefault, "%d", nDefault);
  if (!GetPrivateProfileString(szApp, szKey, szDefault, 
                               szBuf, sizeof(szBuf), szProfileFile))
    return nDefault;
  else
  {
    if (szBuf[0] == '0' && (szBuf[1] == 'x' || szBuf[1] == 'X'))
      return xtoi(szBuf+2);
    return atoi((char *) szBuf);
  }
}


UINT FAR PASCAL GetProfileInt(szApp, szKey, nDefault)
  LPCSTR szApp;
  LPCSTR szKey;
  int  nDefault;
{
  return GetPrivateProfileInt(szApp, szKey, nDefault, PROFILE_FILE);
}


/****************************************************************************/
/*                                                                          */
/* _ProfileFindApp()                                                        */
/*                                                                          */
/* Scans the MEWEL.INI file until a line with the [appname] is found.       */
/* Returns TRUE or FALSE, depending on if the app is found.                 */
/*                                                                          */
/****************************************************************************/
static BOOL FAR PASCAL _ProfileFindApp(szApp)
  LPCSTR szApp;
{
  char line[256];

  while (fgets((char *) line, sizeof(line), fpProfile))
  {
    if (fpProfTemp)
    {
      if (*line == '[')
        _ulSeekPosOfLastAppLineWritten = ftell(fpProfTemp);
      fputs((char *) line, fpProfTemp);
    }

    if (*line == '[')
    {
      /*
        We have a line with an appname. Is it the right one?
      */
      PSTR s = (PSTR) strrchr((char *) line, ']');
      if (s)
        *s = '\0';
      if (!lstricmp((LPSTR) line+1, (LPSTR) szApp))
      {
        /*
          We found it! Save the file position for the key scan.
          (The file ptr is positioned at the first key.)
        */
        ProfileCurrSeekPos = ftell(fpProfile);
        return TRUE;
      }
    }
  }

  return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* _ProfileFindKey()                                                        */
/*                                                                          */
/* Find a particular key within a group of key-value pairs. Returns the     */
/* value associated with the key.                                           */
/*                                                                          */
/****************************************************************************/

static int FAR PASCAL _ProfileFindKey(szKey, szRetStr, nSize)
  LPCSTR szKey;
  LPSTR  szRetStr;
  int    nSize;
{
  char line[256], *szLine, ch;
  int  nKeyLen = lstrlen(szKey);
  PSTR s;
  int nLen;

  /* strip trailing whitespace */
  while (nKeyLen && isspace(szKey[nKeyLen - 1]))
    nKeyLen--;
  if (!nKeyLen)
    return 0;

  /*
    Go through all of the keys in this app...
    The keys are of the form
      keyname=value
  */
  while (fgets((char *) line, sizeof(line), fpProfile))
  {
    /*
      Span leading whitespace
    */
    for (szLine = line;  (ch = *szLine) != '\0' && isspace(ch);  szLine++)
      ;

#ifdef STOP_AT_BLANK_LINE
    if (*szLine == '[' || *szLine == '\0')  /* the start of a new app... */
      return 0;
#else
    if (*szLine == '[')                     /* the start of a new app... */
      return 0;
    if (*szLine == '\0')
      goto next;
#endif

    /*
      Save the current file position in case we find the key
    */
    ProfileCurrSeekPos = ftell(fpProfile);

    /*
      Now split the line into key and value. Any trailing whitespace
      in the key is ignored.
    */
    s = (PSTR) strchr((char *) szLine, '=');
    if (!s)
      s = strchr(szLine, '\0');
    nLen = s - szLine;
    while (nLen && isspace(szLine[nLen - 1]))
      nLen--;

    if (nLen == nKeyLen && !lstrnicmp((LPSTR) szLine, (LPSTR) szKey, nLen))
    {
      /*
        We found the key! Now extract the value.
      */
      LPSTR t = szRetStr;
      if (szRetStr != NULL)
      {
        /* Span initial blanks. s first points to the '='. */
        for (s++;  *s && isspace(*s);  s++)
          ;
        /* Span a quotation (Windows does this) */
        if (*s == '"')
          s++;
        /* Copy the value - up to nSize bytes */
        for (  ;  nSize > 0 && *s && *s != '\n';  *t++ = *s++)
          nSize--;
        /* Don't include the ending quote */
        if (t[-1] == '"')
          t--;
        *t = '\0';
        return (t - szRetStr + 1);    /* Return the # of bytes copied */
      }
      else
        return 1;
    }

next:
    if (fpProfTemp)
    {
      fputs((char *) line, fpProfTemp);
      if (!strchr ((char *) line, '\n'))
        fputs("\n", fpProfTemp);
    }
  }

  return 0;
}


/****************************************************************************/
/*                                                                          */
/*  _ProfileEnumAppKeys()                                                   */
/*                                                                          */
/*    Finds all of the keys in an app and returns a buffer of all keys.     */
/*  Each key is separated by a NULL byte.  There are two NULLS at the       */
/*  end of the key buffer.                                                  */
/*                                                                          */
/****************************************************************************/

static int FAR PASCAL _ProfileEnumAppKeys(szRetStr, nSize)
  LPSTR szRetStr;     /* ptr to the buffer to store the key names */
  int  nSize;         /* max size of key buffer */
{
  char line[256], *s, c;
  LPSTR origStr = szRetStr;

  /*
    We are positioned at the 1st line of keys of an [appname].
    Read all of the keys of this app.
  */
  while (fgets((char *) line, sizeof(line), fpProfile))
  {
    if (*line == ';')                   /* don't copy comment lines */
      continue;

    if (*line == '[' || *line == '\0')  /* the start of a new app... */
      break;
    /*
      We have a line of the form
        keyname=value
      Copy the key to the key buffer.
    */
    if ((s = (PSTR) strchr((char *) line, '=')) != NULL)
    {
      *s = '\0';
      for (s = line;  nSize > 1 && (c = *s++) != '\0';  *szRetStr++ = c)
        nSize--;
      *szRetStr++ = '\0';
      if (--nSize <= 1)
        break;
    }
  }

  *szRetStr = '\0';      /* put in the 2nd terminating null */
  return (szRetStr - origStr);
}


static int FAR PASCAL _FindFile(szFile)
  PSTR szFile;
{
  char buf[132];

  if (!bProfileExists   /* it doesn't exist ... don't churn the disk */
      /*
        New addition for private profiles...
        If opening a different profile file than the last one, try the
        disk search...
      */
      && !strcmp((char *) szFile, (char *) szCurrProfileFile))
    return 0;

  /*
    New addition for private profiles...
    Save the name of this profile file
  */
  strcpy((char *) szCurrProfileFile, (char *) szFile);

  if (_DosSearchPath("PATH", szFile, buf) != NULL)
  {
    strcpy((char *) szFile, (char *) buf);
    return (bProfileExists = TRUE);
  }
  else
    return (bProfileExists = FALSE);
}


VOID FAR PASCAL CloseProfile(void)
{
  if (fpProfile)
  {
    fclose(fpProfile);
    fpProfile = NULL;
  }
}

/* xtoi - converts a hex string to an unsigned long */
static int PASCAL xtoi(LPSTR lpStr)
{
  int  sum = 0;
  char c;
  
  while ((c = *lpStr++) != '\0' && isxdigit(c))
  {
    c = (isalpha(c)) ? (toupper(c) - 'A' + 10) : (c - '0');
    sum = (sum << 4) + c;
  }
  return sum;
}



#if 0
#define TEST
#endif
#ifdef TEST

main()
{
  char val[120];
  int  n;

  WriteProfileString("mewel", "doubleclick", "250");
  WriteProfileString("mewel", "mouserepeat", "1000");
  WriteProfileString("mewel", "mousedelay",  "125");
  WriteProfileString("mewel", "scrollbar",  "RED");
  WriteProfileString("editor", "insert",  "<INSERT>");
  WriteProfileString("editor", "delchar",  "<DELETE>");
  WriteProfileString("editor", "up",  "<UP>");
  WriteProfileString("editor", "down",  "<DOWN>");
  WriteProfileString("editor", "left",  "<LEFT>");
  WriteProfileString("editor", "pgdn",  "<PGDN>");
  WriteProfileString("me", "hscroll",  "40");


#if 0
  WritePrivateProfileString("editor", "insline",  "<F2>", "FOO.INI");
  WritePrivateProfileString("editor", "delline",  "<F1>", "FOO.INI");
  WritePrivateProfileString("editor", "deleol",   "<SH NUM 5>", "FOO.INI");


  n = GetProfileInt("mewel", "mousedelay", 1000);
  printf("mousedelay is %d\n", n);
  GetProfileString("editor", "left", "foo", val, sizeof(val));
  printf("Left is %s\n", val);
  GetPrivateProfileString("editor", NULL, "foo", val, sizeof(val), "FOO.INI");
  printf("PRIVATE - val is %s\n", val);
  GetProfileString("editor", NULL, "foo", val, sizeof(val));
  printf("val is %s\n", val);

  /*
    Now try deleting a key
  */
  WriteProfileString("editor", "left",  NULL);
  GetProfileString("editor", "left", "foo", val, sizeof(val));
  printf("After deleting, Left is %s\n", val);

  /*
    Now try deleting an app
  */
  WriteProfileString("mewel", NULL,  NULL);
  n = GetProfileInt("mewel", "mousedelay", 1000);
  printf("After deleting [mewel], mousedelay is %d\n", n);
#endif

  return 0;
}

BYTE FAR *FAR PASCAL lstrcpy(BYTE FAR *s1,BYTE FAR *s2)
{
  char FAR *orig_s1 = s1;

  while ((*s1++ = *s2++) != '\0') ;
  return orig_s1;
}


PSTR FAR PASCAL _DosSearchPath(szEnvVar, szFile, szRetName)
  PSTR szEnvVar;
  PSTR szFile;
  PSTR szRetName;
{
  char szPath[132];
  char szName[MAXPATH];
  PSTR pDir, pSemi, pEnd;

  /*
    First, search the current directory for the file
  */
  if (access((char *) szFile, 0) == 0)
    return (PSTR) strcpy((char *) szRetName, (char *) szFile);


  /*
    Check the directory where the file was executed from (in DOS 3.0 or above).
    However, don't do this if the filename is a path specification.
  */
#ifdef DOS
  if (_osmajor >= 3 && strchr((char *) szFile, CH_SLASH) == NULL)
  {
    GetModuleFileName(0, szPath, sizeof(szPath));
    if ((pEnd = (PSTR) strrchr((char *) szPath, CH_SLASH)) != NULL)
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
  pDir = szPath;

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
    if (pEnd[-1] != CH_SLASH)    /* append the final backslash */
      *pEnd++ = CH_SLASH;
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

#if defined(_MSC_VER) && (_MSC_VER <= 700) && !defined(__WATCOMC__)
extern char FAR *_pgmptr;
#elif defined(__TURBOC__) || defined(__ZTC__) || defined(__WATCOMC__)
extern char **_argv;
#define _pgmptr           (_argv[0])
#endif


int FAR PASCAL GetModuleFileName(hModule, lpFileName, nSize)
  HANDLE hModule;
  LPSTR  lpFileName;
  int    nSize;
{
  (void) hModule;

  lpFileName[0] = '\0';

#if defined(__TURBOC__) || defined(ZORTECH) || defined(__TSC__)
  strcpy(lpFileName, _argv[0]);
#elif defined(MSC)
  lstrcpy(lpFileName, _pgmptr);;
#endif

  return lstrlen(lpFileName);
}
#endif /* TEST */
