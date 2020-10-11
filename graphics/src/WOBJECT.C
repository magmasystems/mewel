/*===========================================================================*/
/*                                                                           */
/* File    : WOBJECT.C                                                       */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible Object functions               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#ifdef NOGDI
#undef NOGDI
#endif
#define NO_STD_INCLUDES
#define NOKERNEL

#include "wprivate.h"
#include "window.h"
#include "wobject.h"

#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#else
#define RealizeBitmap(hDC, hBmp)
#endif


#ifndef NOOBJECTS


/*--------------------------------------------------------------------*/

UINT       _ObjectTblSize = 0;
LPOBJECT  *_ObjectTable = NULL;

#if (MODEL_NAME == MODEL_LARGE) || defined(MEWEL_32BITS)
#define NEW_SEARCHING_METHOD
static INT  nObjEmpty = -1;  /* Next empty object */
#define IS_VALID_OBJECT_PTR(lpObj) (HIWORD(lpObj) != 0)
#endif

/*
  Hook to GUIs and graphics engines
*/
typedef int  (FAR PASCAL *SETFONTPROC)(HDC);
SETFONTPROC lpfnSetFontHook = (SETFONTPROC) 0;

typedef VOID (FAR PASCAL *DELETEBMPPROC)(HANDLE);
DELETEBMPPROC lpfnDeleteBitmapHook = (DELETEBMPPROC) 0;

typedef VOID (FAR PASCAL *DELETERGNPROC)(HANDLE);
DELETEBMPPROC lpfnDeleteRegionHook = (DELETERGNPROC) 0;

/*-----------------------------------------------------------------------*/

HANDLE FAR PASCAL SelectObject(hDC, hObj)
  HDC   hDC;
  HANDLE hObj;
{
  HANDLE hOldObj = (HANDLE) NULL;
  LPOBJECT lpObj;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HANDLE) NULL;

  /*
    Increment the new object's reference count
  */
  lpObj->nRefCount++;

  switch (lpObj->iObjectType)
  {
    case OBJ_BRUSH :
      hOldObj = lphDC->hBrush;
      lphDC->hBrush = hObj;
      break;
    case OBJ_PEN   :
      hOldObj = lphDC->hPen;
      lphDC->hPen = hObj;
      break;
    case OBJ_FONT  :
      hOldObj = lphDC->hFont;
      lphDC->hFont = hObj;
      if (lpfnSetFontHook)
        (*lpfnSetFontHook)(hDC);
      break;
    case OBJ_RGN   :
      return SelectClipRgn(hDC, (HRGN) hObj);
    case OBJ_BMP   :
      hOldObj = lphDC->hBitmap;
      /*
        We should really examine the type of DC we have, and transform
        the bits in the DIBitmap to be compatible with the DC.
      */
      lphDC->hBitmap = hObj;
      lphDC->hCanvas = hObj;
      RealizeBitmap(hDC, hObj);
      break;

    case OBJ_PALETTE :
      hOldObj = lphDC->hPalette;
      lphDC->hPalette = hObj;
      break;
  }


  /*
    Decrement the old object's reference count
  */
  if ((lpObj = _ObjectDeref(hOldObj)) != NULL)
    lpObj->nRefCount--;

  return hOldObj;
}


