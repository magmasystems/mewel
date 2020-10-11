/*===========================================================================*/
/*                                                                           */
/* File    : WINEXEC.C                                                       */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*         : Modifications by Mark Hamilton 03-Aug-90                        */
/* (3)       Modified WINEXEC to look for UseEMS in MS Windows [Windows]     */
/*           variable. If UseEMS is 1, then WinExec() uses EMS (if there is  */
/*           Expanded Memory available) for swapping. If UseEMS is 0, then   */
/*           WinExec will attempt to set up a disk swap file. It first looks */
/*           for an entry "swapdisk" in [Windows], if this is missing, it    */
/*           looks for TEMP or TMP in the Dos environment. (See modified     */
/*           WINEXEC.C)                                                      */
/*                                                                           */
/*         : Modifications by Steve Rogers [SMR] 12-Sep-90                         */
/*           Modified WINEXEC to look for UseEMS variable in                 */
/*           [MEWEL.SWAPEXEC] application in MEWEL.INI.  If UseEMS is > 0,   */
/*           i.e., UseEMS = 1, WinExec() uses EMS (if available) for         */
/*           swapping.  If UseEMS is 0, WinExec() attempts to swap to a      */
/*           file.  If WinExec uses a swapfile, it no longer checks for      */
/*           "swapdisk" in [Windows].  (I never saw the code to do that      */
/*           check anyway. [SMR])                                            */
/*                                                                           */
/*           Modified WINEXEC to prevent WinExec() from executing the        */
/*           text of a batch file it WinExec() was passed a *.BAT file       */
/*           as an executable file.  This actually occurs in DESKTOP when    */
/*           the user selects a batch file from the directory.               */
/*                                                                           */
/*           Recommend version 2.3 of T. Wagner's spawn.asm (here            */
/*           renamed winspawn.asm) as it closes swap file and prevents       */
/*           lost clusters from developing when exec'd programs              */
/*           terminate abnormally and do not return to MEWEL.  Also,         */
/*           closing of swap file provides exec'd application with one       */
/*           more file handle.                                               */
/*                                                                           */
/*           WARNING -- did not modify MYSystem() function, only             */
/*                      MYSystemWithSwap().  MYSystem() will probably fail   */
/*                      to process *.BAT type commands properly.             */
/*                                                                           */
/*         : Modifications by Thomas Wagner 20-Aug-91                        */
/*           Changed to version 3.1 of spawn, deleted the EXEC functions     */
/*           from this module to ease future support for new versions.       */
/*           This module now includes the original EXEC.H, and               */
/*           requires the do_exec function to be compiled separately.        */
/*           Since EXEC 3.1 supports BAT-files and redirection directly,     */
/*           the logic for BAT and redirection detection was removed.        */
/*           Also, an empty command is no longer translated to COMMAND.COM,  */
/*           the logic in EXEC.C to get the correct command processor is     */
/*           a lot safer than the simple translation done here.              */
/*                                                                           */
/*         : Modifications by Wolfgang Lorenz 23-Apr-93                      */
/*           Added capability to read messages for the INTERNATIONAL_MEWEL   */
/*           scheme out of the resource file.                                */
/*           Changed 2nd parameter of WinExec from BOOL to WORD. Use a       */
/*           combination of:                                                 */
/*             EXEC_SWAP  - Swaps to disk (same as any number from 1 to 255) */
/*             EXEC_PAUSE - Displays "Press any key..." after executing      */
/*             EXEC_80X25 - Resets the video to 80x25 text mode (not all     */
/*             programs are running well in extended text or graphic modes). */
/*           Restoring the video mode will now recognize non-standard text   */
/*           character heights, EGA palette entries on VGA and the blink     */
/*           flag. If you've changed the EGA palette on EGA cards, you'll    */
/*           have to restore it manually after calling WinExec(), as EGA     */
/*           palete registers are write-only.                                */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#define INCLUDE_MOUSE
#include "wprivate.h"
#include "window.h"

#if defined(DOS) || defined(OS2)
#include <process.h>
#endif

#ifdef __TURBOC__
#define OS_MAJOR  (_version & 0xff)
#endif

#if defined(MSC) || defined(__WATCOMC__)

#define OS_MAJOR  _osmajor

#ifndef ZORTECH
#define farmalloc(x)    ((void far *)halloc(x,1))
#define farfree(x)      hfree((void huge *)x)
#endif

