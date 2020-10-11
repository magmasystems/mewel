/*===========================================================================*/
/*                                                                           */
/* File    : WINDC.C                                                         */
/*                                                                           */
/* Purpose : Routines to validate/invalidate update rectangles, and routines */
/*           to deal with device contents and paintstructs.                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  Array of pointers to active device contexts, and the number of slots
  currently allocated.
*/
static LPHDC FAR *_HDCcache = (LPHDC FAR *) 0;
static UINT       nHDCcache = 0;
static BOOL       bCreatingCompatDC = FALSE;
static BOOL       bDoingWindowDC = FALSE;

/*
  Hook to GUIs and graphics engines
*/
typedef INT (FAR PASCAL *GETDCPROC)(HDC);
GETDCPROC lpfnGetDCHook = (GETDCPROC) 0;

typedef INT (FAR PASCAL *RELEASEDCPROC)(HDC);
RELEASEDCPROC lpfnReleaseDCHook = (RELEASEDCPROC) 0;

extern int FAR PASCAL _VidDevCaps(HDC, int);
static VOID PASCAL _SetupDC(HWND, HDC);



LPHDC FAR PASCAL _GetDC(hDC)
  HDC hDC;
{
  if (hDC > 0 && hDC <= nHDCcache)
    return _HDCcache[(UINT) hDC-1];
  else
    return (LPHDC) NULL;
}


HDC WINAPI GetDCEx(HWND hWnd, HRGN hrgnClip, DWORD fdwOptions)
{
  (void) hrgnClip;
  (void) fdwOptions;
  return GetDC(hWnd);
}


/*
  GetDC()
    Allocates a display context
*/
HDC FAR PASCAL GetDC(hWnd)
  HWND hWnd; /* can be HWND_NONDISPLAY if we are getting a DC for a device */
{
  HDC    hDC;
  LPHDC  lphDC;
  WINDOW *w;

  if (hWnd != HWND_NONDISPLAY)
  {
    /*
      In MS Windows, GetDC(0) returns a DC for the desktop
    */
    if (hWnd == NULLHWND)
      hWnd = _HwndDesktop;

    /*
      Get a ptr to the window structure for additional info
    */
    if ((w = WID_TO_WIN(hWnd)) == NULL)
      return (HDC) 0;

    /*
      Take care of class-owned and window-owned DCs. If we are not
      creating a compatible DC (were we definitely need a new DC),
      then see if we acan use the existing window or class DC.
    */
    if (!bCreatingCompatDC)
    {
      DWORD dwClassStyle = GetClassStyle(hWnd);
      if (dwClassStyle & CS_CLASSDC)
      {
        if ((hDC = ClassIDToClassStruct(w->idClass)->hDC) != (HDC) 0)
        {
          lphDC = _GetDC(hDC);
          lphDC->fFlags |= (DC_RESETING_OWNDC | DC_ISCLASSDC);
          lphDC->iLockCnt++;
          goto setup;
        }
      }
      else if ((dwClassStyle & CS_OWNDC) && w->hDC != 0)
      {
        lphDC = _GetDC(hDC = w->hDC);
        lphDC->fFlags |= (DC_RESETING_OWNDC | DC_ISOWNDC);
        lphDC->iLockCnt++;
        goto setup;
      }
    }
  }


  /*
    Set up the array of DC pointers the very first time
  */
  if (nHDCcache == 0)
  {
    nHDCcache = 8;
    _HDCcache = (LPHDC FAR *) emalloc_far((DWORD) nHDCcache * sizeof(LPHDC));
    if (_HDCcache == NULL)
      return (HDC) 0;
  }          

  /*
    Search for the first free DC slot. This is a slot with no allocated
    DC structure, or a DC which has its "in-use" bit off.
  */
  for (hDC = 0;  hDC < nHDCcache;  hDC++)
  {
    if (_HDCcache[(UINT)hDC] == (LPHDC) NULL || IS_DCFREE(_HDCcache[(UINT)hDC]))
      break;
  }

  /*
    If we have no more slots, then reallocate the DC cache.
  */
  if (hDC == nHDCcache)
  {
    if (!GmemRealloc((LPSTR FAR *) &_HDCcache, &nHDCcache, sizeof(LPHDC), 2))
      return (HDC) 0;
  }


  /*
    Allocate a DC structure
  */
  if ((lphDC = _HDCcache[(UINT)hDC]) == NULL)
  {
    if ((lphDC = (LPHDC) emalloc(sizeof(DC))) == NULL)
      return (HDC) 0;
  }
  else
    lmemset((LPSTR) lphDC, 0, sizeof(DC));
  _HDCcache[(UINT)hDC] = lphDC;
  lphDC->fFlags |= DC_ISUSED;

  /*
    For future referencing, the hDC should be 1-based
  */
  hDC++;

setup:
  _SetupDC(hWnd, hDC);
  lphDC->fFlags &= ~DC_RESETING_OWNDC;
  return hDC;
}


