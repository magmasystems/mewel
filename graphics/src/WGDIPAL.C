/*===========================================================================*/
/*                                                                           */
/* File    : WGDIPAL.C                                                       */
/*                                                                           */
/* Purpose : Palette manipulation routines                                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_PALETTE_MGR
#define USE_PALETTES
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"


LPINTLOGPALETTE PASCAL HPaletteToPtr(HPALETTE, HGLOBAL *);
static UINT PASCAL GetLogPaletteIndexEntry(LPINTLOGPALETTE, UINT);
static VOID FAR PASCAL XMEWELCreatePaletteColormap(HPALETTE);
static VOID FAR PASCAL XMEWELInstallPalette(HDC, HPALETTE, HPALETTE);

/*
  This variable defines a "base" palette. We will not muck with the hardware
  palette below the base palette.
*/
static int idxBasePalette = 16;


/****************************************************************************/
/*                                                                          */
/* Function : InitPaletteManager(void)                                      */
/*                                                                          */
/* Purpose  : Internal function to initialize the palette management.       */
/*            Things which need to be done :                                */
/*              1) Create a system palette                                  */
/*              2) Copy the current VGA palette into the system palette     */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
static BOOL bPaletteMgrInit = FALSE;

BOOL FAR PASCAL InitPaletteManager(void)
{
#if !defined(XWINDOWS)
  union REGS     r;
  struct SREGS   seg;
#endif
  LPLOGPALETTE   lpPal;
  LPPALETTEENTRY lppe;
  LPSTR          lpRGB, lpRGB2;
  INT            i;
  INT            nColors;

  if (bPaletteMgrInit)
    return TRUE;

  /*
    Palette management is only available for 256-color modes or above.
  */
  if ((nColors = SysGDIInfo.nColors) < 256)
    return FALSE;

  /*
    Watch out for 16-bit or 24-bit systems
  */
  if (nColors > 256)
    nColors = 256;

  /*
    Allocate memory for the logical palette.
  */
  lpPal = (LPLOGPALETTE) emalloc(sizeof(LOGPALETTE) + 
                             (nColors * sizeof(PALETTEENTRY)));
  if (lpPal == NULL)
    return FALSE;

  if ((lpRGB = emalloc(3 * nColors)) == NULL)
    return FALSE;

  bPaletteMgrInit = TRUE;

  /*
    The BasePalette entry in MEWEL.INI specifies whether we should leave
    the lower 16 colors alone.
  */
  idxBasePalette = GetProfileInt("boot", "BasePalette", 16);

  /*
    Get the hardware colors into the palette entries.
  */
#if defined(XWINDOWS)
  {
  /*
    The actual RGB values for the system palette get filled in the function
    FinishUpSystemPalette() which is called at the end of XMEWELCreateColors().
  */
  XColor xClr[256];
  for (i = 0;  i < nColors;  i++)
    xClr[i].pixel = i;
  XQueryColors(XSysParams.display, XSysParams.MEWELDefaultColormap, xClr, 
               nColors);

  lppe = lpPal->palPalEntry;
  for (i = 0;  i < nColors;  i++, lppe++)
  {
    lppe->peRed   = (BYTE) (xClr[i].red   >> 8);
    lppe->peGreen = (BYTE) (xClr[i].green >> 8);
    lppe->peBlue  = (BYTE) (xClr[i].blue  >> 8);
    lppe->peFlags = (BYTE) ((i < idxBasePalette) ? PC_RESERVED : 0);
  }
  }

#else
  segread(&seg);
  r.x.ax = 0x1017;
  r.x.bx = 0;
  r.x.cx = nColors;
  seg.es = FP_SEG(lpRGB);
  r.x.dx = FP_OFF(lpRGB);
  int86x(0x10, &r, &r, &seg);

  lppe = lpPal->palPalEntry;
  lpRGB2 = lpRGB;
  for (i = 0;  i < nColors;  i++, lppe++)
  {
    lppe->peRed   = (*lpRGB2++ >> 2);
    lppe->peGreen = (*lpRGB2++ >> 2);
    lppe->peBlue  = (*lpRGB2++ >> 2);
    if (i < idxBasePalette)
      lppe->peFlags = PC_RESERVED;
  }
#endif


  /*
    Create the default system palette. This will be referenced by the
    system as the DEFAULT_PALETTE stock object.
  */
  lpPal->palVersion    = 0x300;
  lpPal->palNumEntries = nColors;
  SysGDIInfo.hSysPalette = CreatePalette(lpPal);

  MyFree(lpRGB);
  MyFree(lpPal);
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : CreatePalette(LPLOGPALETTE lpLogPal)                          */
/*                                                                          */
/* Purpose  : Creates a logical palette GDI object.                         */
/*                                                                          */
/* Returns  : The handle of the palette object, or NULL if not created.     */
/*                                                                          */
/****************************************************************************/
HPALETTE FAR PASCAL CreatePalette(lpLogPal)
  CONST LOGPALETTE FAR *lpLogPal;
{
  HANDLE   hObj;
  LPOBJECT lpObj;
  HPALETTE hPal;
  LPSTR    lpMem;
  LPPALINDEX lpPalIndex;
  UINT     wSize;
  INT      i, nEntries;


  InitPaletteManager();


  /*
    Allocate an empty GDI object
  */
  if ((hObj = _ObjectAlloc(OBJ_PALETTE)) == BADOBJECT)
    return (HPALETTE) NULL;
  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HPALETTE) NULL;

  /*
    Get the number of colors into a local var
  */
  nEntries = lpLogPal->palNumEntries;

  /*
    Allocate memory for the logical palette. Then copy the passed palette 
    data over to the new memory.
  */
  wSize = sizeof(INTLOGPALETTE) +  ((nEntries-1) * sizeof(INTPALETTEENTRY)) +
                                   (nEntries * sizeof(PALINDEX));
  if ((hPal=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (DWORD)wSize)) == NULL ||
      (lpMem = GlobalLock(hPal)) == NULL)
  {
    if (hPal)
      GlobalFree(hPal);
    DeleteObject(hObj);
    return (HPALETTE) NULL;
  }

  /*
    Copy the LOGPALETTE header and the entries
  */
  lmemcpy(lpMem, (LPSTR) lpLogPal, 
          sizeof(INTLOGPALETTE) + (nEntries * sizeof(INTPALETTEENTRY)));

  /*
    Set the identity palette index
  */
  lpPalIndex = (LPPALINDEX) (&((LPINTLOGPALETTE)lpMem)->palPalEntry[nEntries]);
  for (i = 0;  i < nEntries;  i++)
    *lpPalIndex++ = i;

  GlobalUnlock(hPal);

  /*
    The GDI header points to the palette's memory block
  */
  lpObj->uObject.uhPalette = hPal;

