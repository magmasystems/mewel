/*===========================================================================*/
/*                                                                           */
/* File    : wchocolr.c                                                      */
/*                                                                           */
/* Purpose : Implements the standard choose color dialog box                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/* #define CC_TEST */

#ifdef CC_TEST
#define _EXTERN extern /* Use External definitions from main module */
#include "ccolor.h"

BOOL WINAPI _ChooseColor(CHOOSECOLOR FAR*);
#else
#include "wprivate.h"
#include "window.h"
#include "commdlg.h"

static BOOL WINAPI _ChooseColor(CHOOSECOLOR FAR*);
#endif /* CC_TEST */

static BOOL CALLBACK _export _ChooseColorDlgProc(HWND, UINT, WPARAM, LPARAM);
static long WINAPI StaticWndProc(HWND, UINT, WPARAM, LPARAM);
static VOID WINAPI SetOptions(BOOL, HWND, COLORREF);
static VOID WINAPI PaintColorBox(HWND, LPDWORD);
static VOID WINAPI PaintSelectedBox(HWND, WORD, BOOL);
static VOID WINAPI PaintCurrentColor(HWND, COLORREF);
static VOID WINAPI ShowOptions(BOOL, HWND, HWND);
static VOID RGBtoHLS(DWORD);
static WORD HueToRGB(WORD, WORD, WORD);
static DWORD HLStoRGB(WORD, WORD, WORD);

static CHOOSECOLOR FAR* lpcc;
static FARPROC lpfnOldStatic,lpfnNewStatic;
static WORD color[3], boxw, boxh, sColor, H, S, L;

#if defined(MEWEL) && !defined(MOTIF)

/* 16 basic colors, 4 custom ones */

#define BOX1_COLS           4
#define BOX1_ROWS           4
#define BOX1_SPAC           6
#define CUSTOM1_ROWS        1
#define DOT_FREQ            2

static COLORREF colorBtn[] = {
  RGB(192,192,192), RGB(  0,  0,128), RGB(  0,128,  0), RGB(  0,128,128),
  RGB(128,  0,  0), RGB(128,  0,128), RGB(128,128,  0), RGB(128,128,128),
  RGB(  0,  0,  0), RGB(  0,  0,255), RGB(  0,255,  0), RGB(  0,255,255),
  RGB(255,  0,  0), RGB(255,  0,255), RGB(255,255,  0), RGB(255,255,255),
};
#else

/* 48 basic colors, 16 custom ones */

#define BOX1_COLS           8
#define BOX1_ROWS           6
#define BOX1_SPAC           6
#define CUSTOM1_ROWS        2
#define DOT_FREQ            2

static COLORREF colorBtn[] = {
  RGB(255,128,128), RGB(255,255,232), RGB(128,255,128), RGB(  0,255,128),
  RGB(128,255,255), RGB(  0,128,255), RGB(255,128,192), RGB(255,128,255),
  RGB(255,  0,  0), RGB(255,255,128), RGB(128,255,  0), RGB(  0,255, 64),
  RGB(  0,255,255), RGB(  0,128,192), RGB(128,128,192), RGB(255,  0,255),
  RGB(128, 64, 64), RGB(255,255,  0), RGB(  0,255,  0), RGB(  0,128,128),
  RGB(  0, 64,128), RGB(128,128,255), RGB(128,  0, 64), RGB(255,  0,128),
  RGB(128,  0,  0), RGB(255,128,  0), RGB(  0,128,  0), RGB(  0,128, 64),
  RGB(  0,  0,255), RGB(  0,  0,160), RGB(128,  0,128), RGB(128,  0,255),
  RGB( 64,  0,  0), RGB(128, 64,  0), RGB(  0, 64,  0), RGB(  0, 64, 64),
  RGB(  0,  0,128), RGB(  0,  0, 64), RGB( 64,  0, 64), RGB( 64,  0,128),
  RGB(  0,  0,  0), RGB(128,128,  0), RGB(128,128, 64), RGB(128,128,128),
  RGB( 64,128,128), RGB(192,192,192), RGB(130,130,130), RGB(255,255,255),
};
#endif /* MEWEL && !MOTIF */

#define NUM_BASIC_COLORS    (BOX1_COLS * BOX1_ROWS)
#define NUM_CUSTOM_COLORS   (BOX1_COLS * CUSTOM1_ROWS)
#define MIXED_COLOR         (NUM_BASIC_COLORS + NUM_CUSTOM_COLORS)

