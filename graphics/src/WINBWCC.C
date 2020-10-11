/*===========================================================================*/
/*                                                                           */
/* File    : WINBWCC.C                                                       */
/*                                                                           */
/* Purpose : Provides some BWCC control emulation for MEWEL/GUI.             */
/*                                                                           */
/* History : 4/93 (maa) - Created                                            */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include <windows.h>
#include <bwcc.h>

static LRESULT CALLBACK BwccDlgWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK BwccStaticWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK BwccShadeWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK BwccBtnWndProc(HWND, UINT, WPARAM, LPARAM);

static VOID PASCAL DeleteButtonBitmaps(HWND);


/*
  Brush used for the background of BWCC dialog boxes
*/
static HBRUSH hBrDialog = NULL;


DWORD FAR PASCAL BWCCGetVersion(void)
{
  EXTWNDCLASS ewc;
  extern INT MewelCurrOpenResourceFile;  /* file descriptor */

  memset((char *) &ewc, 0, sizeof(ewc));

  ewc.hInstance     = (HANDLE)  MewelCurrOpenResourceFile;
  ewc.lpszClassName = (LPSTR)   "BorDlg";
  ewc.lpszBaseClass = (LPSTR)   "Dialog";
  ewc.lpfnWndProc   = (WNDPROC) BwccDlgWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;
  hBrDialog = ewc.hbrBackground;

  ewc.lpszClassName = (LPSTR)   "BorShade";
  ewc.lpszBaseClass = (LPSTR)   "Static";
  ewc.lpfnWndProc   = (WNDPROC) BwccShadeWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;

  ewc.lpszClassName = (LPSTR)   "BorStatic";
  ewc.lpszBaseClass = (LPSTR)   "Static";
  ewc.lpfnWndProc   = (WNDPROC) BwccStaticWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;

  ewc.lpszClassName = (LPSTR)   "BorBtn";
  ewc.lpszBaseClass = (LPSTR)   "Pushbutton";
  ewc.lpfnWndProc   = (WNDPROC) BwccBtnWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  ewc.cbWndExtra    = sizeof(LONG) * 3;
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;

  ewc.lpszClassName = (LPSTR)   "BorRadio";
  ewc.lpszBaseClass = (LPSTR)   "RadioButton";
  ewc.lpfnWndProc   = (WNDPROC) BwccBtnWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;

  ewc.lpszClassName = (LPSTR)   "BorCheck";
  ewc.lpszBaseClass = (LPSTR)   "Checkbox";
  ewc.lpfnWndProc   = (WNDPROC) BwccBtnWndProc;
  ewc.hbrBackground = (HBRUSH)  GetStockObject(LTGRAY_BRUSH);
  if (!ExtRegisterClass((LPEXTWNDCLASS) &ewc))
    return FALSE;

  return BWCCVERSION;
}

static LRESULT CALLBACK BwccDlgWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  switch (message)
  {
    default :
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

static LRESULT CALLBACK BwccShadeWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  switch (message)
  {
    case WM_CREATE :
      SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_GROUP);
      return TRUE;

    case WM_PAINT :
    {
      PAINTSTRUCT ps;
      RECT        r, r2;
      DWORD       ulStyle;

      BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, (LPRECT) &r);

      ulStyle = (GetWindowLong(hWnd, GWL_STYLE) & 0x07L);
      switch (ulStyle)
      {
        case BSS_GROUP   :
        case BS_GROUPBOX :
          SetRect(&r2, 0, 0, r.right, 1);
          FillRect(ps.hdc, &r2, GetStockObject(DKGRAY_BRUSH));
          SetRect(&r2, 0, 0, 1, r.bottom);
          FillRect(ps.hdc, &r2, GetStockObject(DKGRAY_BRUSH));
          SetRect(&r2, r.right-1, 0, r.right, r.bottom);
          FillRect(ps.hdc, &r2, GetStockObject(WHITE_BRUSH));
          SetRect(&r2, 0, r.bottom-1, r.right, r.bottom);
          FillRect(ps.hdc, &r2, GetStockObject(WHITE_BRUSH));
          InflateRect(&r, -1, -1);
          FillRect(ps.hdc, &r, GetStockObject(LTGRAY_BRUSH));
          break;

        case BSS_HDIP  :
        case BSS_HBUMP :
          SetRect(&r2, 0, 0, r.right, 2);
          FillRect(ps.hdc, &r2, GetStockObject(WHITE_BRUSH));
          if (ulStyle == BSS_HDIP)
            SetRect(&r2, 1, 1, r.right-1, 2);
          else
            SetRect(&r2, 1, 0, r.right-1, 1);
          FillRect(ps.hdc, &r2, GetStockObject(DKGRAY_BRUSH));
          break;

        case BSS_VDIP  :
        case BSS_VBUMP :
          SetRect(&r2, 0, 0, 2, r.bottom);
          FillRect(ps.hdc, &r2, GetStockObject(WHITE_BRUSH));
          if (ulStyle == BSS_HDIP)
            SetRect(&r2, 0, 1, 1, r.bottom-1);
          else
            SetRect(&r2, 1, 1, 2, r.bottom-1);
          FillRect(ps.hdc, &r2, GetStockObject(DKGRAY_BRUSH));
          break;
      }

      EndPaint(hWnd, &ps);
      return TRUE;
    }

    default :
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

static LRESULT CALLBACK BwccStaticWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  switch (message)
  {
    default :
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
}


