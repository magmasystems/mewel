/*
 *
 * File    :  WPUSHBUT.C
 *
 * Purpose :  PushButtonCreate()
 *
 * History :
 * 12/31/89 (maa) - added row2, col2 to the args in order to support
 *                  bordered pushbuttons.
 *
 * (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved
 *
 * $Log:   E:/vcs/mewel/wpushbut.c_v  $
//	
//	   Rev 1.4   30 Nov 1993 17:09:20   Adler
//	Added CX_BEV constant for the width of lines under graphics mode.
//	Added #ifdef TELEVOICE for their code to wrap long text in multiple-line
//	pushbuttons.
//	
//	   Rev 1.3   25 Oct 1993 13:48:08   Adler
//	Added new arg for PrepareWMCtlColor().
//	
//	   Rev 1.2   04 Oct 1993 13:08:56   Adler
//	Enabled Inmark's look for the pushbutton. The bevelled box is not used.
//	
//	   Rev 1.1   24 May 1993  8:08:32   unknown
//	Added ifdef ZAPP to the pushbutton drawing code.
//	
//	   Rev 1.0   23 May 1993 21:05:56   adler
//	Initial revision.
 * 
 *    Rev 1.2   16 Aug 1991 15:32:08   MLOCKE
 * Modified action of pushbutton color attribute selection
 * not to use system colors if the color attribute of the
 * button is not SYSTEM_COLOR.
 * 
 *    Rev 1.1   15 Aug 1991 15:57:04   MLOCKE
 * Altered header for use with PVCS.
 * 
 * Altered treatment of pushbuttons to handle single line,
 * shadowed pushbuttons more gracefully:
 * 
 * 	Added separate colors in WINCOLOR.C to allow
 * 	alteration of pushbutton colors without altering
 * 	radio button colors.
 * 
 * 	Text of default pushbutton is displayed in converse
 * 	colors of normal pushbutton.  Accelerator is in
 * 	SYSCLR_PUSHBUTTON, other text in SYSCLR_PUSHBUTTONHILITE.
 * 	(Marc, if the dialog handler would send a redraw message
 * 	to the default pushbutton when some other pushbutton is
 * 	highlighted we could draw the default pushbutton in
 * 	SYSCLR_PUSHBUTTONDOWN when no other pushbutton has focus
 * 	and draw it in standard pushbutton colors when another
 * 	pushbutton has the focus.)
 * 
 * 	When a pushbutton has the BS_INVERSE flag set it is
 * 	drawn in SYSCLR_PUSHBUTTONDOWN to difereniate it from
 * 	the shadow background.  Old code used a MAKE_HIGHLITE
 * 	to determine the color.
 */

#define INCLUDE_CURSES

#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOOWNERDRAWN
#define OWNERDRAWN
extern VOID FAR PASCAL _DrawOwnerDrawnButton(HWND, UINT, UINT);
#endif
extern BOOL  FAR PASCAL _IsButtonClass(HWND, UINT);

#ifdef __cplusplus
}
#endif

/*
  This variable records the state of the pushbutton and is used to
  simulate a 3-D look and feel. This is also references in WinDrawShadow.
  It can be 0 (not pushed), 1 (pushed or in focus), 2 (mouse depressed),
  or 3 (set the inverse on because of a call to BM_SETSTATE).
*/
BOOL _fPushButtonPushed = FALSE;

#define ZAPP
#ifdef ZAPP
#define DOT_RGB  RGB(0xC0, 0xC0, 0xC0)
#else
#define DOT_RGB  RGB(0x80, 0x80, 0x80)
#endif

/*
  Width of pushbutton bevel
*/
#if defined(META)
#define CX_BEV  3
#else
#define CX_BEV  2
#endif


