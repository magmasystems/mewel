/*===========================================================================*/
/*                                                                           */
/* File    :  WINTASK.C                                                      */
/*                                                                           */
/* Purpose :  Implements GetWindowTask()                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

HTASK FAR PASCAL GetWindowTask(hWnd)
  HWND hWnd;
{
#if defined(PLTNT)
  USHORT psp;
#endif

  (void) hWnd;

#if defined(__DPMI16__) || defined(__DPMI32__)
  return GetCurrentTask();
#elif defined(PLTNT)
  _dos_getpsp(&psp);
  return psp;
#elif defined(UNIX) || defined(__GNUC__)
  return getpid();
#elif defined(DOS) || defined(OS2)
  return (HTASK) _psp;
#else
  return 0;
#endif
}

UINT WINAPI GetNumTasks(void)
{
  return 1;
}


#if defined(__DPMI32__)
HTASK FAR PASCAL GetCurrentTask(void)
{
  extern HANDLE WINAPI GetCurrentProcess(VOID);
  return (HTASK) GetCurrentProcess();
}
#endif

