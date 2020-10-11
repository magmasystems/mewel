/*===========================================================================*/
/*                                                                           */
/* File    : WCHOFONT.C                                                      */
/*                                                                           */
/* Purpose : Implements the Choose Font common dialog box.                   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "commdlg.h"

/*
  Control IDs
*/
#define ID_FONTCOMBO    100
#define ID_STYLECOMBO   101
#define ID_SIZECOMBO    102
#define ID_APPLY        103
#define ID_EFFECTS      104
#define ID_STRIKEOUT    105
#define ID_UNDERLINE    106
#define ID_COLORTEXT    107
#define ID_COLORCOMBO   108
#define ID_AABBYYZZ     109
#define ID_INFO         110

#define WM_DISPLAYFONTBOX   (WM_USER + 478)


typedef struct tagInternalOFN
{
  CHOOSEFONT   cf;
  UINT         fIntFlags;
#define CF_CREATEDDC  0x0001
  HHOOK        hHook;
  LOGFONT      lf;     /* used to build the font */
  COLORREF     rgb;    /* Current color */
  INT          iStyle; /* index of current style */
  HDC          hDCStatic;  /* DC of the sample string static control */
} INTERNALCHOOSEFONT, *LPINTERNALCHOOSEFONT;


static LPINTERNALCHOOSEFONT  lpInternalCF = NULL;

static INT                   iCommDlgError;
#define CDERR_INITIALIZATION   1
#define CDERR_NOTEMPLATE       2
#define CDERR_LOADRESFAILURE   3


/*
  Color strings and RGB values for the color combo
*/
typedef struct tagColorInfo
{
  LPSTR    lpszColor;
  COLORREF rgb;
} COLORINFO, *PCOLORINFO;

static COLORINFO ColorInfo[] =
{
  "Black",      RGB(0x00, 0x00, 0x00),
  "Blue",       RGB(0x00, 0x00, 0x80),
  "Green",      RGB(0x00, 0x80, 0x00),
  "Cyan",       RGB(0x00, 0x80, 0x80),
  "Red",        RGB(0x80, 0x00, 0x00),
  "Magenta",    RGB(0x80, 0x00, 0x80),
  "Yellow",     RGB(0x80, 0x80, 0x00),
  "Lt Gray",    RGB(0x80, 0x80, 0x80),
  "Dk Gray",    RGB(0x20, 0x20, 0x20),
  "HiBlue",     RGB(0x00, 0x00, 0xFF),
  "HiGreen",    RGB(0x00, 0xFF, 0x00),
  "HiCyan",     RGB(0x00, 0xFF, 0xFF),
  "HiRed",      RGB(0xFF, 0x00, 0x00),
  "HiMagenta",  RGB(0xFF, 0x00, 0xFF),
  "HiYellow",   RGB(0xFF, 0xFF, 0x00),
  "White",      RGB(0xFF, 0xFF, 0xFF),
};

typedef struct tagStyleInfo
{
  LPSTR lpszStyle; /* style name */
  int   iWeight;   /* gets put into lf.lfWeight field */
  int   bItalic;   /* gets put into lf.lfItalic field */
} STYLEINFO, *PSTYLEINFO;

static STYLEINFO StyleInfo[] =
{
  { "Regular",       FW_NORMAL,  FALSE },
  { "Bold",          FW_BOLD,    FALSE },
  { "Bold Italic",   FW_BOLD,    TRUE  },
  { "Italic",        FW_NORMAL,  TRUE  },
};


/*
  Sentinel for the Motif version of EnumFonts
*/
BOOL bInChooseFontDialog = FALSE;


/*
  Prototypes
*/
static INT  FAR PASCAL ChooseFontDlgProc(HWND, UINT, WPARAM, LPARAM);
static VOID     PASCAL DisplaySampleString(HWND, HDC, COLORREF, LPLOGFONT);
static VOID     PASCAL RefreshSizeCombo(HWND, CHOOSEFONT FAR *);
static INT      PASCAL GetCurrentSize(HWND);
static BOOL     PASCAL GetFace(HWND, LPINTERNALCHOOSEFONT);
static INT      PASCAL GetCurrentStyle(HWND, LPSTR);
static INT  CALLBACK EnumFaces(LPLOGFONT, LPTEXTMETRICS, int, LPARAM);
static INT  CALLBACK EnumSizes(LPLOGFONT, LPTEXTMETRICS, int, LPARAM);
extern VOID     PASCAL SetCommDlgError(INT);



