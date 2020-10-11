/*===========================================================================*/
/*                                                                           */
/* File    : WINRES2.C                                                       */
/*                                                                           */
/* Purpose : Low-level resource handlign functions for MEWEL                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "winrc.h"

static LPRESINFO FAR *_ResourceTable = NULL;
static INT nResourceTable = 0;

extern INT MewelCurrOpenResourceFile;

static HANDLE PASCAL DefResourceLoader(HANDLE);
static HANDLE PASCAL DataToResource(HANDLE);
static LPRESINFO PASCAL ResHandleToPtr(HANDLE);

#define FUDGE_FACTOR  16    /* to get around lstrchr bug in BC4's DPMI16 */


#if !defined(__DLL__) && !defined(__DPMI16__)
/****************************************************************************/
/*                                                                          */
/* Function : AccessResource(hInst, hRes)                                   */
/*                                                                          */
/* Purpose  : Opens the resource file and seeks to the specified resource.  */
/*            The caller will them be ready to read the resource data.      */
/*                                                                          */
/* Returns  : The file handle of the open resource file, -1 if failure.     */
/*                                                                          */
/****************************************************************************/
int FAR PASCAL AccessResource(hInst, hRes)
  HANDLE hInst;
  HANDLE hRes;
{
  LPRESINFO pRes;

  if ((pRes = ResHandleToPtr(hRes)) == NULL)
    return -1;

  /*
    If we are loading from the resource data at the end of the EXE file,
    then open the EXE file if it's not opened.
  */
  if (MewelCurrOpenResourceFile == -1)
  {
    if ((hInst = OpenResourceFile(NULL)) == 0)
      return -1;
  }
  else
    hInst = MewelCurrOpenResourceFile;

  /*
    Seek to the resource position
  */
  lseek((UINT)hInst & 0xFF, pRes->ulSeekPos, 0);

  /*
    Return the file handle (it could have the high bit OR'ed if it's an EXE)
  */
  return ((UINT)hInst & 0x00FF);
}


/****************************************************************************/
/*                                                                          */
/* Function : FindResource(hInst, lpszID, lpszType)                         */
/*                                                                          */
/* Purpose  : Locates a specified resource within the resource file.        */
/*            Creates a resource structure and fills it with info about the */
/*            resource. This function *must* be called first before any of  */
/*            the other resource functions are called.                      */
/*                                                                          */
/* Returns  : A handle to the resource structure.                           */
/*                                                                          */
/****************************************************************************/
HRSRC FAR PASCAL FindResource(hInst, lpszID, lpszType)
  HANDLE  hInst;
  LPCSTR  lpszID;
  LPCSTR  lpszType;
{
  LPRESINFO pRes;
  UINT      hRes;

  DWORD    cb;
  DWORD    ulResPos;

  /*
    Get the resource. By calling _GetResource(), we do a seek for
    information about the resource. The resource is really not read in
    nor is memory allocated for it.
  */
  if (!_GetResource(hInst, (LPSTR) lpszID, (LPSTR) lpszType, &ulResPos, &cb))
    return (HANDLE) 0;

  /*
    Set up the array of RESINFO pointers for the first time.
  */
  if (nResourceTable == 0)
  {
    nResourceTable = 16;
    _ResourceTable = (LPRESINFO FAR *)
                     emalloc_far((DWORD) nResourceTable * sizeof(LPRESINFO));
    if (_ResourceTable == NULL)
      return 0;
  }          


  /*
    We want to see if an entry exists for this resource already.
    The best way to test this is to match the seek positions.
  */
  for (hRes = 0;  hRes < (UINT) nResourceTable;  hRes++)
    if (_ResourceTable[hRes] && _ResourceTable[hRes]->ulSeekPos == ulResPos)
      goto bye;
      

  /*
    Find a free entry in the resource table
  */
  for (hRes = 0;  hRes < (UINT) nResourceTable;  hRes++)
    if (_ResourceTable[hRes] == NULL)
      break;

  /*
    Reallocate if we are out of room.
  */
  if (hRes >= (UINT) nResourceTable)
  {
    LPRESINFO FAR *newTable;

    newTable = (LPRESINFO FAR*)
                 emalloc_far((DWORD) nResourceTable * 2 * sizeof(LPRESINFO));
    if (newTable == NULL)
      return 0;
    lmemcpy((LPSTR) newTable, (LPSTR) _ResourceTable,
            nResourceTable * sizeof(LPRESINFO));
    MyFree_far((LPSTR) _ResourceTable);
    nResourceTable *= 2;
    _ResourceTable = newTable;
  }

  /*
    Allocate a RESINFO structure and fill in the file handle, the size,
    and the seek position.
  */
  if ((pRes = _ResourceTable[hRes] = (LPRESINFO) emalloc(sizeof(RESINFO))) == NULL)
    return (HANDLE) 0;
  pRes->ulSeekPos = ulResPos;
  pRes->dwResSize = cb;
  pRes->hInst     = hInst;
  pRes->lpszType  = lpszType;

  /*
    Kludge for the memory flags. These should be part of the RES structure.
  */
  pRes->fMemFlags = GMEM_DISCARDABLE | GMEM_MOVEABLE;

bye:
  return (HRSRC)(((UINT)hRes+1) | HRES_MAGIC_BITS);
}


