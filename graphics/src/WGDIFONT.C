/*===========================================================================*/
/*                                                                           */
/* File    : WGDIFONT.C                                                      */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible font functions                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wobject.h"

HFONT FAR PASCAL CreateFont(int nHeight, int nWidth, int nEscapement, 
                        int nOrientation, int nWeight, BYTE cItalic,
                        BYTE cUnderline, BYTE cStrikeOut, BYTE cCharSet,
                        BYTE cOutPrecision, BYTE cClipPrecision,
                        BYTE cQuality, BYTE cPitchAndFamily, LPCSTR lpFacename)
{
  LOGFONT lf;

  lf.lfHeight      = nHeight;
  lf.lfWidth       = nWidth;
  lf.lfEscapement  = nEscapement;
  lf.lfOrientation = nOrientation;
  lf.lfWeight      = nWeight;
  lf.lfItalic      = cItalic;
  lf.lfUnderline   = cUnderline;
  lf.lfStrikeOut   = cStrikeOut;
  lf.lfCharSet     = cCharSet;
  lf.lfOutPrecision = cOutPrecision;
  lf.lfClipPrecision= cClipPrecision;
  lf.lfQuality       = cQuality;
  lf.lfPitchAndFamily = cPitchAndFamily;
  if (lpFacename)
    lstrcpy(lf.lfFaceName, (LPSTR) lpFacename);
  else
    lf.lfFaceName[0] = '\0';
  return CreateFontIndirect(&lf);
}


HFONT FAR PASCAL CreateFontIndirect(lpLogFont)
  CONST LOGFONT FAR *lpLogFont;
{
  HANDLE   hObj;
  LPOBJECT lpObj;

  if ((hObj = _ObjectAlloc(OBJ_FONT)) == BADOBJECT)
    return (HFONT) NULL;

  if ((lpObj = _ObjectDeref(hObj)) == NULL)
    return (HFONT) NULL;

  /*
    Copy the logfont structure to the object memory
  */
  lpObj->uObject.uLogFont = *lpLogFont;
  return (HBRUSH) hObj;
}


/****************************************************************************/
/*                                                                          */
/* Function : SetMapperFlags()                                              */
/*                                                                          */
/* Purpose  : Controls the way the font mapping algorithm works             */
/*                                                                          */
/* Returns  : The old mapper flags.                                         */
/*                                                                          */
/****************************************************************************/
DWORD FAR PASCAL SetMapperFlags(hDC, dwFlags)
  HDC   hDC;
  DWORD dwFlags;
{
  (void) hDC;
  (void) dwFlags;
  return 0L;
}