BOOL WINAPI ChooseFont(lpCF)
  CHOOSEFONT FAR *lpCF;
{
  HWND hDlg = NULLHWND;
  INT  rc;
  FARPROC lpfnDlgProc;
  LPINTERNALCHOOSEFONT lpSaveInternalCF;
  INTERNALCHOOSEFONT   myInternalCF;


  (void) hDlg;

  if (lpCF == NULL)
  {
    SetCommDlgError(CDERR_INITIALIZATION);
    return FALSE;
  }

  bInChooseFontDialog++;
  lpSaveInternalCF = lpInternalCF;
  lpInternalCF = &myInternalCF;

  /*
    Set the MEWEL instance to the fd of the open resource file
  */
  if (lpCF->hInstance <= 0)
    lpCF->hInstance = MewelCurrOpenResourceFile;

  /*
    Save the CHOOSEFONT values which the app passed down
  */
  lmemset((LPSTR) lpInternalCF, 0, sizeof(INTERNALCHOOSEFONT));
  lmemcpy((LPSTR) &lpInternalCF->cf, (LPSTR) lpCF, sizeof(CHOOSEFONT));

  lpfnDlgProc = MakeProcInstance(ChooseFontDlgProc, lpCF->hInstance);

  /*
    Should we invoke the dialog box from a template or from the RC file?
  */
  if (lpCF->Flags & CF_ENABLETEMPLATE)
  {
    if (!lpCF->lpTemplateName)
    {
      SetCommDlgError(CDERR_NOTEMPLATE);
      lpInternalCF = lpSaveInternalCF;
      return FALSE;
    }

    rc = DialogBoxParam(lpCF->hInstance, (LPSTR) lpCF->lpTemplateName,
                     lpCF->hwndOwner, lpfnDlgProc, (LONG) lpCF);
  }
  else
  {
    rc = DialogBoxParam(lpCF->hInstance, (LPSTR) "ChooseFontDlg",
                     lpCF->hwndOwner, lpfnDlgProc, (LONG) lpCF);
  }

  /*
    Copy the changed values back to the user if we succeed
  */
  if (rc)
    lmemcpy((LPSTR) lpCF, (LPSTR) &lpInternalCF->cf, sizeof(CHOOSEFONT));

  lpInternalCF = lpSaveInternalCF;
  bInChooseFontDialog--;
  return (BOOL) rc;
}


