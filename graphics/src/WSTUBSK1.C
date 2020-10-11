/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSK1.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           These stubs are for KERNEL.                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"

/*
  AllocCStoDSAlias is defined in WSELECTR.C, but this file isn't
  included for BC++ DPMI. However, AllocCStoDSAlias is not defined
  by Borland's DPMI16.DLL, so we need to do it here.
*/
#if defined(__DPMI16__)
UINT WINAPI AllocCStoDSAlias(UINT sel)  /* KERNEL.170 */
{
  asm mov bx, sel
  asm mov ax, 0AH
  asm int 31H
  asm jnc bye
  asm xor ax, ax
bye:
  return _AX;
}
#endif


VOID WINAPI DebugBreak(void)  /* KERNEL.203 */
{
#if defined(DOS)
#if defined(__TURBOC__)
  geninterrupt(3);
#endif
#endif
}

void WINAPI DirectedYield(HTASK hTask) /* KERNEL.150 */
{
  /*
    Puts the current task to sleep and wakes up the passed task
  */
  (void) hTask;
}

BOOL WINAPI FreeModule(HINSTANCE h) /* KERNEL.46 */
{
  (void) h;
  return FALSE;
}

HGLOBAL WINAPI GetCodeHandle(FARPROC lpfn)  /* KERNEL.93 */
{
  /*
    Returns the code segment which contains the function lpfn
  */
  return (HGLOBAL) FP_SEG(lpfn);
}

VOID WINAPI GetCodeInfo(FARPROC lpProc, SEGINFO FAR* lpSegInfo) /* KERNEL.104 */
{
  (void) lpProc;
  (void) lpSegInfo;
}

UINT WINAPI GetCurrentPDB(void)  /* KERNEL.37 */
{
  return 0;
}

HANDLE WINAPI GetExePtr(HANDLE h)  /* KERNEL.133 */
{
#if defined(__DPMI16__)
  extern HANDLE _hInstance;
  (void) h;
  return _hInstance;
#else
 (void) h;
 return NULL;
#endif
}

int WINAPI GetInstanceData(HINSTANCE h, BYTE* npData, int cbData)/* KERNEL.54 */
{
  (void) h;
  lmemset((LPSTR) npData, 0, cbData);
  return 0;
}

BOOL WINAPI GetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags) /* KERNEL.355 */
{
  (void) lpwdi;
  (void) flags;
  return FALSE;
}

#ifdef STRICT
void FAR* WINAPI GlobalWire(HGLOBAL h)  /* KERNEL.111 */
#else
char FAR* WINAPI GlobalWire(HGLOBAL h)
#endif
{
  /*
    This function should not be used in Windows 3.1
  */
  (void) h;
  return NULL;
}

BOOL WINAPI GlobalUnWire(HGLOBAL h)  /* KERNEL.112 */
{
  /*
    This function should not be used in Windows 3.1
  */
  (void) h;
  return FALSE;
}


BOOL WINAPI IsBadCodePtr(FARPROC lpfn)  /* KERNEL.336 */
{
  (void) lpfn;
  return (BOOL) (lpfn != NULL);
}

BOOL WINAPI IsBadHugeReadPtr(CONST VOID HUGE *lp, DWORD cb)  /* KERNEL.346 */
{
  (void) lp;
  (void) cb;
  return (BOOL) (lp != NULL);
}

BOOL WINAPI IsBadHugeWritePtr(CONST VOID HUGE *lp, DWORD cb)  /* KERNEL.347 */
{
  (void) lp;
  (void) cb;
  return (BOOL) (lp != NULL);
}

BOOL WINAPI IsBadReadPtr(CONST VOID FAR *lp, UINT cb)  /* KERNEL.334 */
{
  (void) lp;
  (void) cb;
  return (BOOL) (lp != NULL);
}

BOOL WINAPI IsBadStringPtr(CONST VOID FAR *lp, UINT cb)  /* KERNEL.337 */
{
  (void) lp;
  (void) cb;
  return (BOOL) (lp != NULL);
}

BOOL WINAPI IsBadWritePtr(CONST VOID FAR *lp, UINT cb)  /* KERNEL.335 */
{
  (void) lp;
  (void) cb;
  return (BOOL) (lp != NULL);
}


BOOL WINAPI IsDBCSLeadByte(BYTE ch)  /* KERNEL.207 */
{
  (void) ch;
  return FALSE;
}

BOOL WINAPI IsTask(HTASK hTask)  /* KERNEL.320 */
{
  (void) hTask;
  return TRUE;
}

void WINAPI LimitEmsPages(DWORD cAppKB)  /* KERNEL.156 */
{
  /*
    Obsolete in Windows 3.1
  */
  (void) cAppKB;
}

HINSTANCE WINAPI LoadModule(LPCSTR lpModuleName, LPVOID lpParam) /* KERNEL.45 */
{
  (void) lpModuleName;
  (void) lpParam;
  return HINSTANCE_ERROR;
}

VOID WINAPI LogError(UINT uErr, VOID FAR *lpvInfo)  /* KERNEL.324 */
{
  (void) uErr;
  (void) lpvInfo;
}

VOID WINAPI LogParamError(UINT uErr, FARPROC lpfn, VOID FAR *lpvParam)
/* KERNEL.325 */
{
  (void) uErr;
  (void) lpfn;
  (void) lpvParam;
}


UINT WINAPI SetHandleCount(UINT nHandles)  /* KERNEL.199 */
{
  /*
    Changes the number of file handles available to a task
  */
  (void) nHandles;
  return nHandles;
}

LONG WINAPI SetSwapAreaSize(UINT cCodeParagraphs) /* KERNEL.106 */
{
  (void) cCodeParagraphs;
  return 0L;
}

BOOL WINAPI SetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi) /* KERNEL.356 */
{
  (void) lpwdi;
  return FALSE;
}

void WINAPI SwapRecording(UINT fuFlags) /* KERNEL.204 */
{
  /*
    Obsolete in Windows 3.1
  */
  (void) fuFlags;
}

VOID WINAPI SwitchStackBack(void)  /* KERNEL.109 */
{
  /*
    Undoes the effect of SwitchStackTo
  */
}

VOID WINAPI SwitchStackTo(UINT wStackSeg, UINT wStackPtr, UINT wStackTop)
/* KERNEL.108 */
{
  (void) wStackSeg;
  (void) wStackPtr;
  (void) wStackTop;
}

void WINAPI ValidateCodeSegments(void) /* KERNEL.100 */
{
  /*
    Only for real mode debugging Windows earlier than 3.1
  */
}

void WINAPI ValidateFreeSpaces(void) /* KERNEL.200 */
{
}

void WINAPI Yield(void)  /* KERNEL.29 */
{
  /*
    Stops the current task and starts any waiting task
  */
}