HDC FAR PASCAL GetWindowDC(hWnd)
  HWND hWnd;
{
  HDC   hDC;

  bDoingWindowDC = TRUE;
  hDC = GetDC(hWnd);
  bDoingWindowDC = FALSE;
  return hDC;
}


HDC FAR PASCAL CreateCompatibleDC(hDC)
  HDC  hDC;
{
  HDC  hDCNew = (HDC) NULL;
  HWND hWnd;
  LPHDC lphDC;

  if (hDC)
  {
    lphDC = _GetDC(hDC);
    hWnd  = lphDC->hWnd;
  }
  else
  {
    lphDC = NULL;
    hWnd = _HwndDesktop;
  }

  /*
    Let GetDC know that we need a new DC, no matter if the window
    has a window or class DC associated with it.
  */
  bCreatingCompatDC = TRUE;
  hDCNew = GetDC(hWnd);
#if !91294 || 101394
  if (lphDC)
  {
    LPHDC lphNewDC = _GetDC(hDCNew);
    lphNewDC->wBitsPerPixel = lphDC->wBitsPerPixel;
  }
#endif
  bCreatingCompatDC = FALSE;
  return hDCNew;
}



static VOID PASCAL _SetupDC(hWnd, hDC)
  HWND  hWnd;
  HDC   hDC;
{
  LPHDC  lphDC = _GetDC(hDC);
  WINDOW *w;
  COLOR  attr;

  /*
    Fill it with some info, including the clipping rectangle
  */
  lphDC->hWnd = hWnd;

  /*
    DC color attributes
  */
  if (hWnd != HWND_NONDISPLAY)
  {
    w = WID_TO_WIN(hWnd);
    if ((attr = w->attr) == SYSTEM_COLOR)
      attr = WinGetClassBrush(hWnd);
  }
  else
    attr = MAKE_ATTR(BLACK, INTENSE(WHITE));


  if (!IS_RESETING_OWNDC(lphDC))
  {
    /*
      Fill in the attribute info.
    */
    lphDC->iLockCnt      = 1;
    lphDC->attr          = attr;
    lphDC->clrText       = AttrToRGB(GET_FOREGROUND(attr));
    lphDC->clrBackground = AttrToRGB(GET_BACKGROUND(attr));

    /*
      Fill in the GDI stuff
    */
    lphDC->hPen            = GetStockObject(BLACK_PEN);
    lphDC->hBrush          = SysBrush[COLOR_WINDOW];
    lphDC->hBitmap         = SysGDIInfo.hDefMonoBitmap;
    lphDC->hPalette        = SysGDIInfo.hSysPalette;
    lphDC->wBackgroundMode = OPAQUE;
    lphDC->wDrawingMode    = R2_COPYPEN;
    lphDC->wMappingMode    = MM_TEXT;
    lphDC->extView.cx      = 1;
    lphDC->extView.cy      = 1;
    lphDC->extWindow.cx    = 1;
    lphDC->extWindow.cy    = 1;
    lphDC->wBitsPerPixel   = bCreatingCompatDC ? 1 : SysGDIInfo.nBitsPerPixel;

    /*
      Fill in some of the devcaps info
    */
    lphDC->devInfo.cxPixels    = SysGDIInfo.cxScreen;
    lphDC->devInfo.cyPixels    = SysGDIInfo.cyScreen;
    lphDC->devInfo.cxMM        = SysGDIInfo.cxScreenMM;
    lphDC->devInfo.cyMM        = SysGDIInfo.cyScreenMM;
    lphDC->devInfo.cxLogPixels = SysGDIInfo.cxLogPixel;
    lphDC->devInfo.cyLogPixels = SysGDIInfo.cyLogPixel;

    /*
      If the window does not have a custom font attached to it, then
      use the stock system font.
    */
    lphDC->hFont           = GetStockObject(SYSTEM_FONT);
    if (hWnd != HWND_NONDISPLAY && w->hFont)
      lphDC->hFont = w->hFont;
  } /* end if not reseting owner dc */


  /*
    If we got here from CreateCompatibleDC, then tell MEWEL that this
    DC is a memory DC.
  */
  if (bCreatingCompatDC)
    lphDC->fFlags |= DC_ISMEMDC;

  /*
    Compute the clipping rectangle. This is in screen coordinates.
    Note - we shouldn't calculate a clipped clipping rect. The 
    clipping rect should be the whole client area to start with.
  */
  if (hWnd != HWND_NONDISPLAY)
  {
    if (bDoingWindowDC)
    {
      lphDC->fFlags |= DC_ISWINDOWDC;
      lphDC->rClipping = w->rect;
    }
    else
      lphDC->rClipping = w->rClient;

    /*
      Get the DC origin
    */
    lphDC->ptOrg.x = lphDC->rClipping.left;
    lphDC->ptOrg.y = lphDC->rClipping.top;
  }
  else
  {
    lphDC->ptOrg.x = 0;
    lphDC->ptOrg.y = 0;
  }

  /*
    Get a pointer to the GetDeviceCaps() procedure
  */
  lphDC->lpfnDevCapsProc = _VidDevCaps;

  /*
    Call the GUI-specific hook
  */
  if (hWnd != HWND_NONDISPLAY && lpfnGetDCHook)
    (*lpfnGetDCHook)(hDC);
}