#define stpcpy(d,s)     (strcpy (d, s), d + strlen (s))

#endif

#ifdef WAGNER_FFAX
extern char winini[];
#endif

#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
#define CANSWAP 1
#include "exec.h"

/*
  International strings
*/
#define MSG(x) (apcExecStr[(x) - INTL_EXEC_FIRST])
static LPSTR apcExecStr[INTL_EXEC_COUNT] =
{
  "Enter EXIT to return to program\n",    /* INTL_ENTER_EXIT_TO_RETURN */
  "\nPress any key to continue...",       /* INTL_PRESS_ANY_KEY        */
  "Error writing swap file!",             /* INTL_SWAP_FILE_ERR        */
  "No room for swapping!",                /* INTL_NO_ROOM_FOR_SWAPPING */
  "Error allocating environment space!",  /* INTL_CANT_ALLOC_ENV_SPACE */
  "DOS error calling EXEC!",              /* INTL_DOS_EXEC_ERROR       */
  "Internal error calling EXEC!"          /* INTL_INTERNAL_EXEC_ERROR  */
};

/*
  If an error has occured, convert it to string and display it
*/
static void DisplayError(int errcode)
{
  UINT wMsg;

  if (errcode == 0x502)
    wMsg = INTL_SWAP_FILE_ERR;
  else if (errcode == 0x101)
    wMsg = INTL_NO_ROOM_FOR_SWAPPING;
  else if (errcode == 0x400)
    wMsg = INTL_CANT_ALLOC_ENV_SPACE;
  else if ((errcode & 0xff00) == 0x0300)
    wMsg = INTL_DOS_EXEC_ERROR;
  else if (errcode & 0xff00)
    wMsg = INTL_INTERNAL_EXEC_ERROR;
  else return;

  MessageBox(NULL, MSG(wMsg), NULL, MB_ICONEXCLAMATION | MB_OK);
}

/*
  Read the international strings out of the resource file
*/
#ifdef INTERNATIONAL_MEWEL
static void GetIntlExecStrings(void)
{
  extern int MewelCurrOpenResourceFile;
  static BOOL fStringsRead = FALSE;

  if (MewelCurrOpenResourceFile != -1 && !fStringsRead)
  {
    int i;

    for (i = 0;  i < INTL_EXEC_COUNT;  i++)
    {
      char acBuffer[80];
      int iLength;

      iLength = LoadString(MewelCurrOpenResourceFile,
                           INTL_ENTER_EXIT_TO_RETURN + i, acBuffer, 80);
      if (iLength > 0) apcExecStr[i] = lstrsave(acBuffer);
    }

    fStringsRead = TRUE;
  }
}
#else
#define GetIntlExecStrings()
#endif /* INTERNATIONAL_MEWEL */


static BOOL bLoadCommand;
static BOOL bHasSwapped;
#ifdef MODSOFT
static char *pszCmdMsg;
#endif

static int spcheck (int cmdbat, int swapping, char *execfn, char *progpars)
{
  (void) progpars;  (void) execfn;  (void) swapping;  (void) cmdbat;

   bHasSwapped = TRUE;
   WinTerminate();

#ifdef MODSOFT
   if (pszCmdMsg)
      puts(pszCmdMsg);
   else
#endif
     if (bLoadCommand)
       printf (MSG(INTL_ENTER_EXIT_TO_RETURN));

   return 0;
}
#endif /* defined(DOS) && !defined(MEWEL_32BITS) */


extern int PASCAL WinGetShellCmd(char *cmd);
extern int PASCAL MYsystem(char *cmd);
extern int PASCAL MYsystemWithSwap(char *cmd);

#ifdef MODSOFT

static char szProfileFile[30] = "MEWEL.INI";
static char szProfileApp[30] = "MEWEL.SWAP_EXEC";
static char szProfileKey[30] = "UseEMS";

int PASCAL WinExecVars(char *file, char *app, char *key)
{

  if (((file) && (strlen(file) >= sizeof(szProfileFile))) ||
      ((app) && (strlen(app) >= sizeof(szProfileApp))) ||
      ((key) && (strlen(key) >= sizeof(szProfileKey))))
    return (FALSE);

  if (file)
    strcpy(szProfileFile,file);
  if (app)
    strcpy(szProfileApp,app);
  if (key)
    strcpy(szProfileKey,key);

  return (TRUE);
}

#endif /* MODSOFT */


#ifdef MODSOFT

