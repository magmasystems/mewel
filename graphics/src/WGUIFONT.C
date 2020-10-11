/*===========================================================================*/
/*                                                                           */
/* File    : WGUIFONT.C                                                      */
/*                                                                           */
/* Purpose : Font handling functions for MEWEL/GUI                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_FONTS

#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if defined(FLEMING)
#include "graphadd.h"
#endif

static BOOL PASCAL AssociateFontResource(PCSTR, PSTR, INT, LPVOID);
extern VOID PASCAL ReadINIFontInfo(HDC);
extern VOID FAR PASCAL MEWELInitSystemFont(HDC);
static VOID PASCAL TMCalcExternalLeading(LPTEXTMETRIC);

/*
  These two strings are used for testing the avg char width and height
*/
static char *pszWidthTestString  = "ABCabc";
static char *pszHeightTestString = "Hg";

/*
  Test to determine if a MetaWindows font is a stroked font. If so, then
  we need to call mwTextSize before using mwStringWidth.
*/
#if defined(META)
#define IS_STROKED_FONT(f)  ((f)->fontFlags & 0x07)
#if !defined(EXTENDED_DOS)
static fontRcd FAR* PASCAL MetaFontHandleToPtr(HFONT);
#endif
#endif

#if defined(MSC)
/*
  The default system typeface name for Microsoft graphics libs
*/
static LPSTR MSCSystemFontName = "Courier";
#endif

/*
  This BGI constant tells us whether we should compensate for the
  descenders in stroked fonts under BGI.
*/
#if defined(BGI) && !defined(METABGI)
#define BGI_STROKED_HEIGHT_KLUDGE
#endif

/*
  Special font-setting function for GX/Text
*/
#if defined(GX) && defined(USE_GX_TEXT)
static VOID PASCAL GXSetFont(TXHEADER FAR *);
#endif


/*
  Important global vars
*/
static BOOL bReadINI = FALSE;
static int  cyHeightWanted = 0, cxWidthWanted = 0;
LIST *MEWELFontNameList = NULL;

#if defined(MSC)
static char oldszParams[32];  /* string "cache" for font mapping */
#endif

/*
  TRUE if the default font is proportional
*/
static BOOL bDefaultFontProportional = FALSE;


/****************************************************************************/
/*                                                                          */
/* Function : AssociateFontResource()                                       */
/*                                                                          */
/* Purpose  : Create a FONTNAMEINFO structure to hold some info about a     */
/*            logical font. The info includes the logical font name,        */
/*            the path name of the font file, a pointer to the font data    */
/*            (for MetaWindows), and a handle to the font memory.           */
/*                                                                          */
/* Returns  : TRUE if font added, FALSE if not.                             */
/*                                                                          */
/* Called by: AddFontResource() and ReadINIFontInfo()                       */
/*                                                                          */
/****************************************************************************/
static BOOL PASCAL AssociateFontResource(pszFontName, pszPath, hFont, lpFontBuf)
  PCSTR  pszFontName; /* logical font name */
  PSTR   pszPath;     /* pathname of font file */
  INT    hFont;       /* NULL when called from ReadINIFontInfo */
  LPVOID lpFontBuf;   /* NULL when called from ReadINIFontInfo */
{
  LPFONTNAMEINFO lpFNI;

  /*
    See if the font file exists. If it doesn't, return.

    For Microsoft graphics, 'pszPath' can be the logical font name and not
    the path name of the FON file. So don't try to see if the font exists.

    For Pharlap and BGI, since we cannot load fonts, just associate the
    logical name with the physical name
        ie :  Helv=litt
  */
  if (
#if defined(MSC) || ((defined(DOS286X) || defined(__DPMI16__) || defined(__DPMI32__)) && defined(BGI))
      (lpFNI=(LPFONTNAMEINFO)EMALLOC_FAR_NOQUIT(sizeof(FONTNAMEINFO))) != NULL)
#else
      access(pszPath, 0) == 0 &&
      (lpFNI=(LPFONTNAMEINFO)EMALLOC_FAR_NOQUIT(sizeof(FONTNAMEINFO))) != NULL)
#endif
  {
    lpFNI->hFont      = hFont;
    lpFNI->lpFontBuf  = lpFontBuf;
    lpFNI->lpFontName = lstrsave((LPSTR) pszFontName);
    if (!lpFNI->lpPathName)
      lpFNI->lpPathName = lstrsave(pszPath);
    ListAdd(&MEWELFontNameList, ListCreate((LPSTR) lpFNI));
    return TRUE;
  }
  else
    return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : AddFontResource()                                             */
/*                                                                          */
/* Purpose  : Loads a font file in from disk.                               */
/*                                                                          */
/* Returns  : Handle of font memory if successful, 0 if not.                */
/*                                                                          */
/* Called by: ReadINIFontInfo() to add the pathname of the DefaultFont.     */
/*            RealizeFont() to load in the font.                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL AddFontResource(lpFileName)
  LPCSTR lpFileName;  /* pathname of font file (if bReadINI is FALSE),
                         or logical font name (if bReadINI is TRUE)
                      */
{
  char   szPath[MAXPATH+1];
  int    rc;
  HANDLE hFont;
  LPFONTNAMEINFO lpFNI;
  LIST   *pList;
  BOOL   bReloadedFont = FALSE;

#if defined(META)
  fontRcd *f;
  DWORD   ulLen;
  int     fd;
#endif

  LPVOID lpFontBuf = NULL;


  /*
    We test if the font is already loaded into memory
  */
  for (pList = MEWELFontNameList;  pList;  pList = pList->next)
  {
    LPSTR lpsz;

    lpFNI = (LPFONTNAMEINFO) pList->data;

    /*
      If called from RealizeFont, then use the logical font name, or
      else, use the name of the font file.
    */
    lpsz = (LPSTR) (bReadINI ? lpFNI->lpFontName : lpFNI->lpPathName);

    if (!lstricmp(lpsz, lpFileName))
    {
      if (lpFNI->hFont == NULL)
        bReloadedFont = TRUE;
      else
        return lpFNI->hFont;
      break;
    }
  }

  /*
    NOTE:

    We will hit this part of the code only when we are called from
    RealizeFont. Therefore, lpFileName is the logical font name.
  */

  /*
    Search for the font file on the disk
  */
  if (bReloadedFont)
  {
    lstrcpy(szPath, lpFNI->lpPathName);
  }
  else
  {
    lstrcpy(szPath, lpFileName);
    if (lstrchr(szPath, '.') == NULL)
      lstrcat(szPath, FONT_EXTENSION);
    if (_DosSearchPath("PATH", szPath, szPath) == NULL)
      return 0;
  }


#if defined(META)
  /*
    Allocate global memory for the font
  */
  if ((fd = open(szPath, O_BINARY | O_RDONLY)) < 0)
    return 0;
  ulLen = filelength(fd);
  close(fd);

  if ((hFont = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE, ulLen)) == NULL)
    return 0;
  if ((f = (fontRcd *) GlobalLock(hFont)) == NULL)
    goto err;

  /*
    Load in the font file
  */
  rc = mwFileLoad(szPath, f, (UINT) ulLen);

  /*
    Unlock the memory and return the handle to the font.
    We must dereference this handle in calls to RealizeFont().
  */