BOOL FAR PASCAL DeleteObject(hObj)
  HANDLE hObj;
{
  LPOBJECT lpObj;

  /*
    Get a pointer to the OBJECT structure. Don't delete it if
    something currently has it selected.
  */
  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return FALSE;

  if (lpObj->nRefCount > 0)
    return FALSE;

  /*
    Get the index into the array of objects
  */
  hObj = (UINT) hObj & ~OBJECT_SIGNATURE;

  /*
    Don't delete a stock object
  */
  if (hObj <= STOCK_OBJECT_LAST && !TEST_PROGRAM_STATE(STATE_EXITING))
    return FALSE;

  /*
    If the object is a bitmap, free the memory occupied by the bitmap
  */
  if (lpObj->iObjectType == OBJ_BMP)
  {
    if (lpfnDeleteBitmapHook)
      (*lpfnDeleteBitmapHook)((UINT) hObj | OBJECT_SIGNATURE);
    GlobalFree(lpObj->uObject.uhBitmap);
  }
  else if (lpObj->iObjectType == OBJ_RGN)
  {
    if (lpfnDeleteRegionHook)
      (*lpfnDeleteRegionHook)((UINT) hObj | OBJECT_SIGNATURE);
  }

  /*
    Sometime, the GUI stores some info pointed to by the generic
    'lpExtra' pointer. It could be the handle of a font or some
    other info.
  */
  if (lpObj->lpExtra)
  {
    if (lpObj->fFlags & OBJ_LPEXTRA_IS_HANDLE)
    {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      if (lpObj->iObjectType == OBJ_FONT)
#if 71694
        ;  /* there are some bugs in this - we should free the whole font */
#else
        RemoveFontResource(lpObj->uObject.uLogFont.lfFaceName);
#endif
      else
#endif
        GlobalFree((HANDLE) (LONG) (lpObj->lpExtra));
    }
#if defined(XWINDOWS)
    else if (lpObj->iObjectType == OBJ_FONT)
      XMEWELRemoveFontFromCache((XFontStruct *) lpObj->lpExtra);
    else if (lpObj->iObjectType == OBJ_PALETTE)
      XMEWELFreePalette(lpObj);
#endif
    else
      MYFREE_FAR(lpObj->lpExtra);

  }



#if defined(XWINDOWS)
  /*
    Get rid of the bitmap attached to the pattern brush
  */
  if (lpObj->iObjectType == OBJ_BRUSH && lpObj->uObject.uLogBrush.lbStyle == BS_PATTERN)
    DeleteObject(lpObj->uObject.uLogBrush.lbHatch);
#endif


  /*
    Free the object data structure and set the entry in the object
    table to NULL.
  */
  MYFREE_FAR(lpObj);
#ifdef NEW_SEARCHING_METHOD
  _ObjectTable[(UINT) hObj] = (LPOBJECT) (DWORD) nObjEmpty;
  nObjEmpty = hObj;								/* 6/22/93 */
#else
  _ObjectTable[(UINT) hObj] = NULL;
#endif
  return TRUE;
}


int FAR PASCAL GetObject(hObject, nCount, lpObject)
  HANDLE hObject;
  INT    nCount;
  VOID FAR *lpObject;
{
  LPOBJECT lpObj;
  int      nCopied = 0;

  if ((lpObj = _ObjectDeref(hObject)) == NULL)
    return 0;

  switch (lpObj->iObjectType)
  {
    case OBJ_BRUSH :
      nCopied = min(nCount, sizeof(LOGBRUSH));
      lmemcpy((LPSTR)lpObject, (LPSTR)&lpObj->uObject.uLogBrush, nCopied);
      break;
    case OBJ_PEN   :
      nCopied = min(nCount, sizeof(LOGPEN));
      lmemcpy((LPSTR)lpObject, (LPSTR)&lpObj->uObject.uLogPen, nCopied);
      break;
    case OBJ_FONT  :
      nCopied = min(nCount, sizeof(LOGFONT));
      lmemcpy((LPSTR)lpObject, (LPSTR)&lpObj->uObject.uLogFont, nCopied);
      break;
    case OBJ_RGN   :
      nCopied = min(nCount, sizeof(RECT));
      lmemcpy((LPSTR)lpObject, (LPSTR)&lpObj->uObject.uRgn, nCopied);
      break;
    case OBJ_BMP   :
    {
      LPBITMAP lpbm;
      nCopied = min(nCount, sizeof(BITMAP));
      lpbm = (LPBITMAP) GlobalLock(lpObj->uObject.uhBitmap);
      /*
        Re-resolve the conv memory address of the bits in case we
        swapped out the bitmap.
      */
      lpbm->bmBits = ((LPSTR) &lpbm->bmBits) + 4;
      lmemcpy((LPSTR)lpObject, (LPSTR) lpbm, nCopied);
      GlobalUnlock(lpObj->uObject.uhBitmap);
      break;
    }
    case OBJ_PALETTE :
    {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      LPINTLOGPALETTE lpIntPal;
      lpIntPal= (LPINTLOGPALETTE) GlobalLock(lpObj->uObject.uhPalette);
      nCopied = min(nCount, 
         (sizeof(INTLOGPALETTE) - sizeof(lpIntPal->palNumEntries)) + 
          (lpIntPal->palNumEntries * sizeof(INTPALETTEENTRY)));

      /*
        According to the SDK SHOWDIB example, doing 
           GetObject(hPal, sizeof(int), &nEntries);
        returns the palNumEntries field.
      */
      lmemcpy((LPSTR)lpObject, (LPSTR) &lpIntPal->palNumEntries, nCopied);
      GlobalUnlock(lpObj->uObject.uhPalette);
#endif
      break;
    }
  }

  return nCopied;
}

