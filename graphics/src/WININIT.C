/*===========================================================================*/
/*                                                                           */
/* File    : WININIT.C                                                       */
/*                                                                           */
/* Purpose : WinInit()                                                       */
/*                                                                           */
/* History :                                                                 */
/*  1/16/90 (maa) Broke WinInit() into two parts, the one-time initialization*/
/*     called _WinInit(), and the I/O initialization (WinInit) which can     */
/*     be called from an app in order to restart the I/O portion.            */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_DOSFILEMGR

#include "wprivate.h"
#include "window.h"
#include "wobject.h"

/*
  For directory saving and restoring...
*/
static char     szDirStartup[MAXPATH];
static unsigned usDriveStartup;

static BOOL   bDid_WinInit = 0;

extern VOID PASCAL ReadINIFontInfo(HDC);


VOID FAR PASCAL _WinInit(void)
{
#if defined(__BORLANDC__) && defined(__DPMI16__) && defined(__DLL__)
  InitRTMSwitches();
#endif

  /*
    Initialize the message queue and set the event handlng procedure
  */
  InitEventQueue();
#if !defined(OS2) || defined(NOTHREADS)
  EventSetHandler(GetEvent);
#endif

  /*
    Register all of the system-defined window classes
  */
  _RegisterPredefinedClasses();

  /*
    Save the current drive and directory so we can restore it in the end...
  */
  _WinSaveDirectory();

  /*
    Set up exit post-processors
  */
#if defined(__TSC__) || defined(__WATCOMC__)
  atexit((void (*)()) WinTerminate);
  atexit((void (*)()) _WinRestoreDirectory);
#elif defined(SUNOS)
  on_exit(WinTerminate, 0);
  on_exit(_WinRestoreDirectory, 0);
#else
  atexit(WinTerminate);
  atexit(_WinRestoreDirectory);
#endif
}


INT FAR PASCAL WinInit(void)
{
  extern char BrkFound;

  CLR_PROGRAM_STATE(STATE_EXITING);

  /*
    Do low-level initialization
  */
  if (!bDid_WinInit)
    _WinInit();

  /*
    Initialize the screen. VidGetMode() does most of the work.
    We clear the screen and hide the hardware cursor.
  */
  if (SysGDIInfo.StartingVideoMode < 0)
    SysGDIInfo.StartingVideoMode = VidGetMode();
  SysGDIInfo.StartingVideoLength = VideoInfo.length;
  VidClearScreen(WinQuerySysColor(NULLHWND, SYSCLR_BACKGROUND));
#if !defined(USE_NATIVE_GUI)
  VidHideCursor();
#endif

  /*
    For the GUI version, initialize the low-level graphics engine
  */
#if defined(MEWEL_GUI)
  if (WinOpenGraphics(MEWELInitialGraphicsMode) == FALSE)
    exit(1);
#endif

  /*
    Initialize the input devices.
  */
#ifdef OS2
  if (!bDid_WinInit)
#endif
  KBDInit();
  MouseInitialize();

  /*
    Init the memory handler
  */
#if !defined(__DPMI16__) && !defined(__DPMI32__)
  MemoryInit();
#endif

  /*
    Set up the signal and critical error handlers
  */
  InternalSysParams.fOldBrkFlag = getbrk();
  rstbrk();
  int23ini((LPSTR) &BrkFound);
#ifdef DOS
  Int24Install();
#endif

  /*
    Initialize the GDI objects
  */
  _ObjectsInit();
  InitSysBrushes();

  /*
    Create the desktop, the root window in the system.
  */
  if (!InternalSysParams.wDesktop)
    WinCreateDesktopWindow();

  /*
    Start up any threads
  */
#ifdef OS2
  if (!bDid_WinInit)
    InitEventThreads();
  else
    ResumeEventThreads();
#endif

#if defined(XWINDOWS) && !defined(MOTIF)
  GUIInitialize();
#endif

  /*
    For the GUI version, read the colors and font info from MEWEL.INI
  */
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  WinReadINI();
  ReadINIFontInfo(0);
#endif

  bDid_WinInit++;

  /*
    Do the MOTIF setup
  */
#if defined(MOTIF)
  if (!GetProfileInt("motif", "DelayMotifInit", 0))
    XMEWELInitMotif();
#endif

  /*
    Set the startup cursor
  */
  SetCursor(InternalSysParams.hCursor = LoadCursor(NULL, IDC_ARROW));

  return TRUE;
}



