/*===========================================================================*/
/*                                                                           */
/* File    : WISCHAR.C                                                       */
/*                                                                           */
/* Purpose : Implements the IsCharXXX() functions                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

BOOL FAR PASCAL IsCharAlpha(cChar)
  UINT cChar;  /* should really be a char, but some compilers can't hack it */
{
  return isalpha(cChar);
}

BOOL FAR PASCAL IsCharAlphaNumeric(cChar)
  UINT cChar;  /* should really be a char, but some compilers can't hack it */
{
  return isalnum(cChar);
}

BOOL FAR PASCAL IsCharLower(cChar)
  UINT cChar;  /* should really be a char, but some compilers can't hack it */
{
  return islower(cChar);
}

BOOL FAR PASCAL IsCharUpper(cChar)
  UINT cChar;  /* should really be a char, but some compilers can't hack it */
{
  return isupper(cChar);
}


