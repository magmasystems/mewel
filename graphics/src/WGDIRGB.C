/*===========================================================================*/
/*                                                                           */
/* File    : WGDIRGB.C                                                       */
/*                                                                           */
/* Purpose : Rotuines which interface RGB colors to the device context       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_PALETTE_MGR

#include "wprivate.h"
#include "window.h"
#include "wobject.h"

#if defined(MEWEL_GUI) || defined(XWINDOWS)
#define USE_PALETTES
#endif

/*
  Hook to GUIs and graphics engines
*/
typedef DWORD (FAR PASCAL *GETCOLORPROC)(HDC);
GETCOLORPROC lpfnGetTextColorHook = (GETCOLORPROC) 0;
GETCOLORPROC lpfnGetBkColorHook = (GETCOLORPROC) 0;

typedef DWORD (FAR PASCAL *SETCOLORPROC)(HDC, DWORD);
SETCOLORPROC lpfnSetTextColorHook = (SETCOLORPROC) 0;
SETCOLORPROC lpfnSetBkColorHook = (SETCOLORPROC) 0;


DWORD FAR PASCAL SetBkColor(hDC, clr)
  HDC   hDC;
  DWORD clr;
{
  DWORD clrOld;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  if (lpfnSetBkColorHook)
    return (*lpfnSetBkColorHook)(hDC, clr);

  clrOld = lphDC->clrBackground;
  if (clr & 0xFFFFFFF0L)
  {
    /*
      The user passed an RGB value in
    */
    lphDC->clrBackground = clr;
    if ((clr = RGBtoAttr(hDC, clr)) == INTENSE(WHITE) &&
        !TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
      clr = WHITE;
  }
  else
    /*
      The user passed a BIOS color value
    */
    lphDC->clrBackground = AttrToRGB((UINT) clr);

  lphDC->attr = MAKE_ATTR((lphDC->attr & 0x0F), ((UINT) clr));

#if defined(XWINDOWS)
  if (lphDC->gc)
    XSetBackground(XSysParams.display, lphDC->gc, 
                   RGBtoAttr(hDC, lphDC->clrBackground));
#endif

  return clrOld;
}


DWORD FAR PASCAL GetBkColor(hDC)
  HDC  hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  if (lpfnGetBkColorHook)
    return (*lpfnGetBkColorHook)(hDC);
  return lphDC->clrBackground;
}


DWORD FAR PASCAL SetTextColor(hDC, clr)
  HDC   hDC;
  DWORD clr;
{
  DWORD clrOld;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  if (lpfnSetTextColorHook)
    return (*lpfnSetTextColorHook)(hDC, (DWORD) clr);

  clrOld = lphDC->clrText;

#ifdef ZAPP  
  /* Fixes problem for white color */
  if ((clr != 0xFFFFFFFFL) && (clr & 0xFFFFFFF0L))
#else
  if (clr & 0xFFFFFFF0L) 
#endif
  {
    /*
      The user passed an RGB value in
    */
    lphDC->clrText = clr;
    clr = RGBtoAttr(hDC, clr);
  }
  else
    /*
      The user passed a BIOS color value
    */
    lphDC->clrText = AttrToRGB((UINT) clr);


  lphDC->attr = MAKE_ATTR(((UINT) clr), ((lphDC->attr & 0xF0) >> 4));

#if defined(XWINDOWS)
  if (lphDC->gc)
    XSetForeground(XSysParams.display, lphDC->gc, 
                   RGBtoAttr(hDC, lphDC->clrText));
#endif

  return clrOld;
}


DWORD FAR PASCAL GetTextColor(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  if (lpfnGetTextColorHook)
    return (*lpfnGetTextColorHook)(hDC);
  return lphDC->clrText;
}


DWORD FAR PASCAL GetNearestColor(hDC, crColor)
  HDC      hDC;
  COLORREF crColor;
{
  return AttrToRGB(RGBtoAttr(hDC, crColor));
}