#if defined(XWINDOWS)
  XMEWELCreatePaletteColormap(hObj);
#endif

  return (HPALETTE) hObj;
}


/****************************************************************************/
/*                                                                          */
/* Function : ResizePalette(HPALETTE hPal, UINT nEntries)                   */
/*                                                                          */
/* Purpose  : Resizes the logical palette.                                  */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL ResizePalette(hPal, nEntries)
  HPALETTE hPal;
  UINT     nEntries;
{
  LPOBJECT lpObj;
  HANDLE   hMem;
  LPINTLOGPALETTE lpLogPal;


  /*
    If we are doing a SelectPalette(hDC, hOldPal), and hOldPal is the
    system palette (NULL), then use the default palette.
  */
  if (hPal == NULL)
    hPal = SysGDIInfo.hSysPalette;

  /*
    Get the handle of the palette's memory block
  */
  if ((lpObj = _ObjectDeref(hPal)) == NULL)
    return FALSE;
  hMem = lpObj->uObject.uhPalette;


  /*
    Resize the palette's memory block.
  */
  hMem = GlobalReAlloc(hMem,
               (DWORD)(sizeof(INTLOGPALETTE) + nEntries*sizeof(INTPALETTEENTRY) +
                       nEntries*sizeof(PALINDEX)),
               GMEM_MOVEABLE | GMEM_ZEROINIT);
  if (hMem == NULL)
    return FALSE;

  /*
    Set the new number of palette entries.
  */
  if ((lpLogPal = (LPINTLOGPALETTE) GlobalLock(hMem)) != NULL)
  {
    lpLogPal->palNumEntries = nEntries;
    GlobalUnlock(hMem);
  }


  lpObj->uObject.uhPalette = hMem;
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : Get/SetSystemPaletteUse(hDC, fuStatic)                        */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL SetSystemPaletteUse(hDC, fuStatic)
  HDC  hDC;
  UINT fuStatic;
{
  UINT  fOld;
  LPHDC lphDC = _GetDC(hDC);

  fOld = (lphDC->fFlags & DC_SYSPALNOSTATIC) ? SYSPAL_NOSTATIC : SYSPAL_STATIC;
  if (fuStatic == SYSPAL_NOSTATIC)
    lphDC->fFlags |=  DC_SYSPALNOSTATIC;
  else
    lphDC->fFlags &= ~DC_SYSPALNOSTATIC;
  return fOld;
}

UINT FAR PASCAL GetSystemPaletteUse(hDC)
  HDC  hDC;
{
  LPHDC lphDC = _GetDC(hDC);
  return (lphDC->fFlags & DC_SYSPALNOSTATIC) ? SYSPAL_NOSTATIC : SYSPAL_STATIC;
}


/****************************************************************************/
/*                                                                          */
/* Function : Get/SetPaletteEntries(hPal, nStart, nEntries, lpPalEntries)   */
/*                                                                          */
/* Purpose  : Sets and retrieves palette entries from a logical palette.    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
static UINT PASCAL GetOrSetPaletteEntries(HPALETTE,UINT,UINT,LPPALETTEENTRY,
                                          BOOL,BOOL);

static UINT PASCAL GetOrSetPaletteEntries(
  HPALETTE hPal,      /* or a DC if bSystemPalette is TRUE */
  UINT     iStart,
  UINT     cEntries,
  LPPALETTEENTRY lppe,
  BOOL     bSet,
  BOOL     bSystemPalette)
{
  HANDLE   hMem;
  LPINTLOGPALETTE lpLogPal;

  InitPaletteManager();

  /*
    See if we need to reference the system palette.
  */
  if (bSystemPalette)
    hPal = SysGDIInfo.hSysPalette;

  /*
    Get a pointer to the palette.
  */
  if ((lpLogPal = HPaletteToPtr(hPal, &hMem)) != NULL)
  {
    /*
      Point to the proper place within the palette to start
    */
    LPINTPALETTEENTRY lppeDest = &lpLogPal->palPalEntry[iStart];

    /*
      Insure we don't overflow the buffer.
    */
    cEntries = min(cEntries, lpLogPal->palNumEntries - iStart);

    /*
      Copy the entries
    */
    if (bSet)
      lmemcpy((LPSTR) lppeDest, (LPSTR) lppe, sizeof(PALETTEENTRY) * cEntries);
    else
      lmemcpy((LPSTR) lppe, (LPSTR) lppeDest, sizeof(PALETTEENTRY) * cEntries);

    GlobalUnlock(hMem);
    return cEntries;
  }
  else
    return 0;
}