VOID FAR PASCAL _WinSaveDirectory()
{
  /*
    Save the current disk and path
  */
  getcwd(szDirStartup, MAXPATH);
#if defined(DOS)
#if defined(__TURBOC__) || defined(__GNUC__)
  usDriveStartup = getdisk();
#else
  _dos_getdrive(&usDriveStartup);
#endif

#elif defined(OS2)
  {
  ULONG    ulDriveMap;
  DosQCurDisk((PUSHORT) &usDriveStartup, (PULONG) &ulDriveMap);
  }
#endif
}


#if defined(__WATCOMC__)
VOID       _WinRestoreDirectory(void)
#else
VOID CDECL _WinRestoreDirectory(void)
#endif
{
#if defined(DOS) && (defined(MSC) || defined(__WATCOMC__))
  unsigned drives;
#endif

  if (!bRestoreDirectory)
    return;

  /*
    Restore the current drive and path
  */
#if defined(DOS)
#if defined(__TURBOC__) || defined(__GNUC__)
  setdisk(usDriveStartup);
#else
  _dos_setdrive(usDriveStartup, &drives);
#endif

#elif defined(OS2)
  DosSelectDisk(usDriveStartup);
#endif

  chdir(szDirStartup);
}


UINT FAR PASCAL GetSystemDirectory(lpBuffer, nSize)
  LPSTR lpBuffer;
  INT   nSize;
{
  PSTR pSlash;

  InternalGetModuleFileName(lpBuffer, nSize);
  if ((pSlash = strrchr(lpBuffer, CH_SLASH)) != NULL)
  {
    int iSlash = pSlash - lpBuffer;
    /*
      Get rid of the program name and the slash before it. If the
      program was invoked from the root, then leave the slash.
    */
    if (lpBuffer[iSlash-1] == ':')
    {
      lpBuffer[iSlash]   = '\\';
      lpBuffer[iSlash+1] = '\0';
    }
    else
      lpBuffer[iSlash] = '\0';
  }
  return (UINT) lstrlen(lpBuffer);
}


UINT WINAPI GetWindowsDirectory(buf, n)
  LPSTR buf;
  int   n;
{
  return GetSystemDirectory(buf, n);
}



#if defined(__DLL__) && defined(__DPMI16__)
int FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSeg, WORD wHeapSize,
                       LPSTR lpszCmdLine)
{
  (void) hInstance;
  (void) wDataSeg;
  (void) lpszCmdLine;

  if (wHeapSize != 0)
    UnlockData(0);

  /*
    Initialize MEWEL
  */
  WinInit();
  WinUseSysColors(NULLHWND, TRUE);
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY);


  return 1;
}
#endif

int FAR PASCAL WEP(int nParam)
{
  (void) nParam;
  return 1;
}


#if !defined(__DLL__)
BOOL FAR PASCAL LocalInit(uSegment, uStartAddr, uEndAddr)
  UINT uSegment;
  UINT uStartAddr;
  UINT uEndAddr;
{
  (void) uSegment;
  (void) uStartAddr;
  (void) uEndAddr;

  return 0;
}
#endif


/*
  XMEWELInitMotif(void)

  This is the tail end of the MEWEL initialization process. It occurs
  after all the stuff in WinInit() is done. In MFC, it must called in
  main() so that argv and argc are valid.
*/
#if defined(MOTIF)
VOID FAR PASCAL XMEWELInitMotif(void)
{
  extern VOID FAR PASCAL MEWELInitSystemFont(HDC);

  if (WinOpenGraphics(-1) == FALSE)
    exit(1);
  GUIInitialize();
}
#endif

