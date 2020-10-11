/*===========================================================================*/
/*                                                                           */
/* File    : WMEM.C                                                          */
/*                                                                           */
/* Purpose : Memory management emulation for Windows                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MEMMGR

#include "wprivate.h"
#include "window.h"
#include "winrc.h"


/*
  Change the #if 0 to #if 1 if you want to use the XMSLIB library
*/
#if defined(DOS) && !defined(EXTENDED_DOS)
#if 1
#define USE_XMSLIB
#endif
#endif

/*
  Change the #if 0 to #if 1 if you want to use the VMM which comes in MSC 7

  Note : If the MSC VMM is enabled, you *must* set MewelMemoryMgr.uMaxDOSmem
         to some value.
*/
#if defined(DOS) && defined(_MSC_VER) && (_MSC_VER >= 700) && !defined(USE_XMSLIB)
#if 0
#define USE_MSCVM
#endif
#endif


/*
  Choose the proper VM include file
*/
#if defined(USE_XMSLIB)
#include "xmslib.h"
#elif defined(USE_MSCVM)
#include <vmemory.h>
#endif


/*
  MEWEL Memory Manager structure
*/
MEMMGRINFO MewelMemoryMgr =
{
  NULL, 0L, NULL, 0, 0, 0, NULL, 0
};

static VOID PASCAL GlobalDiscardAll(void);

/*
  Certain compilers (MSC) have implementations of _fcalloc() which only
  allocate at most 64K. So, if we do a GlobalAlloc() where we request
  a chunk of memory above 64K, then we must pare the memory down to 64K.
*/
#if defined(MSC) && defined(DOS) && !defined(USE_MSCVM) && !defined(MEWEL_32BITS)

/*
  If the XMS allocations can go over 64K, then we need halloc to allocate
  conventional memory for the block to be moved into.
*/
#if !defined(XMSALLOC_MAX_IS_64K)
#define USE_HALLOC
#endif

/*
  If USE_HALLOC is not defined, then MSC will use _fmalloc, which can only
  allocate 64K.
*/
#if !defined(USE_HALLOC)
#define FARALLOC_MAX_IS_64K
#endif

#endif


/****************************************************************************/
/*                                                                          */
/*                 Routines to manage the Handle table                      */
/*                                                                          */
/****************************************************************************/
extern VOID CDECL MemoryTerminate(void);

VOID FAR PASCAL MemoryInit(void)
{
  if (TEST_PROGRAM_STATE(STATE_SPAWNING))
    return;

#if 0
#if defined(__TSC__) || defined(__WATCOMC__)
  atexit((void (*)()) MemoryTerminate);
#elif defined(SUNOS)
  on_exit(MemoryTerminate, 0);
#else
  atexit(MemoryTerminate);
#endif
#endif

#if defined(USE_XMSLIB)
  if (!TEST_PROGRAM_STATE(STATE_NO_XMS) && XMSinstalled() && XMSopen(0))
    MewelMemoryMgr.fFlags |= MEM_USE_XMSLIB;

#elif defined(USE_MSCVM)
  if (!TEST_PROGRAM_STATE(STATE_NO_XMS) &&
      _vheapinit(0, MewelMemoryMgr.uMaxDOSMem, _VM_ALLSWAP))
    MewelMemoryMgr.fFlags |= MEM_USE_MSCVM;
#endif
}


VOID CDECL MemoryTerminate(void)
{
  if (TEST_PROGRAM_STATE(STATE_SPAWNING))
    return;

#if defined(USE_XMSLIB)
  if (MewelMemoryMgr.fFlags & MEM_USE_XMSLIB)
  {
    XMSclose();
    MewelMemoryMgr.fFlags &= ~MEM_USE_XMSLIB;
  }

#elif defined(USE_MSCVM)
  if (MewelMemoryMgr.fFlags & MEM_USE_MSCVM)
  {
    _vheapterm();
    MewelMemoryMgr.fFlags &= ~MEM_USE_MSCVM;
  }
#endif

}