/*
  ReleaseDC()
    Frees up the memory occupied by a device context
*/
INT FAR PASCAL ReleaseDC(hWnd, hDC)
  HWND hWnd;
  HDC  hDC;
{
  LPHDC lphDC;

  if (hWnd == NULLHWND)
    hWnd = _HwndDesktop;

  if ((lphDC = _GetDC(hDC)) != NULL && lphDC->hWnd == hWnd)
  {
    /*
      Reduce the lock count
    */
    lphDC->iLockCnt--;

    /*
      Call the device-specific DC-releasing hook.
    */
    if (hWnd != HWND_NONDISPLAY && lpfnReleaseDCHook)
      (*lpfnReleaseDCHook)(hDC);

    /*
      Free the visibility region.
    */
    if (lphDC->hRgnVis)
    {
      LocalFree(lphDC->hRgnVis);
      lphDC->hRgnVis = NULL;
    }

    /*
      Free the clipping region
    */
    if (lphDC->hRgnClip)
    {
      DeleteObject(lphDC->hRgnClip);
    }

    if (hWnd != HWND_NONDISPLAY)
    {
      /*
        Do not release a DC which is associated with CS_OWNDC or CS_CLASSDC
      */
      if (IS_OWN_OR_CLASSDC(lphDC))
        return TRUE;
    }

    /*
      Free any saved DCs and extra info.
    */
    if (lphDC->lpDCExtra)
      MYFREE_FAR(lphDC->lpDCExtra);
    if (lphDC->listSavedDCs)
      ListFree(&lphDC->listSavedDCs, TRUE);

#if 101992
    /*
      Don't free the DC memory, but just turn the "in-use" bit off.
    */
    lphDC->fFlags &= ~DC_ISUSED;
#else
    MyFree(lphDC);
    _HDCcache[hDC-1] = (LPHDC) NULL;
#endif

    return TRUE;
  }
  else
    return FALSE;
}


DWORD FAR PASCAL GetDCOrg(hDC)
  HDC hDC;
{
  LPHDC  lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;
  else
    return MAKELONG(lphDC->ptOrg.x, lphDC->ptOrg.y);
}


