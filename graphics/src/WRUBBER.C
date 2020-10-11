/*===========================================================================*/
/*                                                                           */
/* File    : WRUBBER.C                                                       */
/*                                                                           */
/* Purpose : Rubberbanding routines for moving and resizing a window.        */
/*                                                                           */
/* History : 1/2/90 (maa) added to MEWEL kernel                              */
/*           11/11/92 (maa) Removed calls to MouseHide/Show because of new   */
/*                          mouse hiding algorithm in wgraphic.c             */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/*
 * $Log:   E:/vcs/mewel/wrubber.c_v  $
//	
//	   Rev 1.2   12 Aug 1993  9:55:30   Adler
//	Eliminated test for WM_CHAR. Now only use WM_KEYDOWN.
//	
//	   Rev 1.1   24 May 1993 15:57:34   unknown
//	Did a WM_SETFOCUS after deleting the DC instead of before.
//	
//	   Rev 1.0   23 May 1993 21:06:06   adler
//	Initial revision.
 * 
 *    Rev 1.1   10 Sep 1991 15:40:54   MLOCKE
 * Added $Log:   E:/vcs/mewel/wrubber.c_v  $ keyword.
//	
//	   Rev 1.2   12 Aug 1993  9:55:30   Adler
//	Eliminated test for WM_CHAR. Now only use WM_KEYDOWN.
//	
//	   Rev 1.1   24 May 1993 15:57:34   unknown
//	Did a WM_SETFOCUS after deleting the DC instead of before.
//	
//	   Rev 1.0   23 May 1993 21:06:06   adler
//	Initial revision.
 * 
 * Allowed any dialog to be moved around within the area below
 * the main menu bar.
 */
#include "wprivate.h"
#include "window.h"

/*
  WinRubberband()
    Lets the user drag around (or stretch) an outline of a window as the
    user resizes or moves the window.
*/
static int PASCAL WinRestoreFrameSpace(HDC, RECT);
static int PASCAL WinSaveFrameSpace(HDC, RECT);

#define FRAME_TYPE   BORDER_SINGLE


/*
  Define the number of pixels/cells the window moves when using the
  keyboard to perform the moving/resizing.
*/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#define CX_INCREMENT  10
#define CY_INCREMENT  10
#else
#define CX_INCREMENT  1
#define CY_INCREMENT  1
#endif