/*-----------------------------------------------------------------------*/

HANDLE FAR PASCAL GetStockObject(iObject)
  int iObject;
{
  if (iObject >= STOCK_OBJECT_FIRST && iObject <= STOCK_OBJECT_LAST)
    return iObject | OBJECT_SIGNATURE;
  else
    return (HANDLE) NULL;
}

/*-----------------------------------------------------------------------*/

HPEN FAR PASCAL CreatePen(int nPenStyle, int nWidth, DWORD crColor)
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  if ((hObj = _ObjectAlloc(OBJ_PEN)) == BADOBJECT)
    return (HPEN) NULL;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HPEN) NULL;

  lpObj->uObject.uLogPen.lopnStyle = nPenStyle;
  lpObj->uObject.uLogPen.lopnWidth.x = (MWCOORD) nWidth;
  lpObj->uObject.uLogPen.lopnColor = crColor;
  return (HPEN) hObj;
}


HPEN FAR PASCAL CreatePenIndirect(lpLogPen)
  LOGPEN   *lpLogPen;
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  if ((hObj = _ObjectAlloc(OBJ_PEN)) == BADOBJECT)
    return (HPEN) NULL;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HPEN) NULL;

  lpObj->uObject.uLogPen = *lpLogPen;
  return (HPEN) hObj;
}

/*-----------------------------------------------------------------------*/
extern HBRUSH FAR PASCAL CreateNULLBrush(DWORD);


/****************************************************************************/
/*                                                                          */
/* Function : SysCreateSolidBrush(clrBrush)                                 */
/*                                                                          */
/* Purpose  : This is called by the WM_CTLCOLOR processing in               */
/*            StdWindowWinProc() when we need to create a brush for a       */
/*            control window. We search the GDI object list for a brush     */
/*            of the requested color. If one doesn't exist, we create it.   */
/*                                                                          */
/* Returns  : The handle of the brush.                                      */
/*                                                                          */
/****************************************************************************/
HBRUSH FAR PASCAL SysCreateSolidBrush(DWORD clrBrush)
{
  LPOBJECT  lpObj;
  UINT      i;

  for (i = 0;  i < _ObjectTblSize;  i++)
    if ((lpObj = _ObjectTable[i]) != NULL             &&
#ifdef NEW_SEARCHING_METHOD
         IS_VALID_OBJECT_PTR(lpObj)                   &&
#endif
         lpObj->iObjectType == OBJ_BRUSH              &&
         lpObj->uObject.uLogBrush.lbStyle == BS_SOLID &&
         lpObj->uObject.uLogBrush.lbColor == clrBrush)
      return (i | OBJECT_SIGNATURE);

  return CreateSolidBrush(clrBrush);
}

HBRUSH FAR PASCAL CreateSolidBrush(DWORD clrBrush)
{
  LOGBRUSH lbBrush;

  lbBrush.lbStyle = BS_SOLID;
  lbBrush.lbColor = clrBrush;
  lbBrush.lbHatch = 0;
  return CreateBrushIndirect(&lbBrush);
}