/****************************************************************************/
/*                                                                          */
/* Function : PushButtonCreate()                                            */
/*                                                                          */
/* Purpose  : Low-level call to create a pushbutton control.                */
/*                                                                          */
/* Returns  : The handle of the pushbutton if successful, or NULL.          */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL PushButtonCreate(hParent,row1,col1,row2,col2,title,attr,flags,id)
  HWND  hParent;
  int   row1, col1, row2, col2;
  LPSTR title;
  COLOR attr;
  DWORD flags;
  INT   id;
{
  HWND   hWnd;
  
  /*
    Automatically add a border to a pushbutton if it's a single-line,
    non-ownerdrawn button without a shadow
  */
#ifdef MEWEL_TEXT
  if (row1 == row2-1 && !(flags & WS_SHADOW) && 
                         (flags & BS_OWNERDRAW) != BS_OWNERDRAW)
    flags |= WS_BORDER;
  if (flags & WS_SHADOW)
    WID_TO_WIN(hParent)->ulStyle |= WIN_HAS_SHADOWED_KIDS;
#endif

  /*
    Create the pushbutton window
  */
  hWnd = WinCreate(hParent,
                   row1, col1, row2, col2,
                   title, attr,
                   flags | WS_CLIPSIBLINGS,
                   PUSHBUTTON_CLASS, id);

  return hWnd;
}


/****************************************************************************/
/*                                                                          */
/* Function : PushButtonDraw()                                              */
/*                                                                          */
/* Purpose  : Draws a pushbutton on the display.                            */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: ButtonWinProc in response to WM_PAINT and WM_NCPAINT.         */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL PushButtonDraw(hPB)
  HWND hPB;
{
  WINDOW *pbw;
  COLOR  attr, saveAttr;
  BOOL   bHasBorder;
  BOOL   bIsDefault;
  BOOL   bInverted;
  BOOL   bDepressed;
  BOOL   bDidCtlColor = FALSE;
  DWORD  dwFlags;
  CHECKBOXINFO *cbi;
  HDC    hDC = 0;

#ifdef MEWEL_GUI
  HBRUSH hBr;
  HFONT  hFont, hOldFont;
  RECT   pixRect;
  TEXTMETRIC tm;
#endif


  /*
    Get a ptr to the push button structure
  */
  if ((pbw = WID_TO_WIN(hPB)) == NULL || !IsWindowVisible(hPB))
    return;

  dwFlags = pbw->flags;
  cbi = (CHECKBOXINFO *) pbw->pPrivate;

  /*
    If we have an ownerdrawn button, call a special function which notifies
    the app. Then get outta here.
  */
#ifdef OWNERDRAWN
  if ((dwFlags & BS_OWNERDRAW) == BS_OWNERDRAW)
  {
    _DrawOwnerDrawnButton(hPB, ODA_DRAWENTIRE, 0);
    return;
  }
#endif


#if !defined(USE_NATIVE_GUI)
#ifdef MEWEL_GUI
  /*
    Get a window DC for drawing, and determine the font height and the
    window rectangle in pixels.
  */
  hDC = GetWindowDC(hPB);
  /*
    Get the font to draw with
  */
  if ((hFont = (HFONT) SendMessage(hPB, WM_GETFONT, 0, 0L)) != NULL)
    hOldFont = SelectObject(hDC, hFont);
  GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);
  WindowRectToPixels(hPB, (LPRECT) &pixRect);
#endif


  /*
    Get various characteristics into local vars for speed.
  */
  bHasBorder = (BOOL) ((dwFlags & WS_BORDER)   != 0L);
  bIsDefault = (BOOL) ((dwFlags & BS_DEFPUSHBUTTON) != 0L);
  bInverted  = (BOOL) ((dwFlags & BS_INVERTED) != 0L);

  /*
    See if we need to invert the button because the mouse is being clicked
    on it or because someone called BM_SETATE(wParam = 1).
  */
  bDepressed = (BOOL) (_fPushButtonPushed == 2 && cbi->bHasFocus || 
                       _fPushButtonPushed == 3);

  /*
    Determine the color attribute
  */
  bDidCtlColor = _PrepareWMCtlColor(hPB, CTLCOLOR_BTN, hDC);
  saveAttr = attr = pbw->attr;
  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hPB);


  /*
    If the button is in the inverted state (ie - has the focus or
    is being pushed), then determine the proper color. If the button
    is the system color, then do a table lookup. If it has a custom
    color, then swap the foreground and background.
  */
#ifdef MEWEL_TEXT
  if (bInverted)
  {
    if (saveAttr == SYSTEM_COLOR)
      attr = pbw->attr = WinQuerySysColor(hPB, SYSCLR_PUSHBUTTONDOWN);
    else
    {
      pbw->attr = MAKE_HIGHLITE(attr);
      attr = pbw->attr;
    }
    HIGHLITE_ON();
  }
#endif

  /*
    Draw it...
  */