INT FAR PASCAL WinRubberband(hWnd, message, origMouseRow, origMouseCol, iSizeMode)
  HWND hWnd;
  UINT message;   /* WM_MOVE or WM_SIZE */
  int  origMouseRow;
  int  origMouseCol;
  int  iSizeMode;
{
  RECT  r, rNew, rScreen, rOrig, rDummy;
  RECT  rOriginal;
  RECT  rParent;
  MINMAXINFO MinMax;
  RECT  rFrame, rFrameNew;
  HWND  hParent, hOldFocus;
  MSG   msg;
  UINT  attr;
  DWORD dwFlags;
  WINDOW *w;
  BOOL  bWasMoved  = FALSE;
  BOOL  bDoSysMenu = FALSE;
  BOOL  bAborted   = FALSE;  /* did we press ESC to abort the move? */
  int   cxDistance, cyDistance;
  int   iMinWidth, iMaxWidth, iMinHeight, iMaxHeight;

  MWCOORD mouserow;
  MWCOORD mousecol;
  MWCOORD *pLeft, *pTop, *pRight, *pBottom;

  HDC   hDC = (HDC) 0;
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  HPEN  hPen, hOldPen;
#endif


  (void) origMouseRow;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  /*
    Make sure that we are allowed to move or resize.
    We can resize if the window has a thickframe.
  */
  dwFlags = w->flags;
  if (message == WM_SIZE && !(dwFlags & WS_THICKFRAME))
    return FALSE;

#if defined(XWINDOWS)
printf("WinRubberband()\n");
#endif

  /*
    Windows will not allow the window to move if it has a system menu without
    a "Move" item. The same goes for resizing.
  */
  if (w->hSysMenu)
  {
    if (message == WM_MOVE && GetMenuState(w->hSysMenu,SC_MOVE,MF_BYCOMMAND) == (UINT) -1 ||
        message == WM_SIZE && GetMenuState(w->hSysMenu,SC_SIZE,MF_BYCOMMAND) == (UINT) -1)
      return FALSE;
  }


  /*
   * 9/10/91 Mike Locke
   *
   * We want to be able to move dialogs to any location on the screen.
   * This is particularly true when the child dialog occupies more
   * space on the screen than its parent.  The rubberbanding effect in
   * that instance shows the client rectangle of the parent within the
   * child - most confusing for the end user.
   *
   * So, if the window has a parent and its not a dialog limit it
   * to the area of its parent.  This should cover all the normal cases
   * including MDI interactions and does not allow the user to move
   * a control outside of a dialog.  (Not that controls are moveable
   * but who can say what's actually lurking out there.)
   *
   * If the window has a parent but is a dialog, set the limiting rectangle
   * to the screen rectangle below the main menu.
   */
  hParent = GetParent(hWnd);
  if (hParent == NULLHWND)
  {
    rScreen = SysGDIInfo.rectScreen;
  }
  else
  {
    if (dwFlags & WS_POPUP)
    {
     /*
      * Marc,
      *
      * This is just a kludge to let dialogs move anywhere
      * on the screen below the main menu bar.  What I really
      * think we should do here is to limit this to the client
      * area of the first ancestor window (proceeding from
      * this window's parent, to its grandparent...) that has
      * a menu.  In other words, a dialog should be able to be
      * positioned anywhere in the client area of the application
      * or subsystem of which it is a part.
      */
      /*
        9/12/91 (maa)
        Windows lets the dialog box obscure the menu area. So, let's
        make a define for the CompuServe folks.
      */
#ifdef COMPUSERVE_MEWEL
#define TOPROW_MIN 2
#else
#define TOPROW_MIN 0
#endif
      SetRect(&rParent, 0, TOPROW_MIN, VideoInfo.width, VideoInfo.length);
    }
    else
      WinGetClient(hParent, &rParent);
    rScreen = rParent;
  }


  /*
    Set the default max size and max tracking of the window 
  */
  MinMax.ptMaxTrackSize.x = MinMax.ptMaxSize.x = rScreen.right;
  MinMax.ptMaxTrackSize.y = MinMax.ptMaxSize.y = rScreen.bottom;

  /*
    MinMax.ptMinTrackSize is used only by the resizing case below,
    and we are only concerned about the width and height of the rectangle.
    The min height of the window will be enough to show all of the
    non-client area.
  */
  r = w->rClient;
  MinMax.ptMinTrackSize.x = RECT_WIDTH(w->rect)  - RECT_WIDTH(r);
  MinMax.ptMinTrackSize.y = RECT_HEIGHT(w->rect) - RECT_HEIGHT(r);

  /*
    We do not want to cut off the caption-bar decorations. So figure
    how how many decorations we have, and the total width of the
    decorations.
  */
  iMinWidth = 0;
  if (dwFlags & WS_SYSMENU)
    iMinWidth++;
  if (dwFlags & WS_MAXIMIZEBOX)
    iMinWidth++;
  if (dwFlags & WS_MINIMIZEBOX)
    iMinWidth++;
  if (dwFlags & WS_THICKFRAME)
    iMinWidth++;  /* add the thick borders in for good measure */
  iMinWidth *= IGetSystemMetrics(SM_CXSIZE);
  if (MinMax.ptMinTrackSize.x < iMinWidth)
    MinMax.ptMinTrackSize.x = iMinWidth;

  /*
    See if the app wants to change the sizing constraints.
  */
  SendMessage(hWnd, WM_GETMINMAXINFO, 0, (DWORD) (LPSTR) &MinMax);

  /*
    Put a dashed box around the window which we're dragging.
  */
  r = w->rect;
  rNew = rOrig = r;
  if ((attr = w->attr) == SYSTEM_COLOR)
    attr = WinQuerySysColor(NULLHWND, SYSCLR_ACTIVEBORDER);

  /*
    For moving, find the distance between the mouse and the upper-left
    corner of the window. We will use this distance as an offset for
    where the upper-left corner should be moved to.
  */
  cxDistance = origMouseCol - r.left;
  cyDistance = 0;
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  /*
    We want the icon to track the vertical position too.
    (cyDistance can be 0 if we click the mouse on the 'Move' system
    menu entry of an iconic window, so we need to normalize.)
  */
  if ((cyDistance = origMouseRow - r.top) < 0)
    cyDistance = 0;
#endif

  /*
    If the window who has the focus is an edit window, send a WM_KILLFOCUS
    message so the cursor and highlighting are hidden.
    Set the focus window to NULL temporarily so that the edit cursor does
    not constantly get refreshed.
  */
  if ((hOldFocus = InternalSysParams.hWndFocus) != NULLHWND)
  {
    SendMessage(hOldFocus, WM_KILLFOCUS, 0, 0L);
    InternalSysParams.hWndFocus = NULLHWND;
  }


  if (message == WM_SIZE)
  {
    iMinWidth  = MinMax.ptMinTrackSize.x;
    iMaxWidth  = MinMax.ptMaxTrackSize.x;
    iMinHeight = MinMax.ptMinTrackSize.y;
    iMaxHeight = MinMax.ptMaxTrackSize.y;

    switch (iSizeMode)
    {
      case HTTOPLEFT  :
        pLeft = &mousecol;  pTop = &mouserow;  pRight = &r.right;  pBottom = &r.bottom;
        break;
      case HTTOPRIGHT :
        pLeft = &r.left;    pTop = &mouserow;  pRight = &mousecol; pBottom = &r.bottom;
        break;
      case HTBOTTOMLEFT:
        pLeft = &mousecol;  pTop = &r.top;     pRight = &r.right;  pBottom = &mouserow;
        break;
      case HTBOTTOMRIGHT :
        pLeft = &r.left;    pTop = &r.top;     pRight = &mousecol; pBottom = &mouserow;
        break;
      case HTLEFT     :
        pLeft = &mousecol;  pTop = &r.top;     pRight = &r.right;  pBottom = &r.bottom;
        break;
      case HTRIGHT    :
        pLeft = &r.left;    pTop = &r.top;     pRight = &mousecol; pBottom = &r.bottom;
        break;
      case HTTOP      :
        pLeft = &r.left;    pTop = &mouserow;  pRight = &r.right;  pBottom = &r.bottom;
        break;
      case HTBOTTOM   :
        pLeft = &r.left;    pTop = &r.top;     pRight = &r.right;  pBottom = &mouserow;
        break;
    }
  }
  rFrame.top = -1, rFrameNew.top = 1;

  /*
    The window we are moving/resizing has the mouse captured.
  */
  SetCapture(hWnd);

  /*
    For refreshing, save the original rectangle
  */
  IntersectRect(&rOriginal, &rScreen, &rNew);
  _WinAdjustRectForShadow(w, (LPRECT) &rOriginal);


  /*
    Before we save the screen, we should make sure that all of the windows
    have been properly updated. Otherwise, the first _PeekMsg() below
    can cause the invalid windows to be refreshed, thereby making the
    saved area totally wrong.
  */
  if (TEST_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST))
    RefreshInvalidWindows(_HwndDesktop);