#define WM_CCUPDATE   (WM_USER + 64)

/* --------------------------------------------------------------------------
//
// Function: ChooseColor(CHOOSECOLOR FAR*)
//
// Purpose:  Common dialog to choose color.
//
// ------------------------------------------------------------------------*/
#ifdef CC_TEST
#undef MEWEL /* Testing */
#endif
#ifdef MEWEL
BOOL WINAPI ChooseColor(CHOOSECOLOR FAR* lc)
{
  return _ChooseColor(lc);
}
#endif /* MEWEL */

/* --------------------------------------------------------------------------
//
// Function: _ChooseColor(CHOOSECOLOR FAR*)
//
// Purpose:  Implements choose color common dialog.
//
// ------------------------------------------------------------------------*/
#ifndef CC_TEST
static
#endif
BOOL WINAPI _ChooseColor(CHOOSECOLOR FAR* lc)
{
  DLGPROC lpDlgProc;

  lpcc = lc;
#ifdef MEWEL
  /*
    Set the MEWEL instance to the fd of the open resource file
  */
  if (lpcc->hInstance <= 0)
    lpcc->hInstance = MewelCurrOpenResourceFile;
#else
  /*
    Set the data block to the global hInst
  */
  if (lpcc->hInstance <= 0)
    lpcc->hInstance = hInst;
#endif /* MEWEL */

  lpDlgProc = (DLGPROC) MakeProcInstance((FARPROC)_ChooseColorDlgProc,
                                         lpcc->hInstance);
  if (DialogBox(lpcc->hInstance, "ChooseColor", lpcc->hwndOwner, lpDlgProc))
    lpcc->rgbResult = RGB(color[0], color[1], color[2]);
  FreeProcInstance((FARPROC)lpDlgProc);
  return 0;
}