#ifdef MEWEL_GUI

  hBr = (HBRUSH) SendMessage(GetParent(hPB), WM_CTLCOLOR, (WPARAM) hDC,
                             MAKELONG(hPB, CTLCOLOR_BTN));
  SelectObject(hDC, hBr);

  /*
    Frame the button
    Note : 
      For true Windows compat, the top-left border should be
    SysBrush[COLOR_BTNHIGHLIGHT] and the bottom-right border should be
    SysBrush[COLOR_BTNSHADOW].
      We draw the button in a depressed state only if it the mouse is
    depressed on a pushbutton (_fPushButtonPushed == 2) and if this
    control is the window which the mouse is on (cbi->bHasFocus == 1).
  */
#if defined(META)
  DrawBeveledBox(hPB, hDC, (LPRECT) &pixRect, 
                 SM_CXPUSHBUTTONBORDER + bIsDefault,
                 SM_CYPUSHBUTTONBORDER + bIsDefault,
                 bDepressed, hBr);
#else
  {
  int    x1,x2,y1,y2;
  RECT   r;
  HPEN   oldPen;
  HBRUSH hOldBrush;

  /*
    hPenBevel[0] is used to draw the bevel on the left and top.
    hPenBevel[1] is used to draw the bevel on the right and bottom.
    hPenFrame[0] is used to draw the frame around a non-def pushbutton
    hPenFrame[1] is used to draw the frame around a default pushbutton
  */
  static HPEN hPenBevel[2] = { NULL, NULL };
  static HPEN hPenFrame[2] = { NULL, NULL };
  if (hPenBevel[0] == NULL)
  {
    hPenBevel[0] = CreatePen(PS_SOLID, CX_BEV, GetSysColor(COLOR_BTNHIGHLIGHT));
    hPenBevel[1] = CreatePen(PS_SOLID, CX_BEV, GetSysColor(COLOR_BTNSHADOW));
    hPenFrame[0] = CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));
    hPenFrame[1] = CreatePen(PS_SOLID, 2, RGB(0x00, 0x00, 0x00));
  }

  FillRect(hDC, &pixRect, hBr);

  SetRect(&r, pixRect.left+1, pixRect.top+1, pixRect.right-1, pixRect.bottom-1);

  /*
    Draw a black frame around the button
  */
  oldPen = SelectObject(hDC, hPenFrame[bIsDefault]);
  hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
  Rectangle(hDC, r.left, r.top, r.right, r.bottom);
  SelectObject(hDC, hOldBrush);
		  
  /*
    Draw the left and top bevels
  */
  SelectObject(hDC, hPenBevel[bDepressed]);
  InflateRect(&r, bIsDefault ? -2 : -1, bIsDefault ? -2 : -1);
  r.bottom--;  r.right--;
  MoveTo(hDC, r.left,  r.bottom);
  LineTo(hDC, r.left,  r.top);
  LineTo(hDC, r.right, r.top);

  /*
    Draw the right and bottom bevels
  */
  SelectObject(hDC, hPenBevel[!bDepressed]);
  InflateRect(&r, -1, -1);
  MoveTo(hDC, r.right, r.top);
  LineTo(hDC, r.right, r.bottom);
  LineTo(hDC, r.left,  r.bottom);

  SelectObject(hDC,oldPen);
  }
#endif

  
#else
  WinClear(hPB);
  WinDrawShadow(hPB);
#endif

  pbw->attr = saveAttr;


  /*
    Draw the border around a bordered pushbutton correctly.
  */
#if !defined(MEWEL_GUI) && !defined(MOTIF)
  if (bHasBorder)
  {
    if (RECT_HEIGHT(pbw->rect) >= 3)
    {
      /*
        We have a 3-line pushbutton. Draw the border one way if it is
        being pushed or if it's the default button, and draw it another
        way if it is neutral.
      */
      WinDrawBorder(hPB,(bDepressed||bIsDefault) ? BORDER_3DPUSHED : BORDER_3D);
      _fPushButtonPushed = FALSE;
    }
    else  /* single line bordered pushbutton */
    {
      BOOL bUseDefault = bIsDefault;

      if (TEST_PROGRAM_STATE(STATE_NORTON_BUTTON))
      {
        if (bIsDefault)
        {
          if (hPB != InternalSysParams.hWndFocus) 
            if (_IsButtonClass(InternalSysParams.hWndFocus, PUSHBUTTON_CLASS))
              bUseDefault = FALSE;
        }
        else
        {
          if (hPB == InternalSysParams.hWndFocus) 
            bUseDefault = TRUE;
        }
      }

      /*
        Draw the opening '<' in a single-line bordered pushbutton
      */
      {
      WinPutc(hPB, 0, 0,
              (bUseDefault) ? WinGetSysChar(SYSCHAR_DEFPUSHBUTTON_LBORDER)
                            : WinGetSysChar(SYSCHAR_PUSHBUTTON_LBORDER),
               attr);
      WinPutc(hPB, 0, RECT_WIDTH(pbw->rect)-1,
              (bUseDefault) ? WinGetSysChar(SYSCHAR_DEFPUSHBUTTON_RBORDER)
                            : WinGetSysChar(SYSCHAR_PUSHBUTTON_RBORDER),
               attr);
      }
    }
  } /* end border drawing */
