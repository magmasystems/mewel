/*===========================================================================*/
/*                                                                           */
/* File    : WLOADMNU.C                                                      */
/*                                                                           */
/* Purpose : Implements the LoadMenu() function                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#include "wprivate.h"
#include "window.h"

#define USE_WINDOWS_FORMAT

static HMENU PASCAL InternalLoadMenuIndirect(LPSTR *);

extern BOOL bDeferMenuRecalc;
extern LPSTR PASCAL SpanString(LPSTR);


HMENU FAR PASCAL LoadMenu(hModule, idMenu)
  HINSTANCE hModule;
  LPCSTR idMenu;
{
  LPSTR  pData = LoadResourcePtr(hModule, (LPSTR) idMenu, RT_MENU, NULL);
  HMENU  hMenu;

  if (pData == NULL)
    return NULL;

  bDeferMenuRecalc = TRUE;
  hMenu = LoadMenuIndirect(pData);
  bDeferMenuRecalc = FALSE;

  UnloadResourcePtr(hModule, pData, NULL);
  return hMenu;
}


HMENU FAR PASCAL LoadMenuIndirect(lpMenuTemplate)
  CONST VOID FAR *lpMenuTemplate;
{
  LPSTR s = (LPSTR) lpMenuTemplate;
  if (s == NULL)
    return NULL;

#ifdef USE_WINDOWS_FORMAT
  s += sizeof(MENUITEMTEMPLATEHEADER);
#endif
  return InternalLoadMenuIndirect(&s);
}



static HMENU PASCAL InternalLoadMenuIndirect(paData)
  LPSTR *paData;
{
  LPSTR  pData = *paData;
  HMENU  hMenu;

#ifdef USE_WINDOWS_FORMAT
  HMENU hPop;
  LPSTR pszItem;
  UINT  wFlags;

  /*
    Create an empty menu
  */
  if ((hMenu = CreateMenu()) == NULL)
    return NULL;

  do
  {
    MENUITEMTEMPLATE FAR *lpMIT = (MENUITEMTEMPLATE FAR *) pData;

    /*
      Get the menu item flags and move the data pointer past the flags
    */
    wFlags = lpMIT->mtOption;
    pData += sizeof(lpMIT->mtOption);

    if (wFlags & MF_POPUP)
    {
      hPop = 0;
    }
    else
    {
      /*
        Get the menu id and move the data pointer past the id.
      */
      hPop = lpMIT->mtID;
      pData += sizeof(lpMIT->mtID);

    }

    /*
      Get the menu string and move past the string
    */
    pszItem = pData;
    pData   = SpanString(pData);

    /*
      See if we have a separator
    */
    if (*pszItem == '\0' && hPop == 0 && wFlags == 0)
    {
      wFlags = (wFlags & MF_END) | MF_SEPARATOR;
      pszItem = NULL;
    }

    if (wFlags & MF_POPUP)
    {
      if ((hPop = InternalLoadMenuIndirect(&pData)) == NULL)
        goto badmenu;
    }

    if (!AppendMenu(hMenu, (wFlags & ~MF_END), hPop, pszItem))
      goto badmenu;

  } while (!(wFlags & MF_END));

  *paData = pData;
  return hMenu;

badmenu:
  DestroyMenu(hMenu);
  return NULL;


#else

  UINT   nChars;
  UINT   nItems;
  char   buf[MAXBUFSIZE];

#if defined(UNIX) || defined(VAXC)
  /* These declarations, and assignments help make the reading of the */
  /* resource file more machine independant -- see later #ifdef's     */
  /* Courtesy of West Publishing                                      */
  MTI *mti, Mti;
  mti = &Mti;
#endif

  /*
    Get the number of menu items
  */
#if defined(UNIX) || defined(VAXC)
  /*
    This fixes the problem of pData being an address that is not valid
    for the start of a WORD -- this will work no matter what address
    a machine needs for that start of a WORD.
    Courtesy of West Publishing
  */
  memcpy(&nItems, pData, sizeof(nItems));
#else
  nItems = * (LPWORD) pData;
#endif
  pData += sizeof(WORD);

  /*
    Create an empty menu
  */
  if ((hMenu = CreateMenu()) == NULLHWND)
    return NULLHWND;

  while (nItems-- > 0)
  {
#if defined(UNIX) || defined(VAXC)
    /* 
      Structures on some machines are required to start on a WORD or DWORD
      boundary -- this makes the code independant so that it doesn't matter
      at what address pData is at this time.
      Courtesy of West Publishing
    */
    memcpy(mti, pData, sizeof(MTI));
#else
    MTI FAR *mti = (MTI FAR *) pData;
#endif

    pData += sizeof(MTI);

    if (mti->style & MF_SEPARATOR)
    {
      ChangeMenu(hMenu, 0, NULL, 0, MF_SEPARATOR | MF_APPEND);
    }
    else
    {
      HMENU hPop;

#if defined(UNIX) || defined(VAXC)
      /*
        This fixes the problem of pData being an address that is not valid
        for the start of a WORD -- this will work no matter what address
        a machine needs for that start of a WORD.
        Courtesy of West Publishing
      */
      memcpy(&nChars, pData, sizeof(nChars));
#else
      nChars = * (LPWORD) pData;
#endif
      /*
        Extract the menuitem string
      */
      lmemcpy(buf, pData + sizeof(WORD), nChars);
      buf[nChars] = '\0';
      pData += sizeof(WORD) + nChars;

      /*
        Recurse on submenus
      */
      if (mti->style & MF_POPUP)
        hPop = InternalLoadMenuIndirect(&pData);
      else
        hPop = mti->idItem;

      /*
        Add the menu item to the menu
      */
      ChangeMenu(hMenu, 0, buf, hPop, mti->style | MF_APPEND | mti->attr);
    }
  }

  *paData = pData;
  return hMenu;
#endif  /* USE_WINDOWS_FORMAT */
}

