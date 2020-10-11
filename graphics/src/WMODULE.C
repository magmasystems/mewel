/*===========================================================================*/
/*                                                                           */
/* File    : WMODULE.C                                                       */
/*                                                                           */
/* Purpose : Routines to deal with the filename of the module                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  For retrieving the path of the running app
*/
#if defined(__HIGHC__)
#ifdef HC311
// LCH _mwprognamep disappeared for HC++ version 3.11... so I have added
// special support for this version.  I convinced MetaWare to add it back
// in, just for this sort of third-party library support; but we still
// need to support HC++ version 3.11, so see below.
extern _Far char *_mwenvp;
#else
extern char *_mwprognamep;
#define _pgmptr           (_mwprognamep)
#endif /* HC311 */

/* Microsoft C 6,7,8 */
#elif defined(_MSC_VER) && !defined(__WATCOMC__) && !defined(__HIGHC__) && !defined(PLTNT)
#if (_MSC_VER <= 700)
extern char far * cdecl _pgmptr;
#else 
extern char __far * __near __cdecl _pgmptr;
#endif

/* MSVC++/32 with Pharlap TNT */
#elif defined(PLTNT) && !defined(__WATCOMC__)
extern char **__argv;
#define _pgmptr           (__argv[0])

/* BC++, Zortech, Watcom, High C */
#elif defined(__TURBOC__) || defined(__ZTC__) || defined(__WATCOMC__)
extern char **_argv;

#elif defined(XWINDOWS)
extern int _Xargc;
extern char **_Xargv;
#endif

/*
  Holds the module name
*/
static char szModuleName[MAXPATH];


static VOID FAR PASCAL InternalInitModname(void)
{
  /*
    System-dependent way to retrieve the name of the app which is
    currently running.
  */
  szModuleName[0] = '\0';

#if defined(DOS)

  /*
    Get around bug in Pharlap 286 and MSVC where _osmajor is 0.
  */
#if defined(DOS286X) && defined(_MSC_VER) && (_MSC_VER >= 700)
  if (1)
#else
  if (_osmajor >= 3)
#endif

#ifdef HC311
/*
  LCH In HC++ 3.11, the library initialization code was re-written from
  scratch.  While it looks lovely, it no longer supported _mwprognamep,
  a global char * that pointed to the path to the currently executing
  program.  While this _is_ of course passed in to main(), there are some
  global/static constructors for MFC that call GetModuleFileName() before
  main() gets called (from _cinit()).
 
  What I've done here is parrot the code from the HC++ 3.11 startup code
  that finds the path to the program name in the first place, from
  initenvp.c.  It uses the _mwenvp pointer, which is also undocumented
  but _is_ supported -- it's used by the MetaWare library calls getenv()
  and putenv().
*/
  {
    char *temp = szModuleName;
    char _Far *eptr;

    eptr = _mwenvp;
    while (*eptr != '\0')
    {
      eptr = (_Far unsigned char *)
      _findchr((_Far const char *)eptr, 0, (unsigned)-1);
      eptr++; /* skip past nul terminator */
    }

    /* found end of environment, advance to program name */
    eptr += 3;

    if (*eptr)
      do
      {
        *temp++ = *eptr++;
      } while (*eptr);
  }
/* end of HC311 code */

#elif defined(__DLL__)
    strcpy((char *) szModuleName, "");  /* no argv in DLLs */
#elif defined(__TURBOC__) || defined(__ZTC__) || defined(__TSC__) || defined(__WATCOMC__)
    strcpy((char *) szModuleName, _argv[0]);
#elif defined(MSC)
    lstrcpy(szModuleName, _pgmptr);
#endif


#elif defined(OS2)
  {
  USHORT sel, off;
  if (DosGetEnv((PUSHORT) &sel, (PUSHORT) &off) == 0)
    lstrcpy(szModuleName, (LPSTR) MK_FP(sel, off));
  }

#elif defined(XWINDOWS)
  strcpy((char *) szModuleName, _Xargv[0]);

#endif
}


int FAR PASCAL InternalGetModuleFileName(lpFileName, nSize)
  LPSTR  lpFileName;
  int    nSize;
{
  static BOOL bInitModule = FALSE;

  /*
    First time initialization of the module name
  */
  if (!bInitModule)
  {
    InternalInitModname();
    bInitModule = TRUE;
  }

  /*
    Copy the module name into the buffer and return the length
  */
  lstrncpy(lpFileName, szModuleName, nSize);
  return lstrlen(lpFileName);
}


#if !defined(__DLL__) && !defined(__DPMI16__)
int FAR PASCAL GetModuleFileName(hModule, lpFileName, nSize)
  HANDLE hModule;
  LPSTR  lpFileName;
  int    nSize;
{
  (void) hModule;
  return InternalGetModuleFileName(lpFileName, nSize);
}

HMODULE WINAPI GetModuleHandle(lpModule)
  LPCSTR lpModule;
{
  (void) lpModule;
  return 0;
}

int WINAPI GetModuleUsage(h)
  HINSTANCE h;
{
  (void) h;
  return 1;
}
#endif

