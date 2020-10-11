/*===========================================================================*/
/*                                                                           */
/* File    : WLOADSTR.C                                                      */
/*                                                                           */
/* Purpose : Implements the LoadString() function                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#include "wprivate.h"
#include "window.h"

/*
  Comments on the inner-workings of string loading....

  The "master index" functions like a dictionary, and we use it to preload
  groups of 16 ("CACHESIZE") strings at a time. Each entry in the master index
  contains the low and high string identifier in a block of 16 strings, and
  the starting and ending seek position of the strings within that block.

  When we call GetResource(RT_STRING), then we are actually loading the
  master index, and not a string. The first word of the master index
  contains the number of indicies, with the actual index following this
  value.

  The StringCache contains a pointer to the master index (StringCache.pIndex)
  and the number of indicies in the master index (StringCache.nIndex). It
  also contains the lowest and highest identifier of the strings which are
  currently loaded in the cache (StringIndex.rangeMin and StringCache.rangeMax).

  The actual strings and their ids are loaded into StringCache.cache[]. When
  the app calls LoadString(), the following takes place :

  First, we see if the StringCache is being used for the very first time,
  or if the user is dealing with a different resource file than the one
  he opened last time. If that is the case, then we will load the master
  index from the new resource file and initialize the StringCache. If we
  are dealing with the same resource file, then a check is made to see
  if the StringCache currently contains strings whose ids intersect the
  id of the desired string. If it doesn't, then we have to do some searchig.
  However, if the id matches a the id of string in the cache, we search
  for that string and copy it into the app-supplied buffer.

  Back to the searching.... we scan the master index for the index block
  which contains the string ids which intersect the id of the desired 
  string. If we find such an index block, then we load the actual strings
  specified by that index block into StringCache.cache[]. If we do not
  find the index block, then we return FALSE.
*/

extern void FAR PASCAL _StringCacheInit(UINT, HMODULE, BOOL, UINT, LPINDEX);

STRINGCACHE StringCache = { (UINT) -1, (HMODULE) -1, -1, 0, 0, NULL };


VOID FAR PASCAL _StringCacheInit(UINT idStr, HMODULE hModule, BOOL bOpnd, 
                                 UINT nIndex, LPINDEX pIndex)
{
  int  i;

  /*
    Zero out all of the entries
  */
  for (i = 0;  i < CACHESIZE;  i++)
    if (StringCache.cache[i].pszString)
    {
      MYFREE_FAR(StringCache.cache[i].pszString);
      StringCache.cache[i].pszString = NULL;
      StringCache.cache[i].idString  = (UINT) -1;
    }

  StringCache.rangeMin = (idStr & ~(CACHESIZE-1));
  StringCache.rangeMax = StringCache.rangeMin + CACHESIZE - 1;
  StringCache.hModule  = hModule;
  StringCache.bStringOpened = bOpnd;
  StringCache.nIndex   = nIndex;
  StringCache.pIndex   = pIndex;
}

VOID FAR PASCAL _StringCacheClose(void)
{
  if (StringCache.bStringOpened && StringCache.hModule != -1)
    close(StringCache.hModule & 0xFF);
  StringCache.bStringOpened = 0;
  StringCache.hModule       = -1;
}


