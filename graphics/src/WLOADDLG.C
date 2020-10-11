/*===========================================================================*/
/*                                                                           */
/* File    : WLOADDLG.C                                                      */
/*                                                                           */
/* Purpose : Implements the dialog loading functions                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#include "wprivate.h"
#include "window.h"

/*
  For certain systems (like the SUN) which require integer addressing on
  4-byte boundaries, the dialog font size must be a 4-byte integer.
*/
#if defined(WORD_ALIGNED)
typedef       int RCSHORT;
#else
typedef short int RCSHORT;
#endif


typedef struct tagFontInfo
{
  RCSHORT   PointSize;
  char      szTypeface[1];  /* null-terminated string */
} FONTINFO, FAR *LPFONTINFO;

typedef struct DlgTemplate
{
  DWORD dwStyle;
  /*
  BYTE  nItems;
  INT   x, y, cx, cy;
  char szMenuName[];
  char szClassName[];
  char szCaption[];
  if DS_SETFONT set
  struct tagFontInfo FontInfo;
  */
  /*
  INT  attr;   MEWEL-specific 
  */
} WDLGHEADER, FAR *LPWDLGHEADER;


typedef struct DlgItemTemplate
{
  INT   x, y, cx, cy;
  INT   id;
  DWORD dwStyle;
  /*
  char szClass[];
    if first byte is > 0x80, then use the CT_xxx constant
  char szTitle[];
  BYTE nInfoBytes;
  BYTE szInfo[];
  */
  /*
  INT  attr;  MEWEL-specific
  */
} WDLGITEMHEADER, FAR *LPWDLGITEMHEADER;


BOOL bWinCompatDlg = TRUE;

LPSTR PASCAL SpanString(LPSTR);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
static HFONT PASCAL CreateDialogFont(HDC hDC, LPSTR lpszFont, int iPointSize);
#endif


HWND FAR PASCAL LoadDialog(hModule, idDialog, hParent, dlgfunc)
  HINSTANCE hModule;
  LPCSTR  idDialog;
  HWND    hParent;
  FARPROC dlgfunc;
{
  HWND   hDlg;
  HANDLE hRes;
  LPSTR  pData = LoadResourcePtr(hModule, (LPSTR) idDialog, RT_DIALOG, &hRes);

  if (pData)
  {
    hDlg = _CreateDialogIndirect(hModule, pData, hParent, dlgfunc);

    /*
      If we pass hRes as the last arg to UnloadResourcePtr, then it will
      free up the memory occupied by the resource, but the next time
      we load the dialog, the resource functions will have to go out
      to disk to load the resource. If we pass NULL as the third arg,
      then the memory will not be freed, but the disk access will not
      have to be done.
    */
#if 1
    UnloadResourcePtr(hModule, pData, NULL);
#else
    UnloadResourcePtr(hModule, pData, hRes);
#endif

    return hDlg;
  }
  else
    return NULL;
}


