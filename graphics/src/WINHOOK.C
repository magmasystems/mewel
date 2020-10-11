/*===========================================================================*/
/*                                                                           */
/* File    : WINHOOK.C                                                       */
/*                                                                           */
/* Purpose : Stubs for the Windows hook procedures                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

HHOOK FAR PASCAL SetWindowsHookEx(idHook, hkprc, hInst, hTask)
  int  idHook;
  HOOKPROC  hkprc;
  HINSTANCE hInst;
  HTASK     hTask;
{
  HHOOK hHook;
  PHOOKINFO pHI;

  (void) hInst;
  (void) hTask;

  if (idHook >= MAXHOOKS)
    return NULL;

  if ((pHI = (PHOOKINFO) emalloc(sizeof(HOOKINFO))) == NULL)
    return NULL;

  /*
    Fill in the hook structure.
    The hHook value is an encoding of a unique id in the high 8 bits
    and the index into the hookinfo array in the low 8 bits.
  */
  pHI->lpfnHook = hkprc;
  pHI->hHook    = hHook = ((++InternalSysParams.nTotalHooks << 8) | idHook);

  /*
    Link this hookinfo onto the head of this hook's chain
  */
  pHI->pNextHook = InternalSysParams.pHooks[idHook];
  InternalSysParams.pHooks[idHook] = pHI;

  return hHook;
}


BOOL FAR PASCAL UnhookWindowsHookEx(hHook)
  HHOOK hHook;
{
  PHOOKINFO p, prevp;

  /*
    Do a big search for the hookinfo corresponding to hHook
  */
  prevp = NULL;
  for (p = InternalSysParams.pHooks[(UINT) hHook & 0x00FF];  p;  p=p->pNextHook)
  {
    if (p->hHook == hHook)
    {
      /*
        Found the hook. Unlink it from the chain and free it.
      */
      if (prevp == NULL)
        InternalSysParams.pHooks[(UINT) hHook & 0x00FF] = p->pNextHook;
      else
        prevp->pNextHook = p->pNextHook;
      MyFree(p);
      return TRUE;
    }
    prevp = p;
  }
  return FALSE;
}


LRESULT FAR PASCAL CallNextHookEx(hHook, nCode, wParam, lParam)
  HHOOK  hHook;
  int    nCode;
  WPARAM wParam;
  LPARAM lParam;
{
  PHOOKINFO p;

  /*
    Map the hHook id into a HOOKINFO structure
  */
  for (p = InternalSysParams.pHooks[(UINT) hHook & 0x00FF];  p;  p=p->pNextHook)
    if (p->hHook == hHook)
      break;

  /*
    Get the next hook in the chain
  */
  if (p == NULL || (p = p->pNextHook) == NULL)
    return 0L;

  /*
    Call the next hook proc
  */
  if (p->lpfnHook)
    return (*(WNDPROCHOOKPROC31*)p->lpfnHook)(p->hHook, nCode, wParam, lParam);
  else
    return 0L;
}


#if defined(STRICT)
HHOOK
#else
HOOKPROC
#endif
FAR PASCAL SetWindowsHook(nCode, lpfnHook)
  int      nCode;
  HOOKPROC lpfnHook;
{
  HOOKPROC lpfnOld;

  if (InternalSysParams.pHooks[nCode])
    lpfnOld = InternalSysParams.pHooks[nCode]->lpfnHook;
  else
    lpfnOld = NULL;

  SetWindowsHookEx(nCode, (HOOKPROC) lpfnHook, 0, 0);

  return lpfnOld;
}

BOOL FAR PASCAL UnhookWindowsHook(nCode, lpfnHook)
  int     nCode;
  FARPROC lpfnHook;
{
  PHOOKINFO pHI;

  (void) lpfnHook;

  if ((pHI = InternalSysParams.pHooks[nCode]) != NULL)
  {
    MyFree(pHI);
    InternalSysParams.pHooks[nCode] = NULL;
  }

  return TRUE;
}


LRESULT FAR PASCAL DefHookProc(nCode, wParam, lParam, lpfnOldFunc)
  int     nCode;
  WPARAM  wParam;
  LPARAM  lParam;
  HOOKPROC FAR *lpfnOldFunc;
{
  if (**lpfnOldFunc == NULL)
    return 0;
  return (* (WNDPROCHOOKPROC *) lpfnOldFunc)(nCode, wParam, lParam);
}

