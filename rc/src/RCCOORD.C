/*===========================================================================*/
/*                                                                           */
/* File    : RCCOORD.C                                                       */
/*                                                                           */
/* Purpose : Handles the transformation of Windows dialog units to MEWEL's   */
/*           character-mode coordinates. This only gets called if you        */
/*           specify the -w option to the resource compiler.                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "int.h"
#include "rccomp.h"
#define RC_INVOKED
#include "style.h"
#include <ctype.h>

#ifndef max
#define max(a,b)        ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a,b)        ( ((a) < (b)) ? (a) : (b) )
#endif



WORD bNoHeuristics = 0;  /* use heuristics as a default */


void MEWELizeCoords(pX, pY, pCX, pCY, iClass, szText)
  WORD *pX, *pY, *pCX, *pCY;
  WORD iClass;
  char *szText;
{
  int  sLen;
  WORD xOrig, yOrig, cxOrig, cyOrig;


  if (bScreenRelativeCoords)
  {
    *pX -= (xDlg+1);
    *pY -= (yDlg+1);
  }


  if (!xTranslated && !yTranslated)
    return;

  xOrig  = *pX;
  cxOrig = *pCX;
  yOrig  = *pY;
  cyOrig = *pCY;

  /*
    If the user specified scaling coordinates, then do the translation
    of the starting coords and the width and height.
    2/26/92 (maa)
      Round up or down by adding xTranslated/2 or yTranslated/2
  */
  if (xTranslated)
  {
    *pX =  ((*pX + xTranslated/2-iRounding)  / xTranslated) + xDlg;
    *pCX = ((*pCX + xTranslated/2-iRounding) / xTranslated);
  }
  if (yTranslated)
  {
    *pY =  ((*pY + yTranslated/2-iRounding)  / yTranslated) + yDlg;
    *pCY = ((*pCY + yTranslated/2-iRounding) / yTranslated);
  }

  if (!bNoBorders)
    if (iClass == PUSHBUTTON_CLASS && *pCY > 2 && 
               !(CurrStyle & WS_SHADOW) && (CurrStyle & 0x0F) != BS_OWNERDRAW)
      CurrStyle |= WS_BORDER;


  switch (iClass)
  {
    case TEXT_CLASS :
    case EDIT_CLASS :
      if (iClass == EDIT_CLASS && !(CurrStyle & ES_MULTILINE) && *pCY < 3)
        *pCY = 1;

      /*
        Do not adjust the length for text strings because this will
        affect SS_CENTER and SS_RIGHT justified strings.
      */
      if (iClass == TEXT_CLASS && !bNoHeuristics)
        if (szText != NULL && szText[0] != '\0')
          *pCX = strlen(szText);
        else
          *pCX = max(*pCX, 1);

      if (iClass == EDIT_CLASS && (CurrStyle & WS_BORDER))
        *pCX += 2;
      break;


    case PUSHBUTTON_CLASS  :
    case RADIOBUTTON_CLASS :
    case CHECKBOX_CLASS    :
      /*
        Radiobuttons and checkboxes must be 1 row high in text mode.
        Non-ownerdrawn pushbuttons start out at 1 row high if the
        conversion heuristics are enabled.
      */
      if (iClass != PUSHBUTTON_CLASS)
      {
        *pCY = 1;
      }
      else
      {
        if (!bNoHeuristics && (CurrStyle & 0x0F) != BS_OWNERDRAW)
          *pCY = 1;
      }

      /*
        If the button has text, then size the button to the text. For
        radiobuttons and checkboxes, we must make sure that there
        is enough room for the symbols "[x] " and "(x) ".
      */
      if (szText != NULL && szText[0] != '\0')
      { 
        sLen = strlen(szText);
        if (strchr(szText, '~') != NULL || strchr(szText, '&') != NULL)
          sLen--;
        if (iClass == PUSHBUTTON_CLASS)
        {
          if (!bNoHeuristics)
            *pCX = max(*pCX, sLen);
        }
        else
        {
          *pCX = max(*pCX, sLen);
          if (*pCX < sLen + 4)
            *pCX = sLen + 4;  /* add 4 for the "( ) " or "[ ] " */
        }
      }

      else  /* the title field is null */
      {
        if (iClass == PUSHBUTTON_CLASS)
        {
          *pCX = max(*pCX, 2);  /* add 2 for the borders */
        }
        else
        {
          *pCX = max(*pCX, 3);  /* for "[x]" */
        }
      }

      /*
        Pusbuttons which have a border or are owner-drawn should take on
        the customized height.
      */
      if (iClass == PUSHBUTTON_CLASS && (CurrStyle & WS_BORDER))
      {
        *pCX += 2;
        if ((CurrStyle & 0x0F) != BS_OWNERDRAW && !bNoHeuristics)
          *pCY = nPushButtonHeight;
      }
      break;

     case COMBO_CLASS :
       CurrStyle &= ~WS_VSCROLL;
       *pCX += 2;   /* for the border */
       *pCY += 2;   /* for the border */
       break;

     case SCROLLBAR_CLASS :
       break;
 
     case LISTBOX_CLASS :
       if (CurrStyle & WS_BORDER)
       {
         *pCX += 2;  *pCY +=2;
       }
       *pCY = max(*pCY, 4);
       *pCX = max(*pCX, 3);
       break;
 
     case FRAME_CLASS   :
       break;
  }

  /*
    Make sure that the origin of the control doesn't extend outside of
    the dialog box
  */
  if ((int) *pX < 0)
    *pX = 0;
  if ((int) *pY < 0)
    *pY = 0;

  /*
    11/10/91 (maa)
      Forget about bounding controls to the dialog box, cause we might get
    into the situation where several controls are "hidden" outside the
    borders of the dialog box, and will be revealed when the dialog box
    is "unfolded".
  */
#if 0
  if (*pX >= cxDlg)
    *pX = (cxDlg - 1) - *pCX;
  if (*pY >= cyDlg)
    *pY = (cyDlg - 1) - *pCY;
#endif

  if (bEchoTranslation)
  {
    printf("\tTranslated [%d %d %d %d] to [%d %d %d %d]\n",
              xOrig, yOrig, cxOrig, cyOrig, *pX, *pY, *pCX, *pCY);
  }
}

