/*===========================================================================*/
/*                                                                           */
/* File    : WSELECTR.C                                                      */
/*                                                                           */
/* Purpose : Stubs for the Windows selector functions                        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if defined(DOS286X)

#undef APIENTRY
#define APIENTRY pascal _far
typedef unsigned short SEL;
typedef unsigned short _far *PSEL;

typedef struct
{
  ULONG  base;	/* Segment linear base address */
  ULONG  size;	/* Size in bytes of segment */
  USHORT attrib;/* Attribute byte */
#define CODE16		1 /* Code segment */
#define DATA16		2 /* Data segment */
#define CODE16_NOREAD	3 /* Execute only code segment */
#define DATA16_NOWRITE	4 /* Read only data segment */
} DESC, FAR *PDESC;

USHORT APIENTRY DosCreateCSAlias(SEL dsel, PSEL cselp);
USHORT APIENTRY DosCreateDSAlias(SEL sel,  PSEL aselp);
USHORT APIENTRY DosFreeSeg(SEL sel);
USHORT APIENTRY DosGetSegDesc(SEL sel, PDESC descp);
USHORT APIENTRY DosSetSegAttrib(SEL sel, USHORT attrib);
#endif


/*
  These 4 functions are used by Borland's OWL
*/
UINT FAR PASCAL FreeSelector(sel)
  UINT sel;
{
  (void) sel;
#if defined(DOS286X)
  DosFreeSeg(sel);
#endif
  return 0;  /* 0 means success */
}


UINT FAR PASCAL AllocDStoCSAlias(sel)
  UINT sel;
{
#if defined(DOS286X)
  USHORT newSel;
  if (DosCreateCSAlias(sel, &newSel) != 0)
    return 0;
  return (UINT) newSel;
#else
  return sel;
#endif
}

UINT FAR PASCAL AllocCStoDSAlias(sel)
  UINT sel;
{
#if defined(DOS286X)
  USHORT newSel;
  if (DosCreateDSAlias(sel, &newSel) != 0)
    return 0;
  return (UINT) newSel;
#else
  return sel;
#endif
}

UINT FAR PASCAL PrestoChangoSelector(sel1, sel2)
  UINT sel1, sel2;
{
#if defined(DOS286X)
  DESC desc;
  unsigned short usAttr;

  if (DosGetSegDesc(sel1, &desc))
    return 0;
  usAttr = (desc.attrib==CODE16 || desc.attrib==CODE16_NOREAD) ? DATA16 : CODE16;
  if (DosSetSegAttrib(sel2, usAttr))
    return 0;
  return sel2;
#else
  (void) sel2;
  return sel1;
#endif
}

