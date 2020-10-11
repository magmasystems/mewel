/*===========================================================================*/
/*                                                                           */
/* File    : WSPRINTF.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

#include <stdarg.h>


#if defined(wsprintf)
#undef wsprintf
#endif

#if defined(wvsprintf)
#undef wvsprintf
#endif

#if (MODEL_NAME == MODEL_LARGE)
int FAR CDECL wsprintf(LPSTR s, LPCSTR fmt, ...)
#else
int FAR CDECL wsprintf(PSTR s, PCSTR fmt, ...)
#endif
{
  va_list vaStart;
  int     rc;

  va_start(vaStart, fmt);
  rc = vsprintf(s, fmt, vaStart);
  va_end(vaStart);
  return rc;
}

#if (MODEL_NAME == MODEL_MEDIUM)
int WINAPI wvsprintf(PSTR s, PSTR fmt, PSTR args)
#else
int WINAPI wvsprintf(LPSTR s, LPCSTR fmt, CONST void FAR* args)
#endif
{
#if defined(__WATCOMC__)
  return sprintf(s, fmt, args);
#else
  return vsprintf(s, fmt, (va_list) args);
#endif
}
