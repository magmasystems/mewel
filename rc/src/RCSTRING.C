/*===========================================================================*/
/*                                                                           */
/* File    : RCSTRING.C                                                      */
/*                                                                           */
/* Purpose : Handles STRINGTABLE entries.                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "int.h"
#include "rccomp.h"

#define CACHESIZE     16

#define MAXSTRINGS    256
#define MAXINDEX      256

typedef struct index
{
  WORD minRange,
       maxRange;
  unsigned long ulSeekPos;
  unsigned long ulSeekPosEnd;
} INDEX;
INDEX  *Index;


typedef struct stringinfo
{
  WORD idxSymbol;   /* symbol table index of the string */
  WORD idString;    /* id of the string */
} STRINGINFO;

STRINGINFO *StringInfo;
static WORD nStrings = 0,
            maxStrings = 0;
static WORD maxIndex = 0;

StringInfoInit()
{
  static BOOL bDidInit = FALSE;

  if (bDidInit)
    return TRUE;

  StringInfo = (STRINGINFO *) calloc(MAXSTRINGS * sizeof(STRINGINFO), 1);
  maxStrings = MAXSTRINGS;
  Index = (INDEX *) calloc(MAXINDEX * sizeof(INDEX), 1);
  maxIndex = MAXINDEX;
  bDidInit = TRUE;
  return TRUE;
}

StringInfoAddString(idxSymbol, idString)
  int idxSymbol, idString;
{
  if (nStrings >= maxStrings)
  {
    maxStrings += MAXSTRINGS;
    StringInfo=(STRINGINFO*) realloc(StringInfo,maxStrings*sizeof(STRINGINFO));
    if (StringInfo == NULL)
    {
      fprintf(stderr, "Too many strings. Tried to allocate %d.\n", maxStrings);
      exit(1);
    }
  }

  StringInfo[nStrings].idxSymbol = idxSymbol;
  StringInfo[nStrings].idString  = (WORD) idString;
  nStrings++;
}


int StringInfoDumpStrings()
{
  /*
    1) Sort the strings by the id values.
    2) Traverse the stringinfo list, organizing the strings into groups
       of n through (n + GROUPSIZE - 1). For each group, we want to write
       an index.
    3) Dump all of the strings. We dump the id, the length, and the string
       without the null-terminator.
  */

  extern int resFD;
  int    i;
  int    iStart;
  unsigned long ulSeekPos = 0L;
  unsigned long ulStartingSeekPos;
  unsigned int  uIndexBytes;
  int    nIndex;

  /*
    Sort the strings according to their ids
  */
  qsort(StringInfo, nStrings, sizeof(STRINGINFO), StringInfoComp);

  /*
    This loop fills in the Index[] structure. it calculates the
    min and max id value which each index covers, along with
    beginning and ending file-seek position of the strings within
    the index.
  */
  for (i = nIndex = 0;  i < nStrings;  i++, nIndex++)
  {
    if (nIndex >= maxIndex)
    {
      maxIndex += MAXINDEX;
      Index = (INDEX *) realloc(Index, maxIndex*sizeof(INDEX));
      if (Index == NULL)
      {
        fprintf(stderr, "Too many indicies. Tried to allocate %d.\n", maxIndex);
        exit(1);
      }
    }

    /*
      Compute the min and max id values in this group
    */
    Index[nIndex].minRange  = StringInfo[i].idString & ~(CACHESIZE-1);
    Index[nIndex].maxRange  = Index[nIndex].minRange + (CACHESIZE-1);
    Index[nIndex].ulSeekPos = ulSeekPos;
    if (nIndex > 0)
      Index[nIndex-1].ulSeekPosEnd = ulSeekPos;
    iStart = i;

    /*
      Search for the next string above maxRange
    */
    for (i++;  i < nStrings && StringInfo[i].idString <= Index[nIndex].maxRange;
         i++)
      ;
    i--;

    /*
      The strings from iStart to i are in this group. Compute the
      seek position of the start of the next group.
    */
    for (  ;  iStart <= i;  iStart++)
    {
      char *str = Symtab[StringInfo[iStart].idxSymbol].u.sval;
#ifdef WORD_ALIGNED /* Keep resource data word-aligned */
      WORD len  = (WORD) word_pad(strlen(str));
#else
      WORD len  = (WORD) strlen(str);
#endif
      ulSeekPos += sizeof(WORD) + sizeof(len) + len;
    }
  }  /* end for */


  /*
    Take care of the final index's seek end position
  */
  if (nIndex > 0)
    Index[nIndex-1].ulSeekPosEnd = ulSeekPos;

  /*
    Dump the indices
  */
  ulStartingSeekPos = tell(resFD);
  uIndexBytes = nIndex * sizeof(INDEX);

  /*
    Write each index out to disk
  */
  for (i = 0;  i < nIndex;  i++)
  {
    /*
      Correct for the absolute seek position
    */
    Index[i].ulSeekPos    += ulStartingSeekPos + uIndexBytes;
    Index[i].ulSeekPosEnd += ulStartingSeekPos + uIndexBytes;
    nwrite(resFD, (char *) &Index[i], sizeof(INDEX));
  }

  /*
    Finally, dump the strings
  */
  for (i = 0;  i < nStrings;  i++)
  {
    char *str = Symtab[StringInfo[i].idxSymbol].u.sval;
#ifdef WORD_ALIGNED /* Keep resource data word-aligned */
    WORD len  = (WORD) word_pad(strlen(str));
#else
    WORD len  = (WORD) strlen(str);
#endif
    nwrite(resFD, (char *) &StringInfo[i].idString, sizeof(WORD));  /* the id */
    nwrite(resFD, (char *) &len, sizeof(len));  /* write the len */
    nwrite(resFD, (char *) str,  len);          /* write the string */
  }


  if (nIndex >= maxIndex)
  {
    fprintf(stderr, "String index overflow. Only %d indices allowed.\n",
                    maxIndex);
  }

  return nIndex;
}


StringInfoComp(s1, s2)
  const void *s1, *s2;
{
  /*
    Use a long value to record the result to prevent overflow.
  */
  if (((STRINGINFO *) s1)->idString < ((STRINGINFO *) s2)->idString)
    return -1;
  if (((STRINGINFO *) s1)->idString > ((STRINGINFO *) s2)->idString)
    return 1;
  return 0;
}


/*#define TEST*/

#ifdef TEST
main()
{
  int idxSymbol, idString;
  int i;

  StringInfoInit();

  printf("Enter the numbers...\n");
  while (scanf("%d %d", &idxSymbol, &idString) == 2)
    StringInfoAddString(idxSymbol, idString);

  StringInfoDumpStrings();

  for (i = 0;  i < nStrings;  i++)
  {
    printf("StringInfo[%d]  ID %d  INDEX %d\n", i, StringInfo[i].idString,
                                                   StringInfo[i].idxSymbol);
  }
}
#endif