/* --------------------------------------------------------------------------
//
// Function: _ChooseColorDlgProc(HWND, UINT, WORD, LONG)
//
// Purpose:  Processes Dialog Messages
//
// ------------------------------------------------------------------------*/
static
BOOL CALLBACK _export _ChooseColorDlgProc(HWND hDlg, UINT uMsg, 
                                          WPARAM wParam, LPARAM lParam)
{
  static BOOL bOptions = FALSE;
  static RECT rBox1, rCustom1;
  static WORD cColor = 0;
  HWND hCtrl;
  PAINTSTRUCT ps;
  DWORD rgb;
  WORD i, n;

  switch (uMsg)
  {
    case WM_INITDIALOG:
      /* Initialize color and control values */
      for (i = 0 ; i < 3 ; i++)
      {
        color[i] = 0;
        SetScrollRange(GetDlgItem(hDlg, COLOR_REDSB + i),
                       SB_CTL, 0, 255, FALSE);
        SetScrollPos(GetDlgItem(hDlg, COLOR_REDSB + i), 
                     SB_CTL, color[i], FALSE);
        SetDlgItemInt(hDlg, COLOR_RED + i, color[i], FALSE);
      }
      RGBtoHLS(RGB(color[0], color[1], color[2]));
      SetDlgItemInt(hDlg, COLOR_HUE, H, FALSE);
      SetDlgItemInt(hDlg, COLOR_SAT, S, FALSE);
      SetDlgItemInt(hDlg, COLOR_LUM, L, FALSE);
      sColor = MIXED_COLOR;

      /* Save rectangle coordinates */
      hCtrl = GetDlgItem(hDlg, COLOR_BOX1);
      GetClientRect(hCtrl, &rBox1);
      boxw = (rBox1.right - rBox1.left) / BOX1_COLS;
      boxh = (rBox1.bottom - rBox1.top) / BOX1_ROWS;
      MapWindowPoints(hCtrl, hDlg, (POINT FAR*)&rBox1, 2);
      hCtrl = GetDlgItem(hDlg, COLOR_CUSTOM1);
      GetClientRect(hCtrl, &rCustom1);
      MapWindowPoints(hCtrl, hDlg, (POINT FAR*)&rCustom1, 2);

      /* Subclass the static boxes to get the keystrokes */
      lpfnNewStatic = MakeProcInstance((FARPROC)StaticWndProc,
                                       lpcc->hInstance);
      lpfnOldStatic = (FARPROC) SetWindowLong(GetDlgItem(hDlg, COLOR_BOX1),
                                           GWL_WNDPROC, (LONG)lpfnNewStatic);
      SetWindowLong(GetDlgItem(hDlg, COLOR_CUSTOM1),
                                           GWL_WNDPROC, (LONG)lpfnNewStatic);

      SetOptions(bOptions, hDlg, RGB(color[0], color[1], color[2]));

      /* Process option flags */
      if ( !(lpcc->Flags && CC_SHOWHELP) )
      {
        hCtrl = GetDlgItem(hDlg, IDHELP);
        ShowWindow(hCtrl, SW_HIDE);
      }
      if ( lpcc->Flags && CC_PREVENTFULLOPEN )
      {
        hCtrl = GetDlgItem(hDlg, COLOR_MIX);
        ShowWindow(hCtrl, SW_HIDE);
      }
      else if ( lpcc->Flags && CC_FULLOPEN )
        SetOptions(bOptions = !bOptions, hDlg, 
                   RGB(color[0], color[1], color[2]));

      /* Set focus to first color box and return */
      SetFocus(GetDlgItem(hDlg, COLOR_BOX1));
      return TRUE;

    case WM_COMMAND:
      switch (wParam)
      {
        case COLOR_MIX:
          if (HIWORD(lParam) != BN_CLICKED)
            break;
          SetOptions(!bOptions, hDlg, RGB(color[0], color[1], color[2]));
          bOptions = !bOptions;
          break;
        case COLOR_ADD:
          if ( cColor >= NUM_CUSTOM_COLORS )
            cColor = 0;
          PaintSelectedBox(hDlg, sColor, FALSE);
          lpcc->lpCustColors[cColor++] = RGB(color[0], color[1], color[2]);
          PaintColorBox(GetDlgItem(hDlg, COLOR_CUSTOM1), lpcc->lpCustColors);
          sColor = cColor + NUM_BASIC_COLORS - 1;
          PaintSelectedBox(hDlg, sColor, TRUE);
          break;
        case COLOR_RED:
        case COLOR_GREEN:
        case COLOR_BLUE:
          if (HIWORD(lParam) != EN_CHANGE)
            break;
          i = wParam - COLOR_RED;
          color[i] = GetDlgItemInt(hDlg, wParam, NULL, FALSE);
          RGBtoHLS(RGB(color[0], color[1], color[2]));
          SendMessage(hDlg, WM_CCUPDATE, 1, 0L);
          break;
        case COLOR_HUE:
        case COLOR_SAT:
        case COLOR_LUM:
          if (HIWORD(lParam) != EN_CHANGE)
            break;
          switch (wParam)
          {
            case COLOR_HUE:
              H = GetDlgItemInt(hDlg, wParam, NULL, FALSE); break;
            case COLOR_SAT:
              S = GetDlgItemInt(hDlg, wParam, NULL, FALSE); break;
            case COLOR_LUM:
              L = GetDlgItemInt(hDlg, wParam, NULL, FALSE); break;
          }
#if 0
          rgb = HLStoRGB(H, L, S);
          color[0] = GetRValue(rgb);
          color[1] = GetGValue(rgb);
          color[2] = GetBValue(rgb);
#endif
          SendMessage(hDlg, WM_CCUPDATE, 1, 0L);
          break;
        case IDOK:
        case IDCANCEL:
          /* Remove subclassing from static boxes */
          SetWindowLong((HWND)GetDlgItem(hDlg,COLOR_CUSTOM1),
                        GWL_WNDPROC,(LONG)lpfnOldStatic);
          SetWindowLong((HWND)GetDlgItem(hDlg,COLOR_BOX1),
                        GWL_WNDPROC,(LONG)lpfnOldStatic);
          FreeProcInstance((FARPROC)lpfnNewStatic);
          /* Close the dialog */
          EndDialog(hDlg, wParam == IDOK ? TRUE : FALSE);
          break;
        case IDHELP:
          MessageBox(hDlg, "Please choose a color.", "Help", MB_OK);
          break;
      } 
      break;

    case WM_HSCROLL :
      hCtrl   = HIWORD(lParam);
      i = GetWindowWord(hCtrl, GWW_ID) - COLOR_REDSB;

      switch (wParam)
      {
        case SB_PAGEDOWN:
          color[i] += 15;
        case SB_LINEDOWN:
          color[i] = min(255, color[i] + 1);
          break;
        case SB_PAGEUP:
          color[i] -= 15;
        case SB_LINEUP:
          color[i] = max(0, color[i] - 1);
          break;
        case SB_TOP:
          color[i] = 0;
          break;
        case SB_BOTTOM:
          color[i] = 255;
          break ;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
          color[i] = LOWORD(lParam);
          break ;
        default:
          return TRUE;
      }
      SendMessage(hDlg, WM_CCUPDATE, 0, 0L);
      break;

    case WM_PAINT:
      {
      /* Paint basic and custom colors */
      RECT r, rInt;
      BeginPaint(hDlg, &ps);
      CopyRect(&r, &ps.rcPaint);
      EndPaint(hDlg, &ps);

      /* Check if on basic colors box */
      IntersectRect(&rInt, &rBox1, &r);
      if ( !IsRectEmpty(&rInt) )
      {
        if ( sColor < NUM_BASIC_COLORS )
          PaintSelectedBox(hDlg, sColor, FALSE);
        PaintColorBox(GetDlgItem(hDlg, COLOR_BOX1), colorBtn);
      }
      IntersectRect(&rInt, &rCustom1, &r);
      if ( !IsRectEmpty(&rInt) )
      {
        if ( sColor >= NUM_BASIC_COLORS && sColor < MIXED_COLOR )
          PaintSelectedBox(hDlg, sColor, FALSE);
        PaintColorBox(GetDlgItem(hDlg, COLOR_CUSTOM1), lpcc->lpCustColors);
      }
      SendMessage(hDlg, WM_CCUPDATE, 0, 0L);
      }
      return TRUE;

    case WM_LBUTTONDOWN:
    {
      RECT r;
      POINT pt;

      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      n = MIXED_COLOR;
      /* Check if on basic colors box */
      if ( PtInRect(&rBox1, pt) )
        for ( i = 0; i < NUM_BASIC_COLORS; i++)
        {
          r.left = rBox1.left + (i % BOX1_COLS) * boxw + (BOX1_SPAC / 2);
          r.top = rBox1.top + (i / BOX1_COLS) * boxh + (BOX1_SPAC / 2);
          r.right = r.left + boxw - BOX1_SPAC;
          r.bottom = r.top + boxh - BOX1_SPAC;
          if ( PtInRect(&r, pt) )
          {
            color[0] = GetRValue(colorBtn[i]);
            color[1] = GetGValue(colorBtn[i]);
            color[2] = GetBValue(colorBtn[i]);
            n = i;
            SetFocus(GetDlgItem(hDlg, COLOR_BOX1));
            break;
          }
        }
      /* Check if on custom colors box */
      else if ( PtInRect(&rCustom1, pt) )
        for ( i = 0; i < NUM_CUSTOM_COLORS; i++)
        {
          r.left = rCustom1.left + (i % BOX1_COLS) * boxw + (BOX1_SPAC / 2);
          r.top = rCustom1.top + (i / BOX1_COLS) * boxh + (BOX1_SPAC / 2);
          r.right = r.left + boxw - BOX1_SPAC;
          r.bottom = r.top + boxh - BOX1_SPAC;
          if ( PtInRect(&r, pt) )
          {
            color[0] = GetRValue(lpcc->lpCustColors[i]);
            color[1] = GetGValue(lpcc->lpCustColors[i]);
            color[2] = GetBValue(lpcc->lpCustColors[i]);
            n = i + NUM_BASIC_COLORS;
            SetFocus(GetDlgItem(hDlg, COLOR_CUSTOM1));
            break;
          }
        }
      /* If a color was selected, paint a border */
      if (n < MIXED_COLOR && n != sColor)
      {
        sColor = n;
        SendMessage(hDlg, WM_CCUPDATE, 0, 0L);
      }
    }
    break;

    case WM_CCUPDATE:
      /* Update controls */
      if ( (sColor < NUM_BASIC_COLORS &&
             colorBtn[sColor] != RGB(color[0], color[1], color[2])) ||
           (sColor >= NUM_BASIC_COLORS &&
             lpcc->lpCustColors[sColor - NUM_BASIC_COLORS] !=
               RGB(color[0], color[1], color[2])))
      {
        PaintSelectedBox(hDlg, sColor, FALSE);
        sColor = MIXED_COLOR;
      }
      else
        PaintSelectedBox(hDlg, sColor, TRUE);
      /* Paint current color and update scroll bars if opt area is exposed */
      if (bOptions)
      {
        for (i = 0 ; i < 3 ; i++)
        {
          SetScrollPos(GetDlgItem(hDlg, COLOR_REDSB + i), 
                       SB_CTL, color[i], TRUE);
          if (wParam != 1) /* Avoid recursive calls */
            SetDlgItemInt(hDlg, COLOR_RED + i, color[i], FALSE);
        }
        if (wParam != 1) /* Avoid recursive calls */
        {
          SetDlgItemInt(hDlg, COLOR_HUE, H, FALSE);
          SetDlgItemInt(hDlg, COLOR_SAT, S, FALSE);
          SetDlgItemInt(hDlg, COLOR_LUM, L, FALSE);
        }
        PaintCurrentColor(GetDlgItem(hDlg, COLOR_CURRENT),
                          RGB(color[0], color[1], color[2]));
      }
      break;

    case WM_CLOSE:
      PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
      break; 

    case WM_DESTROY:
      break;

    default:
      break;
  }
  return FALSE;
}