int PASCAL WinExecOpt(char *szCmd, char *msg, BOOL bSwapToDisk, BOOL pause)
{
  char cmd[256];
  int  errcode;

  WINDOW *wOldWindowList = InternalSysParams.WindowList;
  WINDOW *wOldDesktop    = InternalSysParams.wDesktop;
  HWND   hOldDesktop     = _HwndDesktop;

  GetIntlExecStrings();

  bLoadCommand = !szCmd || !*szCmd;
  if (bLoadCommand)
  {
#ifdef CANSWAP
    if (bSwapToDisk)
       cmd [0] = '\0';
    else
#endif
    if (!WinGetShellCmd(cmd))
      return FALSE;
  }
  else
  {
    strcpy(cmd, szCmd);
  }

  /*
    Close down the window system
  */
#ifdef CANSWAP
  if (bHasSwapped = !bSwapToDisk)
  {
#endif
    WinTerminate();

    if (msg) {
      puts(msg);
    }
#ifdef CANSWAP
  }
else
  pszCmdMsg = msg;
#endif

  /*
    Do the exec
  */
#ifdef CANSWAP
  if (bSwapToDisk)
    errcode = MYsystemWithSwap(cmd);
  else
#endif
    errcode = MYsystem(cmd);

#ifdef CANSWAP
  if (bHasSwapped)
   {
#endif
  if (pause) {
    printf(MSG(INTL_PRESS_ANY_KEY));
    ngetc();
  }

  /*
    Reinitialize the system and refresh the screen
  */
  WinInit();
  _HwndDesktop = hOldDesktop;
  HwndArray[0] = InternalSysParams.wDesktop = wOldDesktop;
  InternalSysParams.WindowList   = wOldWindowList;
  WinDrawAllWindows();
#ifdef CANSWAP
   }
#endif

  /*
    Check for errors
  */
#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  DisplayError(errcode);
#endif

  return errcode;
}

#endif /* MODSOFT */



int FAR PASCAL WinExec(char *szCmd, UINT wFlags)
{
  char cmd[256];
  int  errcode;
  UINT wVideoMode;
  UINT wCharHeight;
  BOOL fTextMode;
  BOOL fBlinking;

#if defined(EXTENDED_DOS)       // 11/4/94 -- blg
  BOOL fMyBlinking;             // 11/4/94 -- blg
#endif                          // 11/4/94 -- blg

#if defined(DOS) && !defined(EXTENDED_DOS)
  union REGS r;
#endif

#if defined(SAVE_THE_PALETTE)
  struct SREGS seg;
  BYTE abPalette[17];
#endif

  WINDOW *wOldWindowList = InternalSysParams.WindowList;
  WINDOW *wOldDesktop    = InternalSysParams.wDesktop;
  HWND   hOldDesktop     = _HwndDesktop;

  if (!szCmd || !*szCmd)
  {
#ifdef CANSWAP
    bLoadCommand = TRUE;
    if (wFlags & EXEC_SWAP)
       cmd[0] = '\0';
    else
#endif
    if (!WinGetShellCmd(cmd))
      return FALSE;
  }
  else
  {
#ifdef CANSWAP
    bLoadCommand = FALSE;
#endif
    strcpy(cmd, szCmd);
  }

#if defined(DOS) && !defined(EXTENDED_DOS)
  /*
    Get the current video settings
  */
  wVideoMode  = * (BYTE far *) 0x00000449L;
  fTextMode   = !VID_IN_GRAPHICS_MODE();
  wCharHeight = * (WORD far *) 0x00000485L;
#if defined(SAVE_THE_PALETTE)
  if (IsVGA())
  {
    r.x.ax = 0x1009;
    seg.es = (WORD) ((DWORD) (VOID FAR *) &abPalette >> 16);
    r.x.dx = (WORD) &abPalette;
    int86x(0x10, &r, &r, &seg);
  }
#endif
  fBlinking = !TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS);
#endif /* DOS */

#if defined(EXTENDED_DOS)                                   // 11/4/94 -- blg
    fMyBlinking = !TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS);// 11/4/94 -- blg
#endif                                                      // 11/4/94 -- blg

  /*
    Let MEWEL know we are spawning
  */
  SET_PROGRAM_STATE(STATE_SPAWNING);

  /*
    Close down the window system
  */
#ifdef CANSWAP
#ifdef MODSOFT
  pszCmdMsg = NULL;
