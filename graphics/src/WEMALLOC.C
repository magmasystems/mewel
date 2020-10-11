/*===========================================================================*/
/*                                                                           */
/* File    : WEMALLOC.C                                                      */
/*                                                                           */
/* Purpose : Low-level memory allocation routines.                           */
/*                                                                           */
/* History :                                                                 */
/*    10/19/92 (maa) - added GmemRealloc().                                  */
/*    03/02/93 AJP - Added calls to pharlap DosAllocSeg() etc. in            */
/*             emalloc_far, My_Free, realloc_far to return selector          */
/*             and offset=0 so GlobalHandle(lPtr) will return the            */
/*             correct handle under MSC 7.0                                  */ 
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MEMMGR
#include "wprivate.h"
#include "window.h"

/*
  Change the '#if 0' to '#if 1' if you want to use the Pharlap segment
  allocation functions.
*/
#if 0
#define USE_ALLOCSEG
#endif

/*
  Internal memory variables
*/
BOOL MemDebug = FALSE;


static VOID FAR PASCAL MemDebugOut(PSTR);
static VOID FAR PASCAL _WinDie(void);

#if defined(__TURBOC__) && !defined(EXTENDED_DOS)
void DumpHeapWalk(char FAR *);
#endif


/****************************************************************************/
/*                                                                          */
/* Function : strsave(string)                                               */
/*                                                                          */
/* Purpose  : Saves a copy of a string in near memory.                      */
/*                                                                          */
/* Returns  : Pointer to the saved string.                                  */
/*                                                                          */
/****************************************************************************/
#ifdef NOTUSED
PSTR FAR PASCAL strsave(s)
  PSTR s;
{
  PSTR t;
  if ((t = (PSTR) emalloc(strlen((char *) s) + 1)) != NULL)
    strcpy((char *) t, (char *) s);
  return t;
}
#endif

/****************************************************************************/
/*                                                                          */
/* Function : lstrsave(string)                                              */
/*                                                                          */
/* Purpose  : Saves a copy of a string in global memory.                    */
/*                                                                          */
/* Returns  : Far pointer to the saved string.                              */
/*                                                                          */
/* Called by:                                                               */
/*   RegisterClass() to save the class, menu, and base class names.         */
/*   ChangeMenu() to save the menu string in MENUITEM.text                  */
/*   AddAtom() to save the atom name in ATOM.pszAtomName                    */
/*   RegisterClipboardFormat() to save the clipboard format name.           */
/*   AddFontResource() to save the font and path names.                     */
/*   GetOpenFileName() to save a list of filters.                           */
/*   WinCreate() to save the title.                                         */
/*   StdWindowWinProc() to save the title in response to WM_SETTEXT.        */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL lstrsave(s)
  LPSTR s;
{
  LPSTR t;

  if (s == NULL)
    return NULL;
  if ((t = (LPSTR) EMALLOC_FAR_NOQUIT(lstrlen(s) + 1)) != NULL)
    lstrcpy(t, s);
  return t;
}


PSTR FAR PASCAL emalloc_noquit(n)
  UINT n;
{
  LPSTR s;
  DWORD flOldState;

  /*
    Insure that the DONT-DIE state is on
  */
  flOldState = InternalSysParams.fProgramState;
  SET_PROGRAM_STATE(STATE_DONT_DIE);

  /*
    Allocate the memory
  */
  s = emalloc(n);

  /*
    Restore the old state and return the pointer.
  */
  InternalSysParams.fProgramState = flOldState;
  return s;
}

PSTR FAR PASCAL emalloc(n)
  UINT n;
{
  PSTR s;

  if (n == 0)
    return NULL;

  if (MemDebug)
  {
    MemDebugOut(NULL);
  }

  if ((s = (PSTR) calloc(n, 1)) == NULL)
  {
    /*
      Give the app a chance to do some housecleaning
    */
    if (InternalSysParams.WindowList && SendMessage(0xFFFF, WM_SYSTEMERROR, 8, 0L))
      return (s = (PSTR) calloc(n, 1));
    else if (TEST_PROGRAM_STATE(STATE_DONT_DIE))
      return NULL;
    else
      _WinDie();
  }

  if (MemDebug)
  {
    char buf[80];
    sprintf((char *) buf, "EMALLOC s [%08lx]  s-2 [%x]  s-4 [%u]  n [%d]\r\n",
             s, * (unsigned *) (s-2), * (unsigned *) (s-4), n);
    MemDebugOut(buf);
  }

  return s;
}


