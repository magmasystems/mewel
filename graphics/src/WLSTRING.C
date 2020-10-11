/*===========================================================================*/
/*                                                                           */
/* File    : LSTRING.C                                                       */
/*                                                                           */
/* Purpose : Far ptr equivalents of standard C string and mem functions.     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

#if !defined(sco)
#include <stdlib.h>
#endif

#ifndef lmemcpy
#if 0
/*
The lmemcpy() routine is implemented in assembler.  See LMEMCPY.ASM.
*/
LPSTR FAR PASCAL lmemcpy(dest, src, n)
  LPSTR dest;
  LPSTR src;
  UINT n;
{
  LPSTR orig_dest;

  orig_dest = dest;
  while (n-- > 0)
    *dest++ = *src++;

  return orig_dest;
}
#endif
#endif


#ifndef lmemset
#if defined(DOS) && !defined(M_I86LM) && !defined(M_I86HM) && !defined(__LARGE__) && !defined(DOS386) && !defined(WC386)
LPSTR FAR PASCAL lmemset(s, c, n)
  LPSTR s;
  int c;
  UINT n;
{
  LPSTR orig_s;

  orig_s = s;
  while (n-- > 0)
    *s++ = (BYTE) c;
  return orig_s;
}
#endif
#endif


#if !defined(DOS) && !defined(OS2) && !defined(UNIX)
#ifndef	lstrchr
LPSTR FAR PASCAL lstrchr(s, c)
  LPCSTR s;
  int  c;
{
  while (*s != '\0' && *s != (BYTE) c)
    s++;
  return (*s == (BYTE) c) ? s : NULL;
}
#endif

#ifndef lstrcpy
LPSTR FAR PASCAL lstrcpy(s1, s2)
  LPSTR s1;
  LPCSTR s2;
{
  LPSTR orig_s1 = s1;
  while ((*s1++ = *s2++) != '\0') ;
  return orig_s1;
}
#endif

#ifndef lstrncpy
LPSTR FAR PASCAL lstrncpy(s1, s2, n)
  LPSTR s1;
  LPCSTR s2;
  UINT  n; 
{
  LPSTR orig_s1 = s1;

  if (n > 0)
  {
    while (n-- > 0 && (*s1++ = *s2++) != '\0') ;
    *--s1 = '\0';
  }
  return orig_s1;
}
#endif

#ifndef lstrlen
INT FAR PASCAL lstrlen(s)
  LPCSTR s;
{
  register INT len = 0;
  while (*s++)  len++;
  return len;
}
#endif
#endif

#if defined(__BORLANDC__) && defined(__DLL__)
LPSTR FAR PASCAL lstrcpyn(s1, s2, n)
  LPSTR s1;
  LPCSTR s2;
  UINT  n; 
{
  return lstrncpy(s1, s2, n);
}
#endif


#if defined(DOS)

#ifndef lstrcat
LPSTR FAR PASCAL lstrcat(s1, s2)
  LPSTR s1;
  LPCSTR s2;
{
#if (MODEL_DATA_PTR == MODEL_FAR)
  /*
    Try to use intrinsic functions if we are in large model
  */
  return (PSTR) strcat((char *) s1, (char *) s2);  
#else
  LPSTR orig_s1 = s1;
  while (*s1++)
    ;
  lstrcpy(s1 - 1, s2);
  return orig_s1;
#endif
}
#endif


#ifndef lstrcmp
int FAR PASCAL lstrcmp(s1, s2)
  LPCSTR s1;
  LPCSTR s2;
{
#if (MODEL_DATA_PTR == MODEL_FAR)
  /*
    Try to use intrinsic functions if we are in large model
  */
  return strcmp((char *) s1, (char *) s2);  
#else
  while (*s1 && *s1 == *s2)
    s1++, s2++;
  return (*s1 - *s2);
#endif
}
#endif

#endif /* DOS */


int FAR PASCAL lstricmp(s1, s2)
  LPCSTR s1;
  LPCSTR s2;
{
  while (*s1 && toupper(*s1) == toupper(*s2))
    s1++, s2++;
  return (toupper(*s1) - toupper(*s2));
}

int FAR PASCAL lstrcmpi(s1, s2)
  LPCSTR s1;
  LPCSTR s2;
{
  return lstricmp(s1, s2);
}

int FAR PASCAL lstrnicmp(s1, s2, n)
  LPCSTR s1;
  LPCSTR s2;
  UINT n;
{
  while (--n && *s1 && toupper(*s1) == toupper(*s2))
    s1++, s2++;
  return (toupper(*s1) - toupper(*s2));
}


