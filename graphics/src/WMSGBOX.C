/*===========================================================================*/
/*                                                                           */
/* File    : WMSGBOX.C                                                       */
/*                                                                           */
/* Purpose : Implements the message box facility                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI)
#include "wgraphic.h"
#endif

/*
  Message Box spacing values :
   1) Space from the border of the message box to the icon
   2) Space between pushbuttons
   3) Space underneath pushbuttons
   4) Space before first line of text
   5) X and Y multiplier for text
*/
#ifdef MEWEL_GUI
#define CX_BEFOREICON  (IGetSystemMetrics(SM_CXBORDER) + 8)
#define CY_ABOVEICON   (IGetSystemMetrics(SM_CYBORDER) + IGetSystemMetrics(SM_CYCAPTION) + tm.tmHeight + tm.tmExternalLeading)
#define CX_AFTERICON   8
#define CX_BETWEEN     16
#define CY_UNDERBUTTON (IGetSystemMetrics(SM_CYBORDER) + 8)
#define CY_ABOVETEXT   (IGetSystemMetrics(SM_CYBORDER) + IGetSystemMetrics(SM_CYCAPTION) + tm.tmHeight + tm.tmExternalLeading)
#define CX_CHAR        (tm.tmAveCharWidth)
#define CY_CHAR        (tm.tmHeight + tm.tmExternalLeading)
#else
#define CX_BEFOREICON  (IGetSystemMetrics(SM_CXBORDER) + 1)
#define CY_ABOVEICON   (IGetSystemMetrics(SM_CYBORDER) + 1)
#define CX_AFTERICON   1
#define CX_BETWEEN     2
#define CY_UNDERBUTTON (IGetSystemMetrics(SM_CYBORDER) + 1)
#define CY_ABOVETEXT   (IGetSystemMetrics(SM_CYBORDER) + 1)
#define CX_CHAR        1
#define CY_CHAR        1
#endif



static INT  FAR PASCAL MsgBoxWinProc(HWND,UINT,WPARAM,LPARAM);

BOOL bMessageBoxHasShadow = FALSE;
static int LongestButton = 0;

#define NUM_BUTTONS  8

typedef struct buttonclass
{
  int  numbuttons;
  int  iDefaultButton;
  int  min_msgboxlen;
  int  idButtons[3];
} BUTTONCLASS;

static BUTTONCLASS DefMsgBoxButtons[] =
{
#ifdef MEWEL_GUI
/* <OK> */                      { 1, 0, 66, IDOK,                        },
/* <OK> <CANCEL> */             { 2, 0,128, IDOK,    IDCANCEL,           },
/* <ABORT> <RETRY> <IGNORE> */  { 3, 0,192, IDABORT, IDRETRY, IDIGNORE   },
/* <YES> <NO> <CANCEL> */       { 3, 0,192, IDYES,   IDNO,    IDCANCEL   },
/* <YES> <NO> */                { 2, 0,128, IDYES,   IDNO,               },
/* <RETRY> <CANCEL> */          { 2, 0,128, IDRETRY, IDCANCEL            },
#else
/* <OK> */                      { 1, 0, 10, IDOK,                        },
/* <OK> <CANCEL> */             { 2, 0, 22, IDOK,    IDCANCEL,           },
/* <ABORT> <RETRY> <IGNORE> */  { 3, 0, 34, IDABORT, IDRETRY, IDIGNORE   },
/* <YES> <NO> <CANCEL> */       { 3, 0, 34, IDYES,   IDNO,    IDCANCEL   },
/* <YES> <NO> */                { 2, 0, 22, IDYES,   IDNO,               },
/* <RETRY> <CANCEL> */          { 2, 0, 22, IDRETRY, IDCANCEL            },
#endif
};


