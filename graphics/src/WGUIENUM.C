/*===========================================================================*/
/*                                                                           */
/* File    : WGUIENUM.C                                                      */
/*                                                                           */
/* Purpose : Font handling functions for MEWEL/GUI                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_FONTS

#include "wprivate.h"
#include "window.h"
#include "wobject.h"
#include "wgraphic.h"

#if 0
typedef struct tagFontNameInfo
{
  LPSTR lpFontName;  /* logical font name (Helv, Tms Rmn, etc) */
  LPSTR lpPathName;  /* DOS path name of graphics-engine font  */
  INT   hFont;       /* numeric id of font (for BGI) or mem handle */
  LPVOID lpFontBuf;  /* address of locked font. NULL if not locked */
} FONTNAMEINFO, FAR *LPFONTNAMEINFO;
#endif

typedef int (FAR PASCAL *EnumFontProc)(LPLOGFONT, LPTEXTMETRICS, int, LPARAM);


INT FAR PASCAL EnumFonts(hDC, lpszFace, lpFontFunc, lParam)
  HDC    hDC;
  LPCSTR lpszFace;
  OLDFONTENUMPROC lpFontFunc;
  LPSTR  lParam;
{
  LOGFONT        lf;
  TEXTMETRIC     tm;

  LPFONTNAMEINFO lpFNI;
  LIST           *pList;
  HDC            hOrigDC;
  LPHDC          lphDC;
  LPOBJECT       lpObj;
  HFONT          hFont, hPrevFont, hOrigFont;
  INT            rc = TRUE;


  /*
    Get a DC and a pointer to the DC structure
  */
  if ((hOrigDC = hDC) == NULL)
    hDC = GetDC(NULL);
  lphDC = _GetDC(hDC);
    
  /*
    Save the handle to the DCs font so we can restore it later.
  */
  hPrevFont = NULL;
  hOrigFont = lphDC->hFont;

  /*
    Get some sensible values into the LOGFONT structure
  */
  lpObj = _ObjectDeref(lphDC->hFont);
  lmemcpy((LPSTR) &lf, (LPSTR) &lpObj->uObject.uLogFont, sizeof(lf));


  /*
    Go through the list of registered fonts.
  */
  if (lpszFace == NULL)
  {
    for (pList = MEWELFontNameList;  pList;  pList = pList->next)
    {
      lpFNI = (LPFONTNAMEINFO) pList->data;

      /*
        See if we are referencing one of MEWEL's special font names
      */
      if (!lstricmp(lpFNI->lpFontName, "DefaultFontFile") ||
          !lstricmp(lpFNI->lpFontName, "System")          ||
          !lstricmp(lpFNI->lpFontName, "menu.check"))
        continue;

      /*
        Fill the LOGFONT structure with the appropriate values
      */
      lstrcpy(lf.lfFaceName, lpFNI->lpFontName);

      /*
        Get rid of the previously created font.
      */
      if (hPrevFont)
      {
        DeleteObject(hPrevFont);
        hPrevFont = NULL;
      }

      /*
        Create and select the new font
      */
      hFont = CreateFontIndirect(&lf);
      SelectObject(hDC, hFont);
      hPrevFont = hFont;

      /*
        Fill the TEXTMETRIC structure
      */
      GetTextMetrics(hDC, &tm);

      /*
        Call the enumeration function.
      */
      rc = (* (EnumFontProc) lpFontFunc)(&lf, &tm, 0, (LONG) lParam);

      /*
        If the enum function returns 0, then stop the enumeration.
      */
      if (rc == 0)
        break;
    } /* for */
  }
  else
  {
    int i;
    int cyPrevFont;
    static int aiSizes[] =
      { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };

    lstrcpy(lf.lfFaceName, lpszFace);
    cyPrevFont = 0;

    for (i = 0;  i < sizeof(aiSizes)/sizeof(aiSizes[0]);  i++)
    {
      /*
        Fill the LOGFONT structure with the appropriate values
      */
      lf.lfHeight = aiSizes[i];
      lf.lfWidth  = (aiSizes[i] * 2) / 3;

      /*
        Get rid of the previously created font.
      */
      if (hPrevFont)
      {
        DeleteObject(hPrevFont);
        hPrevFont = NULL;
      }

      /*
        Create and select the new font
      */
      hFont = CreateFontIndirect(&lf);
      SelectObject(hDC, hFont);
      hPrevFont = hFont;

      /*
        Fill the TEXTMETRIC structure
      */
#if defined(MSC)
      tm.tmHeight = aiSizes[i];
#else
      GetTextMetrics(hDC, &tm);
#endif

      /*
        Call the enumeration function.
      */
#if !defined(MSC)
      if (tm.tmHeight != cyPrevFont)
#endif
      {
        rc = (* (EnumFontProc) lpFontFunc)(&lf, &tm, 0, (LONG) lParam);
        /*
          If the enum function returns 0, then stop the enumeration.
        */
        if (rc == 0)
          break;
      }


      cyPrevFont = tm.tmHeight;
    } /* for */
  }

  if (hPrevFont)
  {
    DeleteObject(hPrevFont);
  }

  /*
    Restore the DC's original font and free the DC
  */
  lphDC->hFont = hOrigFont;
  if (hOrigDC == NULL)
    ReleaseDC(NULL, hDC);

  return rc;
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