static LRESULT CALLBACK BwccBtnWndProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  DWORD  ulStyle;
  WORD   ulStyle2;
  BOOL   bPushBut;


  /*
    See if we have a pushbutton
  */
  ulStyle  = GetWindowLong(hWnd, GWL_STYLE);
  ulStyle2 = (WORD) (ulStyle & 0x0FL);
  bPushBut = (ulStyle2 == BS_PUSHBUTTON || ulStyle2 == BS_DEFPUSHBUTTON);


  switch (message)
  {
    case WM_CREATE     :
      if (bPushBut)
      {
        WORD   idCtrl = GetWindowWord(hWnd, GWW_ID);
        HANDLE hInst  = GetWindowWord(hWnd, GWW_HINSTANCE);
        if (hInst == (HANDLE) -1)
          hInst = 0;
        SetWindowLong(hWnd, 0, LoadBitmap(hInst, MAKEINTRESOURCE(idCtrl+1000)));
        if (!(ulStyle & BBS_BITMAP))
        {
          SetWindowLong(hWnd, 4, LoadBitmap(hInst, MAKEINTRESOURCE(idCtrl+3000)));
          SetWindowLong(hWnd, 8, LoadBitmap(hInst, MAKEINTRESOURCE(idCtrl+5000)));
        }
      }
      break;

    case WM_NCPAINT    :
    case WM_ERASEBKGND :
      if (bPushBut)
        return TRUE;
      else
        break;


    case WM_PAINT :
      if (bPushBut)
      {
        PAINTSTRUCT ps;
        BITMAP  bmp;
        HBITMAP hBitmap, hOldBitmap;
        HDC     hDC, hMemDC;
        UINT    index;
        UINT    fState;

        /*
          Is the button selected, in focus, or normal?
        */
        if (ulStyle & BBS_BITMAP)
          index = 0;
        else if ((fState = SendMessage(hWnd, BM_GETSTATE, 0, 0L)) & 0x04)
          index = 1;    /* highlighted */
        else if (fState & 0x08)
          index = 2;    /* in focus */
        else
          index = 0;

        /*
          Get the appropriate bitmap to draw
        */
        hBitmap = GetWindowLong(hWnd, sizeof(LONG) * index);

        /*
          Draw the bitmap
        */
        if (hBitmap)
        {
          hDC = BeginPaint(hWnd, &ps);
          if ((hMemDC = CreateCompatibleDC(hDC)) != NULL)
          {
            GetObject(hBitmap, sizeof(bmp), (LPSTR) &bmp);
            hOldBitmap = SelectObject(hMemDC, hBitmap);
            BitBlt(hDC, 0, 0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0, 0, SRCCOPY);
            SelectObject(hMemDC, hOldBitmap);
            DeleteDC(hMemDC);
          }

          /*
            If the button is the default pushbutton, draw a black
            rectangle around the button.
          */
          if (ulStyle2 == BS_DEFPUSHBUTTON)
          {
            RECT r;
            SetRect(&r, 0, 0, bmp.bmWidth, bmp.bmHeight);
            InflateRect(&r, -1, -1);
            SelectObject(hDC, GetStockObject(NULL_BRUSH));
            Rectangle(hDC, 0, 0, r.right, r.bottom);
          }
  
          EndPaint(hWnd, &ps);
          return TRUE;
        }
      }
      break;


    case WM_NCDESTROY :
      if (bPushBut)
        DeleteButtonBitmaps(hWnd);
      break;


      /*
        BBM_SETBITS
          wParam = not used
          lParam = far pointer to 3 bitmaps to use the draw the button
      */
      case BBM_SETBITS :
      {
        HBITMAP FAR *lpBitmaps = (HBITMAP FAR *) lParam;

        /*
          First, get rid of the old bitmaps
        */
        DeleteButtonBitmaps(hWnd);

        /*
          Set the new bitmap values
        */
        SetWindowLong(hWnd, 0, *lpBitmaps++);
        SetWindowLong(hWnd, 4, *lpBitmaps++);
        SetWindowLong(hWnd, 8, *lpBitmaps++);
        return TRUE;
      }
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}


static VOID PASCAL DeleteButtonBitmaps(hWnd)
  HWND hWnd;
{
  HBITMAP hBitmap;
  int     i;

  for (i = 0;  i < 3 * sizeof(LONG);  i += sizeof(LONG))
    if ((hBitmap = GetWindowLong(hWnd, i)) != NULL)
      DeleteObject(hBitmap);
}


/****************************************************************************/
/*                                                                          */
/* Function : BWCCMessageBox()                                              */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : Whatever MessageBox() returns.                                */
/*                                                                          */
/****************************************************************************/
int FAR EXPORT PASCAL BWCCMessageBox(hWndParent, lpText, lpCaption, wType)
  HWND   hWndParent;
  LPCSTR lpText;
  LPCSTR lpCaption;
  UINT   wType;
{
  return MessageBox(hWndParent, lpText, lpCaption, wType);
}


/****************************************************************************/
/*                                                                          */
/* Function : BWCCGetPattern()                                              */
/*                                                                          */
/* Purpose  : Returns a handle to the dialog background brush.              */
/*                                                                          */
/* Returns  : A brush.                                                      */
/*                                                                          */
/****************************************************************************/
HBRUSH FAR EXPORT PASCAL BWCCGetPattern(void)
{
  return hBrDialog;
}