#endif

  /*
    If we are not swapping to disk/EMS, then we have to shut down MEWEL.
    Otherwise, defer the shutting down until we determine whether we
    were able to spawn the child.
  */
  if ((bHasSwapped = !(wFlags & EXEC_SWAP)) == TRUE)
#endif /* CANSWAP */

  WinTerminate();

#if defined(DOS) && !defined(EXTENDED_DOS)
  /*
    Switch to 80 by 25 text mode, if desired. Monochrome adapters will
    automatically replace 0x03 with 0x07
  */
  if (wFlags & EXEC_80X25)
  {
    MOUSE_HideCursor();
#if defined(__DPMI16__) || defined(__DPMI32__)
    _AX = 3;
    geninterrupt(0x10);
#else
    r.x.ax = 0x0003;
    int86(0x10, &r, &r);
#endif
  }
#endif /* DOS */

  /*
    Do the exec
  */
#if defined(MSC) && defined(DOS) && defined(MEWEL_GUI) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  _cexit();
#endif

#ifdef CANSWAP
  if (wFlags & EXEC_SWAP)
    errcode = MYsystemWithSwap(cmd);
  else
#endif
    errcode = MYsystem(cmd);

  /*
    Reinitialize the system and refresh the screen
  */
#ifdef CANSWAP
  if (bHasSwapped)
  {
#endif

    /*
      Display the pause message, if desired
    */
    if (wFlags & EXEC_PAUSE)
    {
#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
      printf(MSG(INTL_PRESS_ANY_KEY));
      ngetc();
#endif
    }

#if defined(DOS) && !defined(EXTENDED_DOS)
    /*
      Restore previous video settings
    */
    r.x.ax = wVideoMode;
    int86(0x10, &r, &r);
    if (fTextMode && *(WORD far *)0x00000485L != wCharHeight)
    {
      r.x.ax = 0x1111;
      r.h.bl = 0;
      switch (wCharHeight) {
      case 16:
        r.x.ax += 2;
      case 8:
        r.x.ax++;
      case 14:
        int86(0x10, &r, &r);
      }
    }
#if defined(SAVE_THE_PALETTE)
    if (IsVGA())
    {
      r.x.ax = 0x1002;
      seg.es = (WORD) ((DWORD) (VOID FAR *) &abPalette >> 16);
      r.x.dx = (WORD) &abPalette;
      int86x(0x10, &r, &r, &seg);
    }
#endif
    VidSetBlinking(fBlinking);
#endif /* DOS */

    WinInit();

#if defined(EXTENDED_DOS)                  // 11/4/94 -- blg
    VidSetBlinking(fMyBlinking);           // 11/4/94 -- blg
#endif                                     // 11/4/94 -- blg

    /*
      Restore the crucial system variables
    */
    _HwndDesktop = hOldDesktop;
    HwndArray[0] = InternalSysParams.wDesktop = wOldDesktop;
    InternalSysParams.WindowList   = wOldWindowList;
    WinDrawAllWindows();
#ifdef CANSWAP
   }
#endif

  /*
    Let MEWEL know we are done spawning
  */
  CLR_PROGRAM_STATE(STATE_SPAWNING);

  /*
    Check for errors
  */
#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  DisplayError(errcode);
#endif

  return errcode;
}


/*
  WinGetShellCmd()
    Returns the name of the system-dependent command processor.
*/
int PASCAL WinGetShellCmd(char *cmd)
{
#if defined(OS2)
  strcpy(cmd, "CMD");
#elif defined(DOS)
  strcpy(cmd, getenv("COMSPEC"));
#elif defined(UNIX)
  strcpy(cmd, getenv("SHELL"));
#endif
  return TRUE;
}


/*
  MYsystem - my front-end for the system() call
*/
int PASCAL MYsystem(char *cmd)
{
  int  rc;
  char *args[33];
  char save_cmd[256];
  int  nargs;
  int  UseShell = 1;

#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  GetIntlExecStrings();
#endif

#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  if (bLoadCommand) printf (MSG(INTL_ENTER_EXIT_TO_RETURN));
#endif

  /*
    If we have redirection, use system()
  */
  if (strchr(cmd, '>'))
    goto do_sys;

  strcpy(save_cmd, cmd);

  /*
    Create a vector of arguments
  */
  if ((args[0] = strtok(cmd, " \t")) == NULL)
    return -1;
  for (nargs = 1;  (args[nargs] = strtok(NULL, " \t")) != NULL;  nargs++) ;

  /*
    Try doing a spawn first
  */
  if ((rc = spawnvp(0, args[0], &args[0])) == -1)
  {
    /*
      The spawn didn't find the file ...
      If the UseShell flag is set, then try system()
    */
    if (UseShell)
    { 
      strcpy(cmd, save_cmd);
do_sys:
      rc = system(cmd);
    }
    else
#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
      rc = _doserrno;
#else
      rc = -1;
#endif
  }

  return rc;
}


