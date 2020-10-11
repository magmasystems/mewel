/*===========================================================================*/
/*                                                                           */
/* File    : WENUMFNT.C                                                      */
/*                                                                           */
/* Purpose : Implements the EnumFonts() and GetTextFace() font calls         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


typedef int (FAR PASCAL *EnumFontProc)(LPLOGFONT, LPTEXTMETRICS, short, LPSTR);

INT FAR PASCAL EnumFonts(hDC, lpFacename, lpFontFunc, lpData)
  HDC    hDC;
  LPCSTR lpFacename;
  OLDFONTENUMPROC lpFontFunc;
  LPSTR  lpData;
{
  TEXTMETRIC tm;
  LOGFONT    lf;

  (void) lpFacename;

  if (!lpFontFunc)
    return FALSE;

  GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);

  memset((char *) &lf, 0, sizeof(lf));
  lf.lfHeight  = lf.lfWidth = 1;
  lf.lfWeight  = FW_NORMAL;
  lf.lfCharSet = ANSI_CHARSET;
  strcpy(lf.lfFaceName, "System");

  return (* (EnumFontProc) lpFontFunc)(&lf, &tm, 0, lpData);
}


/****************************************************************************/
/*                                                                          */
/* Function : GetTextFace(hDC, nCount, lpFaceName)                          */
/*                                                                          */
/* Purpose  : Retrieves the face name of the currently selected font.       */
/*                                                                          */
/* Returns  : The number of characters in the face name.                    */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL GetTextFace(hDC, nCount, lpFaceName)
  HDC   hDC;
  INT   nCount;
  LPSTR lpFaceName;
{
  LOGFONT lf;
  LPHDC   lphDC;

  if ((lphDC = _GetDC(hDC)) != NULL)
  {
    GetObject(lphDC->hFont, sizeof(lf), &lf);
    lstrncpy(lpFaceName, lf.lfFaceName, nCount);
    return lstrlen(lpFaceName);
  }
  else
  {
    lstrncpy(lpFaceName, "System", nCount);
    return 6;
  }
}

