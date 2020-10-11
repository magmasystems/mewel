/*===========================================================================*/
/*                                                                           */
/* File    : WGUIICON.C                                                      */
/*                                                                           */
/* Purpose : Implements the icon functions.                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#define INCLUDE_SYSICONS

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif



static VOID PASCAL InitSystemIcons(void);
static LPCSTR PASCAL LookupIconName(HINSTANCE, LPCSTR, BOOL *);

#if defined(XWINDOWS)
static VOID PASCAL ReverseBitmapRows(LPSTR, int, int);
#endif

static BOOL   bSysIconsLoaded = FALSE;
static HANDLE hSysIcons[5];

extern BYTE HUGEDATA SysIconBitmap16[5][16*32];
extern BYTE HUGEDATA _ApplIconAndMask[16*32];
extern BYTE HUGEDATA _DefIconAndMask[16*32];


HICON FAR PASCAL LoadIcon(hModule, lpszIcon)
  HINSTANCE hModule;
  LPCSTR    lpszIcon;
{
  BOOL               bSkipHeader;
  HBITMAP            hBitmap = 0;
  HANDLE             hResData;
  LPSTR              lpMem, lpOrig;
  LPICONHEADER       lpIconHeader;
  LPICONDIRECTORY    lpIconDir;
  LPBITMAPINFOHEADER lpbmi;
  INT                nBytesPerLine;
  INT                nColors;
  LONG               dwBytes;

  HBITMAP            hAndBitmap = 0;
  LPICONINFO         lpII;


  /*
    A system icon?
  */
#if 0
  if (hModule == NULL)
#endif
  {
    UINT wIcon = FP_OFF(lpszIcon);
    if (HIWORD(lpszIcon) == 0 && wIcon >= 32512 && wIcon <= 32516)
    {
      int idx = ((int) wIcon) - 32512;  /* kludge for MSC codegen bug */
      if (!bSysIconsLoaded)
        InitSystemIcons();
      return hSysIcons[idx];
    }
  }

  /*
    See if the resource exists.
  */
  if ((lpOrig = lpMem = LoadResourcePtr(hModule,
                                        LookupIconName(hModule,lpszIcon,&bSkipHeader),
                                        RT_ICON, &hResData)) == 0)
    return NULL;

  /*
    Point to the icon header and the icon directory entry
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
      Make sure that this resource is really an icon. If it is, then
      the icoResourceType field must be 1.
    */
    if (lpIconHeader->icoResourceType != 1)
      goto bye;

    swIconDirectory(lpIconDir = (LPICONDIRECTORY) lpMem);
    lpMem += sizeof(ICONDIRECTORY);
  }

  /*
    Get a pointer to the the BITMAPINFOHEADER and the RGB part.
  */
  swBitmapInfoHeader(lpbmi = (LPBITMAPINFOHEADER) lpMem);

  /*
    For some reason, MS Windows puts twice the actual height in the
    lpbmi->biHeight field. So, get the actual height from the
    icon directory and put it into the bitmap header.
  */
  lpbmi->biHeight >>= 1;

  /*
    Make sure that SetDIBits() does not use the value in biSizeImage
    (which is usually 640) in order to calculate the # of bits per line.
  */
  lpbmi->biSizeImage = 0;

  /*
    Allocate a global buffer to hold the bitmap data. The size of the
    bitmap is the area in the pixel rectangle divided by 8-bits-per-byte,
    times the number of color planes.
  */
  if ((nBytesPerLine = BitmapBitsToLineWidth(lpbmi->biBitCount,
                                             lpbmi->biWidth,
                                             &nColors)) < 0)
    return (HICON) NULL;

  dwBytes = sizeof(BITMAPINFOHEADER);
  if (nColors != 24)
    dwBytes += nColors*sizeof(RGBQUAD);

  /*
    Create the icon bitmap.
  */
  hBitmap =
    CreateDIBitmap((HDC) 0, 
                   (LPBITMAPINFOHEADER) lpbmi,
                   CBM_INIT,
                   lpMem + dwBytes,
                   (LPBITMAPINFO) lpbmi, 
                   DIB_RGB_COLORS);


  dwBytes += ((lpbmi->biWidth*lpbmi->biBitCount+31) / 32) * 4 * lpbmi->biHeight;
  lpbmi->biBitCount = 1;

#if defined(XWINDOWS)
  ReverseBitmapRows(lpMem+dwBytes, lpbmi->biWidth/8, lpbmi->biHeight);
  hAndBitmap=CreateBitmap(lpbmi->biWidth, lpbmi->biHeight, 1, 1, lpMem+dwBytes);