#if defined(MEWEL_GUI) || defined(XWINDOWS)
  /*
    We need to grab a DC to the desktop in order to draw the 
    rubberbanding lines. Use a grey solid pen and a null brush
    in XOR mode to draw the outline.
  */
  hDC = GetDC(_HwndDesktop);
  SelectObject(hDC, GetStockObject(NULL_BRUSH));
#if defined(MOTIF)
  hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
#else
  hPen = CreatePen(PS_SOLID, 1, AttrToRGB(15));  /* 15 seems to work for BGI */
#endif
  hOldPen = SelectObject(hDC, hPen);
  SetROP2(hDC, R2_XORPEN);
#endif


  /*
    Message-processing loop
  */
  for (;;)
  {
    IntersectRect(&rFrameNew, &rScreen, &rNew);

    /*
      Figure out the framing rectangle
      If the framing rectangle is empty (ie - we were trying to drag the
      window completely off the screen), then reset it to something valid.
    */
    if (RECT_HEIGHT(rFrameNew) <= 1)
    {
      if (rFrameNew.top)
        --rFrameNew.top,    --rOrig.top, --rOrig.bottom;
      else
        ++rFrameNew.bottom, ++rOrig.top, ++rOrig.bottom;
    }
    if (RECT_WIDTH(rFrameNew) <= 1)
    {
      if (rFrameNew.left)
        --rFrameNew.left,   --rOrig.left, --rOrig.right;
      else
        ++rFrameNew.right,  ++rOrig.left, ++rOrig.right;
    }

    if (!EqualRect(&rFrameNew, &rFrame))
    {
      /*  Changed restore to match new save */
      WinRestoreFrameSpace(hDC, rFrame);

      rFrame = rFrameNew;
      /*
       Show the screen again with the dashed box at the new position.
       Adjust the size or position of the dragged window to match the
       dashed box.
      */
      if (!WinSaveFrameSpace(hDC, rFrame))
      {
        ReleaseCapture();
        return FALSE;
      }

#if defined(MEWEL_GUI) || defined(XWINDOWS)
      (void) attr;
#else
      VidFrame(rFrame.top,rFrame.left,rFrame.bottom,rFrame.right,attr,FRAME_TYPE);
#endif
    }
    /*
      Give background stuff a chance to run...
    */
    while (!_PeekMessage(&msg))
      ;

    /*
      Get out if the mouse button was released
    */
    if (_WinGetMessage(&msg)==WM_LBUTTONUP || msg.message==WM_NCLBUTTONUP)
    {
      if (msg.message == WM_LBUTTONUP)
        msg.lParam = _UnWindowizeMouse(hWnd, msg.lParam);
      break;
    }

    if (msg.message == WM_MOUSEMOVE || msg.message == WM_NCMOUSEMOVE)
    {
      if (msg.message == WM_MOUSEMOVE)
        msg.lParam = _UnWindowizeMouse(hWnd, msg.lParam);

      /* Get the 0-based screen coords of the mouse */
      mouserow = HIWORD(msg.lParam);
      mousecol = LOWORD(msg.lParam);

      /*
        Don't move outside of the parent
      */
      if (hParent && !PtInRect(&rParent, MAKEPOINT(msg.lParam)))
        continue;

      if (message == WM_MOVE)
      {
        SetRect(&rNew, mousecol - cxDistance, mouserow - cyDistance,
                       (mousecol - cxDistance) + RECT_WIDTH(r),
                       (mouserow - cyDistance) + RECT_HEIGHT(r));
      }
      else /* Resizing */
      {
#if defined(MEWEL_TEXT)
        /*
          Increment the bottom right
        */
        if (pRight  == &mousecol)
          mousecol++;
        if (pBottom == &mouserow)
          mouserow++;
#endif
        SetRect(&rNew, *pLeft, *pTop, *pRight, *pBottom);
      }
    }

    else if (msg.message == WM_KEYDOWN)
    {
      int incr;

      rNew = rOrig;  /* rOrig holds the current valid position */

      switch (msg.wParam)
      {
        case VK_LEFT :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          incr = (GetKeyState(VK_CONTROL) == 0) ? CX_INCREMENT : 1;
#else
          incr = CX_INCREMENT;
#endif
          rNew.right -= incr;
          if (message == WM_MOVE)
            rNew.left -= incr;
          break;
        case VK_RIGHT:
#if defined(USE_WINDOWS_COMPAT_KEYS)
          incr = (GetKeyState(VK_CONTROL) == 0) ? CX_INCREMENT : 1;
#else
          incr = CX_INCREMENT;
#endif
          rNew.right += incr;
          if (message == WM_MOVE)
            rNew.left += incr;
          break;
        case VK_UP :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          incr = (GetKeyState(VK_CONTROL) == 0) ? CY_INCREMENT : 1;
#else
          incr = CY_INCREMENT;
#endif
          rNew.bottom -= incr;
          if (message == WM_MOVE)
            rNew.top -= incr;
          break;
        case VK_DOWN :
#if defined(USE_WINDOWS_COMPAT_KEYS)
          incr = (GetKeyState(VK_CONTROL) == 0) ? CY_INCREMENT : 1;
#else
          incr = CY_INCREMENT;
#endif
          rNew.bottom += incr;
          if (message == WM_MOVE)
            rNew.top += incr;
          break;

#if (defined(MEWEL_GUI) || defined(XWINDOWS)) && !defined(USE_WINDOWS_COMPAT_KEYS)
        /*
          In the GUI version, the CTRL-arrow keys move by single pixels
        */
        case VK_CTRL_LEFT :
          rNew.right -= 1;
          if (message == WM_MOVE)
            rNew.left -= 1;
          break;
        case VK_CTRL_RIGHT:
          rNew.right += 1;
          if (message == WM_MOVE)
            rNew.left += 1;
          break;
        case VK_CTRL_UP :
          rNew.bottom -= 1;
          if (message == WM_MOVE)
            rNew.top -= 1;
          break;
        case VK_CTRL_DOWN :
          rNew.bottom += 1;
          if (message == WM_MOVE)
            rNew.top += 1;
          break;
#endif

        case VK_ESCAPE    :
          bAborted = TRUE;
          /* fall through... */
        case VK_RETURN :
          goto bye;
        default :
          break;
      } /* switch */
    } /* WM_KEYDOWN */


    /*
      1) Is the rect empty? If so, forget about this operation.
      2) Don't move totally outside of the parent
    */
    if (IsRectEmpty(&rNew) ||                                    /* 1) */
       (hParent && !IntersectRect(&rDummy, &rParent, &rNew)))    /* 2) */
    {
      rNew = rOrig;
      continue;
    }


#if defined(MEWEL_GUI) || defined(XWINDOWS)
    /*
      In the GUI, do not move the icon so that any part of it
      falls outside the parent. Otherwise, the icon can overwrite
      the parent.
    */
    if (IsIconic(hWnd) &&
       (!IntersectRect(&rDummy, &rScreen, &rNew) || !EqualRect(&rDummy, &rNew)))
    {
      rNew = rOrig;
      continue;
    }
#endif

    /*
      See if we exceeded the min or max tracking rectangles.
    */
    if (message == WM_SIZE)
    {
#if 101394
      if (RECT_WIDTH(rNew)  < iMinWidth  ||
          RECT_WIDTH(rNew)  > iMaxWidth  ||
          RECT_HEIGHT(rNew) < iMinHeight ||
          RECT_HEIGHT(rNew) > iMaxHeight)
      {
        rNew = rOrig;
        continue;
      }
#else
      if (RECT_WIDTH(rNew) < iMinWidth)
        rNew.right = rNew.left + iMinWidth;
      else if (RECT_WIDTH(rNew) > iMaxWidth)
        rNew.right = rNew.left + iMaxWidth;

      if (RECT_HEIGHT(rNew) < iMinHeight)
        rNew.bottom = rNew.top + iMinHeight;
      else if (RECT_HEIGHT(rNew) > iMaxHeight)
        rNew.bottom = rNew.top + iMaxHeight;
#endif
    }

    /*
      OK! We're cool! Copy the new rect dimensions to rOrig.
    */
    rOrig = rNew;
  } /* end for (;;) */