#if !defined(EXTENDED_DOS)
  /*
    For MetaWindows real mode, we must keep the font in conventional
    memory for mwSetFont to work.
  */
  if (rc > 0)
    lpFNI->lpFontBuf = lpFontBuf = f;
  else
#endif
  GlobalUnlock(hFont);
  if (rc <= 0)
  {
err:
    GlobalFree(hFont);
    return 0;
  }

#elif defined(GX)
#if defined(USE_GX_TEXT)
  {
  TXHEADER *lpTX;

  if ((hFont = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(TXHEADER))) == NULL)
    return 0;
  if ((lpTX = (TXHEADER *) GlobalLock(hFont)) == NULL)
    goto err;

  if ((rc = txFileFont(gxCMM, szPath, lpTX)) != txSUCCESS)
    rc = txRomFont(gxCMM, txTXT8X14, lpTX);

  GlobalUnlock(hFont);
  if (rc != txSUCCESS)
  {
err:
    GlobalFree(hFont);
    return 0;
  }
  }
#else
  (void) lpFileName;
  return 0;
#endif


#elif defined(GURU)
  (void) lpFileName;
  return 0;

#elif defined(MSC)
  /*
    lpFileName must be the path name where the font files are located.
    lpFileName can be "*.FON".
  */
  (void) hFont;
  (void) lpFileName;
  if ((rc = _registerfonts(szPath)) < 0)
    return 0;
  else
    return 1;

#elif defined(BGI)
  /*
    BGI uses installuserfont to load a font file into memory. It 
    returns a handle to the font which is used by settextstyle().
    Note :
      It seems that installuserfont/settextstyle needs the font name
    in the DS, not the SS, so we must pass a DS-based address to
    installuserfont.
  */
  if (!bReloadedFont)
    lpFNI->lpPathName = lstrsave(szPath);

#if defined(DOS286X) || defined(__DPMI16__) || defined(__DPMI32__)
  /*
    Since the BGI .CHR file is actually a small real-mode program,
    we cannot use registerbgifont() in protected mode. Therefore,
    just do a small table lookup of the font name and the font id.
  */
  {

  /*
    These names should be on the right-hand-side of the
      logfont=physfont
    entry in the MEWEL.INI file

    For example :

      MS Sans Serif=Simplex
  */
  static PSTR pszBGIFonts[] =
  {
    "8x8",
    "tms rmn",
    "helv",
    "sans serif",
    "gothic",
    "script",
    "simplex",
    "triplex",
    "complex",
    "euro",
    "bold",
  };
  /*
    Cache the index of the last successful match.
  */
  static int idxLast = 0;

  int i;

  if (!lstricmp(lpFNI->lpPathName, pszBGIFonts[idxLast]))
    return lpFNI->hFont = i;

  for (i = 0;  i < sizeof(pszBGIFonts)/sizeof(pszBGIFonts[0]);  i++)
  {
    if (!lstricmp(lpFNI->lpPathName, pszBGIFonts[i]))
      return lpFNI->hFont = idxLast = i;
  }
  return 0;

  }

#else
  {
  int   fd;
  long  fLen;
  LPSTR lpBuf;

  if ((fd = _lopen(lpFNI->lpPathName, READ)) < 0)
    return 0;

#if defined(__GNUC__)
  lseek(fd, 0L, 2);
  fLen = tell(fd);
  lseek(fd, 0L, 0);
#else
  fLen = filelength(fd);
#endif

  if ((lpBuf = EMALLOC_FAR_NOQUIT(fLen)) == NULL)
  {
    _lclose(fd);
    return 0;
  }
  _lread(fd, lpBuf, (unsigned int) fLen);
  _lclose(fd);

#if defined(__DPMI16__) || defined(__DPMI32__)
  if (((int) (hFont = registerfarbgifont( (void (*)(void)) lpBuf))) < 0)
#else
  if (((int) (hFont = registerfarbgifont(lpBuf))) < 0)
#endif
    return 0;
  }