/****************************************************************************/
/*                                                                          */
/* Function : FindFreeHandle()                                              */
/*                                                                          */
/* Purpose  : Searches the handle table (linearly) for the first free       */
/*            handle. Tries to reallocate the table if there are no free    */
/*            handles available.                                            */
/*                                                                          */
/* Returns  : The non-zero index of the first free handle                   */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL FindFreeHandle(void)
{
  UINT hMem;

  /*
    Allocate the initial handle table if not allocated already.
  */
  if (MewelMemoryMgr.nHandlesAlloced == 0)
  {
    MewelMemoryMgr.nHandlesAlloced = 64;
    MewelMemoryMgr.HandleTable = (LPMEMHANDLEINFO) 
     emalloc_far((DWORD)MewelMemoryMgr.nHandlesAlloced * sizeof(MEMHANDLEINFO));
    if (MewelMemoryMgr.HandleTable == NULL)
      return 0;
  }          

  /*
    Find the first free entry in the table.
    Note - handle 0 is never used...
  */
  for (hMem = 1;  hMem < MewelMemoryMgr.nHandlesAlloced;  hMem++)
    if (MewelMemoryMgr.HandleTable[hMem].fMemFlags == 0)
      return (HANDLE) hMem;

  /*
    If we exceeded the number of handles in the table, then reallocate
    the table.
  */
  if (hMem >= MewelMemoryMgr.nHandlesAlloced)
  {
    if (!GmemRealloc((LPSTR FAR *) &MewelMemoryMgr.HandleTable,
                     &MewelMemoryMgr.nHandlesAlloced,sizeof(MEMHANDLEINFO),2))
      return (HANDLE) 0;
    else
      return (HANDLE) hMem;
  }
  else
    return (HANDLE) 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : DerefHandle()                                                 */
/*                                                                          */
/* Purpose  : Returns a pointer to the handle's data structure              */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LPMEMHANDLEINFO FAR PASCAL DerefHandle(hMem)
  HANDLE hMem;
{
  if (hMem == 0 || hMem >= MewelMemoryMgr.nHandlesAlloced)
  {
    return NULL;
  }
  else
  {
    LPMEMHANDLEINFO lpMI = MewelMemoryMgr.HandleTable + (UINT) hMem;
    lpMI->dwLRUCount = ++MewelMemoryMgr.ulLRUCount;
    return lpMI;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalAlloc()                                                 */
/*                                                                          */
/* Purpose  : Allocates dwBytes from the global heap.                       */
/*                                                                          */
/* Returns  : A non-zero handle if successful, 0 if not.                    */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL GlobalAlloc(wFlags, dwBytes)
  UINT  wFlags;
  DWORD dwBytes;
{
  HANDLE hMem;
  LPMEMHANDLEINFO lpMI;

  /*
    Find a free handle.
  */
  if ((hMem = FindFreeHandle()) == 0)
    return (HANDLE) 0;

  /*
    Get a pointer to the handle info.
  */
  lpMI = MewelMemoryMgr.HandleTable + (UINT) hMem;


  /*
    If the number of bytes asked for is 0, then set the block to the
    discarded state.
  */
  if (dwBytes == 0L)
  {
    lpMI->fMemFlags = GLOBAL_BLOCK;
    lpMI->dwBytes   = 0L;
    lpMI->wFlags    = GMEM_DISCARDED;
    goto bye;
  }

  /*
    Round up to nearest multiple of 2
  */
  if (dwBytes & 0x01L)
    dwBytes++;


#if defined(FARALLOC_MAX_IS_64K)
  /*
    Limit the allocation to 64K minus a fudge factor
  */
  if (dwBytes >= (64L * 1024L) - 32)
    dwBytes = (64L * 1024L) - 32;
#endif


#if defined(USE_XMSLIB)
  if (MewelMemoryMgr.fFlags & MEM_USE_XMSLIB)
  {
    /*
      Get the allocation down to below 64K
    */
#if defined(XMSALLOC_MAX_IS_64K)
    if (dwBytes >= (64L * 1024L) - 32)
    {
#if defined(FARALLOC_MAX_IS_64K)
      dwBytes = (64L * 1024L) - 32;
#else
      /*
        10/19/92 (maa)
          Since the XMS manager can only handle 64K allocations, and
        since the Borland faralloc can handle > 64K allocations, we pass
        all requests for > 64K allocations to emalloc_far.
      */
      goto do_dosalloc;
#endif
    }
#endif

    /*
      Alloc from XMS
    */
#if defined(XMSALLOC_MAX_IS_64K)
    if ((lpMI->uOutMem.ulXMSMem = XMSalloc((UINT) dwBytes)) == NULL)
#else
    if ((lpMI->uOutMem.ulXMSMem = XMSalloc(       dwBytes)) == NULL)
#endif
    {
      if (!(wFlags & (GMEM_NODISCARD | GMEM_NOCOMPACT)))
        GlobalDiscardAll();
#if defined(XMSALLOC_MAX_IS_64K)
      if ((lpMI->uOutMem.ulXMSMem = XMSalloc((UINT) dwBytes)) == NULL)
#else
      if ((lpMI->uOutMem.ulXMSMem = XMSalloc(       dwBytes)) == NULL)
#endif
        return (HANDLE) NULL;
    }

    lpMI->fMemFlags = GLOBAL_BLOCK | BLOCK_IN_XMS;
    lpMI->dwBytes   = (DWORD) XMSgetLen(lpMI->uOutMem.ulXMSMem);
    if (wFlags & GMEM_ZEROINIT)
    {
#if defined(__ZTC__)
      char far *lpMem = (char far *) GlobalLock(hMem);
#else
      char _huge *lpMem = (char _huge *) GlobalLock(hMem);
#endif
      DWORD dwWords;
      UINT  wWords;

      if (lpMem)
      {
        for (dwWords = lpMI->dwBytes;  dwWords > 0;  dwWords -= wWords)
        {
          wWords = (UINT) (min(dwWords, 0x3000));
          lmemset(lpMem, 0, wWords);
          lpMem += wWords;
        }
        GlobalUnlock(hMem);
      }
    }
    goto bye;
  }

#elif defined(USE_MSCVM)
  if (MewelMemoryMgr.fFlags & MEM_USE_MSCVM)
  {
    /*
      Alloc from VM
    */
    if ((lpMI->uOutMem.ulXMSMem = _vmalloc(dwBytes)) == _VM_NULL)
    {
      if (!(wFlags & (GMEM_NODISCARD | GMEM_NOCOMPACT)))
        GlobalDiscardAll();
      if ((lpMI->uOutMem.ulXMSMem = _vmalloc(dwBytes)) == _VM_NULL)
        return (HANDLE) NULL;
    }

    lpMI->fMemFlags = GLOBAL_BLOCK | BLOCK_IN_XMS;
    lpMI->dwBytes   = (DWORD) _vmsize(lpMI->uOutMem.ulXMSMem);
    if (wFlags & GMEM_ZEROINIT)
    {
      char _huge *lpMem = (char _huge *) GlobalLock(hMem);
      DWORD dwWords;
      UINT  wWords;

      if (lpMem == NULL)
      {
        MessageBox(_HwndDesktop, "GlobalAlloc - GlobalLock returns NULL", NULL, MB_OK);
        exit(1);
      }

      for (dwWords = lpMI->dwBytes >> 1;  dwWords > 0;  dwWords -= wWords)
      {
        wWords = (UINT) (min(dwWords, 0x3000));
        lwmemset((LPWORD) lpMem, 0, wWords);
        lpMem += (wWords << 1);
      }

      GlobalUnlock(hMem);
    }
    goto bye;
  }
#endif



  /*
    Allocate from conventional memory. Use the C run-time memory allocator
    for this.
  */
do_dosalloc:
#if defined(USE_HALLOC)
  if (dwBytes >= 64L * 1024L - 32)
  {
    if ((lpMI->uMem.lpMem = halloc(dwBytes, 1)) == NULL)
    {
      if (!(wFlags & (GMEM_NODISCARD | GMEM_NOCOMPACT)))
        GlobalDiscardAll();
      if ((lpMI->uMem.lpMem = halloc(dwBytes, 1)) == NULL)
        return (HANDLE) NULL;
    }
  }
  else
#endif
  {
  if ((lpMI->uMem.lpMem = emalloc_far_noquit(dwBytes)) == NULL)
  {
    if (!(wFlags & (GMEM_NODISCARD | GMEM_NOCOMPACT)))
      GlobalDiscardAll();
    if ((lpMI->uMem.lpMem = emalloc_far_noquit(dwBytes)) == NULL)
      return (HANDLE) NULL;
  }
  }

  lpMI->fMemFlags  = GLOBAL_BLOCK | BLOCK_IN_CONV;
  lpMI->dwBytes    = dwBytes;
#if defined(USE_HALLOC)
  if (dwBytes >= 64L * 1024L)
    lpMI->fMemFlags  = HALLOC_BLOCK;
#endif


  /*
    Fill in the rest of the handle info
  */
bye:
  lpMI->wFlags = (wFlags & (GMEM_MOVEABLE   | GMEM_DISCARDABLE | 
                            GMEM_NOT_BANKED | GMEM_DDESHARE));
  if (wFlags & GMEM_NOTIFY)    /* cause GMEM_NOTIFY == GMEM_DISCARDED */
    lpMI->fMemFlags |= NOTIFY_BLOCK;
  lpMI->iLockCount = 0;
  lpMI->dwLRUCount = 0L;
  return hMem;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalCompact()                                               */
/*                                                                          */
/* Purpose  : Supposed to compact memory.                                   */
/*            Note : this implementation is a KLUDGE                        */
/*                                                                          */
/* Returns  : The number of bytes of DOS memory available                   */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL GlobalCompact(dwMinFree)
  DWORD dwMinFree;
{
#if defined(DOS)
  union REGS regs;

  (void) dwMinFree;
  regs.h.ah = 0x48;
  regs.x.bx = 0xFFFF;
  INT86(0x21, &regs, &regs);
  return (16L * regs.x.bx);
#else
  return 0L;
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalDiscard()                                               */
/*                                                                          */
/* Purpose  : Discards the memory taken up by handle hMem. Note : this is   */
/*            not implemented correctly yet....                             */
/*                                                                          */
/* Returns  : The handle of the discarded memory, if successful. 0 if not.  */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL GlobalDiscard(hMem)
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
  if ((lpMI->wFlags & GMEM_DISCARDABLE) && !(lpMI->wFlags & GMEM_DISCARDED))
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

    /*
      Free the memory.
    */
    MewelMemoryMgr.fFlags |=  MEMMGR_DISCARDING;
#if defined(USE_XMSLIB)
    /*
      If the block is in XMS, then free only the conventional memory part.
    */
    if (lpMI->fMemFlags & BLOCK_IN_XMS)
      GlobalUnlock(hMem);
    else
#endif
    GlobalFree(hMem);
    MewelMemoryMgr.fFlags &= ~MEMMGR_DISCARDING;

    /*
      Set fMemFlags to GLOBAL_BLOCK so that FindFreeHandle() will not
      consider this handle free. Also, retain the old memory flags and
      OR in the DISCARDED bit.
    */
    lpMI->wFlags    = wOldFlags | GMEM_DISCARDED;
    lpMI->fMemFlags = GLOBAL_BLOCK;
  }

  return hMem;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalFlags()                                                 */
/*                                                                          */
/* Purpose  : Returns some status info on handle hMem.                      */
/*                                                                          */
/* Returns  : The lockcount in the lower byte and discard info in the       */
/*            high byte.                                                    */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL GlobalFlags(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  else
  {
    UINT wFlags = (lpMI->iLockCount & 0x00FF) | 
                  (lpMI->wFlags & (GMEM_DISCARDABLE | GMEM_DISCARDED));
    return wFlags;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalFree()                                                  */
/*                                                                          */
/* Purpose  : Frees the memory associated with handle hMem.                 */
/*                                                                          */
/* Returns  : 0 if successful, hMem if not.                                 */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL GlobalFree(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  /*
    Get a pointer to the handle info.
  */
  if ((lpMI = DerefHandle(hMem)) == NULL)
    return (HANDLE) hMem;

  /*
    Ignore the lockcount for now.
  */
#if 0
  if (lpMI->iLockCount > 0)
     return (HANDLE) hMem;
#endif


  /*
    Free the memory
  */
  if (lpMI->fMemFlags & LOCAL_BLOCK)
    return LocalFree(hMem);


#if !defined(USE_MSCVM)
  if (lpMI->fMemFlags & BLOCK_IN_CONV)
  {
#if defined(USE_HALLOC)
    if (lpMI->fMemFlags & HALLOC_BLOCK)
      hfree(lpMI->uMem.lpMem);
    else
#endif
    MyFree_far(lpMI->uMem.lpMem);
  }
#endif


  if ((lpMI->fMemFlags & BLOCK_IN_XMS) && lpMI->uOutMem.ulXMSMem)
  {
#if defined(USE_XMSLIB)
    XMSfree(lpMI->uOutMem.ulXMSMem);

#elif defined(USE_MSCVM)
    unsigned int nLocks = _vlockcnt(lpMI->uOutMem.ulXMSMem);
    while (nLocks-- != 0)
      _vunlock(lpMI->uOutMem.ulXMSMem, _VM_DIRTY);
    _vfree(lpMI->uOutMem.ulXMSMem);
#endif
  }

  /*
    Clear out all the values in the handle's data structure
  */
  lmemset((LPSTR) lpMI, 0, sizeof(MEMHANDLEINFO));
  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalHandle()                                                */
/*                                                                          */
/* Purpose  : Given a segment, tries to find the corresponding handle.      */
/*                                                                          */
/* Returns  : The handle in the high word and the selector in the low word  */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL GlobalHandle(wMem)
  UINT wMem;
{
  UINT hMem;
  LPMEMHANDLEINFO lpMI = MewelMemoryMgr.HandleTable + 1;

  for (hMem = 1;  hMem < MewelMemoryMgr.nHandlesAlloced;  hMem++, lpMI++)
  {
#if defined(MEWEL_32BITS)
    if (lpMI->uMem.lpMem == (LPSTR) wMem)
      return MAKELONG(hMem, 0);
#else
    if (FP_SEG(lpMI->uMem.lpMem) == wMem)
      return MAKELONG(hMem, wMem);
#endif
  }
  return 0L;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalLock()                                                  */
/*                                                                          */
/* Purpose  : Dereferences a block. If the block is in XMS, swaps it in.    */
/*                                                                          */
/* Returns  : A FAR pointer to the data in conventional memory.             */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL GlobalLock(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
  {
    return (LPSTR) NULL;
  }
  else
  {
    /*
      If the block has been discarded, return NULL.
    */
    if (lpMI->wFlags & GMEM_DISCARDED)
      return (LPSTR) NULL;

    if (++lpMI->iLockCount > 1)
      return lpMI->uMem.lpMem;

#if defined(USE_XMSLIB)
    /*
      If the block is in XMS memory also, write the contents from XMS to
      conventional memory.
    */
    if ((lpMI->fMemFlags & BLOCK_IN_XMS) && lpMI->uMem.lpMem == NULL)
    {
#if defined(USE_HALLOC)
      if (lpMI->dwBytes >= 64L * 1024L - 32)
      {
        if ((lpMI->uMem.lpMem = halloc(lpMI->dwBytes, 1)) != NULL)
        {
          XMSget(lpMI->uMem.lpMem, lpMI->uOutMem.ulXMSMem);
          lpMI->fMemFlags |= BLOCK_IN_CONV | HALLOC_BLOCK;
        }
      }
      else
#endif
      if ((lpMI->uMem.lpMem = emalloc_far_noquit(lpMI->dwBytes)) != NULL)
      {
        XMSget(lpMI->uMem.lpMem, lpMI->uOutMem.ulXMSMem);
        lpMI->fMemFlags |= BLOCK_IN_CONV;
      }
    }

#elif defined(USE_MSCVM)
    if ((lpMI->fMemFlags & BLOCK_IN_XMS) && lpMI->uMem.lpMem == NULL)
    {
      if ((lpMI->uMem.lpMem = _vlock(lpMI->uOutMem.ulXMSMem)) != NULL)
      {
        lpMI->fMemFlags |= BLOCK_IN_CONV;
      }
    }
#endif

    return lpMI->uMem.lpMem;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalLRUOldest/Newest()                                      */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : The handle if successful, 0 if not.                           */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL GlobalLRUNewest(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  else
  {
    lpMI->dwLRUCount = 0;
    return hMem;
  }
}

HANDLE FAR PASCAL GlobalLRUOldest(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  else
  {
    lpMI->dwLRUCount = (DWORD) 0x7FFF;  /* kludge */
    return hMem;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalNotify()                                                */
/*                                                                          */
/* Purpose  : Sets the system notification proc. Called when a block is     */
/*            about to be discarded.                                        */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL GlobalNotify(lpNotifyProc)
  FARPROC lpNotifyProc;
{
  MewelMemoryMgr.lpfnNotify = (NOTIFYPROC *) lpNotifyProc;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalReAlloc()                                               */
/*                                                                          */
/* Purpose  : Increases the size of a memory block.                         */
/*                                                                          */
/* Returns  : The handle to the original reallocated block.                 */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL GlobalReAlloc(hMem, dwBytes, wFlags)
  HANDLE hMem;
  DWORD  dwBytes;
  UINT   wFlags;
{
  LPMEMHANDLEINFO lpMI, lpMInew;
  HANDLE hNew;
  LPSTR  lpOldMem, lpNewMem;

  /*
    Windows will take a 0 handle and will return memory, just
    like GlobalAlloc()
  */
  if (hMem == 0)
    return GlobalAlloc(wFlags, dwBytes);

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
#if 0
  if (lpMI->iLockCount)
    return 0;
#endif
  if (dwBytes <= lpMI->dwBytes)
    return hMem;

  /*
    Allocate new memory for the block.
  */
  if (lpMI->fMemFlags & GLOBAL_BLOCK)
    hNew = GlobalAlloc(wFlags, dwBytes);
  else
    hNew = LocalAlloc(wFlags, (UINT) dwBytes);
  if (hNew == 0)
    return 0;

  /*
    Deref the old handle's structure again in case we reallocated the
    memory handle table.
  */
  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0;
  lpMInew = DerefHandle(hNew);

  /*
    Copy the data in the old block to the new block.
  */
  if ((lpNewMem = GlobalLock(hNew)) != NULL)
  {
    if ((lpOldMem = GlobalLock(hMem)) != NULL)
    {
      lmemcpy(lpNewMem, lpOldMem, (UINT) lpMI->dwBytes);
      GlobalUnlock(hMem);
    }
    GlobalUnlock(hNew);
  }

  /*
    Free the old block
  */
  if (lpMI->fMemFlags & GLOBAL_BLOCK)
    GlobalFree(hMem);
  else
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


/****************************************************************************/
/*                                                                          */
/* Function : GlobalSize()                                                  */
/*                                                                          */
/* Purpose  : Given a handle, determines the amount of memory allocated to  */
/*            the handle.                                                   */
/*                                                                          */
/* Returns  : The number of bytes taken by this memory block.               */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL GlobalSize(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;

  if ((lpMI = DerefHandle(hMem)) == NULL)
    return 0L;
  else
    return lpMI->dwBytes;
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalUnlock()                                                */
/*                                                                          */
/* Purpose  : Unlocks a global mem block, making it available to be freed   */
/*            or discarded.                                                 */
/*                                                                          */
/* Returns  : TRUE if unlocked, FALSE if not.                               */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL GlobalUnlock(hMem)
  HANDLE hMem;
{
  LPMEMHANDLEINFO lpMI;


  if ((lpMI = DerefHandle(hMem)) == NULL)
    return FALSE;
  else
  {
#if 0
    if (lpMI->iLockCount <= 0 && !(MewelMemoryMgr.fFlags & MEMMGR_DISCARDING))
      MessageBox(_HwndDesktop, "GlobalUnlock < 0", NULL, MB_OK);
#endif
    if (lpMI->iLockCount > 0)
      lpMI->iLockCount--;

    /*
      Don't page memory if the block is still locked.
    */
    if (lpMI->iLockCount > 0)
      return TRUE;

#if defined(USE_XMSLIB)
    /*
      If the block is in XMS memory also, write the contents back from
      conventional memory to XMS. Also, free the conventional memory.
    */
    if (lpMI->fMemFlags & BLOCK_IN_XMS)
    {
      if (lpMI->uMem.lpMem)
      {
        XMSput(lpMI->uOutMem.ulXMSMem, lpMI->uMem.lpMem, (UINT)lpMI->dwBytes);
        /*
          10/23/92 (maa)
            Only free the conventional memory if we are discarding.
        */
#if 0
        if (MewelMemoryMgr.fFlags & MEMMGR_DISCARDING)
#endif
        {
#if defined(USE_HALLOC)
          if (lpMI->fMemFlags & HALLOC_BLOCK)
            hfree(lpMI->uMem.lpMem);
          else
#endif
          MyFree_far(lpMI->uMem.lpMem);
          lpMI->uMem.lpMem = NULL;
          lpMI->fMemFlags &= ~(BLOCK_IN_CONV | HALLOC_BLOCK);
        }
      }
    }

#elif defined(USE_MSCVM)
    if (lpMI->fMemFlags & BLOCK_IN_XMS)
    {
      if (lpMI->uMem.lpMem)
      {
        _vunlock(lpMI->uOutMem.ulXMSMem, _VM_DIRTY);
        if (MewelMemoryMgr.fFlags & MEMMGR_DISCARDING)
        {
          lpMI->uMem.lpMem = NULL;
          lpMI->fMemFlags &= ~BLOCK_IN_CONV;
        }
      }
    }

#endif

    return TRUE;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GlobalDiscardAll()                                            */
/*                                                                          */
/* Purpose  : Compacts memory by discarding discardable memory blocks.      */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL GlobalDiscardAll(void)
{
  register UINT hMem;

  for (hMem = 1;  hMem < MewelMemoryMgr.nHandlesAlloced;  hMem++)
  {
    GlobalDiscard(hMem);
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : GetFreeSpace()                                                */
/*                                                                          */
/* Purpose  : Gets the amount of free memory in the global heap.            */
/*                                                                          */
/* Returns  : The amount of free memory, in bytes.                          */
/*                                                                          */
/****************************************************************************/
/*
  For the 386 DOS Extenders, we need to redefine the way that int86()
  is called.
*/
#if defined(__WATCOMC__) && defined(__386__)
#define ax  eax
#define bx  ebx
#define cx  ecx
#define dx  edx
#undef  INT86
#define INT86  int386
#endif

DWORD FAR PASCAL GetFreeSpace(fuFlags)
  UINT fuFlags;  /* ignored in Win 3.1 */
{
#if defined(DOS)

  /*
    In DOS, call the int 21 function to get the number of free bytes of
    DOS memory. This is the easiest thing to do.
  */
  union REGS regs;
  DWORD      ulDOSfree;

  (void) fuFlags;
  regs.h.ah = 0x48;
  regs.x.bx = 0xFFFF;
  INT86(0x21, &regs, &regs);
  ulDOSfree = 16L * regs.x.bx;
  return ulDOSfree;
#else

  (void) fuFlags;
  return 0x7FFFFFFFL;

#endif
}