COLOR FAR PASCAL RGBtoAttr(hDC, clrText)
  HDC   hDC;
  DWORD clrText;
{
  BYTE uchAttr;
  BYTE uchRValue;
  BYTE uchGValue;
  BYTE uchBValue;
  UINT usColorTotal;


  (void) hDC;

  /*
    Is it a PALETTEINDEX?
    (It's not if we have a color like 0xFF808080. This kind of
     color can occur with Borland's sign-extension bugs.)
  */
  if ((clrText & 0xFF000000L) == 0x01000000L)
  {
#if defined(USE_PALETTES)
    return MapPaletteIndex(hDC, (INT) (clrText & 0x00FFL));
#else
    return (COLOR) (clrText & 0x00FFL);
#endif
  }

#if defined(XWINDOWS)
  return _XRGBtoColor(clrText);
#endif

  uchAttr = 0x00;

  uchRValue = (BYTE) GetRValue(clrText);
  uchGValue = (BYTE) GetGValue(clrText);
  uchBValue = (BYTE) GetBValue(clrText);

  usColorTotal = (UINT) uchRValue + (UINT) uchGValue + (UINT) uchBValue;

  if (usColorTotal && usColorTotal > (UINT) 256 *
      ((uchRValue != 0) + (uchGValue != 0) + (uchBValue !=  0)) / 2)
    uchAttr |= 0x08;
  if (uchRValue >= 0x80)
    uchAttr |= 0x04;
  if (uchGValue >= 0x80)
    uchAttr |= 0x02;
  if (uchBValue >= 0x80)
    uchAttr |= 0x01;

  if (uchAttr==0x07 && (uchRValue + uchGValue + uchBValue) <= (UINT)256*3/4)
    uchAttr = 0x08;
  /*
    See if we passed something line 0x202020. This means we should have
    bright black.
  */
  else if (uchAttr == 0 && uchRValue && uchGValue && uchBValue)
    uchAttr = 0x08;

  return (COLOR) uchAttr;
}


/****************************************************************************/
/*                                                                          */
/* Function : _PrepareWMCtlColor()                                          */
/*                                                                          */
/* Purpose  : Does all of the prep work to send a WM_CTLCOLOR message to a  */
/*            window.                                                       */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/

/*
  We define ALWAYS_SEND_CTLCOLOR if we want to send the WM_CTLCOLOR message
  to the window whenever the window is being painted. If this constant is
  not defined, then the WM_CTLCOLOR message will only be send to a
  control once and only once.
*/
#if !defined(MEWEL_TEXT)
#define ALWAYS_SEND_CTLCOLOR
#endif