/****************************************************************************/
/*                                                                          */
/* Function : ChooseFontDlgProc()                                           */
/*                                                                          */
/* Purpose  : Dialog box proc for the Choose Font dialog.                   */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
static INT FAR PASCAL ChooseFontDlgProc(HWND hDlg, UINT message, 
                                        WPARAM wParam, LPARAM lParam)
{
  LPINTERNALCHOOSEFONT lpIntCF;
  CHOOSEFONT FAR *lpCF;
  LPOBJECT lpObj;
  LPHDC    lphDC;
  DWORD dwFlags;
  HDC   hDC;
  int   i;
  HCURSOR hCursor;


  lpIntCF = lpInternalCF;
  lpCF = &lpIntCF->cf;


  switch (message)
  {
    case WM_INITDIALOG :
#if 0
      GetIntlCommFileStrings();
#endif


      dwFlags = lpCF->Flags;
      hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

      SendDlgItemMessage(hDlg, ID_FONTCOMBO,  WM_SETREDRAW, FALSE, 0L);
      SendDlgItemMessage(hDlg, ID_STYLECOMBO, WM_SETREDRAW, FALSE, 0L);
      SendDlgItemMessage(hDlg, ID_SIZECOMBO,  WM_SETREDRAW, FALSE, 0L);
      SendDlgItemMessage(hDlg, ID_COLORCOMBO, WM_SETREDRAW, FALSE, 0L);


      /*
        Set up the font combo by enumerating all of the installed fonts
      */
      if ((hDC = lpCF->hDC) == NULL)
      {
        lpCF->hDC = hDC = GetDC(NULL);
        lpIntCF->fIntFlags |= CF_CREATEDDC;
      }
      lphDC = _GetDC(hDC);

      lpIntCF->hDCStatic = 0;

      /*
        Get some sensible values into the LOGFONT structure
      */
      lpObj = _ObjectDeref(lphDC->hFont);
      lmemcpy((LPSTR) &lpIntCF->lf, (LPSTR) &lpObj->uObject.uLogFont, sizeof(LOGFONT));

      EnumFonts(hDC, (LPCSTR) NULL, (OLDFONTENUMPROC) EnumFaces,
                (LPSTR) (LONG) hDlg);

      /*
        Select the initial font
      */
      if (dwFlags & CF_INITTOLOGFONTSTRUCT)
      {
        i = SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_FINDSTRING, (WPARAM) -1, 
                               (LONG) (LPSTR) lpCF->lpLogFont->lfFaceName);
        if (i == CB_ERR)
          i = 0;
      }
      else
        i = 0;
      SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_SETCURSEL, i, 0L);

      /*
        Set up the style combo
      */
      for (i = 0;  i < sizeof(StyleInfo)/sizeof(STYLEINFO);  i++)
        SendDlgItemMessage(hDlg, ID_STYLECOMBO, CB_ADDSTRING, 0,
                             (LONG) StyleInfo[i].lpszStyle);

      if (dwFlags & CF_USESTYLE)
      {
        for (i = 0;  i < sizeof(StyleInfo)/sizeof(STYLEINFO);  i++)
          if (!lstricmp(lpCF->lpszStyle, StyleInfo[i].lpszStyle))
            break;

        if (i >= sizeof(StyleInfo)/sizeof(STYLEINFO))  /* no match */
          i = 0;
      }
      else
        i = 0;
      SendDlgItemMessage(hDlg, ID_STYLECOMBO, CB_SETCURSEL, i, 0L);

      /*
        Do the size combo
      */
      GetFace(hDlg, lpIntCF);
      RefreshSizeCombo(hDlg, lpCF);


      /*
        Set up the hook stuff. lParam of the WM_INITDIALOG message contains
        the custom hook data.
      */
      if ((dwFlags & CF_ENABLEHOOK) && lpCF->lpfnHook)
        lpIntCF->hHook = SetWindowsHookEx(WH_COMMDLG,(FARPROC)lpCF->lpfnHook,0,0);
      else
        lpIntCF->hHook = NULL;

      /*
        Do some flags stuff
      */
      if (!(dwFlags & CF_SHOWHELP))
        ShowWindow(GetDlgItem(hDlg, IDHELP), SW_HIDE);

      /*
        Hide the effects controls if CF_EFFECTS is not set
      */
      if (!(dwFlags & CF_EFFECTS))
      {
        for (i = ID_EFFECTS;  i <= ID_COLORCOMBO;  i++)
          ShowWindow(GetDlgItem(hDlg, i), SW_HIDE);
      }
      else
      {
        int iStartSel = 0;

        /*
          Insert the strings and the RGB values into the color combo
        */
        for (i = 0;  i < sizeof(ColorInfo)/sizeof(COLORINFO);  i++)
        {
          SendDlgItemMessage(hDlg, ID_COLORCOMBO, CB_ADDSTRING, 0,
                             (LONG) ColorInfo[i].lpszColor);
          SendDlgItemMessage(hDlg, ID_COLORCOMBO, CB_SETITEMDATA, i,
                             (LONG) ColorInfo[i].rgb);
          if (ColorInfo[i].rgb == lpCF->rgbColors)
            iStartSel = i;
        }
        /*
          Set the initial selection
        */
        SendDlgItemMessage(hDlg, ID_COLORCOMBO, CB_SETCURSEL, iStartSel, 0L);
      }

      SetCursor(hCursor);

      SendDlgItemMessage(hDlg, ID_FONTCOMBO,  WM_SETREDRAW, TRUE, 0L);
      SendDlgItemMessage(hDlg, ID_STYLECOMBO, WM_SETREDRAW, TRUE, 0L);
      SendDlgItemMessage(hDlg, ID_SIZECOMBO,  WM_SETREDRAW, TRUE, 0L);
      SendDlgItemMessage(hDlg, ID_COLORCOMBO, WM_SETREDRAW, TRUE, 0L);

      /*
        Send the WM_INITDIALOG through the common dialog hook proc.
      */
      if (InternalSysParams.pHooks[WH_COMMDLG])
        if ((*(COMMDLGHOOKPROC*) InternalSysParams.pHooks[WH_COMMDLG]->lpfnHook)
              (hDlg,message,wParam,lParam))
          return FALSE;

      return TRUE;


    /*
      Owner-drawn combo processing
    */
    case WM_MEASUREITEM :
    {
      MEASUREITEMSTRUCT FAR *lpMI = (MEASUREITEMSTRUCT FAR *) lParam;
      switch (wParam)
      {
        case ID_COLORCOMBO :
          lpMI->itemHeight = 16;
          break;
        case ID_SIZECOMBO  :
          lpMI->itemHeight = 16;
          break;
        case ID_FONTCOMBO  :
          lpMI->itemHeight = 16;
          break;
      }
      return TRUE;
    }

    case WM_DRAWITEM   :
    {
      DRAWITEMSTRUCT FAR *lpDI = (DRAWITEMSTRUCT FAR *) lParam;
      HBRUSH hBrush;
      RECT   r;
      int    iHeight;

      switch (wParam)
      {
        case ID_COLORCOMBO :
          iHeight = HIWORD(GetTextExtent(lpDI->hDC, "Hq", 2));

          r = lpDI->rcItem;

          FillRect(lpDI->hDC, &r, SysBrush[COLOR_WINDOW]);

          r.left  += 4;
          r.right  = (MWCOORD) (r.left + 12);
          r.bottom = (MWCOORD) (r.top + iHeight);

          /*
            Draw the little blotch of color to the left of the color name
          */
          if (ColorInfo[lpDI->itemID].rgb == GetSysColor(COLOR_WINDOW))
          {
            /*
              Since the block of color is the same as the background of
              the combobox, we draw a frame around the block.
            */
            hBrush = SelectObject(lpDI->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(lpDI->hDC, r.left, r.top, r.right, r.bottom);
            SelectObject(lpDI->hDC, hBrush);
          }
          else
          {
            hBrush = CreateSolidBrush(ColorInfo[lpDI->itemID].rgb);
            FillRect(lpDI->hDC, &r, hBrush);
            DeleteObject(hBrush);
          }

          TextOut(lpDI->hDC, lpDI->rcItem.left + 20, lpDI->rcItem.top, 
                  ColorInfo[lpDI->itemID].lpszColor,
                  lstrlen(ColorInfo[lpDI->itemID].lpszColor));

#if !defined(MOTIF)
          if (lpDI->itemState & ODS_SELECTED)
          {
            lpDI->rcItem.left += 20;
            lpDI->rcItem.bottom = lpDI->rcItem.top + iHeight;
            InvertRect(lpDI->hDC, &lpDI->rcItem);
          }
#endif
          break;

        case ID_FONTCOMBO  :
        case ID_SIZECOMBO  :
        {
          char szBuf[64];
          SendDlgItemMessage(hDlg, wParam, CB_GETLBTEXT, lpDI->itemID,
                            (LONG) (LPSTR) szBuf);

          FillRect(lpDI->hDC, &lpDI->rcItem, SysBrush[COLOR_WINDOW]);
          TextOut(lpDI->hDC, lpDI->rcItem.left + 4, lpDI->rcItem.top, 
                  szBuf, lstrlen(szBuf));
          if (lpDI->itemState & (ODS_SELECTED | ODS_FOCUS))
          {
            InvertRect(lpDI->hDC, &lpDI->rcItem);
          }
          break;
        }
      }
      return TRUE;
    }


    case WM_PAINT :
      DefWindowProc(hDlg, WM_PAINT, wParam, lParam);
      PostMessage(hDlg, WM_DISPLAYFONTBOX, 0, 0L);
      return TRUE;

    case WM_DISPLAYFONTBOX :
      if (lpIntCF->hDCStatic == 0)
        lpIntCF->hDCStatic = GetDC(GetDlgItem(hDlg, ID_AABBYYZZ));
      DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
      break;


    case WM_COMMAND    :
      switch (wParam)
      {
        case ID_FONTCOMBO :
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE :
              /*
                Fetch the current entry, and redisplay the sample string
              */
              if (GetFace(hDlg, lpIntCF))
              {
                RefreshSizeCombo(hDlg, lpCF);
                if ((i = GetCurrentSize(hDlg)) > 0)
                {
                  lpIntCF->lf.lfHeight = i;
                  lpIntCF->lf.lfWidth  = (i * 2) / 3;
                }
                DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
              }
              break;

            case LBN_DBLCLK :
              break;
          }
          break;


        case ID_SIZECOMBO :
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE :
              /*
                Fetch the current entry, and redisplay the sample string
              */
              if ((i = GetCurrentSize(hDlg)) > 0)
              {
                lpIntCF->lf.lfHeight = i;
                lpIntCF->lf.lfWidth  = (i * 2) / 3;
                DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
              }
              break;

            case LBN_DBLCLK :
              break;
          }
          break;

        case ID_STYLECOMBO :
          switch (HIWORD(lParam))
          {
            case LBN_SELCHANGE :
              /*
                Fetch the current entry, and redisplay the sample string
              */
              if ((i = GetCurrentStyle(hDlg, NULL)) >= 0)
              {
                lpIntCF->iStyle      = i;
                lpIntCF->lf.lfWeight = StyleInfo[i].iWeight;
                lpIntCF->lf.lfItalic = (BYTE) StyleInfo[i].bItalic;
                DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
              }
              break;

            case LBN_DBLCLK :
              break;
          }
          break;

        case ID_COLORCOMBO:
          /*
            Fetch the current entry, and redisplay the sample string
          */
          i = SendDlgItemMessage(hDlg, ID_COLORCOMBO, CB_GETCURSEL, 0, (LPARAM) 0L);
          if (i != CB_ERR)
          {
            lpIntCF->rgb = SendDlgItemMessage(hDlg, ID_COLORCOMBO, 
                                              CB_GETITEMDATA, i, 0L);
            DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
          }
          break;


        case ID_STRIKEOUT :
        case ID_UNDERLINE :
          /*
            If one of the effects is chosen, then modify the LOGFONT
            structure and redisplay the sample string.
          */
          lpIntCF->lf.lfStrikeOut = (BYTE) IsDlgButtonChecked(hDlg, ID_STRIKEOUT);
          lpIntCF->lf.lfUnderline = (BYTE) IsDlgButtonChecked(hDlg, ID_UNDERLINE);
          DisplaySampleString(hDlg, lpIntCF->hDCStatic, lpIntCF->rgb, &lpIntCF->lf);
          break;


        case IDOK     :
          /*
            Copy all of the values back to the user.
          */
          lpCF->iPointSize = GetCurrentSize(hDlg);
          GetCurrentStyle(hDlg, lpCF->lpszStyle);
          lpCF->rgbColors = lpIntCF->rgb;
          if (lpCF->lpLogFont)
            lmemcpy((LPSTR) lpCF->lpLogFont, &lpIntCF->lf, sizeof(LOGFONT));

          /* fall through to cleanup code... */

        case IDCANCEL :
          if (lpIntCF->hHook)
            UnhookWindowsHookEx(lpIntCF->hHook);
          if (lpIntCF->fIntFlags & CF_CREATEDDC)
          {
            ReleaseDC(NULL, lpCF->hDC);
            lpCF->hDC = NULL;
          }
          ReleaseDC(GetDlgItem(hDlg, ID_AABBYYZZ), lpIntCF->hDCStatic);
          EndDialog(hDlg, (wParam == IDOK));
          break;

        case IDHELP   :
