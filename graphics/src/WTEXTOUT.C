/*===========================================================================*/
/*                                                                           */
/* File    : WTEXTOUT.C                                                      */
/*                                                                           */
/* Purpose : Implements the TextOut and function                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  Hook to GUIs and graphics engines
*/
typedef int (FAR PASCAL *TEXTOUTPROC)(HDC, INT, INT, LPSTR, UINT);
TEXTOUTPROC lpfnTextOutHook = (TEXTOUTPROC) 0;


BOOL FAR PASCAL TextOut(hDC, x, y, pText, cchText)
  HDC    hDC;
  INT    x, y;
  LPCSTR pText;
  INT    cchText;
{
  LPSTR pSave, pTilde;
  BYTE  chSave;
  LPHDC lphDC;
  POINT pt;
#if defined(ZAPP) && defined(MEWEL_TEXT)
  POINT ptSaveOrg;
#endif
  UINT  wAlign;
  UINT  cxExtent;

  /*
    No chars to be written?
  */
  if (!cchText)
    return TRUE;

  /*
    Get a pointer to the device context structure
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Transform logical to device points
  */
  pt.x = (MWCOORD) x;  pt.y = (MWCOORD) y;

#if defined(ZAPP) && defined(MEWEL_TEXT)
  /*
    We should transform logical coords to device coords, but we really
    need a functions like WinPutsWithDC() in order to do it correctly. The
    problem here is that WinPuts() needs logical coords, not device coords.
  */
  /*
    5/23/93 (maa)
      Temporarily make the DC's origin 0 so LPtoSP() will not add it in.
  */
  ptSaveOrg = lphDC->ptOrg;
  lphDC->ptOrg.x = lphDC->ptOrg.y = 0;
  LPtoSP(hDC, (LPPOINT) &pt, 1);
  lphDC->ptOrg = ptSaveOrg;
#endif

  /*
    See if there is a "device driver" attached to the DC.
  */
  if (lphDC->lpfnTextOutProc)
    return (*lphDC->lpfnTextOutProc)(hDC, pt.x, pt.y, (LPSTR)pText, cchText);

  /*
    Any hook to a GUI?
  */
  if (lpfnTextOutHook)
    return (*lpfnTextOutHook)(hDC, pt.x, pt.y, (LPSTR) pText, cchText);

#if !defined(MEWEL_GUI) && !defined(USE_NATIVE_GUI)
  /*
    Save the cchText'th character, write the string, and restore the char.
    But first, account for the prefix hilite character.
  */
  pSave = (LPSTR) (pText + cchText);
  if (HilitePrefix && (pTilde = lstrchr((LPSTR) pText, HilitePrefix)) != NULL &&
      pTilde - (LPSTR) pText < (int) cchText)
    pSave++;
  if ((chSave = *pSave) != '\0')
    *pSave = '\0';


  /*
    Adjust the starting position of the text as-per the text alignment 
    which is stored in the passed DC.
  */
  wAlign = lphDC->wAlignment;

  /*
    If TA_UPDATECP, then use the current position stored in the DC
  */
  if (wAlign & TA_UPDATECP)
  {
    pt.x = lphDC->ptPen.x;
    pt.y = lphDC->ptPen.y;
  }

  cxExtent = cchText;

  /*
    Adjust the starting x coordinate if centered or right justified.
  */
  if (wAlign & TA_CENTER)
    pt.x -= (MWCOORD) (cxExtent / 2);
  else if (wAlign & TA_RIGHT)
    pt.x -= (MWCOORD) (cxExtent - 1);

  /*
    Update the current position if the string is right or left justified
  */
  if (wAlign & TA_UPDATECP)
  {
    if (wAlign & TA_RIGHT)            /* right */
      lphDC->ptPen.x -= (MWCOORD) cxExtent;
    else if (!(wAlign & TA_CENTER))   /* left */
      lphDC->ptPen.x += (MWCOORD) cxExtent;
  }


  /*
    Write the string
  */
  _WinPuts(lphDC->hWnd, hDC, pt.y, pt.x, (LPSTR) pText, lphDC->attr, -1, TRUE);

  /*
    Restore the zeroed character
  */
  if (chSave)
    *pSave = chSave;
  return TRUE;
#endif
}