/* --------------------------------------------------------------------------
//
// Function: StaticWndProc(HWND, LPDWORD)
//
// Purpose:  Subclasses the standard window procedure for static controls.
//
// ------------------------------------------------------------------------*/
static
long WINAPI StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HWND hDlg = GetParent(hWnd);
  short n;

  switch (uMsg) 
  {
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;

    case WM_CHAR:
    case WM_KEYDOWN:
      {
      LPDWORD lpColors;
      short nOrg, nMax = NUM_BASIC_COLORS;

      n = sColor;
/*    wParam = (WORD)(BYTE) AnsiLower((BYTE) wParam); */
      if ( n >= MIXED_COLOR )
        break;
      if ( n < NUM_BASIC_COLORS )
        lpColors = colorBtn;
      else
      {
        lpColors = lpcc->lpCustColors;
        n -= NUM_BASIC_COLORS;
        nMax = NUM_CUSTOM_COLORS;
      }
      nOrg = n;

      switch (wParam)
      {
        case VK_HOME:
          n = 0;
          break;
        case VK_END:
          n = nMax - 1;
          break;
        case VK_PRIOR:
        case VK_UP:
        case 'k':
          if (n - BOX1_COLS >= 0)
            n -= BOX1_COLS;
          break;
        case VK_NEXT:
        case VK_DOWN:
        case 'j':
          if (n + BOX1_COLS < nMax)
            n += BOX1_COLS;
          break;
        case VK_LEFT:
        case 'h':
          n = max(0, n - 1);
          break;
        case VK_RIGHT:
        case 'l':
          n = min(nMax - 1, n + 1);
          break;
        case VK_ESCAPE:
          PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
          break;
        case VK_RETURN:
          PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
          break;
        case VK_TAB:
          PostMessage(hDlg, WM_NEXTDLGCTL, 0, 0L);
          break;
        default:
          break;
      }
      if (n != nOrg)
      {
        color[0] = GetRValue(lpColors[n]);
        color[1] = GetGValue(lpColors[n]);
        color[2] = GetBValue(lpColors[n]);
        if ( lpColors == lpcc->lpCustColors )
          n += NUM_BASIC_COLORS;
        sColor = n;
        SendMessage(hDlg, WM_CCUPDATE, 0, 0L);
      }
      }
      return TRUE;

    default:
      break;

    }
  return ( (LONG) CallWindowProc(lpfnOldStatic, hWnd, uMsg, wParam, lParam));
}