#if 32693
          /*
            Send current window handle along with help message
            defined as not used in windows) so that the WinHelp
            enables/disables the correct window

            From commfile.dlg ...
              #define HELPMSGSTRING  "commdlg_help"
          */
          SendMessage(lpCF->hwndOwner, FindAtom(HELPMSGSTRING), hDlg, 0L);
#else
          MessageBox(hDlg, "Help not implemented yet.", "Save File", MB_OK);
#endif
          break;

        case ID_APPLY :
          break;
      }

      return TRUE;


    case WM_KEYDOWN    :
      if (wParam == VK_F1)
      {
        SendMessage(hDlg, WM_COMMAND, IDHELP, 0L);
        return TRUE;
      }
      break;

  }

  return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : DisplaySampleString()                                         */
/*                                                                          */
/* Purpose  : Displays the AaBbYyZz sample string in the current font.      */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL DisplaySampleString(HWND hDlg, HDC hDC, 
                                       COLORREF rgb, LPLOGFONT lpLF)
{
  RECT   r;
  HWND   hStatic;
  HBRUSH hBrush;
  HFONT  hFont, hOldFont;

  static HFONT  hPrevFont = NULL;


  /*
    Get the handle of the font-sample control
  */
  hStatic = GetDlgItem(hDlg, ID_AABBYYZZ);

  /*
    Erase the background so that all previous garbage is wiped out.
  */
  hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  GetClientRect(hStatic, &r);
  FillRect(hDC, &r, hBrush);
  DeleteObject(hBrush);

  /*
    Create the new font
  */
  hFont = CreateFontIndirect(lpLF);

  /*
    Get rid of the previously-used font
  */
  if (hPrevFont)
    DeleteObject(hPrevFont);
  hPrevFont = hFont;

  /*
    Set the new font
  */
#if !110894
  SendMessage(hStatic, WM_SETFONT, hFont, 0L);
#endif

  /*
    Write out the text in the new font and color
  */
  hOldFont = SelectObject(hDC, hFont);
  SetTextColor(hDC, rgb);
  TextOut(hDC, 0, 0, "AaBbYyZz", 8);
  SelectObject(hDC, hOldFont);
}