VOID FAR PASCAL MyFree(s)
  void *s;
{
  if (s)
  {
    if (MemDebug)
    {
      char buf[80];
      sprintf((char *) buf, "MYFREE s [%08lx]  s-2 [%x]  s-4 [%u]\r\n",
               s, * (unsigned *) ((char *) s-2), * (unsigned *) ((char *) s-4));
      MemDebugOut(buf);
    }
    free(s);
    if (MemDebug)
    {
      MemDebugOut(NULL);
    }
  }
}


/*
  Compiler-dependent far allocation/free routines
*/
#if !defined(DOS) || (defined(EXTENDED_DOS) && !defined(__TURBOC__))
#define farcalloc(n,e)    calloc((UINT) (n), e)
#define farrealloc(s,n)   realloc(s, (UINT) (n))
#define farfree(s)        free(s)

#elif (defined(MSC) && !defined(__ZTC__)) || (defined(__WATCOMC__) && !defined(__386__))
#if defined(USE_HALLOC)
#define farcalloc(n,e)    halloc(n, (long) (e))
#define farfree(s)        hfree(s)
/*
  MSC has no hrealloc() call!!!!!
*/
#elif (defined(M_I86SM) || defined(M_I86MM))
#define farcalloc(n,e)    _fcalloc((size_t) n, (size_t) (e))
#define farfree(s)        _ffree(s)
#define farrealloc(s,n)   _frealloc(s, (size_t) (n))
#else
#define farcalloc(n,e)    calloc((size_t) n, (size_t) (e))
#define farfree(s)        free(s)
#define farrealloc(s,n)   realloc(s, (size_t) (n))
#endif

#elif (defined(__WATCOMC__) && defined(__386__)) || defined(DOS386) || defined(WC386)
#define farcalloc(n,e)    calloc(n, e)
#define farfree(s)        free(s)
#define farrealloc(s,n)   realloc(s, (WORD) (n))
#endif


/****************************************************************************/
/*                                                                          */
/* Function : emalloc_far_noquit(n)                                         */
/*                                                                          */
/* Purpose  : Allocates n nytes of far memory. Like emalloc_far, except     */
/*            that MEWEL doesn't shut down if the memory request cannot     */
/*            be filled.                                                    */
/*                                                                          */
/* Returns  : A pointer to the memory, or NULL if no memory is available.   */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL emalloc_far_noquit(n)
  DWORD n;
{
  LPSTR s;
  DWORD flOldState;

  /*
    Insure that the DONT-DIE state is on
  */
  flOldState = InternalSysParams.fProgramState;
  SET_PROGRAM_STATE(STATE_DONT_DIE);

  /*
    Allocate the memory
  */
  s = emalloc_far(n);

  /*
    Restore the old state and return the pointer.
  */
  InternalSysParams.fProgramState = flOldState;
  return s;
}

LPSTR FAR PASCAL emalloc_far(n)
  DWORD n;
{
  LPSTR s;
#if defined(DOS286X) && defined(USE_ALLOCSEG)
  SEL   Selector;
#endif

  if (n == 0)
    return NULL;

#if 0
  /*
    Round up to nearest paragraph.
  */
  if (n & 0x0FL)
    n = (n + 15) & 0xFFFFFFF0L;
#endif

#if defined(DOS286X) && defined(USE_ALLOCSEG)
  if (DosAllocSeg((USHORT)n,&Selector,0)!=0 || (s=MAKEP(Selector,0))==NULL)
#else
  if ((s = (LPSTR) farcalloc(n, 1)) == NULL)
#endif
  {
    /*
      Give the app a chance to do some housecleaning
    */
    if (InternalSysParams.WindowList && SendMessage(0xFFFF,WM_SYSTEMERROR,8,0L))
      return (s = (LPSTR) farcalloc(n, 1));
    else if (TEST_PROGRAM_STATE(STATE_DONT_DIE))
      return NULL;
    else
      _WinDie();
  }

  if (MemDebug)
  {
    char buf[80];
    sprintf((char *) buf, "EMALLOC_FAR s [%08lx]  s-2 [%x]  s-4 [%u]  n [%d]\r\n",
             s, * (unsigned *) (s-2), * (unsigned *) (s-4), n);
    MemDebugOut(buf);
  }

  return s;
}