/* --------------------------------------------------------------------------
//
// Function: PaintColorBox(HWND, LPDWORD)
//
// Purpose:  Update color box with choice of colors.
//
// ------------------------------------------------------------------------*/
static
VOID WINAPI PaintColorBox(HWND hWnd, LPDWORD lpColors)
{
  HDC hdc;
  HBRUSH hbr, hbrOld;
  HPEN hpenOld;
  PAINTSTRUCT ps;
  RECT rc, rb;
  WORD i, j, n;

  InvalidateRect(hWnd, NULL, FALSE);
  hdc = BeginPaint(hWnd, &ps);

  /* Clear the color box */
  GetClientRect(hWnd, &rc);
  hpenOld = SelectObject(hdc, GetStockObject(WHITE_PEN));
  hbrOld = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
  SelectObject(hdc, hbrOld);
  SelectObject(hdc, GetStockObject(BLACK_PEN));

  if (lpColors == colorBtn)
    n = NUM_BASIC_COLORS;
  else
    n = NUM_CUSTOM_COLORS;
  j = MIXED_COLOR;

  /* Paint colored rectangles */
  for ( i = 0; i < n; i++)
  {
    rb.left = rc.left + (i % BOX1_COLS) * boxw + (BOX1_SPAC / 2);
    rb.top = rc.top + (i / BOX1_COLS) * boxh + (BOX1_SPAC / 2);
    rb.right = rb.left + boxw - BOX1_SPAC;
    rb.bottom = rb.top + boxh - BOX1_SPAC;

    hbr = CreateSolidBrush(lpColors[i]);
    hbrOld = SelectObject(hdc, hbr);
    Rectangle(hdc, rb.left, rb.top, rb.right, rb.bottom);
    if (j == MIXED_COLOR
        && lpColors[i] == RGB(color[0], color[1], color[2]))
      if (lpColors == colorBtn)
        j = i;
      else
        j = i + NUM_BASIC_COLORS;
    SelectObject(hdc, hbrOld);
    DeleteObject(hbr);
  }
  if (j != MIXED_COLOR)
    sColor = j;
  SelectObject(hdc, hpenOld);
  EndPaint(hWnd, &ps);
}