/****************************************************************************/
/*                                                                          */
/* Function : GetCurrentSize()                                              */
/*                                                                          */
/* Purpose  : Retrieves the currently selected font size.                   */
/*                                                                          */
/* Returns  : The current point size in th Size combo, or -1.               */
/*                                                                          */
/****************************************************************************/
static INT PASCAL GetCurrentSize(hDlg)
  HWND hDlg;
{
  INT i = SendDlgItemMessage(hDlg, ID_SIZECOMBO, CB_GETCURSEL, 0, (LPARAM) 0L);
  if (i != CB_ERR)
  {
    char szBuf[16];
    SendDlgItemMessage(hDlg, ID_SIZECOMBO, CB_GETLBTEXT, i, (LONG) szBuf);
    return atoi(szBuf);
  }
  else
    return -1;
}

/****************************************************************************/
/*                                                                          */
/* Function : GetFace()                                                     */
/*                                                                          */
/* Purpose  : Gets the currently selected font name, and copies in into     */
/*            the global LOGFONT template we are keeping around.            */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
static BOOL PASCAL GetFace(hDlg, lpIntCF)
  HWND hDlg;
  LPINTERNALCHOOSEFONT lpIntCF;
{
  char szBuf[64];
  int  iSel;

  iSel = SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_GETCURSEL, 0, (LPARAM) 0L);
  if (iSel != CB_ERR)
  {
    SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_GETLBTEXT, iSel, (LONG) szBuf);
    lstrcpy(lpIntCF->lf.lfFaceName, szBuf);
    return TRUE;
  }
  else
    return FALSE;
}

