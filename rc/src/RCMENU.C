/*===========================================================================*/
/*                                                                           */
/* File    : RCMENU.C                                                        */
/*                                                                           */
/* Purpose : Holds the current menu tree.                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "int.h"
#include "rccomp.h"
#include "ytab.h"

typedef struct tagMenuNode
{
  WORD mtOption;
  WORD mtID;
  char *pszItem;
  int  idxSibling;
  int  idxChildren;
} MENUNODE, *PMENUNODE;

static MENUNODE MenuTree[256];
static int CurrNodeCount = 0;


int CreateMenuNode(wFlags, wID, str)
  WORD wFlags;
  WORD wID;
  char *str;
{
  PMENUNODE pNode = &MenuTree[++CurrNodeCount];

  pNode->mtOption   = wFlags;
  pNode->mtID       = wID;
  pNode->pszItem    = str;
  pNode->idxSibling = 0;
  pNode->idxChildren = 0;

  return CurrNodeCount;
}


int LinkMenuNodeToSibling(idxHead, idxNode)
  int idxHead;
  int idxNode;
{
  PMENUNODE p;

  if (idxHead == 0)
    return idxNode;

  p = &MenuTree[idxHead];
  while (p->idxSibling)
    p = &MenuTree[p->idxSibling];
  p->idxSibling = idxNode;
  return idxHead;
}

int LinkMenuNodeToParent(idxParent, idxNode)
  int idxParent;
  int idxNode;
{
  PMENUNODE p;

  if (idxParent == 0)
    return idxNode;

  p = &MenuTree[idxParent];
  p->idxChildren = idxNode;
  return idxParent;
}


void WriteMenuTree(idxNode)
  int idxNode;
{
  PMENUNODE pNode;
  WORD wFlags;
  char *pStr;
  char chNull[2];
  int  len;

  extern int  resFD;

  if (idxNode == 0)
    return;

  pNode = &MenuTree[idxNode];

  /*
    Write the flags
  */
  wFlags = pNode->mtOption;
  if (pNode->idxSibling == 0)
    wFlags |= MF_END;
  nwrite(resFD, (char *) &wFlags, sizeof(wFlags));

  pStr = pNode->pszItem;;
  if (!pStr)
  {
    chNull[0] = '\0';
    pStr = chNull;
  }
  len = word_pad(strlen(pStr)+1);

  if (wFlags & MF_POPUP)
  {
    /*
      Write the string, then recurse on the children
    */
#if 0
printf("Popup [%s], flags [%x]\n", pStr, wFlags);
#endif
    nwrite(resFD, pStr, len);
    WriteMenuTree(pNode->idxChildren);
  }
  else
  {
    /*
      Write the id and the string
    */
#if 0
printf("Item [%s], flags [%x], id [%d]\n", pStr, wFlags, pNode->mtID);
#endif
    nwrite(resFD, (char *) &pNode->mtID, sizeof(pNode->mtID));
    nwrite(resFD, pStr, len);
  }

  /*
    Recurse on the next sibling
  */
  WriteMenuTree(pNode->idxSibling);
}


void FreeMenuTree(void)
{
#if 0
  if (pNode == NULL)
    return;

  /*
    First, destroy the children, then the sibling, then the node itself
  */
  FreeMenuTree(pNode->pChildren);
  FreeMenuTree(pNode->pSibling);
  free((char *) pNode);
#endif

  CurrNodeCount = 0;
}

