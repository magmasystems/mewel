/*===========================================================================*/
/*                                                                           */
/* File    : WBORRTM.C                                                       */
/*                                                                           */
/* Purpose : Borland RTM patching functions                                  */
/*                                                                           */
/* History : Created by Peter Sawatzki. Converted to C by Magma.             */
/*                                                                           */
/*===========================================================================*/
#include "wprivate.h"

UINT WINAPI GlobalPageLock(HGLOBAL);
UINT WINAPI GlobalPageUnlock(HGLOBAL);
UINT WINAPI AllocCSToDSAlias(UINT);


#pragma pack(1)

typedef struct tagJumper
{
  BYTE    jmpFar;
  FARPROC lpfnNewProc;
} JUMPER, FAR *LPJUMPER;

typedef struct tagSwitch
{
  FARPROC lpfnPatchAddr;
  JUMPER  tJumper;
} SWITCH, FAR *LPSWITCH;

typedef struct tagPtrRec
{
  WORD wOffset;
  WORD wSeg;
} PTRREC;


VOID FAR PASCAL 
MakeSwitch(LPSWITCH lpSwitch, FARPROC lpfnOldProc, FARPROC lpfnNewProc)
{
  /*
    Build a switch record
  */
  lpSwitch->lpfnPatchAddr       = lpfnOldProc;
  lpSwitch->tJumper.jmpFar      = 0xEA;
  lpSwitch->tJumper.lpfnNewProc = lpfnNewProc;
}

VOID FAR PASCAL Switch(LPSWITCH lpSwitch)
{
  JUMPER TempCode;

  /*
    Get a DS alias to the patch address
  */
  LPJUMPER lpProcAlias = MK_FP(AllocCSToDSAlias(FP_SEG(lpSwitch->lpfnPatchAddr)),
                                                FP_OFF(lpSwitch->lpfnPatchAddr));

  GlobalPageLock(FP_SEG(lpProcAlias));

  /*
    Save the current 5 bytes code at the patch addr
  */
  TempCode = *lpProcAlias;

  /*
    Patch the 5 code bytes of lpfnPatchAddr so that it is either
      JMP FAR lpfnNewProc
    or the original code
  */
  *lpProcAlias = lpSwitch->tJumper;

  /*
    Get rid of the alias DS
  */
  GlobalPageUnlock(FP_SEG(lpProcAlias));
  FreeSelector(FP_SEG(lpProcAlias));

  /*
    store the 5 bytes code from above
  */
  lpSwitch->tJumper = TempCode;
}


static SWITCH swMessageBox;
static SWITCH swGetDesktopWindow;

VOID FAR PASCAL InitRTMSwitches(void)
{
  /*
    This creates a 'switch' to switch between RTM MessageBox and MEWEL
  */
  MakeSwitch(&swMessageBox, 
             GetProcAddress(GetModuleHandle("USER"), "MESSAGEBOX"),
             GetProcAddress(GetModuleHandle("MEWEL"), "MESSAGEBOX"));
  /*
    Now the DLL will call MEWELs MessageBox
  */
  Switch(&swMessageBox); 

  /*
    Hook the RTM's version of GetDesktopWindow.
  */
  MakeSwitch(&swGetDesktopWindow, 
             GetProcAddress(GetModuleHandle("USER"), "GETDESKTOPWINDOW"),
             GetProcAddress(GetModuleHandle("MEWEL"), "GETDESKTOPWINDOW"));
  Switch(&swGetDesktopWindow); 
}

/*
  This function gets called as part of the WinTerminate processing
*/
VOID FAR PASCAL TerminateRTMSwitches(void)
{
  Switch(&swMessageBox); 
  Switch(&swGetDesktopWindow); 
}

/*
In the DEF file...
PRELOAD FIXED PERMANENT
*/


