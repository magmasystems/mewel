/*===========================================================================*/
/*                                                                           */
/* File    : WGUICURS.C                                                      */
/*                                                                           */
/* Purpose : Implements the cursor functions.                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#define INCLUDE_MOUSE
#define INCLUDE_SYSCURSORS
#define INCLUDE_CURSORS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#ifndef USE_32x32_CURSORS
#define COMPRESS_CURSOR
#endif

/*
#define IDC_ARROW     MAKEINTRESOURCE(32512)
#define IDC_IBEAM     MAKEINTRESOURCE(32513)
#define IDC_WAIT      MAKEINTRESOURCE(32514)
#define IDC_CROSS     MAKEINTRESOURCE(32515)
#define IDC_UPARROW   MAKEINTRESOURCE(32516)
#define IDC_SIZE      MAKEINTRESOURCE(32640)
#define IDC_ICON      MAKEINTRESOURCE(32641)
#define IDC_SIZENWSE  MAKEINTRESOURCE(32642)
#define IDC_SIZENESW  MAKEINTRESOURCE(32643)
#define IDC_SIZEWE	  MAKEINTRESOURCE(32644)
#define IDC_SIZENS	  MAKEINTRESOURCE(32645)
*/

/*
  A cursor handle with SYSCURSOR_MAGIC or'ed into it tells MEWEL
  that a system cursor is involved.

  typedef struct tagSysCursorInfo
  {
    LPSTR  dwID;
    INT    nWidth, nHeight;
    INT    xHotSpot, yHotSpot;
    BYTE   achANDMask[(16/8) * 16];
    BYTE   achXORMask[(16/8) * 16];
      The first mask is the screen mask (AND).
      The second mask is the cursor mask (XOR).
  } SYSCURSORINFO, FAR *LPSYSCURSORINFO;
*/

/*
  For systems which have their own GlobalAlloc (Borland extenders),
  we cannot rely on low-numbered handles. So, to distinguish a
  pre-defined cursor from a cursor resource, just test the cursor
  handle to see if it's less than 16. Predefined cursors will
  have low numbers as their handles.
*/
#if defined(__DPMI16__) || defined(__DPMI32__)
#define SYSCURSOR_MAGIC  0x0000
#define IS_PREDEF_CURSOR(hCursor)    ((hCursor) < 16)
#else
#define SYSCURSOR_MAGIC  0x0800
#define IS_PREDEF_CURSOR(hCursor)    ((hCursor) & SYSCURSOR_MAGIC)
#endif


extern SYSCURSORINFO HUGEDATA SysCursorInfo[11];

#ifdef COMPRESS_CURSOR
static VOID PASCAL CompressMask(PSTR, PSTR, BOOL);
#endif

static LPCSTR PASCAL LookupCursorName(HINSTANCE, LPCSTR, BOOL *);




HCURSOR WINAPI SetCursor(HCURSOR hCursor)
{
  HCURSOR         hLastCursor;
  LPSYSCURSORINFO pCursor;

  if (hCursor == 0)
  {
    return InternalSysParams.hCursor;
  }

  /*
    Do nothing if we are using the same cursor
  */
#if defined(XWINDOWS)
  if (InternalSysParams.hCursor == hCursor && 
      XSysParams.winLastCursor == XSysParams.winCursor)
#else
  if (InternalSysParams.hCursor == hCursor)
#endif
  {
    return hCursor;
  }

  /*
    See if hCursor is a pre-defined system cursor or a custom cursor
    located in a resource file
  */
  if (IS_PREDEF_CURSOR((UINT)hCursor))
  {
    pCursor = (LPSYSCURSORINFO) &SysCursorInfo[((UINT)hCursor & 0x0F) - 1];
  }
  else
  {
    /*
      We have a cursor which was created by LoadCursor or CreateCursor().
      This means that hCursor is the handle of a memory object which points
      to a SYSCURSORINFO structure. We check the dwID field to make sure
      that we are dealing with a cursor object.
    */
    if ((pCursor = (LPSYSCURSORINFO) GlobalLock(hCursor)) == NULL)
    {
      return hCursor;
    }
    if (pCursor->dwID != CURSOR_SIGNATURE)
    {
      GlobalUnlock(hCursor);
      return hCursor;
    }
  }


  /*
    Call the operating system to set the cursor
  */
#if defined(DOS)
#if defined(EXTENDED_DOS) && !defined(DOS286X) && !defined(WC386)
  /*
    Calling this Int 33H function seems to cause a protection fault for the
    DOS extenders during debugging.
  */
  if (!IGetSystemMetrics(SM_DEBUG))
#endif
  MOUSE_SetGraphicsCursor(pCursor, pCursor->xHotSpot, pCursor->yHotSpot,
                          pCursor->achANDMask, pCursor->achXORMask);

#elif defined(XWINDOWS)
  /*
    If there is a valid XWindows cursor, display it now
  */
  if (XSysParams.winCursor != XSysParams.winLastCursor && 
      XSysParams.winLastCursor != NULL && 
      XtWindow(XSysParams.widgetLastCursor) != NULL)
    XUndefineCursor(XSysParams.display, XSysParams.winLastCursor);

  if (pCursor->xCursor && XSysParams.winCursor)
  {
    XSysParams.winLastCursor = XSysParams.winCursor;
    XSysParams.widgetLastCursor = XSysParams.widgetCursor;
    XDefineCursor(XSysParams.display, XSysParams.winCursor, pCursor->xCursor);
  }
  else
  {
    XSysParams.winLastCursor = NULL;
    XSysParams.widgetLastCursor = NULL;
  }
#endif


  /*
    Unlock the cursor's data
  */
  if (!IS_PREDEF_CURSOR((UINT)hCursor))
    GlobalUnlock(hCursor);

  /*
    Set the system variable and return the old cursor.
  */
  hLastCursor = InternalSysParams.hCursor;
  InternalSysParams.hCursor = hCursor;
  return hLastCursor;
}