#else
  hAndBitmap =
    CreateDIBitmap((HDC) 0, 
                   (LPBITMAPINFOHEADER) lpbmi,
                   CBM_INIT,
                   lpMem + dwBytes,
                   (LPBITMAPINFO) lpbmi, 
                   DIB_RGB_COLORS);
#endif

  /*
    Free the icon resource and return a handle to the memory.
  */
bye:
  UnloadResourcePtr(hModule, lpOrig, hResData);

  /*
    Create an icon-info structure and store the handles to the image
    bitmap and the AND bitmap.
  */
  hResData = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONINFO));
  lpII = (LPICONINFO) GlobalLock(hResData);
  lpII->hBitmap    = hBitmap;
  lpII->hAndBitmap = hAndBitmap;
  GlobalUnlock(hResData);
  return (HICON) hResData;
}


static LPCSTR PASCAL LookupIconName(hModule, lpszIcon, pbSkipHeader)
  HINSTANCE hModule;
  LPCSTR    lpszIcon;
  BOOL      *pbSkipHeader;
{
  if (IS_MEWEL_INSTANCE(hModule))
  {
    *pbSkipHeader = FALSE;
    return lpszIcon;
  }
#if defined(__DPMI16__)
  else
  {
    LPSTR  lpMem, lpOrig;
    LPCSTR lpName;
    USHORT wOrd;
    HANDLE hRes;
    LPICONDIRECTORY lpIconDir;

    *pbSkipHeader = TRUE;
    if ((lpOrig = lpMem = LoadResourcePtr(hModule,lpszIcon,RT_GROUP_ICON,&hRes)) == 0)
      return lpszIcon;
    lpMem += sizeof(ICONHEADER);
    /*
      We should really go through all of the entries in the icon directory
      and look for the icon which matches our screen resolution, but let's
      take the first entry for now...
      In an EXE or DLL resource, Windows puts the ordinal of the 
      proper icon in the 'icoDIBOffset' field.
    */
    swIconDirectory(lpIconDir = (LPICONDIRECTORY) lpMem);
    wOrd = (USHORT) lpIconDir->icoDIBOffset;
    lpName = MAKEINTRESOURCE(wOrd);
    UnloadResourcePtr(hModule, lpOrig, hRes);
    return lpName;
  }
#endif
}


BOOL FAR PASCAL DrawIcon(hDC, x, y, hIcon)
  HDC   hDC;
  int   x, y;
  HICON hIcon;
{
  LPICONINFO lpII;

  if ((lpII = (LPICONINFO) GlobalLock(hIcon)) == NULL)
    return FALSE;

#if defined(XWINDOWS)
  DrawBitmapToDC(hDC, x, y, lpII->hAndBitmap, SRCAND);
  DrawBitmapToDC(hDC, x, y, lpII->hBitmap, SRCPAINT);
#else
  if (lpII->hAndBitmap)
    DrawBitmapToDC(hDC, x, y, lpII->hAndBitmap, SRCAND);
  DrawBitmapToDC(hDC, x, y, lpII->hBitmap, SRCINVERT);
#endif

  GlobalUnlock(hIcon);
  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : DestroyIcon(hIcon)                                            */
/*                                                                          */
/* Purpose  : Frees up the memory associated with an icon.                  */
/*                                                                          */
/* Returns  : TRUE if destroyed, FALSE if not.                              */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DestroyIcon(hIcon)
  HICON hIcon;
{
  LPICONINFO lpII;

  /*
    Get a pointer to the internal icon structure.
  */
  if ((lpII = (LPICONINFO) GlobalLock(hIcon)) == NULL)
    return FALSE;

  /*
    Don't free a system icon.
  */
  if (lpII->fFlags & ICON_SYSTEMICON)
    return FALSE;

  /*
    Free the two bitmaps.
  */
  if (lpII->hAndBitmap)
    DeleteObject(lpII->hAndBitmap);
  if (lpII->hBitmap)
    DeleteObject(lpII->hBitmap);

  /*
    Free the ICONINFO structure.
  */
  GlobalUnlock(hIcon);
  GlobalFree(hIcon);
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : InitSystemIcons()                                             */
/*                                                                          */
/* Purpose  : Called when MEWEL draws an icon for the first time. The       */
/*            system icons are created by creating the AND bitmap and       */
/*            the XOR bitmap.                                               */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL InitSystemIcons(void)
{
  char achBitmap[sizeof(BITMAPINFO) + (16 * sizeof(RGBQUAD))];
  int  i;

  extern BITMAPINFO SysIconBMInfo16;
  extern RGBQUAD SysDefaultRGB[16];


  lmemcpy(achBitmap, (LPSTR) &SysIconBMInfo16, sizeof(BITMAPINFOHEADER));
  lmemcpy(((BITMAPINFO *) achBitmap)->bmiColors, (LPSTR) SysDefaultRGB, 16*sizeof(RGBQUAD));


  for (i = 0;  i <= 4;  i++)
  {
    LPICONINFO lpII;
    HBITMAP    hbmpDefault;

    hSysIcons[i] = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONINFO));
    lpII = (LPICONINFO) GlobalLock(hSysIcons[i]);

    /*
      Create the XOR bitmap.
    */
    lpII->hBitmap =
      CreateDIBitmap((HDC) 0, 
                     (LPBITMAPINFOHEADER) achBitmap,
                     CBM_INIT,
                     (LPSTR) SysIconBitmap16[i],
                     (LPBITMAPINFO) achBitmap,
                     DIB_RGB_COLORS);


    /*
      Create the AND bitmap.
    */
    if (i <= 1)
    {
      hbmpDefault = lpII->hAndBitmap =
        CreateDIBitmap((HDC) 0, 
                     (LPBITMAPINFOHEADER) achBitmap,
                     CBM_INIT,
                     (LPSTR) ((i == 0) ? _ApplIconAndMask : _DefIconAndMask),
                     (LPBITMAPINFO) achBitmap,
                     DIB_RGB_COLORS);
    }
    else
    {
      /*
        For system icons 2,3, and 4, use the default AND bitmap for the
        AND mask.
      */
      lpII->hAndBitmap = hbmpDefault;
    }
      
       
    /*
      Flag the icon as a system icon.
    */
    lpII->fFlags |= ICON_SYSTEMICON;

    GlobalUnlock(hSysIcons[i]);
  }


  bSysIconsLoaded++;
}