#ifdef CANSWAP

int PASCAL MYsystemWithSwap(char *cmd)
{
  char *args;
  char save_cmd[256];
  int  rc;
  int options;
  int swapping;

#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(EXTENDED_DOS)
  GetIntlExecStrings();
#endif

  options = USE_ALL;
  spawn_check = spcheck;

/*
   Th. Wagner changed default to use XMS or EMS to swap. Only disable
   EMS and XMS swapping on an explicit disable in the profile 
   (UseEMS=0 or UseXMS=0).
   Also changed the logic to not write back the profile string if it's
   already present.
*/
   
/* ROGERS [SMR] changed profile key from [windows] to [MEWEL.EXEC] */
/*
 * See if the user wants to use EMS for swapping the application. To do this
 * get the integer value of UseEMS from the DeskTop section of the Profile
 * (MEWEL.INI). If EMS is to be used (UseEMS=1), then set the value to this.
 * The default (-1) is used if the UseEMS string is missing or is set to an
 * incorrect value.
 */ 
#if defined(WAGNER_FFAX)
  swapping = GetPrivateProfileInt("MEWEL.SWAPEXEC","UseEMS",-1,winini);
  if (swapping == -1)
  {
    char temp[10];
    WritePrivateProfileString("MEWEL.SWAPEXEC","UseEMS", itoa(1,temp,10), winini);
  }
#else
  swapping = GetProfileInt("MEWEL.SWAPEXEC","UseEMS",-1);
  if (swapping == -1)
  {
    char temp[10];
    WriteProfileString("MEWEL.SWAPEXEC","UseEMS", itoa(1,temp,10));
  }
#endif /* WAGNER_FFAX */
  if (!swapping)
    options &= ~USE_EMS;


#ifdef WAGNER_FFAX
  swapping = GetPrivateProfileInt("MEWEL.SWAPEXEC","UseXMS",-1, winini);
  if (swapping == -1)
  {
    char temp[10];
    WritePrivateProfileString("MEWEL.SWAPEXEC","UseXMS", itoa(1,temp,10), winini);
  }
#else
  swapping = GetProfileInt("MEWEL.SWAPEXEC","UseXMS",-1);
  if (swapping == -1)
  {
    char temp[10];
    WriteProfileString("MEWEL.SWAPEXEC","UseXMS", itoa(1,temp,10));
  }
#endif /* WAGNER_FFAX */
  if (!swapping)
    options &= ~USE_XMS;

  /*
    Make sure that the profile file is closed
  */
  CloseProfile();

  /*
     Get a pointer to the arguments
  */
  strcpy(save_cmd, cmd);
  if ((args = (char *) span_chars((PSTR) cmd)) == NULL)
    args = "";
  else
    *args++ = '\0';

  rc = do_exec(cmd, args, options, 0xFFFF, NULL);

  /*
    If the swap & exec failed with 'file not found', try
    invoking the command interpreter on the command.
  */
  /* ROGERS [SMR] -- Changed rc mask from 0x300 to 0x200 to retry on
   *                 failure to find file (as in internal or trimmed-
   *                 extension BAT commands), or on DOS errors (as
   *                 possibly in DOS internal commands).  Bitwise compare
   *                 with 0x300 would cause a retry on a swap file write
   *                 error.
   * Th. Wagner: Changed this to compare against RC_NOFILE without masking.
   *             Only retry if the program wasn't found, not on any other
   *             error.
  */
  if (rc == RC_NOFILE)
  {
    byteinsert((PSTR) save_cmd, 3);     /* Make room for the "/c "      */
    memcpy(save_cmd, "/c ", 3);         /* Prepend "/c " to the command */

    /*
      The first arg, "", will cause COMMAND.COM to be invoked. The
      string in save_cmd will have something like "/c baz.exe".
    */
    rc = do_exec("", save_cmd, options, 0xFFFF, NULL);
  }

  return rc;
}

#endif /* CANSWAP */