/* --------------------------------------------------------------------------
//
// Function: PaintSelectedBox(HWND, WORD, BOOL)
//
// Purpose:  Draws or undraws a focus rectangle around the selected box.
//
// ------------------------------------------------------------------------*/
static
VOID WINAPI PaintSelectedBox(HWND hDlg, WORD i, BOOL b)
{
  static WORD lastColor = MIXED_COLOR;
  HWND hWnd;
  HDC hDC;
  PAINTSTRUCT ps;
  RECT rc, rb;

  if ( i >= NUM_BASIC_COLORS + NUM_CUSTOM_COLORS )
    return;

  /* If a box is already painted, erase it */
  if (b && lastColor < MIXED_COLOR)
    if ( i == lastColor )
      return;
    else
      PaintSelectedBox(hDlg, lastColor, FALSE);
  /* If erasing a box other than the current one, erase previous one */
  if (!b && lastColor != i)
  {
    PaintSelectedBox(hDlg, lastColor, FALSE);
    return;
  }
  lastColor = b ? i : MIXED_COLOR;

  if ( i < NUM_BASIC_COLORS)
    hWnd = GetDlgItem(hDlg, COLOR_BOX1);
  else
  {
    hWnd = GetDlgItem(hDlg, COLOR_CUSTOM1);
    i -= NUM_BASIC_COLORS;
  }

  InvalidateRect(hWnd, NULL, FALSE);
  hDC = BeginPaint(hWnd, &ps);
  GetClientRect(hWnd, &rc);

  rb.left = rc.left + (i % BOX1_COLS) * boxw;
  rb.top = rc.top + (i / BOX1_COLS) * boxh;
  rb.right = rb.left + boxw;
  rb.bottom = rb.top + boxh;

  DrawFocusRect(hDC, &rb);

  EndPaint(hWnd, &ps);
}

/* --------------------------------------------------------------------------
//
// Function: SetOptions(BOOL, HWND, COLORREF)
//
// Purpose:  Sets dialog options area and paints selected color on a window.
//
// ------------------------------------------------------------------------*/
static
VOID WINAPI SetOptions(BOOL bOpt, HWND hDlg, COLORREF col)
{
  char szOptText[32];

  /* Depending on bOpt, show or hide options dialog area */
  ShowOptions(bOpt, hDlg, GetDlgItem(hDlg, COLOR_DEFAULTBOX));

  /* Update options button text */
  LoadString(lpcc->hInstance, bOpt ? IDS_BASICCOL : IDS_CUSTOMCOL,
             szOptText, sizeof(szOptText)-1);
  SetWindowText(GetDlgItem(hDlg, COLOR_MIX), szOptText);

  /* Paint current color if options area is exposed */
  if (bOpt)
    PaintCurrentColor(GetDlgItem(hDlg, COLOR_CURRENT), col);
}

/* --------------------------------------------------------------------------
//
// Function: PaintCurrentColor(HWND, COLORREF)
//
// Purpose:  Update color box with selected color.
//
// ------------------------------------------------------------------------*/
static
VOID WINAPI PaintCurrentColor(HWND hWnd, COLORREF col)
{
  HBRUSH hbr, hbrOld;
  HDC hdc;
  RECT rc;

  hdc = GetDC(hWnd);
  GetClientRect(hWnd, &rc);
  hbr = CreateSolidBrush(col);
  hbrOld = SelectObject(hdc, hbr);
  Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
  SelectObject(hdc, hbrOld);
  DeleteObject(hbr);
  ReleaseDC(hWnd, hdc);
}