BOOL FAR PASCAL _PrepareWMCtlColor(hWnd, iCode, hDC)
  HWND hWnd;
  UINT iCode;
  HDC  hDC;
{
  HDC    hDCOrig;
  LPHDC  lphDC;
  COLOR  attr;
  HBRUSH hBr;
  WINDOW *w;
  HWND   hParent;
  BOOL   rc = FALSE;

  /*
    Get a pointer to the WINDOW structure
  */
  w = WID_TO_WIN(hWnd);

#if !defined(ALWAYS_SEND_CTLCOLOR)
  /*
    If we already mucked with the window's color, don't do anything.
  */
  if (w->ulStyle & WIN_DIDDLED_COLOR)
    return TRUE;
#endif

  /*
    We need a device context for the wParam of the WM_CTLCOLOR message.
  */
  if ((hDCOrig = hDC) == NULL)
    if ((hDC = GetDC(hWnd)) == 0)
      return FALSE;


#if !defined(ALWAYS_SEND_CTLCOLOR)
  /*
    Don't alter the colors if this control has a custom defined color.
    Message boxes should always get processed however.
  */
#if defined(USE_SYSCOLOR)
  if (!(w->flags & WS_SYSCOLOR) && !IS_MSGBOX(w))
#else
  /*
    5/7/93 (maa) - Attempt to get rid of WS_SYSCOLOR.
      Of course, if a window has the attribute of 0xFF, this logic is
    wrong, but the chances of this are remote.
  */
  if (w->attr != SYSTEM_COLOR && !IS_MSGBOX(w))
#endif
    goto bye;
#endif


  /*
    Determine the window which the WM_CTLCOLOR message will be sent to.
    This is the parent of a window, or in the case of a dialog box, the
    window itself.
  */
  if (!w->parent || IS_DIALOG(w))
  {
    if (iCode == CTLCOLOR_MSGBOX)
      hParent = w->hWndOwner;
    else if (iCode == CTLCOLOR_DLG)
      hParent = hWnd;
    else
      goto bye;
  }
  else
    hParent = w->parent->win_id;


  /*
    Ta da! Sendthe WM_CTlCOLOR message.
  */
  hBr = (HBRUSH)SendMessage(hParent, WM_CTLCOLOR, hDC, MAKELONG(hWnd,iCode));

  /*
    Test the return code and set the attribute.
  */
  if (hBr)
  {
    LOGBRUSH lb;

    lphDC = _GetDC(hDC);

    /*
      Use the brush color as the background color.
    */
    GetObject((UINT) hBr & 0x7FFF, sizeof(lb), (LPSTR) &lb);
    lphDC->clrBackground = lb.lbColor;
    lphDC->hBrush = hBr;

    /*
      Set the 'attr' field of the window.
    */
    attr = MAKE_ATTR(RGBtoAttr(0, lphDC->clrText),
                     RGBtoAttr(0, lphDC->clrBackground));
    if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
      attr &= 0x7F;
    w->attr = attr;
  
    /*
      Tell the window that we do not need to send WM_CTLCOLOR messages anymore.
    */
#if !defined(ALWAYS_SEND_CTLCOLOR)
#if defined(USE_SYSCOLOR)
    w->flags &= ~(WS_SYSCOLOR);
#endif
    w->ulStyle |= WIN_DIDDLED_COLOR;
#endif

    /*
      Successful return code.
    */
    rc = TRUE;
  }


  /*
    Get rid of the DC
  */
bye:
  if (hDCOrig == NULL)
    ReleaseDC(hWnd, hDC);
  return rc;
}



/****************************************************************************/
/*                                                                          */
/* Function : MapPaletteIndex(hDC, i)                                       */
/*                                                                          */
/* Purpose  : Given a device context and a index into its logical palette,  */
/*            returns the index into the system hardware palette. This      */
/*            value will be passed to the underlying graphics engine's      */
/*            _setcolor() routine.                                          */
/*            This is called by RGBtoAttr when a PALETTEINDEX() color is    */
/*            encountered.                                                  */
/*                                                                          */
/* Returns  : The corresponding hardware palette index                      */
/*                                                                          */
/****************************************************************************/
#if defined(USE_PALETTES)
INT FAR PASCAL MapPaletteIndex(hDC, i)
  HDC      hDC;
  INT      i;         /* palette index number */
{
  LPHDC    lphDC;
  HPALETTE hPal;      /* or a DC if bSystemPalette is TRUE */
  LPOBJECT lpObj;
  HANDLE   hMem;
  LPINTLOGPALETTE lpLogPal;

  if ((lphDC = _GetDC(hDC)) == NULL || (hPal = lphDC->hPalette) == NULL)
    return i;

  /*
    Get the handle of the palette's memory block
  */
  if ((lpObj = _ObjectDeref(hPal)) == NULL)
    return i;
  hMem = lpObj->uObject.uhPalette;

  /*
    Get a pointer to the palette.
  */
  if ((lpLogPal = (LPINTLOGPALETTE) GlobalLock(hMem)) != NULL)
  {
    LPPALINDEX lpPalIndex;

    /*
      Make sure that the index does not exceed the number of entries in
      the palette, or else we'll get a protection exception.
    */
    if (i >= lpLogPal->palNumEntries)
      i %= lpLogPal->palNumEntries;
    /*
      Do the mapping.
    */
    lpPalIndex = (LPPALINDEX) (&lpLogPal->palPalEntry[lpLogPal->palNumEntries]);
    i = lpPalIndex[i];
    GlobalUnlock(hMem);
  }
  return i & 0x00FF;
}
#endif