#ifdef NEVER_USED
/*
  5/6/93 (maa)
    It seems as if realloc_far() is never called from another MEWEL function.
  So, let's ifdef it out for now.
*/
LPSTR FAR PASCAL realloc_far(oldS, n)
  void FAR *oldS;
  DWORD n;
{
  LPSTR s;

#if !defined(__386__)

#if defined(DOS286X) && defined(USE_ALLOCSEG)
  SEL Selector = SELECTOROF(oldS);
  Selector = DosReallocSeg((USHORT) n, Selector);
  if (Selector==NULL || (s = (LPSTR) MAKEP(Selector,0)) == NULL)
#else
  if ((s = (LPSTR) farrealloc(oldS, n)) == NULL)
#endif
  {
    /*
      Give the app a chance to do some housecleaning
    */
    if (SendMessage(0xFFFF, WM_SYSTEMERROR, 8, 0L))
      return (s = (LPSTR) farrealloc(oldS, n));
    else if (TEST_PROGRAM_STATE(STATE_DONT_DIE))
      return NULL;
    else
      _WinDie();
  }
#endif

  if (MemDebug)
  {
    char buf[80];
    sprintf((char *) buf, "Realloc_Far %s  [%08lx]  s-2 [%x]  s-4 [%u]  n [%d]\r\n",
             s, * (unsigned *) (s-2), * (unsigned *) (s-4), n);
    MemDebugOut(buf);
  }

  return s;
}
#endif /* NEVER_USED */


VOID FAR PASCAL MyFree_far(s)
  void FAR *s;
{
  if (s)
  {
    if (MemDebug)
    {
      char buf[80];
      sprintf((char *) buf, "MYFREE s [%08lx]  s-2 [%x]  s-4 [%u]\r\n",
               s, * (unsigned *) ((char *) s-2), * (unsigned *) ((char *) s-4));
      MemDebugOut(buf);
#if defined(__TURBOC__) && !defined(EXTENDED_DOS)
      DumpHeapWalk(s);
#endif
    }

#if defined(DOS286X) && defined(USE_ALLOCSEG)
      DosFreeSeg(SELECTOROF(s));
#else
      farfree(s);

#if defined(MSC) && !defined(__HIGHC__) && !defined(__WATCOMC__) && !defined(EXTENDED_DOS) && !defined(UNIX)
      /*
        The Microsoft compiler does not release free memory back to DOS unless
        you call _heapmin(). So, call _heapmin() ocassionally (every 10th
        time we reach here) to free the memory back to DOS.
      */
      {
      static int iTimeToMinimize = 10;
      if (--iTimeToMinimize == 0)
      {
        _heapmin();
        iTimeToMinimize = 10;
      }
      }
#endif

#endif

    if (MemDebug)
    {
#if defined(__TURBOC__) && !defined(EXTENDED_DOS)
      DumpHeapWalk(s);
#endif
      MemDebugOut(NULL);
    }
  }
}


#if defined(__TURBOC__) && !defined(__DPMI32__)
/*
  For Genus GX and Turbo/Borland C
*/
DWORD FAR PASCAL MyFarCoreLeft(void)
{
  return farcoreleft();
}
#endif