bye:
  SendMessage(hWnd, WM_CANCELMODE, 0, 0L);

  /*
    Free the old screen image and give back the mouse.
  */
  WinRestoreFrameSpace(hDC, rFrame);
  ReleaseCapture();

  /*
    See if the window was moved from its original position.
  */
  if (!IsRectEmpty(&rOrig))
  {
    RECT rTmp;

    /*
      If the window had a shadow, then compare the shadow-compensated
      maybe-new rectangle with the already shadow-compensated
      original rectangle.
    */
    rTmp = rOrig;
    _WinAdjustRectForShadow(w, (LPRECT) &rTmp);
    if (!EqualRect(&rTmp, &rOriginal))
      bWasMoved = TRUE;
  }

  /*
    Position the new window by using SetWindowPos()
  */
  if (bWasMoved && !bAborted)
  {
    RECT  r2;
    UINT  flSWP;

    r2 = rOrig;
    if (w->flags & WS_CHILD)
      WinScreenRectToClient(GetParentOrDT(hWnd), (LPRECT) &r2);
    flSWP  = SWP_NOZORDER | SWP_NOACTIVATE;
    if (message == WM_MOVE)
      flSWP |= SWP_NOSIZE;
    SetWindowPos(hWnd,0,r2.left,r2.top,RECT_WIDTH(r2),RECT_HEIGHT(r2),flSWP);
  }


  /*
    If we are rubberbanding a dialog box (ie - a window with no parent),
    then explicitly redraw the dlgbox since it is not part of the
    desktop window's family tree...
  */
  if (bWasMoved)
  {
    _WinAdjustRectForShadow(w, (LPRECT) &rOrig);
    UnionRect(&rOrig, &rOrig, &rOriginal);

#ifdef USE_ICON_TITLE
    if (IsIconic(hWnd))
    {
      HWND hTitle = ((PRESTOREINFO) w->pRestoreInfo)->hIconTitle;
      if (hTitle)
        UnionRect(&rOrig, &rOrig, &(WID_TO_WIN(hTitle)->rect));
    }
#endif
  }
  else
  {
    /*
      If there was no movement and the window is iconic and it has a
      system menu, then invoke the system menu.
    */
    if (IS_MOUSE_MSG(SysEventInfo.lastMessage) && IsIconic(hWnd) && 
        (dwFlags & WS_SYSMENU))
    {
      bDoSysMenu++;
    }
  }



  /*
    Delete the DC and the pen
  */
  if (hDC)
  {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    if (hPen)
      DeleteObject(SelectObject(hDC, hOldPen));
#endif
    ReleaseDC(_HwndDesktop, hDC);
  }


  /*
    If the window who has the focus is an edit window, send a WM_SETFOCUS
    message so the cursor and highlighting are made visible again.
    Note - if we shrunk the window, then the edit control may not be
    visible.
  */
  if (hOldFocus)
  {
    InternalSysParams.hWndFocus = hOldFocus;
    SendMessage(hOldFocus, WM_SETFOCUS, 0, 0L);
  }



  if (bDoSysMenu)
    return WinActivateSysMenu(hWnd);
  else
    return TRUE;
}



