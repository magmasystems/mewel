/*===========================================================================*/
/*                                                                           */
/* File    : WMEMLOCL.C                                                      */
/*                                                                           */
/* Purpose : Memory management emulation for Windows ... Local memory        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MEMMGR

#include "wprivate.h"
#include "window.h"

static VOID PASCAL LocalDiscardAll(void);


/****************************************************************************/
/*                                                                          */
/* Function : LocalAlloc()                                                  */
/*                                                                          */
/* Purpose  : Allocates memory from the local heap.                         */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL LocalAlloc(wFlags, wBytes)
  UINT  wFlags;
  UINT  wBytes;
{
  HANDLE hMem;
  LPMEMHANDLEINFO lpMI;

  if (wBytes == 0 || (hMem = FindFreeHandle()) == 0)
    return (HANDLE) 0;
  lpMI = MewelMemoryMgr.HandleTable + (UINT) hMem;

  /*
    If the number of bytes asked for is 0, then set the block to the
    discarded state.
  */
  if (wBytes == 0)
  {
    lpMI->fMemFlags = LOCAL_BLOCK;
    lpMI->wFlags    = LMEM_DISCARDED;
    goto bye;
  }

  /*
    Round up to nearest multiple of 2
  */
  if (wBytes & 0x01L)
    wBytes++;

  if ((lpMI->uMem.lpMem = (LPSTR) emalloc(wBytes)) == NULL)
  {
    if (!(wFlags & (LMEM_NODISCARD | LMEM_NOCOMPACT)))
      LocalDiscardAll();
    if ((lpMI->uMem.lpMem = (LPSTR) emalloc(wBytes)) == NULL)
      return (HANDLE) 0;
  }

  lpMI->fMemFlags  = LOCAL_BLOCK | BLOCK_IN_CONV;
  lpMI->wFlags     = wFlags;

bye:
  lpMI->dwBytes    = (DWORD) wBytes;
  lpMI->iLockCount = 0;
  lpMI->dwLRUCount = 0L;
  return hMem;
}

UINT FAR PASCAL LocalCompact(wMinFree)
  UINT wMinFree;
{
#if defined(DOS)
  union REGS regs;

  (void) wMinFree;
  regs.h.ah = 0x48;
  regs.x.bx = 0xFFFF;
  INT86(0x21, &regs, &regs);
  return (/*16L* */ regs.x.bx);  /* return number of paras - fits in a WORD */
#else
  return 0;
#endif
}

HANDLE FAR PASCAL LocalDiscard(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;
  UINT wOldFlags;

  if ((lpMI = DerefHandle(hMem)) == NULL || lpMI->iLockCount > 0)
    return 0;

  /*
    If the block is discardable, unlocked, and hasn't been discarded
    yet, then discard it.
  */
  if ((lpMI->wFlags & LMEM_DISCARDABLE) && !(lpMI->wFlags & LMEM_DISCARDED))
  {
    /*
      If this block has the notification flag on and if there is
      a notification function, then notify the app. If the app
      returns 0, then don't free the block.
    */
    if ((lpMI->fMemFlags & NOTIFY_BLOCK) && MewelMemoryMgr.lpfnNotify)
    {
      if (!(*MewelMemoryMgr.lpfnNotify)(hMem))
        return 0;
    }

    wOldFlags = lpMI->wFlags;

    LocalFree(hMem);

    /*
      Set fMemFlags to LOCAL_BLOCK so that FindFreeHandle() will not
      consider this handle free. Also, retain the old memory flags and
      OR in the DISCARDED bit.
    */
    lpMI->wFlags    = wOldFlags | LMEM_DISCARDED;
    lpMI->fMemFlags = LOCAL_BLOCK;
  }

  return hMem;
}

UINT FAR PASCAL LocalFlags(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  else
  {
    UINT wFlags = (lpMI->iLockCount & 0x00FF) | 
                  (lpMI->wFlags & (LMEM_DISCARDABLE | LMEM_DISCARDED));
    return wFlags;
  }
}

HANDLE FAR PASCAL LocalFree(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return (HANDLE) hMem;

#if 0
  if (lpMI->iLockCount > 0)
     return (HANDLE) hMem;
#endif

  if (lpMI->fMemFlags & GLOBAL_BLOCK)
    return GlobalFree(hMem);

  /*
    Free the memory and zero out all of the fields (important!)
  */
  if (lpMI->uMem.lpMem)
    MyFree(lpMI->uMem.pMem);
  lmemset((LPSTR) lpMI, 0, sizeof(MEMHANDLEINFO));
  return 0;
}

HANDLE FAR PASCAL LocalHandle(wMem)
  UINT wMem;
{
  return LOWORD(GlobalHandle(wMem));
}

PSTR FAR PASCAL LocalLock(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return (PSTR) NULL;
  else
  {
    if (lpMI->fMemFlags & GLOBAL_BLOCK)
      return GlobalLock(hMem);

    /*
      If the block has been discarded, return NULL.
    */
    if (lpMI->wFlags & LMEM_DISCARDED)
      return (LPSTR) NULL;

    lpMI->iLockCount++;
    return (PSTR) lpMI->uMem.lpMem;
  }
}

HANDLE FAR PASCAL LocalReAlloc(hMem, wBytes, wFlags)
  HANDLE hMem;
  UINT   wBytes;
  UINT   wFlags;
{
  LPMEMHANDLEINFO lpMI, lpMInew;
  HANDLE hNew;

  /*
    Windows will take a 0 handle and will return memory, just
    like LocalAlloc()
  */
  if (hMem == 0)
    return LocalAlloc(wFlags, wBytes);

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
#if 0
  if (lpMI->iLockCount)
    return 0;
#endif

  /*
    If the amount of memory current in the block is more than the amount
    requested, we don't need to realloc the block.
  */
  if ((DWORD) wBytes <= lpMI->dwBytes)
    return hMem;

  if (lpMI->fMemFlags & GLOBAL_BLOCK)
    return GlobalReAlloc(hMem, (DWORD) wBytes, wFlags);

  hNew = LocalAlloc(wFlags, wBytes);
  if (hNew == 0)
    return 0;

  /*
    Deref the old handle's structure again in case we reallocated the
    memory handle table.
  */
  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  lpMInew = DerefHandle(hNew);
  lmemcpy(lpMInew->uMem.lpMem, lpMI->uMem.lpMem, (UINT) lpMI->dwBytes);
  LocalFree(hMem);

  /*
    Since we want to preserve the handle, copy all the internal
    info from the new handle to the old handle, and then wipe out
    the new handle.
  */
  lmemcpy((LPSTR) lpMI, (LPSTR) lpMInew, sizeof(MEMHANDLEINFO));
  lmemset((LPSTR) lpMInew, 0, sizeof(MEMHANDLEINFO));

  return hMem;
}

UINT FAR PASCAL LocalSize(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  else
    return (UINT) lpMI->dwBytes;
}

BOOL FAR PASCAL LocalUnlock(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return FALSE;
  else
  {
    if (lpMI->fMemFlags & GLOBAL_BLOCK)
      return GlobalUnlock(hMem);
    if (lpMI->iLockCount > 0)
      lpMI->iLockCount--;
    return TRUE;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : LocalDiscardAll()                                             */
/*                                                                          */
/* Purpose  : Compacts memory by discarding discardable memory blocks.      */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL LocalDiscardAll(void)
{
  register UINT hMem;

  for (hMem = 1;  hMem < MewelMemoryMgr.nHandlesAlloced;  hMem++)
  {
    LocalDiscard(hMem);
  }
}