PSTR FAR PASCAL strcpy_until(dest, src, delim)
  PSTR dest, src;
  int  delim;
{
  while ((*dest = *src) != (BYTE) delim && *dest)
    dest++, src++;
  *dest = '\0';
  return src;
}

PSTR FAR PASCAL next_int_token(s, n)
  PSTR s;
  int  *n;
{
  if ((s = next_token(s)) != NULL)
    *n = atoi((char *) s);
  return s; 
}

PSTR FAR PASCAL next_token(s)
  PSTR s;
{
  if ((s = span_chars(s)) != NULL && (s = span_blanks(s)) != NULL)
    return s;
  else
    return NULL;
}

/* span_blanks - goes past white space on one line */
PSTR FAR PASCAL span_blanks(pos)
  register PSTR pos;
{
  for ( ; isspace(*pos); pos++) ;
  return((*pos) ? pos : NULL);
}

/* span_chars - goes past non-whitespace */
PSTR FAR PASCAL span_chars(pos)
  register PSTR pos;
{
  for ( ; !isspace(*pos); pos++)
    if (!*pos) return(NULL);
  return(pos);
}


PSTR FAR PASCAL rtrim(s)
  PSTR s;
{
  PSTR t;
  for (t = s + strlen((char *) s) - 1;  t > s && *t == ' ';  *t-- = '\0')  ;
  return s;
}


LPSTR FAR PASCAL lstrupr(lpStr)
  LPSTR lpStr;
{
  LPSTR lpOrig = lpStr;

  while (*lpStr)
  {
    *lpStr = (BYTE) toupper(*lpStr);
    lpStr++;
  }
  return lpOrig;
}



#if defined(UNIX) || defined(VAXC)

#include <time.h>

char *strlwr(str)
  char *str;
{
  char *sOrig = str;

  while (*str)
  {
    *str = (char) tolower(*str);
    str++;
  }
  return sOrig;
}

char *strrev(str)
  char *str;
{
  char c;
  char *sOrig = str, *sEnd = str + strlen(str) - 1;

  while( sEnd >= str )
    { c = *sEnd; *sEnd-- = *str; *str++ = c; }

  return sOrig;
}

char *strdate(str)
  char *str;
{
  time_t lt;
  struct tm *t;

  time(&lt);
  if ((t = localtime(&lt)) == NULL ||
       sprintf(str, "%.2d/%.2d/%.2d",
               t->tm_mon + 1, t->tm_mday, t->tm_year % 100) < 0)
    str = NULL;

  return str;
}

#if 0
int strncmp(s1, s2, n)
  char *s1, *s2;
  int  n;
{
  while (n-- > 0 && *s1 && *s2 && *s1 == *s2)
    s1++, s2++;
  return *s1 - *s2;
}
#endif

#ifndef strnicmp
int strnicmp(s1, s2, n)
  char *s1, *s2;
  int  n;
{
  while (--n && *s1 && toupper(*s1) == toupper(*s2))
    s1++, s2++;
  return (toupper(*s1) - toupper(*s2));
}
#endif

#ifndef stricmp
int stricmp(s1, s2)
  char *s1, *s2;
{
  while (*s1 && toupper(*s1) == toupper(*s2))
    s1++, s2++;
  return (toupper(*s1) - toupper(*s2));
}
#endif
#endif


LPSTR PASCAL SpanString(lpData)
  LPSTR lpData;
{
  int iLen;

  /*
    In Windows, if the string starts with 0xFF, then the next two bytes
    are an ordinal number.
  */
  if (*lpData == 0xFF)
    iLen = 3;
  else
    iLen = lstrlen(lpData) + 1;

  /*
    Is the string padded to word-length?
  */
#if defined(WORD_ALIGNED)
  if (iLen & MISALIGN)
  {
    iLen = (iLen & ~MISALIGN) + ALIGN;
  }
#endif

  return lpData + iLen;
}


#if defined(MEWEL_32BITS)
LPSTR FAR PASCAL lmemmove(LPSTR dest, LPSTR src, UINT len)
{
  return memmove(dest, src, len);
}

#if defined(sunos)
void *memmove(void *dest, const void *src, size_t len)
{
  void *orig_dest = dest;

  if (FP_OFF(dest) <= FP_OFF(src))  /* dest is to the left */
  {
    lmemcpy(dest, src, len);
  }
  else    /* dest is to the right of the source -- take care of overlap */
  {
    len >> 1;  /* get number of words to move */
    dest = (char *) dest + len;
    src  = (char *) src + len;
    while (len--)
      *--(char *)dest = *--(char *)src;
  }
  return orig_dest;
}
#endif /* sunos */
#endif