#else
  (void) bHasBorder;
#endif


  /*
    Draw the pushbutton text.
    The main work here is in determining the correct color for the hotkey.
  */

  if (pbw->title)
  {
    COLOR oldHilitePrefixAttr = HilitePrefixAttr;
    if (!(dwFlags & WS_DISABLED))
    {
      if (bInverted)
      {
        /*
          An inverted button does not intensify the hotkey.
        */
        if (saveAttr == SYSTEM_COLOR)
          attr = WinQuerySysColor(hPB, SYSCLR_PUSHBUTTONDOWN);
        HilitePrefixAttr = attr;
      }
      else
      {
        /*
          Look up the color of the hotkey. Just use the foreground color
          of the hotkey and mesh it with the background of the button.
        */
        if (saveAttr == SYSTEM_COLOR || bDidCtlColor)
          HilitePrefixAttr = WinQuerySysColor(hPB, SYSCLR_PUSHBUTTONHILITE);
        HilitePrefixAttr = MAKE_ATTR(GET_FOREGROUND(HilitePrefixAttr),
                                     GET_BACKGROUND(attr));
      }
    }
    else  /* disabled */
    {
      /*
        The prefix attribute is the same as the rest of the text in a
        disabled pushbutton.
      */
      HilitePrefixAttr = attr;
    }

    HilitePrefix = HILITE_PREFIX;  /* Tell WinPuts to interpret tildes */

#ifdef MEWEL_GUI
    {
    /*
      Output the text centered vertically and horizontally within the
      pushbutton frame by using TextOut()
    */
    int   sLen, xOffset, yOffset;

    sLen = (int) LOWORD(_GetTextExtent(pbw->title));
    xOffset = (dwFlags & BS_LEFTTEXT) ? SM_CXPUSHBUTTONBORDER
                                      : (RECT_WIDTH(pixRect) - sLen) / 2;
    yOffset = (RECT_HEIGHT(pixRect) - tm.tmHeight) / 2;

    if (xOffset <= 0)
      xOffset = SM_CXPUSHBUTTONBORDER;
    if (yOffset <= 0)
      yOffset = SM_CYPUSHBUTTONBORDER;

    /*
      If the button is being clicked on, shift the text over
    */
    if (bDepressed)
    {
      xOffset += SM_CXFOCUSSHIFT;
      yOffset += SM_CYFOCUSSHIFT;
    }
    SetBkMode(hDC, TRANSPARENT);
    TextOut(hDC, xOffset, yOffset, pbw->title, lstrlen(pbw->title));
    SetBkMode(hDC, OPAQUE);
    if (cbi->bHasFocus)
      SetRect(&pixRect, xOffset-3, yOffset+2-(yOffset>>1),
              xOffset+sLen+3, yOffset-1+tm.tmHeight+(yOffset>>1));	
    }

#else /* !GUI */
    if (dwFlags & BS_LEFTTEXT)
    {
      WinPuts(hPB, RECT_HEIGHT(pbw->rClient)/2, 0, pbw->title, attr);
    }
    else
    {
#ifdef TELEVOICE
      /*
         Televoice-specific change
           If the pushbutton text spans more than one line in a multi-line
         pushbutton, then display the text on multiple lines
      */
      if ((int) strlen(pbw->title) < RECT_WIDTH(pbw->rClient))
        WinPutsCenter(hPB, RECT_HEIGHT(pbw->rClient)/2, pbw->title, attr);
      else
      {
        int   iWidth = RECT_WIDTH(pbw->rClient);
        int   row = 0;
        LPSTR pszNext = pbw->title, pszEnd;

        while (*pszNext)
        {
          if ((int) strlen(pszEnd = pszNext) > iWidth)
          {
            pszEnd += iWidth;
            while (*pszEnd != ' ' && pszEnd != pszNext)
              pszEnd--;
            if (pszEnd != pszNext)
              *pszEnd = '\0';
          }
          WinPutsCenter(hPB, row++, pszNext, attr);
          if (pszEnd != pszNext)
            *pszEnd = ' ';
          pszNext = pszEnd + 1;
        }
    }
#else
      WinPutsCenter(hPB, RECT_HEIGHT(pbw->rClient)/2, pbw->title, attr);
#endif
    }
#endif

    HilitePrefix = '\0';           /* Tell WinPuts to not interpret tildes */
    HilitePrefixAttr = oldHilitePrefixAttr;
  }