#if defined(MEWEL_GUI) || defined(XWINDOWS)
static int    saveSP = 0;
#else
static HANDLE aszSaved[4] = {NULL, NULL, NULL, NULL};
#endif

static int PASCAL WinSaveFrameSpace(hDC, rFrm)
  HDC  hDC;
  RECT rFrm;
{
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  saveSP++;
  Rectangle(hDC, rFrm.left, rFrm.top, rFrm.right, rFrm.bottom);
#else

  RECT r;
  int i;

  (void) hDC;

  for (i = 0;  i < 4;  i++)
  {
    r = rFrm;
    switch(i)
    {
      case 0:  /* top */
        r.bottom = r.top + 1;
        break;
      case 1:  /* bottom */
        r.top = r.bottom - 1;
        break;
      case 2:  /* left */
        r.right = r.left + 1;
        break;
      case 3:  /* right */
        r.left = r.right - 1;
        break;
      }
    if ((aszSaved[i] = _WinSaveRect(NULLHWND, &r)) == NULL)
      return WinRestoreFrameSpace(0, rFrm);
  }
#endif

  return TRUE;
}


static int PASCAL WinRestoreFrameSpace(hDC, rFrm)
  HDC  hDC;
  RECT rFrm;
{
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  if (saveSP > 0)
  {
    Rectangle(hDC, rFrm.left, rFrm.top, rFrm.right, rFrm.bottom);
    saveSP--;
  }
  return TRUE;

#else

  RECT r;
  int i;

  (void) hDC;

  for (i = 3;  i >= 0;  i--)
  {
    r = rFrm;
    switch(i)
    {
      case 3:
        r.left = r.right - 1;
        break;
      case 2:
        r.right = r.left + 1;
        break;
      case 1:
        r.top = r.bottom - 1;
        break;
      case 0:
        r.bottom = r.top + 1;
        break;
    }
    if (aszSaved[i])
    {
      WinRestoreRect(NULLHWND, &r, aszSaved[i]);
      GlobalFree(aszSaved[i]);
      aszSaved[i] = NULL;
    }
  }
  return FALSE;

#endif
}
