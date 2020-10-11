/*===========================================================================*/
/*                                                                           */
/* File    : WCHECKBX.C                                                      */
/*                                                                           */
/* Purpose : Routines to handle checkboxes and radio buttons                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_CURSES

#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI)
#include "wgraphic.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
static VOID PASCAL ButtonSetState(HWND,UINT);
static VOID PASCAL CheckboxDraw(HWND, BOOL);
static VOID PASCAL ButtonNotifyParent(HWND, UINT, UINT);

extern BOOL  FAR PASCAL _IsButtonClass(HWND, UINT);
extern HWND  FAR PASCAL _DlgGetFirstControlOfGroup(HWND);
extern HWND  FAR PASCAL _DlgGetLastControlOfGroup(HWND);
extern INT   FAR PASCAL _CheckRadioButton(HWND,int,int,int, BOOL, BOOL);
extern VOID  FAR PASCAL _DrawOwnerDrawnButton(HWND, UINT, UINT);
#if defined(MEWEL_GUI)
static VOID      PASCAL DrawButtonBitmap(HDC, WINDOW *, INT, BOOL);
#endif
#ifdef __cplusplus
}
#endif

#define IS_OWNERDRAWN(dwFlags)  (((dwFlags) & 0x0FL) == BS_OWNERDRAW)

extern BOOL _fPushButtonPushed;

#if defined(ZAPP) && defined(MEWEL_GUI)
// #define ZAPP_FOCUS_STUFF
#endif


/****************************************************************************/
/*                                                                          */
/* Function : CheckBoxCreate, RadioButtonCreate                             */
/*                                                                          */
/* Purpose  : Low-level function to create a checkbox or a radio button.    */
/*                                                                          */
/* Returns  : The handle of the window, or NULL.                            */
/*                                                                          */
/****************************************************************************/
/*
HWND FAR PASCAL CheckBoxCreate(hParent,row,col,height,width,title,attr,flags,id)
  HWND   hParent;
  INT    row, col;
  INT    height, width;
  LPSTR  title;
  COLOR  attr;
  DWORD  flags;
  UINT   id;
*/
HWND FAR PASCAL CheckBoxCreate(HWND hParent, INT row, INT col, INT height, INT width, LPSTR  title, COLOR  attr, DWORD  flags, UINT   id)
{
  HWND   hWnd;
  WINDOW *cbw;
  
  (void) height;

  /*
    Determine the length of the checkbox. It's equal to the number of chars
    in the text, one column for the space, and two columns for the parens.
    Of course, for graphics controls, this must be adjusted.
  */
  if (width == 0)
  {
    width = (title) ? lstrlen(title) : 0;
    /*
      If no title, then forget about the space after the right paren
    */
    if (width == 0)
      width = -1;
    if (width > 0 && lstrchr(title, HILITE_PREFIX))  /* compensate for highlite */
      width--;
    width += 4;
#ifdef MEWEL_GUI
    width *= SysGDIInfo.tmAveCharWidth;
#endif
  }


#if defined(MEWEL_TEXT)
  height = IGetSystemMetrics(SM_CYCHECKBOX);
#else
  height = max(height, IGetSystemMetrics(SM_CYCHECKBOX));
#endif

  /*
    Windows gives a checkbox the default style of WS_TABSTOP (pg 427, Petzold)
  */
  hWnd = WinCreate(hParent,
                   row, col,
                   row + height,
                   col + width,
                   title, attr,
                   flags | WS_CLIPSIBLINGS,
                   CHECKBOX_CLASS, id);

  if ((cbw = WID_TO_WIN(hWnd)) != (WINDOW *) NULL)
  {
    CHECKBOXINFO *cbi;
    cbi = (CHECKBOXINFO *) cbw->pPrivate;
    cbi->state = 0;
    cbi->checkchar = WinGetSysChar(SYSCHAR_CHECKBOXCHECK);
  }
  return hWnd;
}