static VOID FAR PASCAL MemDebugOut(msg)
  PSTR msg;
{
  int rc;

#ifndef NOTDEF
  if (msg)
  {
    int fd;
    if ((fd = open("MEMDEBUG.OUT", O_WRONLY|O_CREAT|O_BINARY|O_APPEND,
                    S_IREAD | S_IWRITE)) < 0)
      exit(1);
    write(fd, msg, strlen(msg));
    close(fd);
  }
#endif

  (void) msg;

#if defined(DOS) 

#if defined(MSC) && !defined(__ZTC__) && !defined(__HIGHC__)
  switch (rc = _heapchk())
  {
    case _HEAPEMPTY     :
    case _HEAPBADBEGIN  :
    case _HEAPBADNODE   :
    case _HEAPBADPTR    :
      VidClearScreen(0x07);
      VidSetPos(0, 0);
      printf("BAD HEAP rc = %d\n", rc);
      exit(1);
  }
#elif defined(__TURBOC__) && !defined(EXTENDED_DOS)
  farheapcheck();
  switch (rc = farheapcheckfree(1))
  {
    case _HEAPCORRUPT:
    case _BADVALUE:
      VidClearScreen(0x07);
      VidSetPos(0, 0);
      printf("BAD HEAP rc = %d\n", rc);
      exit(1);
  }
#elif defined(__TURBOC__) && defined(__DPMI16__)
  rc = farheapcheck();
  if (rc == _HEAPCORRUPT)
  {
    VidClearScreen(0x07);
    VidSetPos(0, 0);
    printf("BAD FAR HEAP rc = %d\n", rc);
    exit(1);
  }
  rc = heapcheck();
  if (rc == _HEAPCORRUPT)
  {
    VidClearScreen(0x07);
    VidSetPos(0, 0);
    printf("BAD NEAR HEAP rc = %d\n", rc);
    exit(1);
  }
#endif

#endif /* DOS */
}


#if defined(__TURBOC__) && !defined(EXTENDED_DOS)
void DumpHeapWalk(s)
  char far *s;
{
  struct farheapinfo hi;
  int fd;
  char buf[64];

  if ((fd = open("MEMDEBUG.OUT", O_WRONLY|O_CREAT|O_BINARY|O_APPEND,
                  S_IREAD | S_IWRITE)) < 0)
    exit(1);
  sprintf(buf, "After MyFree_far[%08lx]\r\n", s);
  write(fd, buf, strlen(buf));
  hi.ptr = NULL;
  while (farheapwalk(&hi) == _HEAPOK)
  {
    sprintf(buf, "ptr [%08lx]  size [%lu]  %s\r\n", hi.ptr, hi.size,
            hi.in_use ? "used" : "free");
    write(fd, buf, strlen(buf));
  }
  close(fd);
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : _WinDie()                                                     */
/*                                                                          */
/* Purpose  : Called when there is no memory left for the application.      */
/*            Terminates MEWEL, prints an error message, and terminates.    */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID FAR PASCAL _WinDie(void)
{
  LPSTR lpMessage = SysStrings[SYSSTR_OUTOFMEMORY];

#ifdef INTERNATIONAL_MEWEL
  char acBuffer[80];
  if (MewelCurrOpenResourceFile != -1)
    if (LoadString(MewelCurrOpenResourceFile,INTL_OUT_OF_MEMORY,acBuffer,80) > 0)
      lpMessage = acBuffer;
#endif

  WinTerminate();
  /*
    I know that it is rude to just die, but if there's no memory, there's
    no memory!
  */
  printf(lpMessage);
  ngetc();
  exit(1);
}



/****************************************************************************/
/*                                                                          */
/* Function : GmemRealloc(plpMem, piElements, iWidth, iMultiplier)          */
/*                                                                          */
/* Purpose  : General purpose routine used by a few of MEWEL's data structs */
/*            in order to reallocate memory. Used by DCCache, WindowList,   */
/*            etc.                                                          */
/*                                                                          */
/* Returns  : A far pointer to the new memory. Modifies the number of       */
/*            elements allocated.                                           */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL GmemRealloc(plpMem, piElements, iElWidth, iMultiplier)
  LPSTR  FAR *plpMem;
  UINT   *piElements;
  INT    iElWidth;
  INT    iMultiplier;
{
  LPSTR  lpNew;

  /*
    Allocate a new block of memory.
  */
  lpNew = emalloc_far((DWORD) (*piElements * iMultiplier * iElWidth));
  if (lpNew == NULL)
    return FALSE;

  /*
    Copy the data from the old block to the new one.
  */
  lmemcpy(lpNew, *plpMem, *piElements * iElWidth);

  /*
    Free the old block
  */
  MyFree_far(*plpMem);

  /*
    Put the new values back into the passed addresses
  */
  *piElements *= iMultiplier;
  *plpMem = lpNew;

  return TRUE;
}