/****************************************************************************/
/*                                                                          */
/* Function : FreeResource(hResData)                                        */
/*                                                                          */
/* Purpose  : Frees the resource data.                                      */
/*                                                                          */
/* Returns  : FALSE if successful, TRUE if it failed.                       */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL FreeResource(hResData)
  HANDLE    hResData;
{
  HANDLE    hRes;
  LPRESINFO pRes;
  HANDLE    hMem;

  /*
    Free the memory
  */
  hMem = GlobalFree(hResData);

#if defined(__DPMI32__)
  if (hMem)
  {
    int nLocks = (GlobalFlags(hResData) & GMEM_LOCKCOUNT);
  }
  hMem = NULL;
#endif

  if (hMem == NULL)
  {
    /*
      Get a pointer to the resource structure given the data handle
    */
    if ((hRes = DataToResource(hResData)) != NULL)
    {
      pRes = ResHandleToPtr(hRes);
      pRes->hResData = NULL;
    }
  }

  return (hMem != NULL);
}


/****************************************************************************/
/*                                                                          */
/* Function : LoadResource(hInst, hRes)                                     */
/*                                                                          */
/* Purpose  : Returns a handle to a resource's memory. If no memory has     */
/*            been allocated for the resource, then allocated a "discarded" */
/*            handle with 0 bytes. When we call LockResource(), the real    */
/*            memory for the object will be allocated and the resource will */
/*            be read in.                                                   */
/*                                                                          */
/* Returns  : A handle to the resource data in memory.                      */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL LoadResource(hInst, hRes)
  HANDLE hInst;
  HANDLE hRes;
{
  LPRESINFO pRes;

  (void) hInst;

  if ((pRes = ResHandleToPtr(hRes)) == NULL)
    return NULL;

  /*
    Load the resource if not previously loaded. Actually, don't load
    it, but allocate some memory which it will be loaded into when
    LockResource() is called.
  */
  if (pRes->hResData == NULL)
#if defined(__DPMI16__) || defined(__DPMI32__)
    pRes->hResData = GlobalAlloc(pRes->fMemFlags, pRes->dwResSize + FUDGE_FACTOR);
#else
    pRes->hResData = GlobalAlloc(pRes->fMemFlags, 0L);
#endif

  /*
    Return a handle to the resource data.
  */
  return pRes->hResData;
}


/****************************************************************************/
/*                                                                          */
/* Function : LockResource(hResData)                                        */
/*                                                                          */
/* Purpose  : Locks a resource. If the resource is not in memory, it is     */
/*            read in.                                                      */
/*                                                                          */
/* Returns  : A far pointer to the resource data in memory, NULL if fails.  */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL LockResource(hResData)
  HANDLE    hResData;
{
  HANDLE    hRes;
  LPRESINFO pRes;
  LPSTR     lpData;

#if defined(__DPMI16__) || defined(__DPMI32__)
  if (1)
#else
  if ((lpData = GlobalLock(hResData)) == NULL)
#endif
  {
    /*
      Get a pointer to the resource structure given the data handle
    */
    if ((hRes = DataToResource(hResData)) == NULL)
      return NULL;
    pRes = ResHandleToPtr(hRes);
    (void) pRes;

    /*
      Allocate real memory and read in the resource
    */
    DefResourceLoader(hRes);

    /*
      Lock the block
    */
    lpData = GlobalLock(hResData);
  }

  return lpData;
}