HWND FAR PASCAL RadioButtonCreate(hParent,row,col,height,width,title,attr,flags,id)
  HWND   hParent;
  INT    row, col;
  INT    height, width;
  LPSTR  title;
  COLOR  attr;
  DWORD  flags;
  UINT   id;
{
  HWND   hRB;
  WINDOW *rw;

  hRB = CheckBoxCreate(hParent, row, col, height, width, title, attr, flags, id);
  if ((rw = WID_TO_WIN(hRB)) != (WINDOW *) NULL)
  {
    rw->idClass = RADIOBUTTON_CLASS;
    if (!(flags & WS_TABSTOP))
      rw->flags &= ~WS_TABSTOP;
    ((CHECKBOXINFO *) rw->pPrivate)->checkchar = 
                                     WinGetSysChar(SYSCHAR_RADIOBUTTONCHECK);
  }
  return hRB;
}


/****************************************************************************/
/*                                                                          */
/* Function : CheckboxDraw()                                                */
/*                                                                          */
/* Purpose  : Draws a checkbox and a radio button.                          */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: ButtonWinproc, in response to a WM_PAINT.                     */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL CheckboxDraw(HWND hBox, BOOL bJustDrawCheck)
{
#if !defined(USE_NATIVE_GUI)
  WINDOW *cbw;
  CHECKBOXINFO *cbi;
  BOOL     bIsRadio;
  COLOR    attr, saveAttr;
  DWORD    dwFlags;
  HDC      hDC = 0;

#ifdef MEWEL_GUI
  TEXTMETRIC tm;
  RECT     pixRect;
  HFONT    hFont, hOldFont;
  HBRUSH   hBr;
#else
  char     szBuf[8];
  BYTE     chCheck;
#endif


  if ((cbw = WID_TO_WIN(hBox)) == NULL || !IsWindowVisible(hBox))
    return;

  /*
    Get the window flags and the button style.
  */
  dwFlags = cbw->flags;

  /*
    Determine if the button is a radiobutton or a checkbox.
  */
  bIsRadio = _IsButtonClass(hBox, RADIOBUTTON_CLASS);

  /*
    Get a pointer to the check box structure
  */
  cbi = (CHECKBOXINFO *) cbw->pPrivate;

  /*
    Let the app handle owner-drawn buttons.
  */
  if (IS_OWNERDRAWN(dwFlags))
  {
    _DrawOwnerDrawnButton(hBox, 0, (cbi->bHasFocus) ? ODS_FOCUS : 0);
    return;
  }


#ifdef MEWEL_GUI
  /*
    Get a window DC so we can draw over the whole button. Determine the
    height of the font, and get the pixel-based window rectangle.
  */
  hDC = GetWindowDC(hBox);
  WindowRectToPixels(hBox, (LPRECT) &pixRect);
#endif


  /*
    Get color attributes...
  */
  _PrepareWMCtlColor(hBox, CTLCOLOR_BTN, hDC);
  saveAttr = attr = cbw->attr;
  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hBox);

  if (cbi->bHasFocus)
  {
    cbw->attr = attr = MAKE_HIGHLITE(attr);
    HIGHLITE_ON();
  }


#ifdef MEWEL_GUI
  /*
    Frame the button
  */
  hBr = (HBRUSH) SendMessage(GetParent(hBox), WM_CTLCOLOR, (WPARAM) hDC,
                             MAKELONG(hBox, CTLCOLOR_BTN));
  SelectObject(hDC, hBr);
  FillRect(hDC, (LPRECT) &pixRect, hBr);

  pixRect.left += (dwFlags & BS_LEFTTEXT) ? 0
               : IGetSystemMetrics(SM_CXCHECKBOX);  /* start to the right of the bitmap */

#else
  /*
    Erase the checkbox
  */
  if (!bJustDrawCheck)
    WinClear(hBox);
#endif

  cbw->attr = saveAttr;


  /*
    Draw the check box borders and the check mark
  */
#ifdef MEWEL_GUI
  DrawButtonBitmap(hDC, cbw, cbi->state, bIsRadio);

  /*
    Get the font to draw with
  */
  if ((hFont = (HFONT) SendMessage(hBox, WM_GETFONT, 0, 0L)) != NULL)
    hOldFont = SelectObject(hDC, hFont);
  GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);