#if defined(XWINDOWS)
static VOID PASCAL ReverseBitmapRows(LPSTR lpImage, int iWidth, int iHeight)
{
  char  szBuf[32];
  LPSTR lpTop = lpImage;
  LPSTR lpBottom = lpImage + ((iHeight-1) * iWidth);
  int   y1, y2;

  for (y1 = 0, y2 = iHeight-1;  y1 < y2;  y1++, y2--)
  {
    lmemcpy(szBuf, lpTop, iWidth);
    lmemcpy(lpTop, lpBottom, iWidth);
    lmemcpy(lpBottom, szBuf, iWidth);
    lpTop += iWidth;
    lpBottom -= iWidth;
  }
}
#endif

#if !defined(BYTE_SWAP)
/****************************************************************************/
/*                                                                          */
/* Functions to do byte-swapping on icon/bitmap headers:                    */
/*   swIconHeader()       - Swaps structure members on Icon Header.         */
/*   swIconDirectory()    -   "        "       "    "  Icon Directories     */
/*   swBitmapInfoHeader() -   "        "       "    "  Bitmap Info Headers  */
/*                                                                          */
/****************************************************************************/
void swIconHeader(LPICONHEADER p)
{
  p->icoReserved = swis(p->icoReserved);
  p->icoResourceType = swis(p->icoResourceType);
  p->icoResourceCount = swis(p->icoResourceCount);
}

void swIconDirectory(LPICONDIRECTORY p)
{
  p->bReserved2 = swis(p->bReserved2);
  p->bReserved3 = swis(p->bReserved3);
  p->icoDIBSize = swil(p->icoDIBSize);
  p->icoDIBOffset = swil(p->icoDIBOffset);
}

void swBitmapInfoHeader(LPBITMAPINFOHEADER p)
{
  p->biSize = swil(p->biSize);
  p->biWidth = swil(p->biWidth);
  p->biHeight = swil(p->biHeight);
  p->biPlanes = swis(p->biPlanes);
  p->biBitCount = swis(p->biBitCount);
  p->biCompression = swil(p->biCompression);
  p->biSizeImage = swil(p->biSizeImage);
  p->biXPelsPerMeter = swil(p->biXPelsPerMeter);
  p->biYPelsPerMeter = swil(p->biYPelsPerMeter);
  p->biClrUsed = swil(p->biClrUsed);
  p->biClrImportant = swil(p->biClrImportant);
}
#endif