HBRUSH FAR PASCAL CreateNULLBrush(clrBrush)
  DWORD clrBrush;
{
  LOGBRUSH lbBrush;

  lbBrush.lbStyle = BS_NULL;
  lbBrush.lbColor = clrBrush;
  lbBrush.lbHatch = 0;
  return CreateBrushIndirect(&lbBrush);
}


HBRUSH FAR PASCAL CreatePatternBrush(HBITMAP hBitMap)
{
  LOGBRUSH lbBrush;

  lbBrush.lbStyle = BS_PATTERN;
  lbBrush.lbColor = 0;
  lbBrush.lbHatch = (UINT) hBitMap;
  return CreateBrushIndirect(&lbBrush);
}


HBRUSH FAR PASCAL CreateHatchBrush(int nIndex, DWORD clrBrush)
{
  LOGBRUSH lbBrush;

  lbBrush.lbStyle = BS_HATCHED;
  lbBrush.lbColor = clrBrush;
  lbBrush.lbHatch = nIndex;
  return CreateBrushIndirect(&lbBrush);
}



#if defined(MEWEL_GUI) && defined(META)
typedef struct patData
{
  patRcd patHdr;
  char   patImg[32];
} PATDATA, FAR *LPPATDATA;

static PATDATA DefPatData =
{
  8, 8,  /* width, height            */
  0, 0,  /* align, flags             */
  1,     /* patRowBytes  Must be 1.  */
  1,     /* patBits (bits per pixel) */
  1,     /* patPlanes (bitplanes)    */
  { 0 }  /* patImg */
};
#endif


HBRUSH FAR PASCAL CreateBrushIndirect(lpLogBrush)
  LOGBRUSH *lpLogBrush;
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  if ((hObj = _ObjectAlloc(OBJ_BRUSH)) == BADOBJECT)
    return (HBRUSH) NULL;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HBRUSH) NULL;

  lpObj->uObject.uLogBrush = *lpLogBrush;


#if defined(MEWEL_GUI) && defined(META)
  /*
    Handle the bitmap stuff for patterned brushes
  */
  if (lpObj->uObject.uLogBrush.lbStyle == BS_PATTERN)
  {
    BITMAP   bm;
    LPPATDATA lpPD;
    HBITMAP  hBitmap;

    hBitmap = (HBITMAP) lpObj->uObject.uLogBrush.lbHatch;

    /*
      Allocate a MetaWindows PATDATA structure and attach it to the
      brush object. This data will be referenced in RealizeBrush().
    */
    lpObj->lpExtra = lpPD = (LPPATDATA) EMALLOC_FAR(sizeof(PATDATA));
    lmemcpy((char *) lpPD, (char *) &DefPatData, sizeof(PATDATA));

    /*
      Get the planes and the bits-per-pixel values.
    */
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR) &bm);
    lpPD->patHdr.patBits   = bm.bmBitsPixel;
    lpPD->patHdr.patPlanes = bm.bmPlanes;
    if (lpPD->patHdr.patPlanes == 1 && lpPD->patHdr.patBits == 4)
      lpPD->patHdr.patPlanes = 4, lpPD->patHdr.patBits = 1;

    /*
      Retrieve the first 8x8 bits and stick it into the patImg data.
    */
    GetBitmapBits(hBitmap, (LONG) bm.bmWidthBytes*8, lpPD->patImg);
  }
#endif


#if defined(XWINDOWS)
  /*
    Handle the bitmap stuff for patterned brushes
  */
  if (lpObj->uObject.uLogBrush.lbStyle == BS_PATTERN)
  {
    BITMAP   bm;
    HBITMAP  hBitmap, hNewBitmap;
    BYTE     achBits[8*8];

    hBitmap = (HBITMAP) lpObj->uObject.uLogBrush.lbHatch;
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR) &bm);
    lpObj->uObject.uLogBrush.lbHatch = hNewBitmap = CreateBitmapIndirect(&bm);

    /*
      Transfer the 8x8 image from the original bitmap into the new bitmap
    */
    GetBitmapBits(hBitmap,    (LONG) bm.bmWidthBytes*8, achBits);
    SetBitmapBits(hNewBitmap, (LONG) bm.bmWidthBytes*8, achBits);
  }