#else
  szBuf[0] = (bIsRadio) ? 
                      WinGetSysChar(SYSCHAR_RADIOBUTTON_LBORDER) :
                      WinGetSysChar(cbi->state ? SYSCHAR_CHECKBOX_LBORDER_ON
                                               : SYSCHAR_CHECKBOX_LBORDER);
  switch (cbi->state)
  {
    case 0 :
      chCheck = (BYTE) WinGetSysChar(bIsRadio ? SYSCHAR_RADIOBUTTON_OFF
                                              : SYSCHAR_CHECKBOX_OFF);
      break;
    case 1 :
      chCheck = (BYTE) cbi->checkchar;
      break;
    case 2 :
      chCheck = (BYTE) WinGetSysChar(SYSCHAR_3STATECHECK);
      break;
  }
  szBuf[1] = chCheck;
  szBuf[2] = (bIsRadio) ? 
                       WinGetSysChar(SYSCHAR_RADIOBUTTON_RBORDER) :
                       WinGetSysChar(cbi->state ? SYSCHAR_CHECKBOX_RBORDER_ON
                                                : SYSCHAR_CHECKBOX_RBORDER);

  if (dwFlags & BS_LEFTTEXT)
    szBuf[3] = '\0';
  else
    szBuf[3] = (BYTE) cbw->fillchar;
  szBuf[4] = '\0';

  WinPuts(hBox, 0, (dwFlags & BS_LEFTTEXT) ? lstrlen(cbw->title)+1: 0,
          szBuf, attr);
#endif


  /*
    Output the associated text
  */
  if (cbw->title && !bJustDrawCheck)
  {
#ifdef MEWEL_GUI
    /*
      Output the text to the right of the checkbox/radio icon by using
      TextOut()
    */
    int sLen, xOffset, yOffset;

    sLen = (int) LOWORD(_GetTextExtent(cbw->title));
    if (dwFlags & BS_LEFTTEXT)
      xOffset = 0;
    else
      xOffset = IGetSystemMetrics(SM_CXCHECKBOX) + tm.tmAveCharWidth;

    /*
      Figure out the offset from the top. We want to center the text.
      The total blank space is the height of the checkbox minus the height
      of the text. Dividing by two gives us the blank space in the
      top half.
    */
    yOffset = (RECT_HEIGHT(pixRect) - tm.tmHeight) / 2;

    /*
      Write the button text
    */
    SetBkMode(hDC, TRANSPARENT);
    HilitePrefix = HILITE_PREFIX;
    TextOut(hDC, xOffset, yOffset, cbw->title, lstrlen(cbw->title));
    HilitePrefix = '\0';

    /*
      If the checkbox has the focus, set up the rectangle used to draw the
      focus indicator. The horizontal extent of the focus rect is 4 pixels
      to the left of the string and 4 pixels from the right of the string.
      For the vertical extent, we go half the height of the blank space
      at the top and at the bottom of the checkbox.
    */
    if (cbi->bHasFocus)
    {
      int xRight = xOffset + sLen + 4;
      int yBottom = yOffset + 1 + tm.tmHeight + (yOffset>>1);

      SetRect(&pixRect, xOffset - 4,
                        yOffset - (yOffset>>1),
                        min(xRight, pixRect.right),
                        min(yBottom, pixRect.bottom));
    }

#else  /* !MEWEL_GUI */
    COLOR oldHilitePrefixAttr = HilitePrefixAttr;
    /*
      Combine the hotkey color with the button's background if it doesn't
      currently have the focus.
    */
    if (cbi->bHasFocus)
      HilitePrefixAttr = attr;
    else
    {
      HilitePrefixAttr = ((dwFlags & WS_DISABLED) == 0L) ?
                         WinQuerySysColor(hBox, SYSCLR_BUTTONHIGHLIGHT) : attr;
      HilitePrefixAttr = MAKE_ATTR(GET_FOREGROUND(HilitePrefixAttr),
                                   GET_BACKGROUND(attr));
    }

    HilitePrefix = HILITE_PREFIX;
    WinPuts(hBox, 0, (dwFlags & BS_LEFTTEXT) ? 0 : 4, cbw->title, attr);
    HilitePrefix = '\0';
    HilitePrefixAttr = oldHilitePrefixAttr;
#endif
  }
