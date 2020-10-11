/*===========================================================================*/
/*                                                                           */
/* File    : WGDISBIT.C                                                      */
/*                                                                           */
/* Purpose : SetBitmapBits and GetBitmapBits functions                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif

#if defined(META) || defined(GX)
#define STORE_IMAGE_ONCE
#define GX_USE_VS
#endif


/****************************************************************************/
/*                                                                          */
/* Function : Set/GetBitmapBits(hBitmap, ulBytes, lpBits)                   */
/*                                                                          */
/* Purpose  : Set or retrieve the device dependent bits from a DDB          */
/*                                                                          */
/* Returns  : The number of bytes in the bitmap.                            */
/*                                                                          */
/****************************************************************************/
static LONG PASCAL SetOrGetBitmapBits(HBITMAP, LONG, CONST VOID FAR *, BOOL);

LONG FAR PASCAL SetBitmapBits(hObj, ulBytes, lpBits)
  HBITMAP     hObj;    /* handle to the bitmap object */
  DWORD       ulBytes; /* number of bytes to copy */
  CONST VOID FAR *lpBits;  /* address of source or dest bits */
{
  return SetOrGetBitmapBits(hObj, ulBytes, lpBits, TRUE);
}

LONG FAR PASCAL GetBitmapBits(hObj, ulBytes, lpBits)
  HBITMAP     hObj;    /* handle to the bitmap object */
  LONG        ulBytes; /* number of bytes to copy */
  VOID FAR   *lpBits;  /* address of source or dest bits */
{
  return SetOrGetBitmapBits(hObj, ulBytes, lpBits, FALSE);
}

static LONG PASCAL SetOrGetBitmapBits(hObj, ulBytes, lpBits, bSet)
  HBITMAP     hObj;    /* handle to the bitmap object */
  LONG        ulBytes; /* number of bytes to copy */
  CONST VOID FAR *lpBits;  /* address of source or dest bits */
  BOOL        bSet;    /* TRUE if setting, FALSE if getting */
{
  HANDLE      hBitmapMem;
  LPOBJECT    lpObj;
  LPBITMAP    lpbm;
  char HUGE  *pImg;

  /*
    Get a pointer to the actual image bits.
  */
  if ((pImg = (char HUGE *)HObjectToImageBits(hObj,&hBitmapMem,&lpbm,NULL)) == NULL)
    return 0L;


#if defined(GX)
  /*
    Get a pointer to the underlying BITMAP structure
  */
  lpObj = _ObjectDeref(hObj);
  if (lpObj->lpExtra)
  {
#if defined(GX_USE_VS)
    GXHEADER *pGX = (GXHEADER *) lpObj->lpExtra;
    int nRowBytes = lpbm->bmWidthBytes / pGX->nplanes;
    int row, iPlane;

    pImg = (char HUGE *) lpBits;

    for (row = 0;  row < lpbm->bmHeight;  row++)
      for (iPlane = 0;  iPlane < pGX->nplanes;  iPlane++)
      {
        if (bSet)
          gxSetVirtualScanline(pGX, row, (LPSTR) pImg);
        else
          gxGetVirtualScanline(pGX, row, (LPSTR) pImg);
        pImg += nRowBytes;
      }
#else
    pImg = (char HUGE *) ((GXHEADER *) lpObj->lpExtra)->vptr;
    /*
      Set or get the bitmap bits to/from the actual GX bitmap
    */
    if (bSet)
      hmemcpy(pImg, (char HUGE *) lpBits, ulBytes);
    else
      hmemcpy((char HUGE *) lpBits, pImg, ulBytes);
#endif
  }
  else
    ulBytes = 0L;


#elif defined(META)

#if defined(META) && defined(STORE_IMAGE_ONCE)
  pImg = (char HUGE *) lpBits;
  lpObj = _ObjectDeref(hObj);

  if (lpObj->lpExtra)
  {
    grafMap *gm = lpObj->lpExtra;
    int nRowBytes = lpbm->bmWidthBytes / gm->pixPlanes;
    int row, iPlane;

    /*
      Under MetaWindows, there are two ways to fill up the buffer. They are:

      row0plane0, row0plane1, row0plane2, row0plane3, row1plane0, row1plane1...
        or
      row0plane0, row1plane0, row2plane0, row3plane0, row4plane0, row5plane1...

      For the first method, we use the following nested loop
        for (row = 0;  row < lpbm->bmHeight;  row++)
          for (iPlane = 0;  iPlane < gm->pixPlanes;  iPlane++)

      For the second method, we use
        for (iPlane = 0;  iPlane < gm->pixPlanes;  iPlane++)
          for (row = 0;  row < lpbm->bmHeight;  row++)

      I think that MetaWindows' DefinePattern() function needs the
      second method.
    */

    for (iPlane = 0;  iPlane < gm->pixPlanes;  iPlane++)
      for (row = 0;  row < lpbm->bmHeight;  row++)
      {
        unsigned char **pRowTbl;
        unsigned char *pRow;
        int      ip;

        ip = (gm->pixPlanes == 1) ? 0 : 3-iDIBtoDDBPlaneOrder[iPlane];
        pRowTbl = gm->mapTable[ip];
#if defined(__DPMI32__)
        ((char *) pRowTbl) -= ((DWORD) gm->mapAltMgr);
#endif
        pRow = pRowTbl[row];
#if defined(__DPMI32__)
        ((char *) pRow) -= ((DWORD) gm->mapAltMgr);
#endif
        if (bSet)
          lmemcpy(pRow, (LPSTR) pImg, nRowBytes);
        else
          lmemcpy((LPSTR) pImg, pRow, nRowBytes);
        pImg += nRowBytes;
      }
  }

#else
  /*
    Set or get the bitmap bits.
  */
  ulBytes = min(lpbm->bmWidthBytes * lpbm->bmHeight, ulBytes);
  if (bSet)
  {
    hmemcpy(pImg, (char HUGE *) lpBits, ulBytes);

    lpObj = _ObjectDeref(hObj);

    if (lpObj->lpExtra)
    {
      grafMap *gm = lpObj->lpExtra;
      int nRowBytes = lpbm->bmWidthBytes / gm->pixPlanes;
      int row, iPlane;

      for (row = 0;  row < lpbm->bmHeight;  row++)
        for (iPlane = 0;  iPlane < gm->pixPlanes;  iPlane++)
        {
          unsigned char **pRowTbl;
          unsigned char *pRow;

          pRowTbl = gm->mapTable[iPlane];
#if defined(__DPMI32__)
          ((char *) pRowTbl) -= ((DWORD) gm->mapAltMgr);
#endif
          pRow = pRowTbl[row];
#if defined(__DPMI32__)
          ((char *) pRow) -= ((DWORD) gm->mapAltMgr);
#endif
          lmemcpy(pRow, pImg, nRowBytes);
          pImg += nRowBytes;
        }
     }
   }
   else
   {
     hmemcpy((char HUGE *) lpBits, (char HUGE *) pImg, ulBytes);
   }
#endif /* defined(STORE_IMAGE_ONCE) */


#else /* BGI or MSC */

  ulBytes = min(lpbm->bmWidthBytes * lpbm->bmHeight, ulBytes);

  /*
    Set or get the bitmap bits.
  */
  if (bSet)
    hmemcpy(pImg, (char HUGE *) lpBits, ulBytes);
  else
    hmemcpy((char HUGE *) lpBits, pImg, ulBytes);

#endif

  /*
    Unlock the memory taken up by the bitmap. If we are using virtual
    memory, the bitmap bits will be swapped back out to VM.
  */
  GlobalUnlock(hBitmapMem);
  return ulBytes;
}