#endif

  return (HBRUSH) hObj;
}


BOOL FAR PASCAL UnrealizeObject(hBrush)
  HGDIOBJ hBrush;
{
  (void) hBrush;
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _ObjectAlloc()                                                */
/*                                                                          */
/* Purpose  : Internal function which allocates space for a GDI object.     */
/*            All objects are put into a master object table so that they   */
/*            can be identified and enumerated easily.                      */
/*                                                                          */
/* Returns  : The "handle" of the allocated object.                         */
/*                                                                          */
/****************************************************************************/
HANDLE FAR PASCAL _ObjectAlloc(iObjType)
  UINT iObjType;
{
  LPOBJECT  lpObj;
  UINT      i;
  
  /*
    Allocate the object table if we haven't done so yet.
  */
  if (_ObjectTable == NULL)
  {
    _ObjectTable = (LPOBJECT *) emalloc_far(MAXOBJECTS * sizeof(LPOBJECT));
    if (_ObjectTable == (LPOBJECT *) NULL)
      return BADOBJECT;
    _ObjectTblSize = MAXOBJECTS;
#ifdef NEW_SEARCHING_METHOD
    nObjEmpty = 0;
#endif
  }

  /*
    Allocate the object "header"
  */
  if ((lpObj = (LPOBJECT) EMALLOC_FAR(sizeof(OBJECT))) == NULL)
    return BADOBJECT;
  lpObj->iObjectType = iObjType;
  lpObj->nRefCount   = 0;

  /*
    Try to find a free slot for the object.
  */
#ifdef NEW_SEARCHING_METHOD
  i = nObjEmpty;
  if (i != _ObjectTblSize)
  {
    if (_ObjectTable[i] == NULL)
      nObjEmpty += 1;
    else
      nObjEmpty = (UINT) (DWORD) _ObjectTable[i];
  }
#else
  for (i = 0;  i < _ObjectTblSize;  i++)
    if (_ObjectTable[i] == (LPOBJECT) NULL)
      break;
#endif

  /*
    Try to expand the object table.
  */
  if (i == _ObjectTblSize)
  {
    if (!GmemRealloc((LPSTR FAR *) &_ObjectTable, &_ObjectTblSize, 
                     sizeof(LPOBJECT), 2))
      return (HANDLE) BADOBJECT;
#ifdef NEW_SEARCHING_METHOD
    nObjEmpty = i + 1;
#endif
  }

  /*
    Put the object structure into the object table and return the object
    handle. The handle is the slot number combined with the special object
    signature.
  */
  _ObjectTable[i] = lpObj;
  return (i | OBJECT_SIGNATURE);
}


VOID _ObjectsInit(void)
{
  int i;
  DWORD rgbWhite = TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS)
                   ? RGB(0xFF, 0xFF, 0xFF) : RGB(0x80, 0x80, 0x80);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
  int   iSysFontHeight, iSysFontWidth;
#endif


  /*
    Initialized already?
  */
  static BOOL bObjectsInit = FALSE;
  if (bObjectsInit)
    return;


  /*
    Tell MEWEL that we created the stock objects already
  */
  bObjectsInit = TRUE;

  /*
    Create the stock brushes
  */
  CreateSolidBrush(rgbWhite);                  /* WHITE_BRUSH  0 */
  CreateSolidBrush(RGB(0x80, 0x80, 0x80));     /* LTGRAY_BRUSH 1 */
  CreateSolidBrush(RGB(0x20, 0x20, 0x20));     /* GRAY_BRUSH   2 */
  CreateSolidBrush(RGB(0x00, 0x00, 0x00));     /* DKGRAY_BRUSH 3 */
  CreateSolidBrush(RGB(0x00, 0x00, 0x00));     /* BLACK_BRUSH  4 */
  CreateNULLBrush(RGB(0xFF, 0xFF, 0xFF));      /* NULL_BRUSH   5 */

  /*
    Create the stock pens
  */
  CreatePen(PS_SOLID,1,rgbWhite);              /* WHITE_PEN    6 */
  CreatePen(PS_SOLID,1,RGB(0x00, 0x00, 0x00)); /* BLACK_PEN    7 */
  CreatePen(PS_NULL, 1,RGB(0x00, 0x00, 0x00)); /* NULL_PEN     8 */
  CreatePen(PS_SOLID,1,RGB(0x00, 0x00, 0x00)); /* ????????     9 */

  /*
    Create the stock fonts
        #define OEM_FIXED_FONT      10
        #define ANSI_FIXED_FONT     11
        #define ANSI_VAR_FONT       12
        #define SYSTEM_FONT         13
        #define DEVICE_DEFAULT_FONT 14
        #define DEFAULT_PALETTE     15
        #define SYSTEM_FIXED_FONT   16
  */
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  iSysFontHeight = GetProfileInt("fonts", "DefaultHeight", 13);
  iSysFontWidth  = GetProfileInt("fonts", "DefaultWidth",  8);
#endif

  for (i = OEM_FIXED_FONT;  i <= SYSTEM_FIXED_FONT;  i++)
  {
    LPCSTR lpszFont = 
      (i == OEM_FIXED_FONT || i == ANSI_FIXED_FONT || i == SYSTEM_FIXED_FONT)
        ? "FixedSys" : "System";

#if defined(USE_PALETTES)
    if (i == DEFAULT_PALETTE)
    {
#if defined(XWINDOWS)
      /*
        Defer palette creation until XMEWELInitMotif() is called. However,
        we need a dummy placeholder for the stock object....
      */
      CreateSolidBrush(rgbWhite);
#else
      InitPaletteManager();
#endif
      continue;
    }
#endif

    CreateFont(
#if defined(MEWEL_GUI) || defined(XWINDOWS)
              iSysFontHeight, iSysFontWidth, /* height, width */
#else
              1, 1,
#endif
              0,                   /* nEscapement  */
              0,                   /* nOrientation */
              FW_NORMAL,           /* nWeight      */
              FALSE, FALSE, FALSE, /* italic, underline, strikeout */
              DEFAULT_CHARSET,     /* cCharSet */
              OUT_DEFAULT_PRECIS,  /* cOutPrec  */
              CLIP_DEFAULT_PRECIS, /* cClipPrec */
              DEFAULT_QUALITY,     /* cQuality  */
              (i == ANSI_VAR_FONT || i == SYSTEM_FONT) ? VARIABLE_PITCH
                                   /* cPitch */        : FIXED_PITCH, 
              lpszFont);           /* face name */
   }


#if !defined(XWINDOWS)
  /*
    Create the default mono bitmap which a DC is born with.
  */
  SysGDIInfo.hDefMonoBitmap = CreateBitmap(1, 1, 1, 1, NULL);
#endif
}