/****************************************************************************/
/*                                                                          */
/* Function : GetFace()                                                     */
/*                                                                          */
/* Purpose  : Gets the currently selected style name, and copies in into    */
/*            the global LOGFONT template we are keeping around.            */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
static INT PASCAL GetCurrentStyle(hDlg, lpBuf)
  HWND  hDlg;
  LPSTR lpBuf;
{
  int  iSel;

  iSel = SendDlgItemMessage(hDlg, ID_STYLECOMBO, CB_GETCURSEL, 0, (LPARAM) 0L);
  if (iSel != CB_ERR && lpBuf != NULL)
  {
    SendDlgItemMessage(hDlg, ID_STYLECOMBO, CB_GETLBTEXT, iSel, (LONG) lpBuf);
  }
  return iSel;
}


/****************************************************************************/
/*                                                                          */
/* Function : EnumFaces()                                                   */
/*                                                                          */
/* Purpose  : Callback function which adds a font facename to the combo.    */
/*                                                                          */
/* Returns  : TRUE.                                                         */
/*                                                                          */
/****************************************************************************/
static INT CALLBACK 
EnumFaces(LPLOGFONT lpLF, LPTEXTMETRICS lpTM, int iFontSpec, LPARAM lParam)
{
  HWND hDlg = LOWORD(lParam);

  (void) lpTM;   (void) iFontSpec;

  if (lstricmp(lpLF->lfFaceName, "DefaultFontFile"))
    SendDlgItemMessage(hDlg,ID_FONTCOMBO,CB_ADDSTRING,0,(LONG)lpLF->lfFaceName);

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : RefreshSizeCombo()                                            */
/*                                                                          */
/* Purpose  : Called when a new font is chosen. It causes the Size combo    */
/*            to be updated.                                                */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL RefreshSizeCombo(hDlg, lpCF)
  HWND hDlg;
  CHOOSEFONT FAR *lpCF;
{
  char szBuf[64];
  int  iSel;

  iSel = SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_GETCURSEL, 0, (LPARAM) 0L);
  if (iSel == CB_ERR)
    return;
  SendDlgItemMessage(hDlg, ID_FONTCOMBO, CB_GETLBTEXT, iSel, (LONG) szBuf);

  /*
    Wipe out the sizes which are already in the listbox
  */
  SendDlgItemMessage(hDlg, ID_SIZECOMBO, WM_SETREDRAW, FALSE, 0L);
  SendDlgItemMessage(hDlg, ID_SIZECOMBO, CB_RESETCONTENT, 0, 0L);

  EnumFonts(lpCF->hDC, (LPCSTR) szBuf, (OLDFONTENUMPROC) EnumSizes,
            (LPSTR) (LONG) hDlg);

  SendDlgItemMessage(hDlg, ID_SIZECOMBO, CB_SETCURSEL, 0, 0L);
  SendDlgItemMessage(hDlg, ID_SIZECOMBO, WM_SETREDRAW, TRUE, 0L);
}


/****************************************************************************/
/*                                                                          */
/* Function : EnumSizes()                                                   */
/*                                                                          */
/* Purpose  : Callback function which adds a point size to the size combo.  */
/*                                                                          */
/* Returns  : TRUE.                                                         */
/*                                                                          */
/****************************************************************************/
static INT CALLBACK 
EnumSizes(LPLOGFONT lpLF, LPTEXTMETRICS lpTM, int iFontSpec, LPARAM lParam)
{
  HWND hDlg = LOWORD(lParam);
  char szBuf[16];

  (void) lpLF;   (void) iFontSpec;   (void) lpTM;

  /*
    Add the height which was found in the TEXTMETRIC structure
  */
#if defined(MOTIF)
  sprintf(szBuf, "%d", lpLF->lfHeight);  /* avoid creating DC for tm struct */
#else
  sprintf(szBuf, "%d", lpTM->tmHeight);
#endif
  SendDlgItemMessage(hDlg,ID_SIZECOMBO,CB_ADDSTRING,0,(LONG) szBuf);

  return TRUE;
}

#ifdef _cplusplus
}
#endif