HWND FAR PASCAL _CreateDialogIndirect(hInst, lpDlg, hParent, dlgfunc)
  HINSTANCE hInst;
  CONST VOID FAR *lpDlg;
  HWND    hParent;
  FARPROC dlgfunc;
{
  LPSTR  pData;
  HWND   hDialog = NULLHWND;
  int    nItems;
  DWORD  dwStyle;
  LPWDLGHEADER wdlgheader;
  RECT   rDlg;
  COLOR  attr;
  HWND   hCtrl;
  WINDOW *wDlg;

  LPSTR  szTitle;
  LPSTR  szClass;
  LPSTR  szMenu;
  LPSTR  lpszFont;
  char   szTmp[2];

  BYTE   nInfoBytes;
  LPSTR  lpInfoBytes;
  INT    x, y, cx, cy;
  INT    iPointSize;

#if defined(MEWEL_GUI) || defined(XWINDOWS)
  INT    cxNumer, cyNumer;
#endif

  HFONT  hDlgFont = NULL;
  HFONT  hOldFont;


  if ((pData = (LPSTR) lpDlg) == NULL)
    goto bye;

  /*
    Get a pointer to the dialog header
  */
  wdlgheader = (LPWDLGHEADER) pData;

  /*
    Get the dialog style, and then move past the style
  */
  dwStyle = wdlgheader->dwStyle;
  pData += sizeof(wdlgheader->dwStyle);

  /*
    Get the number of controls in this dialog box
  */
  nItems = *pData++;
#if defined(WORD_ALIGNED)
  pData += 3;   /* pad out to 4-byte boundary */
#endif

  /*
    Note - these coordinates are really the coordinates of the
    dialog box's client area
  */
  x  =  ((INT   FAR *) pData)[0];
  y  =  ((INT   FAR *) pData)[1];
  cx =  ((INT   FAR *) pData)[2];
  cy =  ((INT   FAR *) pData)[3];

  pData += sizeof(x) + sizeof(y) + sizeof(cx) + sizeof(cy);

  /*
    Setup menu
  */
  szMenu = pData;
  if (szMenu[0] == 0xFF)
  {
    /*
      The menu has a numeric id. Span the 0xFF and grab the unsigned
      short menu id.
    */
#if defined(WORD_ALIGNED)
    pData += 4;
#else
    pData += 1;
#endif
    szMenu = (LPSTR) MAKEINTRESOURCE(*(USHORT FAR*) (pData));
    pData += sizeof(USHORT);
  }
  else
    pData  = SpanString(pData);

  /*
    Setup class
  */
  szClass = pData;
  pData  = SpanString(pData);

  /*
    Setup caption
  */
  szTitle = pData;
  pData  = SpanString(pData);

  /*
    Span font info
  */
  if (dwStyle & DS_SETFONT)
  {
    iPointSize = * (RCSHORT *) pData;
    pData += sizeof(RCSHORT);  /* span the point size */
    lpszFont = pData;
    pData  = SpanString(pData);
  }
  else
    lpszFont = NULL;

  /*
    Get color attribute
  */
  if (!bWinCompatDlg)
  {
    attr = (COLOR) (* ((INT FAR *) pData));
    pData += sizeof(INT);
  }
  else
    attr = SYSTEM_COLOR;


  /*
    If the control have the WS_VISIBLE attribute, we don't want them
    to be displayed before the dialog box is (unless the dialog box
    itself has WS_VISIBLE). Therefore, turn on the hidden bit in
    the dialog box and turn it back off after we create the controls.
  */
  dwStyle &= ~WS_VISIBLE;

  /*
    Figure out the window area given the client area
  */
  SetRect(&rDlg, x, y, x + cx, y + cy);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
  {
  /*
    We must multiply the coordinates by the dialogs unit. The x and cx
    coordinates are 1/4 of the system char width. Since the width is
    always 8, then we must multiply by (8/4) = 2. The y and cy coords
    are 1/8 of the height of the system character. If the character
    height is 16, then we must multiply by (16/8) = 2. If the height
    is 8 (as it is with BGI), then we should multiply by (8/8) = 1.
  */
  HDC        hDC;
  TEXTMETRIC tm;

  hDC = GetDC(_HwndDesktop);

  if (lpszFont)
    hDlgFont = CreateDialogFont(hDC, lpszFont, iPointSize);
  else
    hDlgFont = CreateDialogFont(hDC, NULL, 8);

  /*
    Get the text metrics
  */
  if (hDlgFont)
    hOldFont = SelectObject(hDC, hDlgFont);
  GetTextMetrics(hDC, &tm);
  if (hDlgFont)
    SelectObject(hDC, hOldFont);
  ReleaseDC(_HwndDesktop, hDC);

  cxNumer = tm.tmAveCharWidth;
  cyNumer = tm.tmHeight + tm.tmExternalLeading;

#if defined(XWINDOWS) && 32095
  cxNumer = max(8,  cxNumer);
  cyNumer = max(16, cyNumer);
#endif

  rDlg.left   = (MWCOORD) (rDlg.left   * cxNumer) >> 2;
  rDlg.top    = (MWCOORD) (rDlg.top    * cyNumer) >> 3;
  rDlg.right  = (MWCOORD) (rDlg.right  * cxNumer) >> 2;
  rDlg.bottom = (MWCOORD) (rDlg.bottom * cyNumer) >> 3;
  AdjustWindowRect(&rDlg, dwStyle, (BOOL) (szMenu[0] != '\0'));
  }
#else
  (void) iPointSize;
  (void) lpszFont;
  (void) hOldFont;
#if defined(MEWEL_TEXT) && defined(__DPMI16__)
  rDlg.left   /= 4;
  rDlg.top    /= 8;
  rDlg.right  /= 4;
  rDlg.bottom /= 8;
  if (HAS_BORDER(dwStyle))
    InflateRect(&rDlg, 1, 1);
#endif
#endif


  /*
    Make sure that the dialog ends up in a good part of the screen.
    Do this only if a dialog is a top-level window.
  */
  if (!(dwStyle & WS_CHILD))
  {
    if (rDlg.right  >= (MWCOORD) VideoInfo.width)
      OffsetRect(&rDlg, VideoInfo.width - rDlg.right, 0);
    if (rDlg.bottom >= (MWCOORD) VideoInfo.length)
      OffsetRect(&rDlg, 0, VideoInfo.length - rDlg.bottom);
    if (rDlg.left < 0)
      OffsetRect(&rDlg, -rDlg.left, 0);
    if (rDlg.top  < 0)
      OffsetRect(&rDlg, 0, -rDlg.top);
  }

  /*
    Create the dialog box
  */
  hDialog = DialogCreate(hParent,
                         rDlg.top, rDlg.left, rDlg.bottom, rDlg.right,
                         szTitle,
                         attr,
                         dwStyle,
                         dlgfunc,
                         0,    /* id */
                         szClass,
                         hInst
                       );
  if (!hDialog)
    goto bye;

  wDlg = WID_TO_WIN(hDialog);
  SET_WS_HIDDEN(wDlg);

  /*
    If the dialog box has a menu, load it in if it hasn't been attached
    already.
  */
  if (szMenu[0] && GetMenu(hDialog) == NULLHWND)
  {
    HMENU hMenu = LoadMenu(hInst, szMenu);
    if (hMenu)
      SetMenu(hDialog, hMenu);
  }


  /*
    Process the dialog box children
  */
  while (nItems-- > 0)
  {
    LPWDLGITEMHEADER wdlgitemheader;
#if defined(MEWEL_TEXT) && defined(__DPMI16__)
    BOOL bHasBorder;
#endif

    wdlgitemheader = (LPWDLGITEMHEADER) pData;
    pData += sizeof(WDLGITEMHEADER);


    /*
      Get the class
    */
    if (*pData & CT_MASK)
    {
      szTmp[0] = *pData++;
      szTmp[1] = '\0';
      szClass  = szTmp;
    }
    else
    {
      szClass = pData;
      pData  = SpanString(pData);
    }

    /*
      Get the title
    */
    szTitle = pData;
    if (szTitle[0] == 0xFF)
      szTitle = (LPSTR) MAKEINTRESOURCE(*(USHORT FAR*) (pData+1));
    pData  = SpanString(pData);

    /*
      Get the create data
    */
    nInfoBytes = (BYTE) (* (LPSTR) pData);
    pData += sizeof(nInfoBytes);
#if defined(WORD_ALIGNED)
    pData += 3;   /* pad out to 4-byte boundary */
#endif
    if (nInfoBytes)
    {
      if ((lpInfoBytes = EMALLOC_FAR(nInfoBytes)) != NULL)
        lmemcpy(lpInfoBytes, pData, nInfoBytes);
      pData += nInfoBytes;
    }

    /*
      Get the color attribute
    */
    if (!bWinCompatDlg)
    {
      attr = (COLOR) (* ((INT FAR *) pData));
      pData += sizeof(INT);
    }
    else
      attr = SYSTEM_COLOR;

#if defined(MEWEL_TEXT) && defined(__DPMI16__)
    bHasBorder = (BOOL) ((wdlgitemheader->dwStyle & WS_BORDER) != 0);
#endif

    /*
      Create the child window
    */
    hCtrl = 
    _CreateWindow(szClass,
                  szTitle, 
                  wdlgitemheader->dwStyle,
#if defined(MEWEL_GUI) || defined(XWINDOWS)
                  (wdlgitemheader->x  * cxNumer) >> 2,
                  (wdlgitemheader->y  * cyNumer) >> 3,
                  (wdlgitemheader->cx * cxNumer) >> 2,
                  (wdlgitemheader->cy * cyNumer) >> 3,
#elif defined(MEWEL_TEXT) && defined(__DPMI16__)
                  (wdlgitemheader->x)  / 4,
                  (wdlgitemheader->y)  / 8,
                  (wdlgitemheader->cx) / 4 + (2 * bHasBorder),
                  (wdlgitemheader->cy) / 8 + (2 * bHasBorder),
#else
                  wdlgitemheader->x,
                  wdlgitemheader->y,
                  wdlgitemheader->cx,
                  wdlgitemheader->cy,
#endif
                  attr,
                  wdlgitemheader->id,
                  hDialog,
                  NULLHWND,
                  hInst,
                  nInfoBytes ? lpInfoBytes : NULL);

     (void) hCtrl;

    /*
      Free the lpCreateParams data
    */
    if (nInfoBytes)
    {
      MYFREE_FAR(lpInfoBytes);
      pData += nInfoBytes;
    }
  }  /* end while (nItems) */


  /*
    Set the visibility state back to its original value. We need to do this
    in case we keep using the template.
  */
  CLR_WS_HIDDEN(wDlg);

#ifndef IMS
  if (wdlgheader->dwStyle & WS_VISIBLE)
#endif
    WID_TO_WIN(hDialog)->flags |= WS_VISIBLE;


  if (hDlgFont)
  {
    SendMessage(hDialog, WM_SETFONT, hDlgFont, 0L);
    for (wDlg = wDlg->children;  wDlg;  wDlg = wDlg->sibling)
      SendMessage(wDlg->win_id, WM_SETFONT, hDlgFont, 0L);
  }


  /*
    Reset bWinCompatDlg so CreateDialogIndirect can use it.
  */
bye:
  bWinCompatDlg = TRUE;
  return hDialog;
}