#ifdef MEWEL_GUI
  else
  {
    if (cbi->bHasFocus)
    {
      InflateRect(&pixRect, -4, -4);
      pixRect.left -= 1;	  
    }
  }
#endif

  HIGHLITE_OFF();

#ifdef MEWEL_GUI
  /*
    If a pushbutton has the focus, draw a focus rect around it
  */
  if (cbi->bHasFocus)
    DrawFocusRect(hDC, &pixRect);

  /*
    Restore the system font to the DC and delete the DC
  */
  if (hFont)
    SelectObject(hDC, hOldFont);
  ReleaseDC(hBox, hDC);
#endif
#endif /* NATIVE_GUI */
}


/*===========================================================================*/
/*                                                                           */
/* ButtonSetState()                                                          */
/*                                                                           */
/* Called when the button gets or loses focus, or when the mouse is pressed  */
/* and released over a button.                                               */
/*                                                                           */
/* If 'state' is 2, then the mouse button is currently being depressed over  */
/* the button.                                                               */
/*                                                                           */
/* Called by : ButtonWinProc() in response to a SET/KILLFOCUS, BM_SETSTATE,  */
/*             or MOUSEBUTTONDOWN/UP                                         */
/*                                                                           */
/*===========================================================================*/
static VOID PASCAL ButtonSetState(hWnd, wState)
  HWND hWnd;
  UINT wState;
{
  WINDOW *w;

  /*
    Process only if we are dealing with a pushbutton.
  */
  if (_IsButtonClass(hWnd, PUSHBUTTON_CLASS))
  {
    w = WID_TO_WIN(hWnd);
    if (wState)
    {
      if (wState != 0xFFFF)
        _fPushButtonPushed = wState;   /* can be 1 or 2 */
      w->flags |= BS_INVERTED;
    }
    else
    {
      _fPushButtonPushed = FALSE;
      w->flags &= ~BS_INVERTED;
    }

    /*
      If mousing on the button, cause an immediate redraw to show the 
      new look of the button.
    */
    InternalInvalidateWindow(hWnd, FALSE);
    if (wState == 2)
      UpdateWindow(hWnd);

#if defined(ZAPP_FOCUS_STUFF)
    if (wState != 2)
    {
      InternalInvalidateWindow(hWnd, FALSE);  
      UpdateWindow(hWnd);
    }
#endif
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : ButtonWinProc()                                               */
/*                                                                          */
/* Purpose  : MEWEL's window procedure for the button class.                */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LONG FAR PASCAL ButtonWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WINDOW *w;
  CHECKBOXINFO *cbi;
  MSG   msg;
  UINT  iClass;
  DWORD dwFlags;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
    return -1;
  if ((cbi = (CHECKBOXINFO *) w->pPrivate) == NULL)
  {
    w->pPrivate = emalloc(sizeof(CHECKBOXINFO));
    if ((cbi = (CHECKBOXINFO *) w->pPrivate) == NULL)
      return FALSE;
  }
  iClass  = _WinGetLowestClass(w->idClass);
  dwFlags = w->flags;

  /*
    If the class is 0 (ie - normal window), then this routine was probably
    reached by calling CallWindowProc() on a subclass of a button
    control (ie - the way that zAPP handles control window creation).
    So, if the class is 0, check the style bits for BS_xxxx.
  */
  if (iClass == NORMAL_CLASS || iClass == BUTTON_CLASS)
  {
    if (_IsButtonClass(hWnd, CHECKBOX_CLASS))
      iClass = CHECKBOX_CLASS;
    else if (_IsButtonClass(hWnd, RADIOBUTTON_CLASS))
      iClass = RADIOBUTTON_CLASS;
    else  /* if (dwFlags & BS_PUSHBUTTON) */
      iClass = PUSHBUTTON_CLASS;
  }



  switch (message)
  {
    case WM_NCPAINT     :
      /*
        We can get sent an WM_NCPAINT message if an invalid area just
        touches the border of a pushbutton without touching its
        client area. In this case, defer all updating to the
        WM_PAINT procedure.
      */
#if !defined(USE_NATIVE_GUI)
      if (iClass == PUSHBUTTON_CLASS && (dwFlags & WS_BORDER))
        InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
#endif
      break;


    case WM_SETFOCUS    :
    case WM_KILLFOCUS   :
      if (iClass == PUSHBUTTON_CLASS)
      {
        BOOL bSettingFocus = (BOOL) (message == WM_SETFOCUS);
        cbi->bHasFocus = bSettingFocus;
        ButtonSetState(hWnd, bSettingFocus);  /* set state & redraw */

        if (IS_OWNERDRAWN(dwFlags))
        {
do_ownerdrawn:
          _DrawOwnerDrawnButton(hWnd, ODA_FOCUS, bSettingFocus ? ODS_FOCUS : 0);
          break;
        }

        else if (TEST_PROGRAM_STATE(STATE_NORTON_BUTTON))  /* the Norton Look */
        {
          HWND hParent = GetParent(hWnd);
          HWND hDefButton;
          if (hParent != NULLHWND && 
              (hDefButton = DlgGetDefButton(hParent, NULL)) != NULLHWND &&
              hDefButton != hWnd)
            InvalidateRect(hDefButton, NULL, FALSE);
        }

#if defined(ZAPP_FOCUS_STUFF)
        else
        {
          BOOL bSettingFocus = (BOOL) (message == WM_SETFOCUS);
          if (bSettingFocus && !cbi->bHasFocus)
            cbi->bHasFocus = bSettingFocus;
          ButtonSetState(hWnd, bSettingFocus);  /* set state & redraw */
          if (!bSettingFocus && cbi->bHasFocus)
            cbi->bHasFocus = bSettingFocus;
          break;
	}
#endif

        break;
      }

      /*
        Owner-drawn checkbox or radiobutton
      */
      if (IS_OWNERDRAWN(dwFlags))
        goto do_ownerdrawn;
      cbi->bHasFocus = (BOOL) (message == WM_SETFOCUS);
      /*
        Don't redrawn the button right away, especially if the dialog
        box is hidden
      */
invalidate_area:
      InternalInvalidateWindow(hWnd, FALSE);
      break;


    case WM_PAINT       :
      if (iClass == CHECKBOX_CLASS || iClass == RADIOBUTTON_CLASS)
        CheckboxDraw(hWnd, FALSE);
      else if (iClass == PUSHBUTTON_CLASS)
        PushButtonDraw(hWnd);
      w->rUpdate = RectEmpty;
      w->ulStyle &= ~WIN_UPDATE_NCAREA;
      break;


    case WM_NCHITTEST   :
      return HTCLIENT;

    case WM_KEYDOWN     :
    case WM_LBUTTONDOWN :
#if !defined(USE_NATIVE_GUI)
      if (!IsWindowEnabled(hWnd))
        break;

      if (message == WM_LBUTTONDOWN)
      {
        /*
          Wait for the user to release the mouse button so that a
          LBUTTONUP message will be contained within here, and *not*
          transmitted up to the dialog box's parent.
        */
#if defined(ZAPP_FOCUS_STUFF)
        if (!cbi->bHasFocus)
          cbi->bHasFocus = TRUE;
#else
        cbi->bHasFocus = TRUE;
#endif
        ButtonSetState(hWnd, (UINT) 2);  /* 2 signals a mouse press */

        if (IS_OWNERDRAWN(dwFlags))
        {
          _DrawOwnerDrawnButton(hWnd, ODA_SELECT, ODS_SELECTED);
        }
#if defined(MOTIF)
        break;
#endif

        /*
          Before we go into this modal loop, make sure that any
          windows which need to be refreshed are refreshed.
        */
        RefreshInvalidWindows(_HwndDesktop);

        while (!GetMouseMessage(&msg) || msg.message != WM_LBUTTONUP)
        {
#ifdef MEWEL_GUI
          if (msg.message == WM_MOUSEMOVE)
          {
            BOOL bInRect = PtInRect(&w->rect, MAKEPOINT(msg.lParam));

            if (_fPushButtonPushed == 0 && bInRect)
            {
              ButtonSetState(hWnd, 2);
            }
            else if (_fPushButtonPushed && !bInRect)
            {
              ButtonSetState(hWnd, 0);
              UpdateWindow(hWnd);
            }
          }
#else
          ;
#endif
        }

        ButtonSetState(hWnd, 0);

        /*
          If we pressed the mouse on a push button, then we want the
          push button to retain the focus color. So, send ButtonSetState
          a special code which tells it to set the inverted flag but
          not muck around with the '_fPushButtonPushed' variable.
        */
        ButtonSetState(hWnd, (UINT) 0xFFFF);

#ifdef MMCAD
        /*
          Let app know about the button being released
            (for Bill Summerlin of MMC AD)
        */
        SendMessage(hWnd, WM_LBUTTONUP, 0, msg.lParam);
#endif

        if (IS_OWNERDRAWN(dwFlags))
          _DrawOwnerDrawnButton(hWnd, ODA_SELECT, 0);
        /*
          Make sure that the user release the mouse inside the pushbutton
        */
        if (!PtInRect(&w->rect, MAKEPOINT(msg.lParam)))
          break;
      }
      else  /* character pressed */
      {
        /*
          Function key or arrow key?
        */
        if (HIBYTE(wParam))
          break;
      }

      /*
        If we pressed a key over a pushbutton, make sure that it's a valid
        character (ie - <SPACE>, <ENTER>, the hilighted letter, or the
        first letter of the pushbutton title). If not, don't do anything.
      */
      if (iClass == PUSHBUTTON_CLASS && message != WM_LBUTTONDOWN)
      {
        LPSTR pHilite = lstrchr(w->title, HILITE_PREFIX);
        if (wParam != ' ' && wParam != VK_RETURN)
        {
          WORD wpUpper = lang_upper(wParam);
          if (pHilite && wpUpper != lang_upper(pHilite[1]) &&
                         wpUpper != lang_upper(w->title[0]))
            break;
          if (!pHilite && wpUpper != lang_upper(w->title[0]))
            break;
        }
      }

      if (iClass == CHECKBOX_CLASS)
      {
        if ((dwFlags & 0x0F) == BS_AUTO3STATE)
          cbi->state = (cbi->state + 1) % 3;
        else if ((dwFlags & 0x0F) == BS_AUTOCHECKBOX)
          cbi->state = !cbi->state;

#ifdef MEWEL_GUI
        {
        HDC hDC = GetWindowDC(hWnd);
        DrawButtonBitmap(hDC,w,cbi->state,_IsButtonClass(hWnd,RADIOBUTTON_CLASS));
        ReleaseDC(hWnd, hDC);		  
        }
#else
        InternalInvalidateWindow(hWnd, FALSE);
#endif
      }
      else if (iClass == RADIOBUTTON_CLASS)
      {
        MEWELCheckRadioGroup(hWnd);
      }

      /*
        Notify the parent.
      */
      message = (dwFlags & BS_HELP) ? WM_HELP : WM_COMMAND;
      ButtonNotifyParent(hWnd, message, BN_CLICKED);
#endif /* USE_NATIVE_GUI */
      break;


    case WM_LBUTTONDBLCLK  :
      /*
        Notify a radiobutton's (or owner-drawn button's) parent of
        a double-click.
      */
      if (IsWindowEnabled(hWnd) && 
               (iClass==RADIOBUTTON_CLASS || 
                (dwFlags & BS_OWNERDRAW) == BS_OWNERDRAW))
        ButtonNotifyParent(hWnd, WM_COMMAND, BN_DOUBLECLICKED);
      break;


    case WM_LBUTTONUP :
#if defined(USE_NATIVE_GUI)
      ButtonSetState(hWnd, 0);
      ButtonSetState(hWnd, (UINT) 0xFFFF);
      if (IS_OWNERDRAWN(dwFlags))
        _DrawOwnerDrawnButton(hWnd, ODA_SELECT, 0);
#endif
      break;

    /*---------------------------------------------------------------*/

    case BM_SETSTATE       :
      /*
        If we get a wParam of 1, then set the internal _fPushButtonPushed
        to the special value of 3. 
      */
      ButtonSetState(hWnd, (wParam == 0) ? 0 : 3);
      if (IS_OWNERDRAWN(dwFlags))
        _DrawOwnerDrawnButton(hWnd, ODA_SELECT, wParam ? ODS_SELECTED : 0);
      /*
        The BM_SETSTATE causes an immediate redraw.
      */
      UpdateWindow(hWnd);
      break;


    case BM_GETSTATE       :
    {
      UINT fState = 0;
      if (iClass != PUSHBUTTON_CLASS)
        fState |= cbi->state;
      if (_fPushButtonPushed == 2 && cbi->bHasFocus ||  _fPushButtonPushed == 3)
        fState |= 0x0004;
      if (InternalSysParams.hWndFocus == hWnd)
        fState |= 0x0008;
      return fState;
    }


    case BM_GETCHECK	   :
      return (LONG) cbi->state;


    case BM_SETCHECK       :
    {
      int iOldState = cbi->state;

      /*
        Set the state. A non-3state button can only have states 0 and 1.
      */
      if ((cbi->state = wParam) > 1 && !((dwFlags & 0x0F) == BS_3STATE))
        cbi->state = 1;

      /*
        Refresh the button if we changed the state.
      */
#if !defined(USE_NATIVE_GUI)
      if (cbi->state != iOldState)
#if defined(MEWEL_GUI)
      {
        HDC hDC = GetWindowDC(hWnd);
        DrawButtonBitmap(hDC,w,cbi->state,_IsButtonClass(hWnd,RADIOBUTTON_CLASS));
        ReleaseDC(hWnd, hDC);		  
      }
#else
        InternalInvalidateWindow(hWnd, FALSE);
#endif
#endif
      break;
    }


    case BM_SETSTYLE       :
      /*
        wParam has the new style bits, lParam is 1 if the control should
        be redrawn. First, wipe out the old BS_xxx style bits and replace
        then with the new style bits. Then, if lParam is 1, refresh the window.
      */
      w->flags &= ~0x00FFL;
      w->flags |= (wParam & 0x00FFL);
#if !defined(USE_NATIVE_GUI)
      if (lParam)
        goto invalidate_area;
#endif
      break;
      

    case WM_GETDLGCODE     :
      if (iClass == PUSHBUTTON_CLASS)
        return DLGC_BUTTON |
          ((dwFlags & BS_DEFPUSHBUTTON) ? DLGC_DEFPUSHBUTTON
                                        : DLGC_UNDEFPUSHBUTTON);
      else if (iClass == RADIOBUTTON_CLASS)
        return DLGC_BUTTON | DLGC_RADIOBUTTON;
      else if (iClass == CHECKBOX_CLASS)
        return DLGC_BUTTON;


    case WM_SYSCOLORCHANGE :
      if (wParam == SYSCLR_CHECKBOX || 
          wParam == SYSCLR_BUTTON   || 
          wParam == SYSCLR_BUTTONDOWN)
        InternalInvalidateWindow(hWnd, FALSE);
      break;


    case WM_DELETEITEM     :
    case WM_DRAWITEM       :
    case WM_MEASUREITEM    :
      return SendMessage(GetParent(hWnd), message, wParam, lParam);


#ifdef MEWEL_GUI
    case WM_CREATE :
      /*
        Make sure that the borders are off on GUI buttons
      */
      if (dwFlags)
      {
        w->flags &= ~WS_BORDER;
        _WinSetClientRect(hWnd);
      }
      break;
#endif

    default                :
      return StdWindowWinProc(hWnd, message, wParam, lParam);
  }
  return FALSE;
}


static VOID PASCAL ButtonNotifyParent(hWnd, wMsg, nCode)
  HWND hWnd;
  UINT wMsg;
  UINT nCode;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  if (w->parent)
    SendMessage(w->parent->win_id, wMsg, w->idCtrl, MAKELONG(hWnd, nCode));
}