#if !defined(__DLL__)
int FAR PASCAL LoadString(hModule, idStr, pBuffer, nMax)
  HANDLE hModule;
  INT  idStr;
  LPSTR pBuffer;
  UINT nMax;
{
  int   nStrings;
  int   nIndex;
  LPSTR pData, pDataEnd;
  LPSTR pStrings;
  LPSTR s;
  int   rc = FALSE;
  int   len;
  BOOL  opnd = FALSE;

  UINT  widStr = (UINT) idStr;  /* to prevent signed/unsigned mismatches */


  /*
    Open the resource file if we are opening an external resource file that
    is not the same as the previously opened external resource file, or
    if we are opening the EXE file for the first time.
  */
  if ((int) hModule != StringCache.hModule && 
      (hModule != 0 || StringCache.hModule == -1))
  {
    /*
      Free the old master index
    */
    if (StringCache.pIndex)
    {
      MYFREE_FAR((LPSTR) StringCache.pIndex - sizeof(UINT));
      StringCache.pIndex = NULL;
    }

    /*
      Close the old resource file
    */
    if (StringCache.bStringOpened && StringCache.hModule != -1)
    {
      close(StringCache.hModule & 0xFF);
      StringCache.bStringOpened = 0;
      StringCache.hModule = -1;
    }

    /*
      Open the EXE file
    */
    if (hModule == 0)
    {
      if ((hModule = OpenResourceFile(NULL)) == 0)
        return FALSE;
      opnd = TRUE;
    }

    /*
      Load the master index
    */
    if ((pData = GetResource(hModule, (LPSTR) 0, RT_STRING)) == NULL)
    {
#ifdef DEBUG_LOADSTRING
      printf("LoadString() : GetResource can't find string table\n");
#endif
      return FALSE;
    }

    _StringCacheInit(widStr, hModule, opnd, * (UINT FAR *) pData,
                     (LPINDEX) (pData+sizeof(UINT)));
  }

  else if (widStr < StringCache.rangeMin || widStr > StringCache.rangeMax)
  {
    /*
      The string isn't in the cache range.... load the strings again.
    */
    _StringCacheInit(widStr, StringCache.hModule, FALSE, StringCache.nIndex, 
                     StringCache.pIndex);
  }

  else
  {
    /*
      The string is probably in the cache. Search all of the string
      in StringCache.cache[] for the one with the desired id.
    */
    for (nStrings = CACHESIZE - 1;  nStrings >= 0;  nStrings--)
      if (StringCache.cache[nStrings].idString == widStr &&
          (s = StringCache.cache[nStrings].pszString) != NULL)
      {
        /*
          Found! Copy the string to the user buffer & return.
        */
        lmemcpy(pBuffer, s, len = min(lstrlen((LPSTR) s), (int) nMax));
        pBuffer[len] = '\0';
#ifdef DEBUG_LOADSTRING
        printf("LoadString() : Found id %d and string [%s]\n", widStr, pBuffer);
#endif
        return len;
      }

    /*
      Didn't find it for some reason.
    */
#ifdef DEBUG_LOADSTRING
    printf("LoadString() : Could not find id %d\n", pBuffer);
#endif
    return FALSE;
  }


  /*
    Find the proper index block within the master index.
  */
  for (nIndex = StringCache.nIndex-1;  nIndex >= 0;  nIndex--)
  {
    if (StringCache.pIndex[nIndex].minRange == StringCache.rangeMin)
    {
      /*
        Seek to the proper place in the resource file and read the
        string information in.
      */
      UINT len = (UINT) (StringCache.pIndex[nIndex].ulSeekPosEnd - 
                                      StringCache.pIndex[nIndex].ulSeekPos);
      lseek(StringCache.hModule & 0xff, StringCache.pIndex[nIndex].ulSeekPos, 0);
      if ((pStrings = pData = EMALLOC_FAR(len)) == NULL)
        return FALSE;
      _lread(StringCache.hModule & 0xFF, pData, len);
      pDataEnd = pData + len;
      break;
    }
  }

  if (nIndex < 0)   /* couldn't find it */
  {
#ifdef DEBUG_LOADSTRING
    printf("LoadString() : Could not find id %d, index is < 0\n", pBuffer);
#endif
    return FALSE;
  }

  /*
    Go through the strings in this index block and add them to the
    string cache. If, while doing this, we happen upon the desired
    string, then copy it into the app-supplied buffer and set the
    return value to the length of the string.
  */
  while (pStrings < pDataEnd)
  {
    LPRCSTRING pRCString = (LPRCSTRING) pStrings;
    UINT     idRCString = pRCString->idString;

    /*
      See if this string is a candidate for the cache. If so, save it.
      Map the string into entries 0...CACHESIZE of the cache
    */
    StringCache.cache[idRCString & (CACHESIZE-1)].idString = idRCString;
    StringCache.cache[idRCString & (CACHESIZE-1)].pszString = s =
                                   EMALLOC_FAR(pRCString->nChars + 1);
    if (s == NULL)
      return FALSE;
    lmemcpy(s, pRCString->string, pRCString->nChars);
    s[pRCString->nChars] = '\0';

    if ((UINT) pRCString->idString == widStr)
    {
      /*
        Found! Copy the string to the user buffer & cache more strings.
      */
      lmemcpy(pBuffer, pRCString->string, len = min(pRCString->nChars, nMax));
      pBuffer[len] = '\0';
      rc = len;
    }
    pStrings += 
      sizeof(pRCString->idString)+sizeof(pRCString->nChars)+pRCString->nChars;
  }


  /*
    Free the 'pData' allocated by the EMALLOC_FAR above, but do not free
    it if it is the master index block as loaded by GetResource.
  */
  if (pData && pData != (LPSTR) StringCache.pIndex)
    MYFREE_FAR(pData);
#ifdef DEBUG_LOADSTRING
  printf("LoadString() : Found id %d and string [%s]\n", widStr, pBuffer);
#endif
  return rc;
}
#endif