#if defined(MEWEL_GUI) || defined(XWINDOWS)
static HFONT PASCAL CreateDialogFont(HDC hDC, LPSTR lpszFont, int iPointSize)
{
  LOGFONT logFont;
  HFONT   hFont;
  BOOL    bCreatingDefaultFont = FALSE;

  static HFONT hDefaultDlgFont = (HFONT) 0;

  if (lpszFont == NULL)
  {
    if (hDefaultDlgFont)
      return hDefaultDlgFont;
    lpszFont = "System";
    bCreatingDefaultFont = TRUE;
  }


  /*
    Build a logfont structure with some good values in it. Get the good
    values from the stock font.
  */
  hFont = GetStockObject(SYSTEM_FONT);
  GetObject(hFont, sizeof(logFont), &logFont);

  /*
    Modify the LOGFONT structure so that it has the values specified
    in the  FONT ptsize,"FontName"  statement.

    We must get lfHeight into a pixel size. There are 72 points in
    one inch, so an 8 point font is 8/72==1/9 of an inch. If there are
    96 pixels to one inch on a VGA display, then an 8-point font
    should be 96 * (1/9) = 10.667 pixels high. (We will add 36 to
    the numerator so we can perform rounding.)
  */
  if (bCreatingDefaultFont)
    iPointSize = logFont.lfHeight;  /* use the existing height */
  logFont.lfHeight = (iPointSize * GetDeviceCaps(hDC,LOGPIXELSY) + 36) / 72;
  logFont.lfWidth  = 0;
  logFont.lfPitchAndFamily = VARIABLE_PITCH;
  lstrcpy(logFont.lfFaceName, lpszFont);

  /*
    Create the font
  */
  hFont = CreateFontIndirect(&logFont);

  if (bCreatingDefaultFont)
    hDefaultDlgFont = hFont;

  return hFont;
}
#endif