#ifdef MEWEL_GUI
  else
  {
    /*
      If the button is being clicked on, shift the text over
    */
    if (bDepressed)
      OffsetRect(&pixRect, SM_CXFOCUSSHIFT, SM_CYFOCUSSHIFT);
    if (cbi->bHasFocus)
      InflateRect(&pixRect, -4, -4);
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
  ReleaseDC(hPB, hDC);
#endif
#endif /* NATIVE_GUI */
}


/****************************************************************************/
/*                                                                          */
/* Function : _DrawOwnerDrawnButton                                         */
/*                                                                          */
/* Purpose  : Notifies the app to draw an owner-drawn button.               */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
#ifdef OWNERDRAWN
VOID FAR PASCAL _DrawOwnerDrawnButton(hPB, itemAction, itemState)
  HWND hPB;
  UINT itemAction;
  UINT itemState;
{
  WINDOW *wPB;
  DRAWITEMSTRUCT dis;
  HDC   hDC = 0;
  LPHDC lphDC;

  wPB = WID_TO_WIN(hPB);

  dis.CtlType    = ODT_BUTTON;
  dis.CtlID      = wPB->idCtrl;
  dis.hwndItem   = hPB;
  dis.itemID     = 0;
  dis.itemData   = 0L;

  dis.itemState  = itemState;
  if (!IsWindowEnabled(hPB))
    dis.itemState |= ODS_DISABLED;
  if (InternalSysParams.hWndFocus == hPB)
    dis.itemState |= ODS_FOCUS;

  dis.itemAction = itemAction;
  SetRect(&dis.rcItem, 0, 0, RECT_WIDTH(wPB->rect), RECT_HEIGHT(wPB->rect));
  dis.hDC      = hDC = GetWindowDC(hPB);
  lphDC        = _GetDC(hDC);
  lphDC->attr  = WinGetClassBrush(hPB);

  SendMessage(hPB, WM_DRAWITEM, wPB->idCtrl, (DWORD)(LPDRAWITEMSTRUCT) &dis);
  if (hDC)
    ReleaseDC(hPB, hDC);
}
#endif



/****************************************************************************/
/*                                                                          */
/* Function : DrawFocusRect(hDC, lpRect)                                    */
/*                                                                          */
/* Purpose  : Draws a dotted rectangle in a button in order to indicate     */
/*            that the button has focus.                                    */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
#if defined(MEWEL_GUI) || defined(MOTIF)
VOID FAR PASCAL DrawFocusRect(hDC, lpRect)
  HDC    hDC;
  CONST RECT FAR *lpRect;
{
  HPEN   hOldPen, hPen;
  UINT   wOldMode;

  /*
    Use a dotted pen and a NULL brush
  */
  /*
    Under GDI, creating a color which will be translated into a BIOS value
    of 7, and then XOR'ing it, will result in a black line.
  */
  hPen = CreatePen(PS_DOT, 1, DOT_RGB);

  hOldPen = SelectObject(hDC, hPen);
  SelectObject(hDC, GetStockObject(NULL_BRUSH));

  wOldMode = SetROP2(hDC, R2_XORPEN);

  /*
    Draw the rectangle slightly inside of the button frame
  */
  Rectangle(hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

  /*
    Restore the old pen and get rid of the dotted pen.
  */
  SelectObject(hDC, hOldPen);
  SetROP2(hDC, wOldMode);
  DeleteObject(hPen);
}
#endif