BOOL FAR PASCAL IsGDIObject(hObj)
  HGDIOBJ hObj;
{
  if (((UINT) hObj & OBJECT_SIGNATURE) != 0)
    return (BOOL) (_ObjectTable[(UINT) hObj & ~OBJECT_SIGNATURE] != NULL);
  else
    return FALSE;
}


HBRUSH SysBrush[COLOR_BTNHIGHLIGHT+1];
HPEN   SysPen[3];

VOID FAR PASCAL InitSysBrushes(void)
{
  int  i;
  static BOOL bInitialized = FALSE;

  /*
    In case we got here again through a WinExec...
  */
  if (bInitialized)
    return;

  for (i = 0;  i <= COLOR_BTNHIGHLIGHT;  i++)
  {
    COLORREF clr = GetSysColor(i);
    SysBrush[i] = CreateSolidBrush(clr);
  }
  SysGDIInfo.fFlags |= GDISTATE_SYSBRUSH_INITIALIZED;
  SysPen[SYSPEN_BTNSHADOW]    = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
  SysPen[SYSPEN_BTNHIGHLIGHT] = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNHIGHLIGHT));
  SysPen[SYSPEN_WINDOWFRAME]  = CreatePen(PS_SOLID,1,GetSysColor(COLOR_WINDOWFRAME));

  bInitialized++;
}


#endif /* NOOBJECTS */