/* --------------------------------------------------------------------------
//
// Function: ShowOptions(BOOL, HWND, HWND)
//
// Purpose:  Turns dialog options area on and off.
//
// ------------------------------------------------------------------------*/
static
VOID WINAPI ShowOptions(BOOL bShowOpt, HWND hDlg, HWND hWndDef)
{
  RECT rcDlg, rcDefArea;
  char szDlgDims[25];
  HWND hWndChild;
  RECT rc;

  /* Save original width and height of dialog box. */
  GetWindowRect(hDlg, &rcDlg);

  /* Retrieve coordinates for default area window. */
  GetWindowRect(hWndDef, &rcDefArea);

  /*
     Cycle through all dialog child windows.
  */
  hWndChild = GetTopWindow(hDlg);

  for (; hWndChild != NULL;
    hWndChild = GetWindow(hWndChild, GW_HWNDNEXT)) {

    /* Calculate child window rectangle in screen coordinates. */
    GetWindowRect(hWndChild, &rc);

    /*
       Enable/Disable child if its
         right  edge is >= the right  edge of hWndDef
         bottom edge is >= the bottom edge of hWndDef
    */
    if ((rc.right  >= rcDefArea.right) ||
        (rc.bottom >= rcDefArea.bottom))
      EnableWindow(hWndChild, bShowOpt);
  }

  if (!bShowOpt) {

    SetWindowLong(hWndDef, GWL_STYLE,
                  (~SS_BLACKRECT & GetWindowLong(hWndDef, GWL_STYLE)));
    SetWindowLong(hWndDef, GWL_STYLE,
                  (SS_LEFT | GetWindowLong(hWndDef, GWL_STYLE)));

    /* Save dialog dimensions on default box */
    wsprintf(szDlgDims, "%05u %05u",
      rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top);
    SetWindowText(hWndDef, szDlgDims);

    /* Resize dialog box to fit only default area. */
    SetWindowPos(hDlg, NULL, 0, 0, 
       rcDefArea.right - rcDlg.left + 2 + GetSystemMetrics(SM_CXDLGFRAME),
       rcDefArea.bottom - rcDlg.top + 2 + GetSystemMetrics(SM_CYDLGFRAME),
       SWP_NOZORDER | SWP_NOMOVE);

    /* Make sure that the Default area box is hidden. */
    ShowWindow(hWndDef, SW_HIDE);
  }
  else {
    /* Retrieve dialog dimensions from default box */
    GetWindowText(hWndDef, szDlgDims, sizeof(szDlgDims));

    /* Restore dialog box to its original size. */
    SetWindowPos(hDlg, NULL, 0, 0, 
       atoi(szDlgDims), atoi(szDlgDims + 6), SWP_NOZORDER | SWP_NOMOVE);
  }
}

/* --------------------------------------------------------------------------
//
// Function: VOID RGBtoHLS(DWORD lRGBColor)
//
// Purpose:  Convert RGB value to HLS; taken from MSCDN
//
// Color Conversion Routines --
// RGBtoHLS() takes a DWORD RGB value, translates it to HLS, and
// stores the results in the global vars H, L, and S. HLStoRGB takes the
// current values of H, L, and S and returns the equivalent value in an
// RGB DWORD. The vars H, L, and S are only written to by:
//    1. RGBtoHLS (initialization)
//    2. The scroll bar handlers
// A point of reference for the algorithms is Foley and Van Dam,
// "Fundamentals of Interactive Computer Graphics," Pages 618-19. Their
// algorithm is in floating point.
// There are potential round-off errors throughout this sample.
// ((0.5 + x)/y) without floating point is phrased ((x + (y/2))/y),
// yielding a very small round-off error. This makes many of the
// following divisions look strange.
// ------------------------------------------------------------------------*/
#define  HLSMAX   240   /* H,L, and S vary over 0-HLSMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
                        /* HLSMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HLSMAX must each fit in a byte. */
/* Hue is undefined if Saturation is 0 (grey-scale) */
/* This value determines where the Hue scrollbar is */
/* initially set for achromatic colors */
#define UNDEFINED (HLSMAX*2/3)