HCURSOR WINAPI LoadCursor(hInst, lpszCursor)
  HINSTANCE hInst;
  LPCSTR    lpszCursor;
{
  SYSCURSORINFO      sysCursorInfo;
  LPSYSCURSORINFO    lpCursor;
  LPICONHEADER       lpIconHeader;
  LPICONDIRECTORY    lpIconDir;
  LPSTR              lpMem, lpOrig;
  HANDLE             hRes;
  BOOL               bSkipHeader;
#ifdef COMPRESS_CURSOR
  BOOL               b32x32 = FALSE;
#endif


  /*
    Is it a system-defined cursor?
  */
  if (hInst == NULL)
  {
    /*
      Fast search down the system cursor list for the cursor ID
    */
    INT i;
    for (i = 0;  i < NSYSCURSORS;  i++)
    {
      lpCursor = &SysCursorInfo[i];
      if (lpCursor->dwID == lpszCursor)
      {
#if defined(XWINDOWS)
        /*
          If it is a system cursor, make sure we created the XWindows cursor
        */
        if (lpCursor->xCursor == NULL)
          lpCursor->xCursor = 
                 XCreateFontCursor(XSysParams.display, lpCursor->idXCursor);
#endif

        return (i + 1) | SYSCURSOR_MAGIC;
      }
    }
  }


  /*
    Find the cursor in the resource data.
  */
  if ((lpOrig = lpMem = LoadResourcePtr(hInst, 
                                        LookupCursorName(hInst, lpszCursor, &bSkipHeader),
                                        RT_CURSOR, &hRes)) == NULL)
  {
    return NULL;
  }

  /*
    Default values for the height, width, and hotspot of the cursor
  */
  sysCursorInfo.dwID     = CURSOR_SIGNATURE;
  sysCursorInfo.nWidth   = 32;
  sysCursorInfo.nHeight  = 32;
  sysCursorInfo.xHotSpot = 0;
  sysCursorInfo.yHotSpot = 0;

  /*
    Move past the cursor header and cursor directory entries. Then
    move past the bitmap header and palette so that we point to the
    actual cursor image.
  */
  if (!bSkipHeader)
  {
    swIconHeader(lpIconHeader = (LPICONHEADER) lpMem);
#if defined(UNIX_STRUCTURE_PACKING)
    lpMem += sizeof(ICONHEADER) + 2;
#else
    lpMem += sizeof(ICONHEADER);
#endif
    /*
      Make sure that this resource is really a cursor. If it is, then
      the icoResourceType field must be 2.
    */
    if (lpIconHeader->icoResourceType != 2)
    {
fail:
      UnloadResourcePtr(hInst, lpOrig, hRes);
      return NULL;
    }
    swIconDirectory(lpIconDir = (LPICONDIRECTORY) lpMem);
    lpMem += sizeof(ICONDIRECTORY);
    sysCursorInfo.nWidth   = lpIconDir->Width;
    sysCursorInfo.nHeight  = lpIconDir->Height;
    sysCursorInfo.xHotSpot = lpIconDir->bReserved2;
    sysCursorInfo.yHotSpot = lpIconDir->bReserved3;
  }

  /*
    Get pastthe bitmap information and palette
  */
  lpMem += sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD);

  /*
    Make sure that the cursor is 16x16 for DOS graphics mode
  */
#ifndef USE_32x32_CURSORS
  if (sysCursorInfo.nWidth != 16 || sysCursorInfo.nHeight != 16)
#ifdef COMPRESS_CURSOR
    b32x32 = TRUE;
#else
    goto fail;
#endif
#endif


  /*
    Set the 'from resource' flag to TRUE. This will tell some of the
    special GUI-mouse routines to not flip the bits of the masks.
  */
  sysCursorInfo.bFromResource = TRUE;

  /*
    Copy the XOR mask and the AND mask
  */