INT FAR PASCAL MessageBox(hParent, lpText, lpCaption, wType)
  HWND   hParent;
  LPCSTR lpText;
  LPCSTR lpCaption;
  UINT   wType; 
{
  int  nb;
  int  left;
  int  height, len, maxlen;
  int  iMaxPossibleWidth;
  int  iDefaultButton;
  int  cxIcon, cxAllButtons;
  RECT r;
  HWND hMB;
  WINDOW *wMB;
  BUTTONCLASS *pMsgBox = &DefMsgBoxButtons[wType & 0x0F];
  COLOR attr;

  UINT nMaxLines;
  UINT iLine;
  int  iLineLen;
  UINT iWidthOffset;
  LPSTR *pLines;
  LPSTR pText;
  BYTE ch;


#ifdef MEWEL_GUI
  TEXTMETRIC tm;
  HDC        hDC;

  /*
    We need to set the current font and keep it around for the width and
    height calculations. We release the DC until after we do all of the
    window and control creation.
  */
  hDC = GetDC(_HwndDesktop);
  GetTextMetrics(hDC, &tm);
#endif


  (void) hParent;


#if defined(DOS286X) || defined(DOS16M) || defined(ERGO) || defined(UNIX)
  /*
    For DOS extenders, we have a potential problem if the static text
    resides in the code segment. Since we manipulate lpText below, we
    will cause a protection fault. The kludge is to make a copy of
    the lpText and then we can safely manipulate the copy.

    This problem can also exist under certain versions of UNIX compilers
    (GNU C++).
  */
  if (lpText == NULL)
    lpText = "";
  lpText = (LPCSTR) lstrsave((LPSTR) lpText);
#endif



#ifdef INTERNATIONAL_MEWEL
   /*
     If we are taking the text of the buttons out of the resource file,
     then load in the text and figure out the width of the longest
     button.
   */
   if (!LongestButton)
   {
     BYTE buttontxt[31];

     if (MewelCurrOpenResourceFile != -1)
     {
       for (nb = 0;  nb < INTL_BUTTON_COUNT;  nb++)
         if (LoadString(MewelCurrOpenResourceFile,INTL_BUTTON_FIRST+nb,buttontxt,30) > 0)
           SysStrings[SYSSTR_OK+nb] = lstrsave(buttontxt);

       /*
         Read the default caption of the message box
       */
       if (LoadString(MewelCurrOpenResourceFile,INTL_ERROR,buttontxt,30) > 0)
         SysStrings[SYSSTR_ERROR] = lstrsave(buttontxt);
     }

     for (nb = 0;  nb < NUM_BUTTONS;  nb++)
     {
       int iLength = lstrlen(SysStrings[SYSSTR_OK+nb]);
       if (lstrchr(SysStrings[SYSSTR_OK+nb], '~')) 
         iLength--;
       LongestButton = max(LongestButton, iLength*CX_CHAR);
     }

     for (nb = 0; nb < (sizeof(DefMsgBoxButtons) / sizeof(BUTTONCLASS)); nb++)
        DefMsgBoxButtons[nb].min_msgboxlen =
          (LongestButton + 2*IGetSystemMetrics(SM_CXBORDER) + CX_CHAR) * 
             DefMsgBoxButtons[nb].numbuttons;
   }
#else
   if (!LongestButton)
     LongestButton = 8 * CX_CHAR;
#endif


  nMaxLines = VideoInfo.length / CY_CHAR;
  if ((pLines = (LPSTR *) emalloc((nMaxLines+1) * sizeof(LPSTR))) == NULL)
    return FALSE;

  /*
    The min height is the borders (2), the button row (3) and a blank lines (3)
    (For the GUI version, we need to calc the size of the pushbuttons (24),
    the space under the pushbutton row (8), the space between the caption
    and the first line of text (CY_ABOVETEXT), and the space between the
    last line of text and the pushbutton row (CY_CHAR)).
  */
#ifdef MEWEL_GUI
  height = (2*IGetSystemMetrics(SM_CYBORDER)) + IGetSystemMetrics(SM_CYCAPTION) +
           (24 + 8) + CY_CHAR + CY_ABOVETEXT;
#else
  height = 8;
#endif

  /*
    Figure how how wide the message box should be
  */
  maxlen = pMsgBox->min_msgboxlen + (2 * IGetSystemMetrics(SM_CXBORDER));
  maxlen = min(maxlen, VideoInfo.width-1);

  /*
    Figure out how much horizontal space to reserve for the icon
  */
  if (wType & MB_ICONMASK)
    cxIcon = IGetSystemMetrics(SM_CXICON) + CX_BEFOREICON + CX_AFTERICON;
  else
    cxIcon = 0;


  nMaxLines = (VideoInfo.length - height) / CY_CHAR;
  iLineLen = 0;
  pLines[iLine = 0] = (LPSTR) lpText;

#ifdef MEWEL_GUI
  iWidthOffset = (wType & MB_ICONMASK) ? 40 : (6*CX_CHAR);
#else
  iWidthOffset = (wType & MB_ICONMASK) ? 17 : 6;
#endif

  /*
    Determine the maximum possible width that the message box can be.
    This is equal to the width of the screen minus the space for
    the icon and window borders.
  */
  iMaxPossibleWidth = (int) 
        (VideoInfo.width - iWidthOffset - 2*IGetSystemMetrics(SM_CXBORDER));

  /*
    Parse the message text into a series of lines.
  */
  for (pText = (LPSTR) lpText;  (ch = *pText) != '\0';  )
  {
    /*
      See if we have a hard line break or if we need to wordwrap.
    */
    if (ch == '\r' || ch == '\n' || iLineLen*CX_CHAR >= iMaxPossibleWidth)
    {
      /*
        If we got here cause the line was too long, move back to the
        start of the word.
      */
      if (ch != '\r' && ch != '\n')
      {
        while ((*pText != ' ' || iLineLen*CX_CHAR >= iMaxPossibleWidth) &&
               pText > pLines[iLine])
        {
          pText--;
          iLineLen--;
        }
      }
      else
      {
        if (ch == '\r' && pText[1] == '\n')  /* gobble up the \r */
          pText++;
        pText++;
      }

      /*
        Get the max width so far
      */
      maxlen = max(maxlen, iLineLen*CX_CHAR);

      /*
        Make sure that the number of lines has not exceeded the maximum
        possible height of the message box.
      */
      if ((height += CY_CHAR) > (int) VideoInfo.length)
        break;

      if (++iLine >= nMaxLines)         /* Go to the next pLine */
      {
        iLine--;
        break;
      }
      pLines[iLine] = pText;
      iLineLen = 0;                     /* Reset the line length counter */
    }
    else
    {
      iLineLen++;
      pText++;
    }
  }
  pLines[iLine+1] = pText;
  height += CY_CHAR;
  maxlen = max(maxlen, iLineLen*CX_CHAR);   /* Get the max width so far */


  /*
    Figure out the length of the caption.
  */
  if (lpCaption == NULL)
    lpCaption = SysStrings[SYSSTR_ERROR];
  len = lstrlen((LPSTR) lpCaption) * CX_CHAR;
  maxlen = max(maxlen, len);

  /*
    Figure out the max length of all of the buttons
  */
  nb = pMsgBox->numbuttons;
  cxAllButtons = (nb * (LongestButton + 2*IGetSystemMetrics(SM_CXBORDER))) +
                 ((nb-1) * CX_BETWEEN);
  maxlen = max(maxlen, cxAllButtons);

  /*
    If we have an icon, then increase the size of the message box
    by the width of the icon.
  */
  if (wType & MB_ICONMASK)
    maxlen += cxIcon;


  /*
    Calculate the coordinates of the message box window
  */
  r.left   = (VideoInfo.width - maxlen) / 2 - IGetSystemMetrics(SM_CXBORDER) - CX_CHAR;
  r.right  = r.left + maxlen + (2*IGetSystemMetrics(SM_CXBORDER)) + 2*CX_CHAR;
  r.top    = (VideoInfo.length - height) / 2;
  r.bottom = r.top + height;

  /*
    Create the message box dialog box
    
    5/9/92 (maa)
      Use NULL as the parent instead of hParent so that the app can
    pop up a message box during WM_INITDIALOG messages. Otherwise,
    the message box won't be shown cause at the time of the WM_INITDIALOG,
    the parent dialog is hidden.
  */
  attr = WinQuerySysColor(NULLHWND, SYSCLR_MESSAGEBOX);
  hMB = DialogCreate(NULL, r.top,r.left,r.bottom,r.right, (LPSTR) lpCaption,
                     attr, 
                     WS_SHADOW  | WS_POPUP  | WS_CAPTION | 
                     WS_BORDER  | WS_SYSMENU | DS_SYSMODAL,
                     (FARPROC) MsgBoxWinProc, 0, (LPSTR) NULL, 0);
  if (!hMB)
  {
    MyFree(pLines);
    return FALSE;
  }
  wMB = WID_TO_WIN(hMB);
  wMB->ulStyle |= WIN_IS_MSGBOX;

  /*
    12/5/95 (maa) 
      Attach the original parent to the msgbox as the 'owner' of the
      msgbox. This will enable the WM_CTLCOLOR message to be routed
      correctly.
  */
  wMB->hWndOwner = hParent;


  if (_PrepareWMCtlColor(hMB, CTLCOLOR_MSGBOX, 0))
    attr = WID_TO_WIN(hMB)->attr;

  /*
    Figure out the x coordinate where we should start placing the
    pushbuttons. If we have an icon occupying the left side, then
    slide the origin over to the right so the buttons are out of the
    way.
  */
  maxlen = RECT_WIDTH(wMB->rClient) - cxIcon;
  left = (maxlen - cxAllButtons) / 2 + cxIcon;

  /*
    Figure out the index of the default button
  */
  iDefaultButton = (wType & MB_DEFMASK) ?
                     ((wType & MB_DEFMASK) >> 8) : pMsgBox->iDefaultButton;

  /*
    Create the button(s)
  */
  for (nb = 0;  nb < pMsgBox->numbuttons;  nb++)
  {
    int   id = pMsgBox->idButtons[nb];
    LPSTR szButton = SysStrings[SYSSTR_OK + id - 1];

    /*
      All of these contortions with fFlags are needed cause BC++ 3.0 produces
      bad code when you do something like :
        WS_SHADOW | ((nb == iDefaultButton) ? BS_DEFAULT : 0L),
      BC++ seems to give the button the BS_OWNERDRAW style!!!
    */
    DWORD fFlags = 0L;
    if (nb == iDefaultButton)
      fFlags |= BS_DEFPUSHBUTTON;
    if (bMessageBoxHasShadow)
      fFlags |= WS_SHADOW;
    else
      fFlags |= WS_BORDER;
    if (TEST_PROGRAM_STATE(STATE_NORTON_BUTTON))  /* the Norton Look */
      fFlags |= WS_BORDER;


    PushButtonCreate(hMB,
#ifdef MEWEL_GUI
                     r.bottom - CY_UNDERBUTTON - 24,
#else
                     r.bottom - CY_UNDERBUTTON - (!bMessageBoxHasShadow) - 1,
#endif
                     wMB->rClient.left + left,
                     r.bottom - CY_UNDERBUTTON + (!bMessageBoxHasShadow) + 0,
                     wMB->rClient.left + left + LongestButton + 1 + 1,
                     szButton,
#ifdef TELEVOICE
                     SYSTEM_COLOR,
#else
                     attr,
#endif
                     fFlags,
                     id);

    /*
      Make sure that the user can use both the TAB and the arrow keys.
    */
    fFlags = WS_TABSTOP;
    if (nb == 0) 
      fFlags |= WS_GROUP;
    SetDlgItemStyle(hMB, id, fFlags);
    left += CX_BETWEEN + LongestButton + (2*IGetSystemMetrics(SM_CXBORDER));
  }


#if defined(MEWEL_GUI)
  /*
    Make sure we have the proper font for the upcoming GetTextExtent.
  */
  RealizeFont(hDC);
#endif

  /*
    Create and display the text
  */
  for (nb = 0;  nb <= (int) iLine;  nb++)
  {
    if (pLines[nb]) 
    {
      BYTE  chSave;
      LPSTR pSave;

      pSave = pLines[nb+1] - 1;
      chSave = *pSave;
      if (chSave != '\n' && chSave != '\r' && chSave != '\0')
        chSave = *++pSave;
      *pSave = '\0';

#if defined(MEWEL_GUI)
      len = LOWORD(_GetTextExtent(pLines[nb]));
#else
      len = lstrlen(pLines[nb]) * CX_CHAR;
#endif

      /*
        'left' is an absolute screen coordinate to start the text at.
        We center the text within the message box.
      */
      left = (maxlen - len) / 2 + cxIcon;

      StaticCreate(hMB,
                   r.top + ((nb+0)*CY_CHAR) + CY_ABOVETEXT,
                   wMB->rClient.left + left,
                   r.top + ((nb+1)*CY_CHAR) + CY_ABOVETEXT,
                   wMB->rClient.left + left + len,
                   pLines[nb], attr, SS_TEXT, -1, 0);

      *pSave = chSave;
    }
  } /* end of creating the static text */


  /*
    If an icon goes into this message box, determine which icon to use.
    Then create the icon window.
  */
  if (wType & MB_ICONMASK)
  {
    LPCSTR idIcon;

    switch (wType & MB_ICONMASK)
    {
      case MB_ICONHAND     :
        idIcon = IDI_HAND;
        break;
      case MB_ICONQUESTION :
        idIcon = IDI_QUESTION;
        break;
      case MB_ICONEXCLAMATION  :
        idIcon = IDI_EXCLAMATION;
        break;
      case MB_ICONASTERISK     :
        idIcon = IDI_ASTERISK;
        break;
    }
    StaticCreate(hMB,
                 r.top  + CY_ABOVEICON,
                 r.left + CX_BEFOREICON, 
                 r.top  + CY_ABOVEICON  + IGetSystemMetrics(SM_CYICON),
                 r.left + CX_BEFOREICON + IGetSystemMetrics(SM_CXICON),
                 (LPSTR) idIcon, 0x07, SS_ICON, (int) (DWORD) idIcon, 0);
  }


#ifdef MEWEL_GUI
  ReleaseDC(_HwndDesktop, hDC);
#endif

#if defined(DOS286X) || defined(DOS16M) || defined(ERGO)
  /*
    Free the copy of lpText which we made above.
  */
  MYFREE_FAR(lpText);
#endif

  /*
    Invoke the message box
  */
  nb = _DialogBox(hMB);

  /*
    Free the line array and return the button which was pushed
  */
  MyFree(pLines);
  return nb;
}


