/*===========================================================================*/
/*                                                                           */
/* File    : WINATOM.C                                                       */
/*                                                                           */
/* Purpose : Atom Management functions for MEWEL                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

typedef struct tagHashTable
{
  UINT nSize;
  LIST **apList;  /* array of 'nSymbols' pointers to lists */
} HASHTABLE;

HASHTABLE HashTable = { 0, NULL };

#define DEFAULT_HASH_SIZE   37

typedef struct tagAtom
{
  LPSTR pszAtomName;   /* pointer to atom name */
  ATOM  iAtom;         /* atom value */
} ATOMINFO, *PATOMINFO, FAR *LPATOMINFO;

/*
extern BOOL FAR PASCAL InitAtomTable(int);
extern ATOM FAR PASCAL AddAtom(BYTE *);
extern ATOM FAR PASCAL DeleteAtom(ATOM);
extern ATOM FAR PASCAL FindAtom(BYTE *);
extern MEMHANDLE FAR PASCAL GetAtomHandle(ATOM);
extern UINT FAR PASCAL GetAtomName(ATOM, BYTE *, INT);
*/

extern UINT       FAR PASCAL _Hash(LPCSTR);
extern LPATOMINFO FAR PASCAL _AtomAlloc(LPSTR, UINT);
extern LPATOMINFO FAR PASCAL _GetAtomHandle(ATOM);


/****************************************************************************/
/*                                                                          */
/* Function : InitAtomTable()                                               */
/*                                                                          */
/* Purpose  : Creates an atom table with nSize buckets. This functions      */
/*            must be called prior to any other atom function.              */
/*                                                                          */
/* Returns  : TRUE if created, FALSE if not.                                */
/*                                                                          */
/****************************************************************************/

BOOL FAR PASCAL InitAtomTable(nSize)
  int nSize;
{
  if (HashTable.apList)
    MyFree(HashTable.apList);

  /*
    The max table size is 255. This way, we can encode the atom value
    in a single word as (Bucket * 256) + (max id in bucket + 1).
  */
  if (nSize > 255)
    nSize = 255;

  if ((HashTable.apList = (LIST**) emalloc((HashTable.nSize = nSize) * sizeof(LIST *))) == NULL)
    return FALSE;
  else
    return TRUE;
}

/****************************************************************************/
/*                                                                          */
/* Function : _Hash()                                                       */
/*                                                                          */
/* Purpose  : Given a string, computes the string's hash value.             */
/*                                                                          */
/* Returns  : The hash value. (A bucket number from 0 to nSize-1)           */
/*                                                                          */
/****************************************************************************/

UINT FAR PASCAL _Hash(lpsz)
  LPCSTR lpsz;
{
  UINT h;

  if (!HashTable.apList)
    InitAtomTable(DEFAULT_HASH_SIZE);

  for (h = 0;  *lpsz;  h += *lpsz++)
    ;
  return (h % HashTable.nSize);
}

/****************************************************************************/
/*                                                                          */
/* Function : _AtomAlloc()                                                  */
/*                                                                          */
/* Purpose  : Allocates an atom data structure. We pass the atom string and */
/*            a bucket number (hash value). We encode the atom number as :  */
/*                ATOM = (Bucket * 256) + (max value in bucket + 1)         */
/*                                                                          */
/* Returns  : A pointer to the atom structure.                              */
/*                                                                          */
/****************************************************************************/

LPATOMINFO FAR PASCAL _AtomAlloc(lpszAtom, h)
  LPSTR lpszAtom;
  UINT  h;
{
  LIST     *pList;
  LPATOMINFO pa;
  UINT      iMax = 0;

  if ((pa = (LPATOMINFO) emalloc(sizeof(ATOMINFO))) == NULL)
    return NULL;

  for (pList = HashTable.apList[h];  pList;  pList = pList->next)
    iMax = max(iMax, ((LPATOMINFO) pList->data)->iAtom);

  pa->pszAtomName = lstrsave(lpszAtom);
  pa->iAtom = ((h << 8) + 0xC000) | (iMax + 1);
  return pa;
}


/****************************************************************************/
/*                                                                          */
/* Function : AddAtom()                                                     */
/*                                                                          */
/* Purpose  : Adds a string to the atom table.                              */
/*                                                                          */
/* Returns  : The string's atom number.                                     */
/*                                                                          */
/****************************************************************************/

ATOM FAR PASCAL AddAtom(lpsz)
  LPCSTR lpsz;
{
  UINT  h = _Hash(lpsz);  /* this will be the bucket number */
  LPATOMINFO pAtom;
  ATOM      iAtom;

  if ((iAtom = FindAtom(lpsz)) == 0)
  {
    if ((pAtom = _AtomAlloc((LPSTR) lpsz, h)) == NULL)
      return (ATOM) 0;
    ListAdd(&HashTable.apList[h], ListCreate((LPSTR) pAtom));
    return pAtom->iAtom;
  }
  else
    return iAtom;
}