UINT FAR PASCAL SetPaletteEntries(hPal, iStart, cEntries, lppe)
  HPALETTE hPal;
  UINT     iStart;
  UINT     cEntries;
  CONST PALETTEENTRY FAR *lppe;
{
  return GetOrSetPaletteEntries(hPal, iStart, cEntries,
                                (LPPALETTEENTRY) lppe,
                                TRUE, FALSE);
}

UINT FAR PASCAL GetPaletteEntries(hPal, iStart, cEntries, lppe)
  HPALETTE hPal;
  UINT     iStart;
  UINT     cEntries;
  LPPALETTEENTRY lppe;
{
  return GetOrSetPaletteEntries(hPal, iStart, cEntries, lppe, FALSE, FALSE);
}

UINT FAR PASCAL GetSystemPaletteEntries(hDC, iStart, cEntries, lppe)
  HDC      hDC;
  UINT     iStart;
  UINT     cEntries;
  LPPALETTEENTRY lppe;
{
  return GetOrSetPaletteEntries(hDC, iStart, cEntries, lppe, FALSE, TRUE);
}


/****************************************************************************/
/*                                                                          */
/* Function : UpdateColors(hDC)                                             */
/*                                                                          */
/* Purpose  : Redraws the client area of the given DC by matching each      */
/*            pixel to the system palette.                                  */
/*                                                                          */
/* Returns  : Not used.                                                     */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL UpdateColors(hDC)
  HDC hDC;
{
  LPHDC lphDC;
  INT   x, y;
  INT   iWidth, iHeight;
  
  /*
    Get the height and width of the window's client area
  */
  lphDC = _GetDC(hDC);
  iWidth  = lphDC->rClipping.right  - lphDC->rClipping.left;
  iHeight = lphDC->rClipping.bottom - lphDC->rClipping.top;

  /*
    Go through each pixel, row by row
  */
  for (y = 0;  y < iHeight;  y++)
  {
    for (x = 0;  x < iWidth;  x++)
    {
      /*
        Map the current pixel to the nearest system palette entry.
        The set the pixel to that color.
      */
      UINT     idxClr;
      COLORREF clr = GetPixel(hDC, x, y);
      idxClr = GetNearestPaletteIndex(SysGDIInfo.hSysPalette, clr);
      SetPixel(hDC, x, y, PALETTEINDEX(idxClr));
    }
  }

  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : SelectPalette(hDC, hPal, bBackground)                         */
/*                                                                          */
/* Purpose  : Selects a logical palette into a DC.                          */
/*                                                                          */
/* Returns  : A handle to the previously selected palette.                  */
/*                                                                          */
/****************************************************************************/
HPALETTE FAR PASCAL SelectPalette(HDC hDC, HPALETTE hPal, BOOL bBackground)
{
  HPALETTE hOldPal;

  (void) bBackground;  /* not used */

  hOldPal = SelectObject(hDC, hPal);
  return hOldPal;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetNearestPaletteIndex(hPal, clrref)                          */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : The index of the palette entry which is the closest match.    */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL GetNearestPaletteIndex(HPALETTE hPal, COLORREF clrref)
{
  HANDLE   hMem;
  LPINTLOGPALETTE lpLogPal;

  InitPaletteManager();

  /*
    Get a pointer to the palette.
  */
  if ((lpLogPal = HPaletteToPtr(hPal, &hMem)) != NULL)
  {
    LPINTPALETTEENTRY lpEntry = lpLogPal->palPalEntry;
    UINT nEntries = lpLogPal->palNumEntries;

    /*
      Get the components of the passed color
    */
    BYTE chRed   = GetRValue(clrref);
    BYTE chBlue  = GetBValue(clrref);
    BYTE chGreen = GetGValue(clrref);
  
    UINT minDistance = 0xFF * 3;  /* worst match is 255 * 3 */
    UINT idxBest = 0;
    UINT idx;

    /*
      Go through all of the colors in the palette
    */
    for (idx = 0;  idx < nEntries;  idx++, lpEntry++)
    {
      /*
        Euclidean distance forumla.....
          diffRed^2 + diffBlue^2 + diffGreen^2
      */
      UINT distance = (chRed-lpEntry->peRed)     * (chRed-lpEntry->peRed) +
                      (chBlue-lpEntry->peBlue)   * (chBlue-lpEntry->peBlue) +
                      (chGreen-lpEntry->peGreen) * (chGreen-lpEntry->peGreen);

      /*
        If we found a better match, then record the index of the best match
      */
      if (distance < minDistance)
      {
        idxBest = idx;
        minDistance = distance;
      }
    }

    GlobalUnlock(hMem);
    return idxBest;
  }

  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : RealizePalette(hDC)                                           */
/*                                                                          */
/* Purpose  : The device context containing the logical palette.            */
/*                                                                          */
/* Returns  : The number of entries in the logical palette which were       */
/*            mapped to different entries in the system palette.            */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL EngineSetPalette(UINT, BYTE, BYTE, BYTE, BYTE);
static UINT PASCAL RealizePalette2(HDC, HPALETTE, BOOL);

UINT FAR PASCAL RealizePalette(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;
  return RealizePalette2(hDC, lphDC->hPalette, FALSE);
}


static UINT PASCAL RealizePalette2(HDC hDC, HPALETTE hPal, BOOL bAnimating)
{
  HANDLE       hMem;
  LPINTLOGPALETTE lpLogPal;

#if defined(XWINDOWS)
  Colormap cmap = XSysParams.MEWELDefaultColormap;
  BOOL bPrivateColormap = FALSE;
  XColor xclr;
  LPHDC  lphDC = _GetDC(hDC);

  xclr.flags = DoRed | DoGreen | DoBlue;

  /*
    If we use the private colormap, then the colors will be displayed
    correctly, but the entire display changes.
  */
  if (XSysParams.ulOptions & XOPT_USEPRIVATECMAPWITHPALETTE)
    cmap = (Colormap) _ObjectDeref(hPal)->lpExtra;

  /*
    Are we using a private colormap?
  */
  bPrivateColormap = (cmap != NULL && 
                      cmap != DefaultColormapOfScreen(XSysParams.screen));

  if (bPrivateColormap)
    XMEWELInstallPalette(hDC, hPal, 0);
#endif

  /*
    Get a pointer to the palette.
  */
  if ((lpLogPal = HPaletteToPtr(hPal, &hMem)) != NULL)
  {
    LPINTPALETTEENTRY lpEntry = lpLogPal->palPalEntry;
    UINT nEntries = lpLogPal->palNumEntries;
    UINT idx;
    LPPALINDEX lpPalIndex = (LPPALINDEX) (&lpLogPal->palPalEntry[nEntries]);

    /*
      Perform the palette matching if we are not animating.
      KLUDGE - we should really do the matching against the system palette.
    */
    if (!bAnimating)
    {
      for (idx = 0;  idx < nEntries;  idx++, lpEntry++)
      {
        if (!(lpEntry->peFlags & PC_EXPLICIT))
          if (idx + idxBasePalette < (UINT) SysGDIInfo.nColors)
            *lpPalIndex++ = idx + idxBasePalette;
          else
            *lpPalIndex++ = idx;
      }
      lpEntry = lpLogPal->palPalEntry;
      lpPalIndex = (LPPALINDEX) (&lpLogPal->palPalEntry[nEntries]);
    }

    /*
      Set the hardware palette
    */
    for (idx = 0;  idx < nEntries;  idx++, lpEntry++, lpPalIndex++)
    {
      if (!(lpEntry->peFlags & PC_EXPLICIT))
      {
#if defined(XWINDOWS)
        xclr.pixel = *lpPalIndex;
        xclr.red   = (((USHORT) lpEntry->peRed)   << 8);
        xclr.green = (((USHORT) lpEntry->peGreen) << 8);
        xclr.blue  = (((USHORT) lpEntry->peBlue)  << 8);
        if (bPrivateColormap)
        {
          XStoreColor(XSysParams.display, cmap, &xclr);
          *lpPalIndex = xclr.pixel;
        }
        else if (XAllocColor(XSysParams.display, cmap, &xclr) != 0)
          *lpPalIndex = xclr.pixel;
        else
          *lpPalIndex = GetNearestPaletteIndex(SysGDIInfo.hSysPalette, RGB(lpEntry->peRed, lpEntry->peGreen, lpEntry->peBlue));
#else
        EngineSetPalette(*lpPalIndex, lpEntry->peRed, lpEntry->peGreen,
                         lpEntry->peBlue, lpEntry->peFlags);
#endif
      }
    }

    GlobalUnlock(hMem);
    return nEntries;
  }

  return 0;
}


#if !defined(XWINDOWS)
static VOID PASCAL EngineSetPalette(UINT index, BYTE chRed, BYTE chGreen, 
                                    BYTE chBlue, BYTE chFlags)
{
  /*
    Kludge - do not muck with the lower 16 colors unless we are doing
    palette animation.
  */
  if ((INT) index < idxBasePalette && !(chFlags & PC_RESERVED))
    return;



#if defined(META)

#elif defined(GX)
  /*
    Use only the lower 6 bits.
  */
#if 0  /* if 0'ed cause gxSetPalette is slow! */
  chRed   =  (BYTE) ((chRed   >> 2) & 0x3F);
  chGreen =  (BYTE) ((chGreen >> 2) & 0x3F);
  chBlue  =  (BYTE) ((chBlue  >> 2) & 0x3F);
  gxSetPaletteRGB(index, chRed, chGreen, chBlue);
#else
  outp(0x3C6, 0xFF);
  outp(0x3C8, (BYTE) index);
  outp(0x3C9, (BYTE) ((chRed   >> 2) & 0x3F));
  outp(0x3C9, (BYTE) ((chGreen >> 2) & 0x3F));
  outp(0x3C9, (BYTE) ((chBlue  >> 2) & 0x3F));
#endif

#elif defined(MSC)
  /*
    Use only the lower 6 bits.
  */
  chRed   =  (BYTE) ((chRed   >> 2) & 0x3F);
  chGreen =  (BYTE) ((chGreen >> 2) & 0x3F);
  chBlue  =  (BYTE) ((chBlue  >> 2) & 0x3F);
  _remappalette(index, RGB(chRed, chGreen, chBlue));

#elif defined(BGI_FOO) || defined(__DPMP32__)
  setrgbpalette(index, (int) chRed, (int) chGreen, (int) chBlue);

#else
#if USE_BIOS
  union REGS r;
  r.x.ax = 0x1010;
  r.x.bx = index;
  r.h.ch = chGreen;
  r.h.cl = chBlue;
  r.h.dh = chRed;
  int86(0x10, &r, &r);
#else
  outp(0x3C6, 0xFF);
  outp(0x3C8, (BYTE) index);
  outp(0x3C9, (BYTE) ((chRed   >> 2) & 0x3F));
  outp(0x3C9, (BYTE) ((chGreen >> 2) & 0x3F));
  outp(0x3C9, (BYTE) ((chBlue  >> 2) & 0x3F));
#endif

#endif

}
#endif



/****************************************************************************/
/*                                                                          */
/* Function : AnimatePalette(hPal, iStart, cEntries, lpPal)                 */
/*                                                                          */
/* Purpose  : Changes the palette entries in the hardware palette.          */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL AnimatePalette(hPal, iStart, cEntries, lpPal)
  HPALETTE hPal;
  UINT     iStart;
  UINT     cEntries;
  CONST PALETTEENTRY FAR *lpPal;
{
  SetPaletteEntries(hPal, iStart, cEntries, lpPal);
  RealizePalette2(0, hPal, TRUE);
}


/****************************************************************************/
/*                                                                          */
/*                   Utility Functions                                      */
/*                                                                          */
/****************************************************************************/
LPINTLOGPALETTE PASCAL HPaletteToPtr(HPALETTE hPal, HGLOBAL *phMem)
{
  LPOBJECT lpObj;
  LPINTLOGPALETTE lpLogPal;

  /*
    Get the handle of the memory block that holds the palette
  */
  if (hPal != NULL && (lpObj = _ObjectDeref(hPal)) != NULL)
  {
    HGLOBAL hMem = lpObj->uObject.uhPalette;

    /*
      Get a pointer to the palette.
    */
    if ((lpLogPal = (LPINTLOGPALETTE) GlobalLock(hMem)) != NULL)
    {
      if (phMem)
        *phMem = hMem;
      return lpLogPal;
    }
  }

  return NULL;
}

static UINT PASCAL GetLogPaletteIndexEntry(LPINTLOGPALETTE lpLogPal, UINT idxDest)
{
  /*
    Point to the palette index table. It follows the last palette entry.
  */
  LPPALINDEX lpPalIndex =
               (LPPALINDEX) (&lpLogPal->palPalEntry[lpLogPal->palNumEntries]);
  return lpPalIndex[idxDest];
}



#if defined(XWINDOWS)
/*
  XMEWELCreatePaletteColormap()
    This is called from CreatePalette() in order to create an XWindows
    Colormap which corresponds to the colors in palette hPal. A pointer
    to the colormap is stored in the palette's lpObj->lpExtra variable.
    Initially, the colormap is initialized with the current XWindows
    system colors.
*/
static VOID FAR PASCAL XMEWELCreatePaletteColormap(HPALETTE hPal)
{
  XColor xClr[256];
  LPOBJECT lpObj;
  Colormap cMap;
  int      i, nColors;

  if ((lpObj = _ObjectDeref(hPal)) == NULL)
    return;

  /*
    Watch out for 16-bit and 24-bit systems
  */
  if ((nColors = SysGDIInfo.nColors) > 256)
    nColors = 256;

  /*
    Create a color map which is associated with this palette and stick it in
    the lpObj->lpExtra field.
  */
  if (!(XSysParams.ulOptions & XOPT_USEPRIVATECMAPWITHPALETTE))
    return;

  lpObj->lpExtra = (LPVOID) XCreateColormap(XSysParams.display,
            RootWindow(XSysParams.display, XSysParams.iScreen),
            XSysParams.visual, AllocAll);
  cMap = (Colormap) lpObj->lpExtra;

  /*
    Insitially copy the colors from the system colormap to the new map
  */
  for (i = 0;  i < nColors;  i++)
  {
    xClr[i].pixel = i;
    xClr[i].flags = DoRed | DoGreen | DoBlue;
  }
  XQueryColors(XSysParams.display, XSysParams.MEWELDefaultColormap, xClr, 
               nColors);
  XStoreColors(XSysParams.display, cMap, xClr, nColors);
}


/*
  XMEWELInstallPalette()
    This is called from RealizePalette2(). It takes the XWindows colormap
    which is attached to 'hPal' and installs it as the current colormap.
*/
VOID FAR PASCAL XMEWELInstallPalette(HDC hDC, HPALETTE hPal, HPALETTE hOldPal)
{
  LPHDC    lphDC = _GetDC(hDC);
  LPOBJECT lpObj;
  Colormap cMap;

  if (lphDC == NULL)
    return;

  /*
    First process the old colormap
  */
  if (hOldPal && (lpObj = _ObjectDeref(hOldPal)) != NULL)
  {
    if ((lphDC->fFlags & DC_CMAPINSTALLED))
    {
#if 0
      if ((cMap = (Colormap) lpObj->lpExtra) != NULL)
        XUninstallColormap(XSysParams.display, cMap);
#endif
      lphDC->fFlags &= ~DC_CMAPINSTALLED;
    }
  }

  /*
    Process new palette
  */
  if (hPal && (lpObj = _ObjectDeref(hPal)) != NULL)
  {
    if ((cMap = (Colormap) lpObj->lpExtra) != NULL)
    {
      XSysParams.CurrentColormap = cMap;
      if (cMap != DefaultColormapOfScreen(XSysParams.screen))
        XInstallColormap(XSysParams.display, cMap);
      lphDC->fFlags |= DC_CMAPINSTALLED;
      if (lphDC->drawable)
        XSetWindowColormap(XSysParams.display, lphDC->drawable, cMap);
    }
  }
}

VOID FAR PASCAL XMEWELFreePalette(LPVOID lpObj)
{
  Colormap cMap;

  /*
    Get a pointer to the XWindows colormap and free it.
  */
  if ((cMap = (Colormap) ((LPOBJECT) lpObj)->lpExtra) != NULL)
  {
    XUninstallColormap(XSysParams.display, cMap);
    XFreeColormap(XSysParams.display, cMap);
    if (cMap == XSysParams.CurrentColormap)
    {
      XSysParams.CurrentColormap = NULL;
    }
  }
}

#endif