static
VOID RGBtoHLS(DWORD lRGBColor)
{
   WORD R,G,B;          /* input RGB values */
   BYTE cMax,cMin;      /* max and min RGB values */
   WORD  Rdelta,Gdelta,Bdelta; /* intermediate value: % of spread from max */
   /* get R, G, and B out of DWORD */
   R = GetRValue(lRGBColor);
   G = GetGValue(lRGBColor);
   B = GetBValue(lRGBColor);
   /* calculate lightness */
   cMax = (BYTE) max( max(R,G), B);
   cMin = (BYTE) min( min(R,G), B);
   L = min((((cMax+cMin)*HLSMAX) + RGBMAX)/(2*RGBMAX), HLSMAX);
   if (cMax == cMin) {           /* r=g=b --> achromatic case */
      S = 0;                     /* saturation */
      H = UNDEFINED;             /* hue */
   }
   else {                        /* chromatic case */
      /* saturation */
      if (L <= (HLSMAX/2))
         S = ( ((cMax-cMin)*HLSMAX) + ((cMax+cMin)/2) ) / (cMax+cMin);
      else
         S = ( ((cMax-cMin)*HLSMAX) + ((2*RGBMAX-cMax-cMin)/2) )
            / (2*RGBMAX-cMax-cMin);
      /* hue */
      Rdelta = ( ((cMax-R)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
      Gdelta = ( ((cMax-G)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
      Bdelta = ( ((cMax-B)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
      if (R == cMax)
         H = Bdelta - Gdelta;
      else if (G == cMax)
         H = (HLSMAX/3) + Rdelta - Bdelta;
      else /* B == cMax */
         H = ((2*HLSMAX)/3) + Gdelta - Rdelta;
      if (H < 0)
         H += HLSMAX;
      if (H > HLSMAX)
         H -= HLSMAX;
   }
}

/* --------------------------------------------------------------------------
//
// Function: WORD HueToRGB(WORD n1, WORD n2, WORD hue)
//
// Purpose:  Get RGV value from Hue; taken from MSCDN
//
// ------------------------------------------------------------------------*/
static
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{
   /* range check: note values passed add/subtract thirds of range */
   if (hue < 0)
      hue += HLSMAX;
   if (hue > HLSMAX)
      hue -= HLSMAX;
   /* return r,g, or b value from this tridrant */
   if (hue < (HLSMAX/6))
      return ( n1 + (((n2-n1)*hue+(HLSMAX/12))/(HLSMAX/6)) );
   if (hue < (HLSMAX/2))
      return ( n2 );
   if (hue < ((HLSMAX*2)/3))
      return ( n1 + (((n2-n1)*(((HLSMAX*2)/3)-hue)+(HLSMAX/12))/(HLSMAX/6))
);
   else
      return ( n1 );
}

/* --------------------------------------------------------------------------
//
// Function: DWORD HLStoRGB(WORD hue, WORD lum, WORD sat)
//
// Purpose:  Convert HLS value to RGB; taken from MSCDN
//
// ------------------------------------------------------------------------*/
static
DWORD HLStoRGB(WORD hue, WORD lum, WORD sat)
{
   WORD R,G,B;                /* RGB component values */
   WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */
   if (sat == 0) {            /* achromatic case */
      R=G=B=(lum*RGBMAX)/HLSMAX;
      if (hue != UNDEFINED) {
         /* ERROR */
      }
   }
   else  {                    /* chromatic case */
      /* set up magic numbers */
      if (lum <= (HLSMAX/2))
         Magic2 = (lum*(HLSMAX + sat) + (HLSMAX/2))/HLSMAX;
      else
         Magic2 = lum + sat - ((lum*sat) + (HLSMAX/2))/HLSMAX;
      Magic1 = 2*lum-Magic2;
      /* get RGB, change units from HLSMAX to RGBMAX */
      R = (HueToRGB(Magic1,Magic2,hue+(HLSMAX/3))*RGBMAX + (HLSMAX/2))/HLSMAX;
      G = (HueToRGB(Magic1,Magic2,hue)*RGBMAX + (HLSMAX/2)) / HLSMAX;
      B = (HueToRGB(Magic1,Magic2,hue-(HLSMAX/3))*RGBMAX + (HLSMAX/2))/HLSMAX;
   }
   return(RGB(R,G,B));
}