BOOL bInCheckRadioGroup = FALSE; /* do Motif Change callback? */

VOID FAR PASCAL MEWELCheckRadioGroup(HWND hButton)
{
  WINDOW *w = WID_TO_WIN(hButton);

  HWND hFirst, hLast;
  HWND hDlg = w->parent->win_id;

  /*
    Get the handles of the first and last radio buttons in the same
    group as hButton.
  */
  hFirst = _DlgGetFirstControlOfGroup(hButton);
  hLast  = _DlgGetLastControlOfGroup(hButton);

  /*
    Let _CheckradioButton handle the hard work of checking and unchecking
    the buttons in the group.
  */
  if (hFirst != NULL && hLast != NULL)
  {
    bInCheckRadioGroup++;
    _CheckRadioButton(hDlg, WID_TO_WIN(hFirst)->idCtrl,
                            WID_TO_WIN(hLast)->idCtrl,
                            WID_TO_WIN(hButton)->idCtrl,
                            FALSE,
                            (w->flags & 0x0FL) == BS_AUTORADIOBUTTON);
    bInCheckRadioGroup--;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : _IsButtonClass()                                              */
/*                                                                          */
/* Purpose  : Tests a button control to see if is of class 'wClassToMatch'  */
/*                                                                          */
/* Returns  : TRUE if the window if of the desired class. FALSE if not.     */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL _IsButtonClass(hWnd, wClassToMatch)
  HWND hWnd;
  UINT wClassToMatch;
{
  WINDOW *w;
  UINT   idClass;

  /*
    Get the window pointer and the lowest base window class
  */
  w = WID_TO_WIN(hWnd);
  idClass = _WinGetLowestClass(w->idClass);

  /*
    If the window is of the class we're looking for (ie - PUSHBUTTON_CLASS,
    CHECKBOX_CLASS or RADIOBUTTON_CLASS), return TRUE.
  */
  if (idClass == wClassToMatch)
    return TRUE;

  /*
    Is it a subclassed window?
  */
  if (idClass == NORMAL_CLASS || idClass == BUTTON_CLASS)
  {
    /*
      Isolate the button styles
    */
    UINT wStyle = (UINT) (w->flags & 0x0FL);

    switch (wStyle)
    {
      case BS_PUSHBUTTON    :
      case BS_DEFPUSHBUTTON :
      case BS_USERBUTTON    :
      case BS_OWNERDRAW     :
        return (wClassToMatch == PUSHBUTTON_CLASS);
      case BS_RADIOBUTTON   :
      case BS_AUTORADIOBUTTON :
        return (wClassToMatch == RADIOBUTTON_CLASS);
      case BS_CHECKBOX      :
      case BS_AUTOCHECKBOX  :
      case BS_3STATE        :
      case BS_AUTO3STATE    :
        return (wClassToMatch == CHECKBOX_CLASS);
    }
  }

  return FALSE;
}


#if defined(MEWEL_GUI)
static VOID PASCAL DrawButtonBitmap(hDC, w, state, bIsRadio)
  HDC  hDC;
  WINDOW *w;
  INT  state;
  BOOL bIsRadio;
{
  INT yOffset;
  INT xOffset;

  static INT aiBitmap[3][2] = 
  {
    { SYSBMP_CHECKBOXOFF, SYSBMP_RADIOOFF },
    { SYSBMP_CHECKBOXON,  SYSBMP_RADIOON  },
    { SYSBMP_3STATEON,    SYSBMP_3STATEON }
  };

  /*
    Center the bitmap within the button rectangle.
  */
  yOffset = (RECT_HEIGHT(w->rect) - IGetSystemMetrics(SM_CYCHECKBOX)) >> 1;

  if (w->flags & BS_LEFTTEXT)
    xOffset = LOWORD(_GetTextExtent(w->title)) + LOWORD(_GetTextExtent(" "));
  else
    xOffset = 0;

  /*
    Draw the bitmap
  */
  DrawSysBitmap(hDC, aiBitmap[state][bIsRadio], xOffset, yOffset);
}
#endif