/****************************************************************************/
/*                                                                          */
/* Function : SizeofResource(hInst, hRes)                                   */
/*                                                                          */
/* Purpose  : Determines the size of a resource.                            */
/*                                                                          */
/* Returns  : Returns a DWORD size.                                         */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL SizeofResource(hInst, hRes)
  HANDLE hInst;
  HANDLE hRes;
{
  LPRESINFO pRes;

  (void) hInst;

  if ((pRes = ResHandleToPtr(hRes)) == NULL)
    return 0;

  /*
    This function should really return a DWORD value!
  */
  return pRes->dwResSize;
}


/****************************************************************************/
/*                                                                          */
/* Function : UnlockResource(hData)                                         */
/*                                                                          */
/* Purpose  : Unlocks the memory associated with the resource data.         */
/*                                                                          */
/* Returns  : TRUE if the data was unlocked, FALSE if not.                  */
/*                                                                          */
/****************************************************************************/
#if 0
BOOL FAR PASCAL UnlockResource(hResData)
  HANDLE hResData;
{
  return GlobalUnlock(hResData);
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : ResHandleToPtr(hRes)                                          */
/*                                                                          */
/* Purpose  : Maps a resource handle to a resource structure.               */
/*                                                                          */
/* Returns  : A pointer to the resource structure, NULL if not found.       */
/*                                                                          */
/****************************************************************************/
static LPRESINFO PASCAL ResHandleToPtr(hRes)
  HANDLE hRes;
{
  LPRESINFO pRes;

  /*
    Make sure that the magic bit(s) is set in the resource handle.
  */
  if (((UINT)hRes & HRES_MAGIC_BITS) == 0)
    return (LPRESINFO) NULL;

  /*
    Turn the handle into a 1-based index into the resource table
  */
  hRes = (UINT)hRes & ~HRES_MAGIC_BITS;

  /*
    Make sure we have a valid handle and a non-null entry in the table.
  */
  if (hRes == 0 || (UINT) hRes > (UINT) nResourceTable || 
                   (pRes = _ResourceTable[(UINT)hRes-1]) == NULL)
    return (LPRESINFO) NULL;

  return pRes;
}

/****************************************************************************/
/*                                                                          */
/* Function : DataToResource(hData)                                         */
/*                                                                          */
/* Purpose  : Internal function which maps a data handle to the resource    */
/*            which owns it.                                                */
/*                                                                          */
/* Returns  : The resource handle if found, 0 if not.                       */
/*                                                                          */
/****************************************************************************/
static HANDLE PASCAL DataToResource(hData)
  HANDLE hData;
{
  UINT hRes;

  for (hRes = 0;  hRes < (UINT) nResourceTable;  hRes++)
    if (_ResourceTable[hRes] && _ResourceTable[hRes]->hResData == hData)
      return (hRes+1) | HRES_MAGIC_BITS;
  return NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : DefResourceLoader                                             */
/*                                                                          */
/* Purpose  : This is MEWEL's default resource loader. It can conceivably   */
/*            be replaced by the app by calling the SetResourceHandler()    */
/*            function.                                                     */
/*                                                                          */
/* Returns  : The handle of the resource data.                              */
/*                                                                          */
/****************************************************************************/
static HANDLE PASCAL DefResourceLoader(hRes)
  HANDLE    hRes;
{
  LPRESINFO pRes;
  HANDLE    hData;
  char HUGE*pData;
  DWORD     dwRemaining;
  UINT      wBytes;
  int       fd;

  /*
    Flag to indicate if AccessResource() opened the RES file
  */
  BOOL      bOpenedRes = (BOOL) (MewelCurrOpenResourceFile == -1);


  /*
    Get a pointer to the resource data structure
  */
  if ((pRes = ResHandleToPtr(hRes)) == NULL)
    return NULL;

  /*
    Open the resource file and seek to the resource position.
  */
  if ((fd = AccessResource(pRes->hInst, hRes)) == -1)
    return NULL;

  hData = pRes->hResData;

  /*
    If the resource was never allocated memory, or if the memory was
    discarded, then allocate memory for it.
  */
  if ((pData = GlobalLock(hData)) == NULL)
  {
    if (hData == NULL)
      hData = GlobalAlloc(pRes->fMemFlags, pRes->dwResSize + FUDGE_FACTOR);
    else
#if defined(__DPMI16__) || defined(__DPMI32__)
      hData = GlobalReAlloc(hData, pRes->dwResSize, 0);
#else
      hData = GlobalReAlloc(hData, pRes->dwResSize, pRes->fMemFlags);
#endif
    if (hData == NULL || (pData = GlobalLock(hData)) == NULL)
      goto bye;
    pRes->hResData = hData;
  }


  /*
    Get the number of bytes of data we can read in.
    (For MSC real mode using _fcalloc instead of halloc, this is 64K).
  */
  dwRemaining = GlobalSize(hData);
  dwRemaining = min(dwRemaining, pRes->dwResSize); 

  /*
    The LockResource(RT_BITMAP) should return a pointer to the
    BITMAPINFO structure which follows the BITMAPFILEHEADER structure.
    So, advance the read pointer past the BITMAPFILEHEADER structure.
  */
  if (pRes->lpszType == RT_BITMAP)
  {
#if defined(UNIX_STRUCTURE_PACKING)
    /* GNUC alignment */
    lseek(fd, sizeof(BITMAPFILEHEADER) - 2, 1);
    dwRemaining -= sizeof(BITMAPFILEHEADER) - 2;
#else
    lseek(fd, sizeof(BITMAPFILEHEADER), 1);
    dwRemaining -= sizeof(BITMAPFILEHEADER);
#endif
  }

#if defined(UNIX_STRUCTURE_PACKING) && !defined(MEWEL_TEXT)
  if (pRes->lpszType == RT_ICON || pRes->lpszType == RT_CURSOR)
  {
    _lread(fd, pData, sizeof(ICONHEADER));
    pData += sizeof(ICONHEADER) + 2;
    dwRemaining -= sizeof(ICONHEADER) + 2;
  }
#endif

  /*
    Read in the resource data. We are not limited to 64K.
  */
  for (  ;  dwRemaining > 0;  dwRemaining -= wBytes)
  {
    wBytes = (UINT) (min(dwRemaining, (WORD) 0x8000));
    _lread(fd, pData, wBytes);
    pData += wBytes;
  }

  /*
    Clean up. Unlock the resource data and close the resource file.
  */
  GlobalUnlock(pRes->hResData);

bye:
  if (bOpenedRes)
    CloseResourceFile(fd);
  return hData;
}

#endif /* DLL || DPMI16 */



/****************************************************************************/
/*                                                                          */
/* Function : AllocResource(hInst, hRes, dwSize)                            */
/*                                                                          */
/* Purpose  : Allocates memory for a resource.                              */
/*            If dwSize is 0, then the resource's present size is used.     */
/*                                                                          */
/* Returns  : The memory handle, NULL if fails.                             */
/*                                                                          */
/****************************************************************************/
GLOBALHANDLE FAR PASCAL AllocResource(hInst, hRes, dwSize)
  HANDLE hInst;
  HANDLE hRes;
  DWORD  dwSize;
{
#if defined(__DPMI16__) || (defined(__DPMI32__) && defined(__DLL__))
  /*
    Use the RTM's resource handling funcs
  */
  (void) hInst;
  return GlobalAlloc(GMEM_MOVEABLE, dwSize + FUDGE_FACTOR);

#else
  LPRESINFO pRes;

  (void) hInst;

  if ((pRes = ResHandleToPtr(hRes)) == NULL)
    return NULL;

  return GlobalAlloc(pRes->fMemFlags, (dwSize==0) ? pRes->dwResSize : dwSize);
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : SetResourceHandler()                                          */
/*                                                                          */
/* Purpose  : Sets a custom resource loader for the type of resource in     */
/*            lpsz.                                                         */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
RSRCHDLRPROC WINAPI SetResourceHandler(hInst, lpsz, lpfn)
  HINSTANCE hInst;
  LPCSTR    lpsz;
  RSRCHDLRPROC lpfn;
{
  (void) hInst;
  (void) lpsz;
  (void) lpfn;

  return (FARPROC) 0;
}

