/*===========================================================================*/
/*                                                                           */
/* File    : WINDEBUG.C                                                      */
/*                                                                           */
/* Purpose : Various debugging functions                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


VOID FAR PASCAL OutputDebugString(lpOutputDebugString)
  LPCSTR lpOutputDebugString;
{
#if (defined(DOS) || defined(OS2)) && !defined(PLTNT) && !defined(__DPMI32__)
#if 0
  FILE *f = fopen("app.dbg", "w+");
  if (f)
  {
    fprintf(f, "%s\n", lpOutputDebugString);
    fclose(f);
  }
#else
  fprintf(stdaux, "%s\n", lpOutputDebugString);
#endif
#else
  fprintf(stderr, "%s\n", lpOutputDebugString);
#endif
}

