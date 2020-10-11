/*===========================================================================*/
/*                                                                           */
/* File    : WLNKLIST.C                                                      */
/*                                                                           */
/* Purpose : Generic linked-list routines                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL ListInsert(headptr, element, elBefore)
  LIST **headptr;
  LIST *element;
  LIST *elBefore;               /* if NULL, insert at end */
{
  if (*headptr == (LIST *) NULL)         /* the list is empty */
  {
    *headptr = element;
  }
  else if (!elBefore)
    ListAdd(headptr, element);
  else if (elBefore == *headptr)	/* Inserting before the first element */
  {
    element->next = elBefore;
    *headptr = element;
  }
  else
  {
    register LIST *p, *prevp;

    /* Walk down the chain until we hit the element to insert p in
       front of. Link the new element to the tail of the last element.
    */
    for (prevp = p = *headptr;  p != elBefore && p->next;  p = p->next)
      prevp = p;

    prevp->next = element;
    element->next = elBefore;
  }
}

/*===========================================================================*/
/*                                                                           */
/* File    : LISTLEN.C                                                       */
/*                                                                           */
/* Purpose : Returns the length of a linked list                             */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL ListGetLength(headptr)
  LIST *headptr;
{
  register LIST *p;
  register int  i = 1;

  if (!headptr)
    return 0;

  for (p = headptr;  p->next;  p = p->next, i++)  ;
  return i;
}

/*===========================================================================*/
/*                                                                           */
/* File    : LISTADD.C                                                       */
/*                                                                           */
/* Purpose : Append an element to the end of the list                        */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL ListAdd(headptr, element)
  LIST **headptr;
  LIST *element;
{
  if (*headptr == (LIST *) NULL)         /* the list is empty */
  {
    *headptr = element;
  }
  else
  {
    register LIST *p;

    /* Walk down the chain until we hit the last element. Link */
    /*  the new element to the tail of the last element.       */
    for (p = *headptr;  p->next;  p = p->next)  ;
    p->next = element;
  }
  element->next = (LIST *) NULL;
}

/*===========================================================================*/
/*                                                                           */
/* File    : LISTGETN.C                                                      */
/*                                                                           */
/* Purpose : Returns a pointer to the Nth element of the list                */
/*                                                                           */
/*===========================================================================*/
LIST *FAR PASCAL ListGetNth(headptr, n)
  LIST *headptr;
  int  n;
{
  LIST *p;
  int  i;

  if (headptr == (LIST *) NULL)
    return (LIST *) NULL;

  for (i = 0, p = headptr;  p && i < n;  p = p->next, i++)  ;
  return p;
}

/*===========================================================================*/
/*                                                                           */
/* File    :                                                                 */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/*===========================================================================*/
LIST *FAR PASCAL ListFindData(head, data)
  LIST *head;
  LPSTR  data;
{
  register LIST *p;
  for (p = head;  p && p->data != data;  p = p->next)  ;
  return p;
}


/*===========================================================================*/
/*                                                                           */
/* File    : LISTDEL.C                                                       */
/*                                                                           */
/* Purpose : Deletes the nth element from a list                             */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL ListDelete(head, element)
  LIST **head;
  LIST *element;
{
  register LIST *p, *prev;

#ifdef NOTDEF
  sprintf(dline, "head %x *head %x element %x", head, *head, element);
  DEBUG("ListDelete");
#endif

  for (p = prev = *head;  p && p != element;  prev = p, p = p->next)  ;

  if (p)        /* p is the element to delete */
  {
    if (p == *head)
    {
      *head = p->next;
    }
    else
      prev->next = p->next;
    MYFREE_FAR(p->data);
    MyFree(p);
  }
}
/*===========================================================================*/
/*                                                                           */
/* File    : LISTFREE.C                                                      */
/*                                                                           */
/* Purpose : Frees a linked link                                             */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL ListFree(headptr, bFreeData)
  LIST **headptr;
  int  bFreeData;
{
  LIST *p, *savep;
  
  for (p = *headptr;  p;  p = savep)
  {
    savep = p->next;
    if (bFreeData)
      MYFREE_FAR(p->data);
    MyFree(p);
  }
  *headptr = (LIST *) NULL;
  return 1;
}

/*===========================================================================*/
/*                                                                           */
/* File    : LISTCRE.C                                                       */
/*                                                                           */
/* Purpose : Creates a list element                                          */
/*                                                                           */
/*===========================================================================*/
LIST *FAR PASCAL ListCreate(data)
  LPSTR data;
{
  LIST *p = (LIST *) emalloc(sizeof(LIST));
  if (p)
  {
    p->data = data;
    p->next = (LIST *) NULL;
  }
  return p;
}