/****************************************************************************/
/*                                                                          */
/* Function : MsgBoxWinProc()                                               */
/*                                                                          */
/* Purpose  : Default dialog box proc for MEWEL message boxes.              */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
static INT FAR PASCAL MsgBoxWinProc(hMB, msg, wParam, lParam)
  HWND   hMB;
  UINT   msg;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND hb;
  int  id;

  (void) lParam;

  switch (msg)
  {
    case WM_INITDIALOG :
      /*
        Set the initial focus to the default pushbutton
      */
      if ((hb = DlgGetDefButton(hMB, &id)) != NULLHWND)
      {
        SetFocus(hb);
        return FALSE;
      }
      return TRUE;


    case WM_SYSKEYDOWN :
    case WM_KEYDOWN    :
      /*
        If the user presses a key which corresponds the to first letter of
        one of the pushbuttons, end the dialog and send back that button
        ID as the result...
      */
#ifdef INTERNATIONAL_MEWEL
      if (!(wParam & 0xFF00))
#else
      if (wParam < 127 && isalpha(wParam))
#endif
      {
        wParam = lang_upper(wParam);
        for (id = 1;  id <= NUM_BUTTONS;  id++)
        {
          LPSTR pAlpha = SysStrings[SYSSTR_OK + id - 1];
          while (isspace(*pAlpha) || *pAlpha == HILITE_PREFIX)
            pAlpha++;
          if ((WPARAM) lang_upper(*pAlpha) == wParam && GetDlgItem(hMB, id))
          {
            EndDialog(hMB, id);
            return TRUE;
          }
        }
        return FALSE;
      }
      else if (wParam == VK_ALT_PLUS)
      {
        MessageBox(NULLHWND, InternalSysParams.pszMEWELcopyright, NULL, MB_OK);
        return TRUE;
      }
      break;

    case WM_QUIT :
      EndDialog(hMB, IDCANCEL);
      return TRUE;

    case WM_COMMAND :
      /*
        Make sure that the command id corresponds to one of the
        currently-displayed pushbuttons. If so, terminate the
        message box, and pass the command-id back to the caller.
      */
      if (GetDlgItem(hMB, wParam))
        EndDialog(hMB, wParam);
      return TRUE;

#if defined(MEWEL_GUI) || defined(XWINDOWS)
    case WM_CTLCOLOR :
      if (HIWORD(lParam) == CTLCOLOR_STATIC)
      {
        WINDOW *wMB = WID_TO_WIN(hMB);
        SetTextColor((HDC) wParam, AttrToRGB(GET_FOREGROUND(wMB->attr)));
        SetBkColor((HDC) wParam, AttrToRGB(GET_BACKGROUND(wMB->attr)));
        return SysCreateSolidBrush(AttrToRGB(GET_BACKGROUND(wMB->attr)));
      }
      else
        return FALSE;
#endif

  }

  return FALSE;
}