#endif
#endif

  /*
    If we merely reloaded the font from disk, return the font handle.
  */
  if (bReloadedFont)
    return lpFNI->hFont = hFont;

  /*
    Allocate a font-path-handle triplet in the font name info structure
  */
  if (AssociateFontResource(lpFileName, szPath, hFont, lpFontBuf))
    return hFont;
  else
    return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : RemoveFontResource()                                          */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/* Called by: DeleteObject()                                                */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL RemoveFontResource(lpFileName)
  LPCSTR lpFileName;  /* logical font name, not the file name as in Windows */
{
  LPFONTNAMEINFO lpFNI;
  LIST   *pList;

  for (pList = MEWELFontNameList;  pList;  pList = pList->next)
  {
    lpFNI = (LPFONTNAMEINFO) pList->data;
    if (!lstricmp(lpFNI->lpFontName, lpFileName))
      break;
  }
  if (!pList)
    return 0;

#if defined(META)
  if ((HFONT) lpFNI->hFont != SysGDIInfo.hFontDefault)
  {
#if !defined(EXTENDED_DOS)
    /*
      For MetaWindows real mode, we must keep the font in conventional
      memory for mwSetFont to work.
    */
    if (lpFNI->lpFontBuf)
      GlobalUnlock(lpFNI->hFont);
#endif
    GlobalFree(lpFNI->hFont);
    lpFNI->hFont     = NULL;
    lpFNI->lpFontBuf = NULL;
  }
  return 0;


#elif defined(GX)
#if defined(USE_GX_TEXT)
  if (lpFNI->hFont != SysGDIInfo.hFontDefault && lpFNI->hFont)
  {
    TXHEADER *lpTX;
    if ((lpTX = (TXHEADER *) GlobalLock(lpFNI->hFont)) != NULL)
    {
      txFreeFont(lpTX);
      GlobalUnlock(lpFNI->hFont);
    }
    GlobalFree(lpFNI->hFont);
    lpFNI->hFont = NULL;
  }
  return 0;
#else
  (void) lpFileName;
  return 0;
#endif

#elif defined(GURU)
  (void) lpFileName;
  return 0;

#elif defined(MSC)
  /*
    We can't use _unregisterfonts() because that function removes *all*
    font information which was registered by _registerfonts().
  */
  (void) lpFileName;
  return 0;

#elif defined(BGI)
  /*
    BGI does not have any way to unload the font.
  */
  (void) lpFileName;
  return 0;

#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : _GGetTextMetrics()                                            */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
extern TEXTMETRIC DefaultTextMetrics;


BOOL FAR PASCAL _GGetTextMetrics(hDC, lpMetrics)
  HDC hDC;
  LPTEXTMETRIC lpMetrics;
{
#if defined(META)
#elif defined(GX) || defined(GURU) || defined(GFX)
#elif defined(MSC)
  struct _fontinfo fontbuf;
#elif defined(BGI)
  struct textsettingstype fontbuf;
#endif

#if 0
  if ((_GetDC(hDC)) == NULL)
    return 0;
#else
  (void) hDC;
#endif

  if (!lpMetrics)
    return FALSE;

  lmemcpy((LPSTR) lpMetrics, (LPSTR) &DefaultTextMetrics, sizeof(TEXTMETRIC));

#if defined(META)
  {
  grafPort *gp;
  fontRcd  *f;

  mwGetPort(&gp);
  f = gp->txFont;

  lpMetrics->tmFirstChar       = (BYTE) f->fontMin;
  lpMetrics->tmLastChar        = (BYTE) f->fontMax;
  lpMetrics->tmPitchAndFamily  = (BYTE) f->fontfamily;
  lpMetrics->tmAscent          = f->ascent;
  lpMetrics->tmDescent         = f->descent;

  /*
    maa
      If this code is done, then bitmap fonts are the correct width,
      but stroked fonts are not.

      For a ROMANSIM font for which we called mwTextSize(6, 8), the results are:
        f->chWidth is 22,
        f->chHeight is 32
        mwStringWidth(a) is 4
        gp->txSize.X is 6
        gp->txSize.y is 8

      For a DYNA08 font for which we called mwTextSize(6, 8), the results are:
        f->chWidth is 10,
        f->chHeight is 12
        mwStringWidth(a) is 7
        gp->txSize.X is 6
        gp->txSize.y is 8

      For normal bitmaps fonts which we don't call mwTextSize on,
      gp->txSize.X and gp->txSize.Y are both 0.
  */
  if (IS_STROKED_FONT(f))  /* TextSize only works with stroked fonts */
  {
    lpMetrics->tmAveCharWidth  = gp->txSize.X;
    lpMetrics->tmMaxCharWidth  = gp->txSize.X;
    lpMetrics->tmHeight        = gp->txSize.Y;
  }
  else
  {
    lpMetrics->tmAveCharWidth  = mwStringWidth(pszWidthTestString) / 6;
    lpMetrics->tmMaxCharWidth  = f->chWidth;
    lpMetrics->tmHeight        = f->chHeight;
  }

  TMCalcExternalLeading(lpMetrics);
  }

#elif defined(GX)
#if defined(USE_GX_TEXT)
  {
  LPHDC    lphDC;
  TXHEADER *lpTX;
  HANDLE   h;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if ((h = ((LPGXDCINFO) lphDC->lpDCExtra)->hTXHeader) == NULL)
    return FALSE;
  if ((lpTX = (TXHEADER FAR *) GlobalLock(h)) == NULL)
    return FALSE;
  GXSetFont(lpTX);
  lpMetrics->tmFirstChar       = lpTX->txminch;
  lpMetrics->tmLastChar        = lpTX->txmaxch;

#if defined(GX3)
  if (txGetType() == txFONTGST)
  {
    lpMetrics->tmHeight = HIWORD(_GetTextExtent(pszHeightTestString));
    TMCalcExternalLeading(lpMetrics);
    lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = 
      textwidth(pszWidthTestString) / 6;
  }
  else
#endif
  {
  lpMetrics->tmAscent          = lpTX->txascent;
  lpMetrics->tmDescent         = lpTX->txdescent;
  lpMetrics->tmAveCharWidth    = lpTX->txfwidth/(lpTX->txmaxch-lpTX->txminch+1);
  lpMetrics->tmMaxCharWidth    = lpTX->txmaxwidth;
  lpMetrics->tmHeight          = lpTX->txfheight;
  TMCalcExternalLeading(lpMetrics);
  }

  GlobalUnlock(h);
  }
#else
#if   GX_DEFAULT_FONT == grTXT8X8
  lpMetrics->tmHeight = 8;
#elif GX_DEFAULT_FONT == grTXT8X14
  lpMetrics->tmHeight = 14;
#elif GX_DEFAULT_FONT == grTXT8X16
  lpMetrics->tmHeight = 16;
#endif
  TMCalcExternalLeading(lpMetrics);
  lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = textwidth("M");
#endif


#elif defined(GFX)
  lpMetrics->tmHeight = 8;
  TMCalcExternalLeading(lpMetrics);
  lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = 8;


#elif defined(GURU)
#if   GURU_DEFAULT_FONT == ROM8x8
  lpMetrics->tmHeight = 8;
#elif GURU_DEFAULT_FONT == ROM8x14
  lpMetrics->tmHeight = 14;
#elif GURU_DEFAULT_FONT == ROM8x16
  lpMetrics->tmHeight = 16;
#endif
  TMCalcExternalLeading(lpMetrics);
  lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = textwidth("M");

#elif defined(MSC)
  if (_getfontinfo((struct _fontinfo FAR *) &fontbuf) < 0)
    return FALSE;
  lpMetrics->tmAscent = fontbuf.ascent;
  lpMetrics->tmHeight = fontbuf.pixheight;
  TMCalcExternalLeading(lpMetrics);
  lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = fontbuf.avgwidth;

#elif defined(BGI)
  lpMetrics->tmHeight = HIWORD(_GetTextExtent(pszHeightTestString));
  TMCalcExternalLeading(lpMetrics);
  lpMetrics->tmAveCharWidth = lpMetrics->tmMaxCharWidth = 
    textwidth(pszWidthTestString) / 6;

#endif

  return TRUE;
}


static VOID PASCAL TMCalcExternalLeading(LPTEXTMETRIC lpMetrics)
{
#if 51495
  lpMetrics->tmExternalLeading = 0;
#else
  if (lpMetrics->tmHeight < 16)
    lpMetrics->tmExternalLeading = 16 - lpMetrics->tmHeight;
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : _GGetTextExtent()                                             */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL _GetTextExtent(lpStr)
  LPCSTR lpStr;
{
  INT nPrefix = 0;
  INT nHeight, nWidth;

  /*
    Process tabs (should really be done by GetTabbedTextExtent())
  */
  if (lstrchr(lpStr, '\t'))
  {
    INT nChars;
    lpStr = _GExpandTabs((LPSTR) lpStr, &nChars, NULL, &nPrefix);
  }
  else if (lstrchr(lpStr, HILITE_PREFIX))
  {
    nPrefix = 1;
  }

#if defined(META)
  {
  grafPort *gp;
  fontRcd  *f;

  /*
    Get a pointer to the MetaWindows port and the current font
  */
  mwGetPort(&gp);
  f = gp->txFont;

  /*
    Get the height and width
  */
  nWidth = mwStringWidth((LPSTR) lpStr);

  /*
    If we have a one-char string and that character is not in the
    current font, then Metawindows returns (char) -1. So, let's
    normalize it to 0.
  */
  if (nWidth == 0xFF && lpStr[1] == '\0')
    nWidth = 0;

  if (IS_STROKED_FONT(f))
    nHeight = gp->txSize.Y;
  else
    nHeight = f->chHeight;

  /*
    Account for the hotkey character
  */
  if (nPrefix && HilitePrefix)
    nWidth -= mwStringWidth("~");
  }

#elif defined(GX) && defined(USE_GX_TEXT)
  nWidth = textwidth((LPSTR) lpStr);
  if (nPrefix && HilitePrefix)
    nWidth -= textwidth("~");
  nHeight = textheight(lpStr);

#elif defined(GX) || defined(GURU) || defined(GFX)
  nWidth  = (lstrlen(lpStr) - nPrefix) * SysGDIInfo.tmAveCharWidth;
  nHeight = SysGDIInfo.tmHeight;

#elif defined(MSC)
  {
  struct _fontinfo fontbuf;
  if (_getfontinfo((struct _fontinfo FAR *) &fontbuf) < 0)
    return 0L;
  nWidth  = _getgtextextent(lpStr);
  if (nPrefix && HilitePrefix)
    nWidth -= _getgtextextent("~");
  nHeight = fontbuf.pixheight;
  }

#elif defined(BGI)
  nWidth  = textwidth((LPSTR) lpStr);
  if (nPrefix && HilitePrefix)
    nWidth -= textwidth("~");
  nHeight = textheight((LPSTR) lpStr);

#ifdef BGI_STROKED_HEIGHT_KLUDGE
  /*
    Kludge for a possible BGI bug when writing stroked fonts.
    We need to make the fonts a little bigger than they actually are, or
    else the descenders will get chopped off.
  */
  if (SysGDIInfo.CurrFontID != DEFAULT_FONT)
    nHeight += 2;
#endif

#endif

  return MAKELONG(nWidth, nHeight);
}


/****************************************************************************/
/*                                                                          */
/* Function : RealizeFont(hDC)                                              */
/*                                                                          */
/* Purpose  : Realizes the currently selected logical font.                 */
/*                                                                          */
/* Returns  : TRUE if the font was realized, FALSE if not.                  */
/*                                                                          */
/* Called by: SelectObject() when a font is selected into a DC.             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL RealizeFont(hDC)
  HDC      hDC;
{
  LPHDC     lphDC;
  LPOBJECT  lpObj;
  LPLOGFONT lpLF;
  LPSTR     lpszFace;
  BYTE      cPitchAndFamily;
  int       cyHeightNow, cxWidthNow;

#if defined(META)
  int       lfAttr;
  grafPort *gp;
  fontRcd  *f;

#elif defined(GX) || defined(GURU)
#if defined(USE_GX_TEXT)
  int       lfAttr;
#else
  static int oldBackMode = OPAQUE;
  static int oldHeight = -1;
#endif

#elif defined(MSC)
  char szParams[32];
  char szBuf[32];

#elif defined(BGI)
  int  font;
#endif


  /*
    Read the font info from the INI file the first time through
  */
  if (!bReadINI)
    ReadINIFontInfo(hDC);


  /*
    Get a pointer to the current DC and to the current font object
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;
  if ((lpObj = _ObjectDeref(lphDC->hFont)) == NULL)
    return FALSE;
  lpLF = &lpObj->uObject.uLogFont;
  lpszFace = lpLF->lfFaceName;

  if (lpLF->lfHeight < 0)
    lpLF->lfHeight = -lpLF->lfHeight;


#if defined(META)
  /*
    See if we are restoring the default font.
  */
  if (lphDC->hFont == GetStockObject(SYSTEM_FONT))
  {
    MetaSetDefaultFont();
    return TRUE;
  }

  /*
    The logical font must have a facename. We map it into a MetaWindows
    font name (using AddFontResource), and set some fields to point to
    the memory buffer which holds the raw MetaWindows font data. Then
    we realize the font by calling mwSetFont.
  */
  if (lpszFace[0] != '\0')
  {
    HANDLE hFontBuf;
    fontRcd FAR *lpFontBuf;

    if ((hFontBuf = AddFontResource(lpszFace)) == NULL)
      return FALSE;
    ((LPMETADCINFO) lphDC->lpDCExtra)->hFontBuf = hFontBuf;
    lpObj->lpExtra = (LPVOID) (DWORD) hFontBuf;
    lpObj->fFlags |= OBJ_LPEXTRA_IS_HANDLE;

#if !defined(EXTENDED_DOS)
    if ((lpFontBuf = MetaFontHandleToPtr(hFontBuf)) != NULL)
    {
      mwSetFont(lpFontBuf);
    }
#else
    if ((lpFontBuf = (fontRcd FAR *) GlobalLock(hFontBuf)) != NULL)
    {
      mwSetFont(lpFontBuf);
      GlobalUnlock(hFontBuf);
    }
#endif
  }

  /*
    Get the actual height and width of the selected font
  */
  mwGetPort(&gp);
  f = gp->txFont;
  if (IS_STROKED_FONT(f))  /* TextSize only works with stroked fonts */
  {
    cxWidthNow  = gp->txSize.X;
    cyHeightNow = gp->txSize.Y;
  }
  else
  {
    cxWidthNow  = f->chWidth;
    cyHeightNow = f->chHeight;
  }


  /*
    Use the current settings if the desired height or width is 0
  */
  if ((cyHeightWanted = lpLF->lfHeight) == 0)
    cyHeightWanted = cyHeightNow;
  if ((cxWidthWanted = lpLF->lfWidth) == 0)
    cxWidthWanted = (cyHeightWanted * 3) / 4;

  /*
    If we are changing sizes from the last time, set the font height and width.
  */
  if (cxWidthWanted != cxWidthNow || cyHeightWanted != cyHeightNow)
    mwTextSize(cxWidthWanted, cyHeightWanted);

  /*
    Set the text rotation
  */
  if (lpLF->lfEscapement)
    mwTextPath(lpLF->lfEscapement * 10);

  /*
    Set the text attributes (weight, italic, underline, strikeout, proportional)
  */
  lfAttr = 0;
  if (lpLF->lfWeight >= 700)
  {
    if (IS_STROKED_FONT(f))  /* Bolding only works with stroked fonts */
    {
      mwTextBold((lpLF->lfWeight - 700) / 100);
      lfAttr += cBold;
    }
  }
  else
    lfAttr += cNormal;
  if (lpLF->lfItalic)
    lfAttr += cItalic;
  if (lpLF->lfUnderline)
    lfAttr += cUnderline;
  if (lpLF->lfStrikeOut)
    lfAttr += cStrikeout;
  cPitchAndFamily = lpLF->lfPitchAndFamily;
  /*
    Default to proportional fonts if we did not explicitly ask for fixed
  */
  if (!(cPitchAndFamily & FIXED_PITCH))
    lfAttr += cProportional;
  mwTextFace(lfAttr);
    
  return TRUE;


#elif defined(GX)
#if defined(USE_GX_TEXT)
  /*
    See if we are restoring the default font.
  */
  if (lphDC->hFont == GetStockObject(SYSTEM_FONT))
  {
    GXSetDefaultFont();
    ((LPGXDCINFO) lphDC->lpDCExtra)->hTXHeader = SysGDIInfo.hFontDefault;
    return TRUE;
  }

  /*
    The logical font must have a facename. We map it into a GX/Text
    font name (using AddFontResource), and set some fields to point to
    the memory buffer which holds the raw GX/Text font data. Then
    we realize the font by calling txSetFont().
  */
  if (lpszFace[0] != '\0')
  {
    HANDLE hFontBuf;
    TXHEADER FAR *lpFontBuf;

    if ((hFontBuf = AddFontResource(lpszFace)) == NULL)
      return FALSE;

    ((LPGXDCINFO) lphDC->lpDCExtra)->hTXHeader = hFontBuf;
    lpObj->fFlags |= OBJ_LPEXTRA_IS_HANDLE;

    if ((lpFontBuf = (TXHEADER FAR *) GlobalLock(hFontBuf)) != NULL)
    {
      txSetFont(lpFontBuf);

#if defined(GX3)
      if (txGetType() == txFONTGST)
      {
        /*
          Reset the stroked font to the default size, or else the font will
          retain the old settings and make the next call to setusercharsize
          off. Do this only if the size of the font we want now is different
          than the one we got last time.
        */
        txSetStrokeSize(1, 1, 1, 1);

        /*
          Get the actual height and width of the selected font
        */
        cyHeightNow = HIWORD(_GetTextExtent(pszHeightTestString));
        cxWidthNow  = LOWORD(_GetTextExtent(pszWidthTestString)) / 6;

        /*
          Use the current settings if the desired height or width is 0
        */
        if ((cyHeightWanted = lpLF->lfHeight) == 0)
          cyHeightWanted = cyHeightNow;
        if ((cxWidthWanted = lpLF->lfWidth) == 0)
        {
          cxWidthWanted = (cyHeightWanted * 3) / 4;
#if 51095
          /*
            Preserve the same height/width ratio as the default font had.
              
                heightWanted          ????
                ------------    =    ----------
                heightNow             widthNow
          */
          cxWidthWanted = (cxWidthNow * cyHeightWanted) / cyHeightNow;
#endif
        }

#if 0
if (IS_PRTDC(lphDC))
{
  cyHeightWanted = (cyHeightWanted * lphDC->extView.cy / lphDC->extWindow.cy);
  cxWidthWanted  = (cxWidthWanted  * lphDC->extView.cx / lphDC->extWindow.cx);
}
#endif

        /*
          Scale the font only if the desired sizes are different from the
          actual sizes.
        */
        if (cxWidthWanted != cxWidthNow || cyHeightWanted != cyHeightNow)
        {
          txSetStrokeSize(cxWidthWanted, cxWidthNow, cyHeightWanted, cyHeightNow);
          cyHeightNow = HIWORD(_GetTextExtent(pszHeightTestString));
          cxWidthNow  = LOWORD(_GetTextExtent(pszWidthTestString)) / 6;
        }
      }
#endif


      /*
        Set the proper face
      */
      lfAttr = txNORMAL;
      if (lpLF->lfWeight >= 700)
        lfAttr += txBOLD;
      if (lpLF->lfItalic)
        lfAttr += txITALIC;
      if (lpLF->lfUnderline)
        lfAttr += txUNDER;
      if (lpLF->lfPitchAndFamily & FIXED_PITCH)
        lfAttr += txFIXED;
      txSetFace(lfAttr);

#if !51195
      GXSetFont(lpFontBuf);
#endif
      GlobalUnlock(hFontBuf);
    }
  }

#else
  if (SysGDIInfo.CurrFontID != GX_DEFAULT_FONT || oldBackMode != lphDC->wBackgroundMode)
  {
    grSetTextStyle(GX_DEFAULT_FONT,
                   (lphDC->wBackgroundMode==TRANSPARENT) ? grTRANS : grOPAQUE);
    SysGDIInfo.CurrFontID = GX_DEFAULT_FONT;
    oldBackMode = lphDC->wBackgroundMode;
  }
#endif
  return TRUE;


#elif defined(GFX)
  return TRUE;


#elif defined(GURU)
  if (SysGDIInfo.CurrFontID != GURU_DEFAULT_FONT || oldBackMode != lphDC->wBackgroundMode)
  {
    SelectRomFont(GURU_DEFAULT_FONT);
    SysGDIInfo.CurrFontID = GURU_DEFAULT_FONT;
    oldBackMode = lphDC->wBackgroundMode;
  }
  return TRUE;


#elif defined(MSC)
  (void) cyHeightNow;   (void) cxWidthNow;

  /*
    Should we use the best-fit approach?
  */
  szParams[0] = (SysGDIInfo.fFlags & GDISTATE_MSC_NOBESTMATCH) ? '\0' : 'b';
  szParams[1] = '\0';

  /*
    Determine the height and width of the font
  */
  if (lpLF->lfHeight)
  {
    sprintf(szBuf, "h%d", lpLF->lfHeight);
    strcat(szParams, szBuf);
  }
  if (lpLF->lfWidth)
  {
    sprintf(szBuf, "w%d", lpLF->lfWidth);
    strcat(szParams, szBuf);
  }

  /*
    Get the typeface
  */
  if (lpszFace[0] != '\0')
  {
    sprintf(szBuf, "t'%s'", 
            (!stricmp(lpszFace, "system")) ? MSCSystemFontName : lpszFace);
    strcat(szParams, szBuf);
  }
  else  /* no typeface explicitly specified */
  {
    /*
      Choose fixed or proportional font
    */
    cPitchAndFamily = lpLF->lfPitchAndFamily;
    if (cPitchAndFamily & FIXED_PITCH)
      strcat(szParams, "f");
    else if (cPitchAndFamily & VARIABLE_PITCH)
      strcat(szParams, "p");

    /*
      Map the FF_xxx style to a typeface
    */
    if (cPitchAndFamily & FF_MODERN)
      strcat(szParams, "t'Modern'");
    else if (cPitchAndFamily & FF_SWISS)
      strcat(szParams, "t'Helv'");
    else if (cPitchAndFamily & FF_ROMAN)
      strcat(szParams, "t'Roman'");
    else if (cPitchAndFamily & FF_SCRIPT)
      strcat(szParams, "t'Script'");
    else
      strcat(szParams, "t'Courier'");
  }

  /*
    Set the new font only if the characteristics are different from the
    last time.
  */
  if (strcmp(szParams, oldszParams) != 0)
  {
    int rc;
    strcpy(oldszParams, szParams);
    if ((rc = _setfont(szParams)) < 0)
    {
      /*
        Couldn't find the font. Try registering it if the typeface
        is something like 'k:\win\system\vgaoem.fon'.
      */
      if (lpszFace[0] != '\0' &&
          _registerfonts(!stricmp(lpszFace, "system") ? MSCSystemFontName
                                                      : lpszFace) > 0)
        rc = _setfont(szParams);
      /*
        Couldn't find the font.
      */
      if (rc < 0)
        rc = _setfont("n1");
    }
    return (rc == 0);  /* 0 means success */
  }
  else
    return TRUE;


#elif defined(BGI)
  font = SysGDIInfo.hFontDefault;

  if (lpszFace[0] != '\0')
  {
    if (!stricmp(lpszFace, "system"))
      font = SysGDIInfo.hFontDefault;
    else
      if ((font = AddFontResource(lpszFace)) == 0)
        return FALSE;
  }
  else
  {
    cPitchAndFamily = lpLF->lfPitchAndFamily;
    if (cPitchAndFamily & VARIABLE_PITCH)
      font = SANS_SERIF_FONT;
    else if (cPitchAndFamily & FF_MODERN)
      font = TRIPLEX_FONT;
    else if (cPitchAndFamily & FF_SWISS)
      font = SANS_SERIF_FONT;
    else if (cPitchAndFamily & FF_ROMAN)
      font = DEFAULT_FONT;
    else if (cPitchAndFamily & FF_SCRIPT)
      font = GOTHIC_FONT;
    else
      font = SysGDIInfo.hFontDefault;
  }

  /*
    If we want the default BGI bitmapped font, and we are currently using
    some other font, then reset the bitmapped font.
  */
  if (font == DEFAULT_FONT)
  {
    if (SysGDIInfo.CurrFontID != DEFAULT_FONT)
    {
#if defined(FLEMING)
      if (IS_PRTDC(lphDC))
        SetFont(font, HORIZ_DIR, 1, 0, VGAunits);
      else
#endif
#if defined(__DPMI16__) || defined(__DPMI32__)
      settextstyle(font, HORIZ_DIR, 0);
#else
      settextstyle(font, HORIZ_DIR, 1);
#endif
    }
  }
  else
  {
    int ge;
    DWORD dwSize;

    /*
      We have a BGI stroked font. Load it and use setusercharsize()
      to scale the height and width to what we want.
    */
#if defined(__DPMI32__)
    if (1)  /* MetaWindows/BGI incompatibility */
#else
    if (font != SysGDIInfo.CurrFontID)
#endif
    {
#if defined(FLEMING)
      if (IS_PRTDC(lphDC))
        SetFont(font, HORIZ_DIR, 1, 0, VGAunits);
      else
#endif
#if defined(__DPMI16__) || defined(__DPMI32__)
      settextstyle(font, HORIZ_DIR, 0);
#else
      settextstyle(font, HORIZ_DIR, 4 /* 0 */);
#endif
      if (font != DEFAULT_FONT)
        setusercharsize(1, 1, 1, 1);
    }

    /*
      Reset the stroked font to the default size, or else the font will
      retain the old settings and make the next call to setusercharsize
      off. Do this only if the size of the font we want now is different
      than the one we got last time.
    */
    if (lpLF->lfHeight != cyHeightWanted || lpLF->lfWidth != cxWidthWanted)
    {
      setusercharsize(1, 1, 1, 1);
    }

    /*
      Get the actual height and width of the selected font
    */
    cyHeightNow = HIWORD(_GetTextExtent(pszHeightTestString));
    cxWidthNow  = LOWORD(_GetTextExtent(pszWidthTestString)) / 6;

#ifdef BGI_STROKED_HEIGHT_KLUDGE
    /*
      Kludge for a possible BGI bug when writing stroked fonts.
      We need to make the fonts a little bigger than they actually are, or
      else the descenders will get chopped off.
    */
    cyHeightNow -= 2;
#endif

    /*
      Use the current settings if the desired height or width is 0
    */
    if ((cyHeightWanted = lpLF->lfHeight) == 0)
      cyHeightWanted = cyHeightNow;
    if ((cxWidthWanted = lpLF->lfWidth) == 0)
      cxWidthWanted = cxWidthNow;

    /*
      Scale the font only if the desired sizes are different from the
      actual sizes.
    */
    if (cxWidthWanted != cxWidthNow || cyHeightWanted != cyHeightNow)
    {
#if defined(FLEMING)
      if (IS_PRTDC(lphDC))
        SetFont(font, HORIZ_DIR, cyHeightWanted, cxWidthWanted, PTunits);
      else
#endif
      {
      setusercharsize(cxWidthWanted, cxWidthNow, cyHeightWanted, cyHeightNow);

      cyHeightNow = HIWORD(_GetTextExtent(pszHeightTestString));
      cxWidthNow  = LOWORD(_GetTextExtent(pszWidthTestString)) / 6;
      }
    }
  }

  SysGDIInfo.CurrFontID = font;
  return TRUE;

#endif
}



/****************************************************************************/
/*                                                                          */
/* Function : ReadINIFontInfo(hDC)                                          */
/*                                                                          */
/* Purpose  : Internal function which reads the font information from the   */
/*            MEWEL.INI file.                                               */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID PASCAL ReadINIFontInfo(hDC)
  HDC  hDC;
{
  BYTE szPath[MAXPATH];
  PSTR pFONTS = "fonts";
  PSTR pNULL  = "";
  PSTR pszKey;
  HANDLE hBuf;
  PSTR pBuf;

  /*
    Allocate a temp buffer
  */
#if defined(__DPMI16__) || defined(__DPMI32__)
  if ((pBuf = emalloc(1024)) == NULL)
#else
  if ((hBuf = LocalAlloc(LMEM_MOVEABLE, 1024)) == NULL ||
      (pBuf = LocalLock(hBuf)) == NULL)
#endif
    return;

  /*
    Enumerate the font names
  */
  GetProfileString(pFONTS, NULL, pNULL, pBuf, 1024);

  /*
    Scan through all of the font names
  */
  for (pszKey = pBuf;  *pszKey;  pszKey += lstrlen(pszKey)+1)
  {
    /*
      Get the path where the font file can be found
    */
    if (GetProfileString(pFONTS, pszKey, pNULL, szPath, sizeof(szPath)))
    {
#if defined(MSC) && !defined(GX) && !defined(META)
      /*
        If we have a font with a FON extension, then we must explicitly
        register it.
      */
      if (lstrchr(szPath, '.'))
        _registerfonts(szPath);
#endif

      /*
        Associate the font name with the font data file. A third arg of 0
        indicates that the font is not loaded yet.
      */
      AssociateFontResource(pszKey, szPath, NULL, NULL);
    }
  }

  /*
    Free the temp buffer
  */
#if defined(__DPMI16__) || defined(__DPMI32__)
  MyFree(pBuf);
#else
  LocalUnlock(hBuf);
  LocalFree(hBuf);
#endif

  /*
    Read in the default font info
  */
#if defined(META)
  if (GetProfileString("fonts","DefaultFontFile","EXTSYS72.FNT",szPath,sizeof(szPath)) > 0)
  {
    LPSTR lpComma = lstrchr((LPCSTR) szPath, ',');
    if (lpComma)
    {
      if (lpComma[1] == 'p' || lpComma[1] == 'P')
        bDefaultFontProportional = TRUE;
      *lpComma = '\0';  /* get rid of the ,p */
    }
    else
      bDefaultFontProportional = FALSE;
    SysGDIInfo.hFontDefault = AddFontResource((LPCSTR) szPath);
    MetaSetDefaultFont();
  }
  else
    SysGDIInfo.hFontDefault = 0;

#elif defined(GX) && defined(USE_GX_TEXT)
  if (GetProfileString("fonts","DefaultFontFile","",szPath,sizeof(szPath)) > 0)
  {
    SysGDIInfo.hFontDefault = AddFontResource((LPCSTR) szPath);
    GXSetDefaultFont();
  }
  else
    SysGDIInfo.hFontDefault = 0;

#elif defined(BGI)
  if (GetProfileString("fonts","DefaultFontFile","",szPath,sizeof(szPath)) > 0)
    SysGDIInfo.hFontDefault = AddFontResource(szPath);
  else
    SysGDIInfo.hFontDefault = 0;
#elif defined(MSC)
  if (GetProfileString("fonts","DefaultFontFile","",szPath,sizeof(szPath)) > 0)
    MSCSystemFontName = lstrsave(szPath);
#endif

  /*
    Tell MEWEL that the font info fromthe INI file has been processed
  */
  bReadINI = TRUE;
  MEWELInitSystemFont(hDC);
}


VOID FAR PASCAL MEWELInitSystemFont(HDC hDC)
{
  TEXTMETRIC tm;
  HDC hOrigDC;

  if ((hOrigDC = hDC) == 0)
    hDC = GetDC(_HwndDesktop);
  RealizeFont(hDC);
  GetTextMetrics(hDC, &tm);
  SysGDIInfo.tmHeight          = tm.tmHeight;
  SysGDIInfo.tmExternalLeading = tm.tmExternalLeading;
  SysGDIInfo.tmHeightAndSpace  = tm.tmHeight + tm.tmExternalLeading;
  SysGDIInfo.tmAveCharWidth    = tm.tmAveCharWidth;
  if (hOrigDC == 0)
    ReleaseDC(_HwndDesktop, hDC);
}


/****************************************************************************/
/*                                                                          */
/* Function : WinCloseGUIFontStuff()                                        */
/*                                                                          */
/* Purpose  : We need this to re-init all of the static vars used by the    */
/*            font manager. We do this in case we spawn a child process     */
/*            and then come back ... we want the font stuff to be set up    */
/*            correctly when we come back.                                  */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinCloseGUIFontStuff(void)
{
  bReadINI       = FALSE;
  SysGDIInfo.CurrFontID     = -1;
  cyHeightWanted = 0;
  cxWidthWanted  = 0;
#if defined(MSC)
  oldszParams[0] = '\0';
#endif
}


#if defined(META)
VOID FAR PASCAL MetaSetDefaultFont(void)
{
  LPOBJECT  lpObj;
  fontRcd FAR *lpFont;

  static grafPort *pLastPort = NULL;

  if (_MetaInfo.thePort == NULL)
    return;

  /*
    Get a pointer to the raw MetaWindows font buffer
  */
#if !defined(EXTENDED_DOS)
  if ((lpFont = MetaFontHandleToPtr(SysGDIInfo.hFontDefault)) != NULL)
#else
  if ((lpFont = (fontRcd FAR *) GlobalLock(SysGDIInfo.hFontDefault)) != NULL)
#endif
  {
    /*
      Set the current MetaWindows font
    */
    mwSetFont(lpFont);

#if 0
    /*
      5/27/94 (maa)
        Make all fonts proportional, like windows
    */
    if (bDefaultFontProportional || IS_STROKED_FONT(lpFont))
#endif
    {
      mwTextFace(cProportional);
      /*
        For a stroked font, MetaWindow requires that we set the 
        height and width... otherwise, the font will be invisible.
      */
      if ((lpObj = _ObjectDeref(GetStockObject(SYSTEM_FONT))) != NULL)
      {
        LPLOGFONT lpLF = &lpObj->uObject.uLogFont;
        mwTextSize(lpLF->lfWidth, lpLF->lfHeight);
      }
    }

#if defined(EXTENDED_DOS)
    GlobalUnlock(SysGDIInfo.hFontDefault);
#endif
    pLastPort = _MetaInfo.thePort;
  }
}


#if !defined(EXTENDED_DOS)
static fontRcd FAR* PASCAL MetaFontHandleToPtr(hFont)
  HFONT hFont;
{
  LIST *pList;

  for (pList = MEWELFontNameList;  pList;  pList = pList->next)
  {
    LPFONTNAMEINFO lpFNI = (LPFONTNAMEINFO) pList->data;
    if ((HFONT) lpFNI->hFont == hFont)
      return (fontRcd FAR *) lpFNI->lpFontBuf;
  }
  return NULL;
}
#endif

#endif



#if defined(GX) && defined(USE_GX_TEXT)
VOID FAR PASCAL GXSetDefaultFont(void)
{
  TXHEADER FAR *lpFont;

  if ((lpFont = (TXHEADER FAR *) GlobalLock(SysGDIInfo.hFontDefault)) != NULL)
  {
    GXSetFont(lpFont);
    GlobalUnlock(SysGDIInfo.hFontDefault);
  }
}

static VOID PASCAL GXSetFont(lpTX)
  TXHEADER FAR *lpTX;
{
  /*
    We keep an in-memory version of the current TXHEADER, since XMS
    memory swapping can swap lpTX out of conventional memory.
  */
  static TXHEADER currTX;
  lmemcpy((LPSTR) &currTX, (LPSTR) lpTX, sizeof(TXHEADER));
  txSetFont(&currTX);
}

#endif