/****************************************************************************/
/*                                                                          */
/* Function : DeleteAtom()                                                  */
/*                                                                          */
/* Purpose  : Given an atom number, deletes the atom from the atom table.   */
/*                                                                          */
/* Returns  : The atom number if successful, and 0 if not successful.       */
/*                                                                          */
/****************************************************************************/
ATOM FAR PASCAL DeleteAtom(nAtom)
  ATOM nAtom;
{
  UINT h = (UINT) (((nAtom-0xC000) >> 8) & 0x00FF);
  LIST *pList;

  /*
    Sanity check.... make sure that it is a valid atom
  */
  if (nAtom < 0xC000)
    return (ATOM) 0;

  /*
    If we are here for the first time, then make sure the atom table is valid
  */
  if (!HashTable.apList)
    InitAtomTable(DEFAULT_HASH_SIZE);

  /*
    Go through the chains in this bucket. When we find the atom, remove
    it from the chain and free the name array.
  */
  for (pList = HashTable.apList[h];  pList;  pList = pList->next)
    if (((LPATOMINFO) pList->data)->iAtom == nAtom)
    {
      MYFREE_FAR(((LPATOMINFO) pList->data)->pszAtomName);
      ListDelete(&HashTable.apList[h], pList);
      return nAtom;
    }
  return (ATOM) 0;
}

/****************************************************************************/
/*                                                                          */
/* Function : FindAtom()                                                    */
/*                                                                          */
/* Purpose  : Given a string, returns the corresponding atom number         */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

ATOM FAR PASCAL FindAtom(lpsz)
  LPCSTR lpsz;
{
  UINT h = _Hash(lpsz);
  LIST *pList;

  for (pList = HashTable.apList[h];  pList;  pList = pList->next)
    if (!lstrcmp(((LPATOMINFO) pList->data)->pszAtomName, (LPSTR) lpsz))
      return ((LPATOMINFO) pList->data)->iAtom;
  return (ATOM) 0;
}

/****************************************************************************/
/*                                                                          */
/* Function : _GetAtomHandle()                                              */
/*                                                                          */
/* Purpose  : Given an atom number, return the atom structure for that item.*/
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

LPATOMINFO FAR PASCAL _GetAtomHandle(nAtom)
  ATOM nAtom;
{
  UINT h = (UINT) (((nAtom-0xC000) >> 8) & 0x00FF);
  LIST *pList;

  if (!HashTable.apList)
    InitAtomTable(DEFAULT_HASH_SIZE);

  for (pList = HashTable.apList[h];  pList;  pList = pList->next)
    if (((LPATOMINFO) pList->data)->iAtom == nAtom)
      return ((LPATOMINFO) pList->data);
  return (LPATOMINFO) NULL;
}

/****************************************************************************/
/*                                                                          */
/* Function : GetAtomHandle()                                               */
/*                                                                          */
/* Purpose  : Given an atom number, returns a ptr to the string.            */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

GLOBALHANDLE FAR PASCAL GetAtomHandle(nAtom)
  ATOM nAtom;
{
  LPATOMINFO p = _GetAtomHandle(nAtom);
  /* WINATOM.c(248) : warning C4059: segment lost in conversion */
#if defined(DOS)
  return (p) ? ((GLOBALHANDLE) FP_OFF(p->pszAtomName)) : (GLOBALHANDLE) 0;
#else
  return (p) ? ((GLOBALHANDLE) (UINT) p->pszAtomName) : (GLOBALHANDLE) 0;
#endif
}

/****************************************************************************/
/*                                                                          */
/* Function : GetAtomName()                                                 */
/*                                                                          */
/* Purpose  : Given an atom number, copies the associated atom string into  */
/*            a user-passed buffer.                                         */
/* Returns  : The number of characters copied.                              */
/*                                                                          */
/****************************************************************************/

UINT FAR PASCAL GetAtomName(nAtom, lpBuf, nSize)
  ATOM  nAtom;
  LPSTR lpBuf;
  INT   nSize;
{
  LPATOMINFO p = _GetAtomHandle(nAtom);
  if (p)
  {
    lstrncpy(lpBuf, p->pszAtomName, nSize);
    lpBuf[nSize-1] = '\0';
    return lstrlen(lpBuf);
  }
  else
  {
    *lpBuf = '\0';
    return 0;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : RegisterWindowMessage                                         */
/*                                                                          */
/* Purpose  : A funny place to put this function, but since it drags in the */
/*            atom manager anyway....                                       */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

UINT FAR PASCAL RegisterWindowMessage(lpString)
  LPSTR lpString;
{
  return AddAtom(lpString);
}