#ifdef COMPRESS_CURSOR
  if (b32x32)
  {
#if !defined(GX)
    CompressMask(lpMem,        sysCursorInfo.achXORMask, FALSE);
    CompressMask(lpMem+(4*32), sysCursorInfo.achANDMask, TRUE);
#else
    CompressMask(lpMem,        sysCursorInfo.achANDMask, FALSE);
    CompressMask(lpMem+(4*32), sysCursorInfo.achXORMask, TRUE);
#endif
    if (sysCursorInfo.xHotSpot != -1)
      sysCursorInfo.xHotSpot >>= 1;
    if (sysCursorInfo.yHotSpot != -1)
      sysCursorInfo.yHotSpot >>= 1;
  }
  else
#endif
  {
    int iMaskSize = (sysCursorInfo.nWidth/8) * sysCursorInfo.nHeight;
    lmemcpy(sysCursorInfo.achANDMask, lpMem,           iMaskSize);
    lmemcpy(sysCursorInfo.achXORMask, lpMem+iMaskSize, iMaskSize);
  }

  /*
    Free the cursor resource
  */
  UnloadResourcePtr(hInst, lpOrig, hRes);

  /*
    Create an cursor-info structure and store the data which we
    accumulated in the sysCursorInfo structure.
  */
  hRes = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(SYSCURSORINFO));
  lpCursor = (LPSYSCURSORINFO) GlobalLock(hRes);
  lmemcpy((LPSTR) lpCursor, (LPSTR) &sysCursorInfo, sizeof(sysCursorInfo));

#if defined(XWINDOWS)
  XMEWELLoadCursor(lpCursor);
#endif

  /*
    Return the memory handle to the SYSCURSOINFO block
  */
  GlobalUnlock(hRes);
  return (HCURSOR) hRes;
}


static LPCSTR PASCAL LookupCursorName(hModule, lpszCursor, pbSkipHeader)
  HINSTANCE hModule;
  LPCSTR    lpszCursor;
  BOOL      *pbSkipHeader;
{
  if (IS_MEWEL_INSTANCE(hModule))
  {
    *pbSkipHeader = FALSE;
    return lpszCursor;
  }
#if defined(__DPMI16__)
  else
  {
    LPSTR  lpMem, lpOrig;
    LPCSTR lpName;
    USHORT wOrd;
    HANDLE hRes;

    *pbSkipHeader = TRUE;
    if ((lpOrig = lpMem = LoadResourcePtr(hModule,lpszCursor,RT_GROUP_CURSOR,&hRes)) == 0)
      return lpszCursor;
    lpMem += sizeof(ICONHEADER);
    /*
      We should really go through all of the entries in the icon directory
      and look for the icon which matches our screen resolution, but let's
      take the first entry for now...
      In an EXE or DLL resource, Windows puts the ordinal of the 
      proper icon in the 'icoDIBOffset' field.
    */
    wOrd = (USHORT) ((LPCURSORDIRECTORY) lpMem)->curDIBOffset;
    lpName = MAKEINTRESOURCE(wOrd);
    UnloadResourcePtr(hModule, lpOrig, hRes);
    return lpName;
  }
#endif
}



#ifdef COMPRESS_CURSOR
static VOID PASCAL CompressMask(lpSrc, lpDest, bIsMaskPart)
  PSTR lpSrc;
  PSTR lpDest;
  BOOL bIsMaskPart;
{
  int  x, y;
  BYTE ch[2];

  /*
    We have a 32x32 Windows cursor which we want to make into a
    16x16 cursor which is compatible with Int33H, func 9.
    Each row of the Windows cursor resource is 4 bytes.
  */

  /*
    Since the cursor is stored upside-down, move to the last row
    of the cursor and work backwards.
  */
  lpSrc += (30 * 4);

  for (y = 0;  y < 32;  y += 2)
  {
    for (x = 0;  x < 2;  x++)
    {
      BYTE b1 = *lpSrc++;
      BYTE b2 = *lpSrc++;

      /*
        Take every other bit
      */
      ch[x] =  ((b1 & 0x80)     ) |
               ((b1 & 0x20) << 1) |
               ((b1 & 0x08) << 2) |
               ((b1 & 0x02) << 3) |
               ((b2 & 0x80) >> 4) |
               ((b2 & 0x20) >> 3) |
               ((b2 & 0x08) >> 2) |
               ((b2 & 0x02) >> 1);
    }

#if !defined(GX) && 0
    if (bIsMaskPart)
    {
      ch[0] ^= 0xFF;
      ch[1] ^= 0xFF;
    }
#endif

    *lpDest++ = ch[1];
    *lpDest++ = ch[0];

    /*
      Skip a row
    */
    lpSrc -= 8;
  }
}
#endif