/****************************************************************************/
/*                                                                          */
/* Function : BeginPaint(hWnd, &ps)                                         */
/*                                                                          */
/* Purpose  : Prepares a window for painting. Allocates a device context    */
/*            the window, sets the clipping rectangle, and possibly erases  */
/*            the window background. The update region is then validated.   */
/*                                                                          */
/* Returns  : The device context for the window to be painted.              */
/*                                                                          */
/****************************************************************************/
HDC FAR PASCAL BeginPaint(hWnd, lpPaint)
  HWND hWnd;
  LPPAINTSTRUCT lpPaint;
{
  RECT   rClipping;
  WINDOW *w;
  LPHDC  lphDC;

  /*
    A valid window and paintstruct pointer?
  */
  if (!lpPaint || (w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return (HDC) 0;

  /*
    Allocate a device context for the window
  */
  if ((lpPaint->hdc = GetDC(hWnd)) == (HDC) 0)
    return (HDC) 0;
  lphDC = _GetDC(lpPaint->hdc);

  /*
    Put the update rect into the paintstruct.
  */
  GetUpdateRect(hWnd, &lpPaint->rcPaint, FALSE);
  rClipping = lphDC->rClipping;
#if !defined(MOTIF)
  WinScreenRectToClient(hWnd, (LPRECT) &rClipping);
#endif
  IntersectRect((LPRECT) &lpPaint->rcPaint, (LPRECT) &lpPaint->rcPaint, 
                (LPRECT) &rClipping);

#if defined(XWINDOWS)
  IntersectClipRect(lpPaint->hdc,
                    lpPaint->rcPaint.left,  lpPaint->rcPaint.top,
                    lpPaint->rcPaint.right, lpPaint->rcPaint.bottom);
#endif

  /*
    Also set the DC's clipping rectangle. This must be in screen coords.
  */

  /*
    6/24/92 (maa)
    The hDC's clipping area should really be the entire window and
    *not* the update region. We should leave it to the app to
    use the only area specified in the rcPaint structure.
    6/25/92 (maa)
    If we set the clipping region to the entire window instead of to
    just the invalid area, then how will the FillRect() caused
    by the WM_ERASEBKGND below know to erase only the invalid area?
  */
  lphDC->rClipping = lpPaint->rcPaint;
#if !defined(MOTIF)
  WinClientRectToScreen(hWnd, (LPRECT) &lphDC->rClipping);
#endif


  /*
    Did the user previously specify fErase==TRUE?
  */
  if (w->ulStyle & WIN_SEND_ERASEBKGND)
  {
    /*
      Take off the "needs erasing" flag and set fErase to TRUE. Do this
      before the sending of WM_ERASEBACKGROUND in case the app calls
      BeginPaint() from WM_ERASEBACKGROUND.
    */
    w->ulStyle &= ~WIN_SEND_ERASEBKGND;
    lpPaint->fErase = TRUE;

    /*
      Send the WM_CTLCOLOR message for dialog boxes and message boxes
    */
    if (IS_DIALOG(w))
      _PrepareWMCtlColor(hWnd, IS_MSGBOX(w) ? CTLCOLOR_MSGBOX : CTLCOLOR_DLG, 
                         lpPaint->hdc);
    /*
      Send either the WM_ERASEBKGND or WM_ICONERASEBKGND message
    */
    SendMessage(hWnd,
                ((w->flags & WS_MINIMIZE) && ClassIDToClassStruct(w->idClass)->hIcon)
                  ? WM_ICONERASEBKGND : WM_ERASEBKGND,
                lpPaint->hdc, 0L);
  }
  else
    lpPaint->fErase = FALSE;

  /*
    Validate the entire window
  */
  SetUpdateRect(hWnd, (LPRECT) NULL);

  /*
    Return the DC
  */
  return lpPaint->hdc;
}


/****************************************************************************/
/*                                                                          */
/* Function : EndPaint(hWnd, &ps)                                           */
/*                                                                          */
/* Purpose  : Deallocates the DC which BeginPaint() created.                */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EndPaint(hWnd, lpPaint)
  HWND hWnd;
  CONST PAINTSTRUCT FAR *lpPaint;
{
  /*
    All we need to do is release the device context.
  */
  if (lpPaint)
    ReleaseDC(hWnd, lpPaint->hdc);
}

