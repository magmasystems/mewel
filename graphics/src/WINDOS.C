/*===========================================================================*/
/*                                                                           */
/* File    : WINDOS.C                                                        */
/*                                                                           */
/* Purpose : Some of the DOS specific calls in Windows are implemented here. */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

#if (defined(MSC) || defined(__TSC__)) && !defined(__ZTC__)
#define MEMALLOCED                  0
#define GOODFREE                    0
#elif defined(__TURBOC__)
#define _dos_allocmem(paras, sel)   allocmem((unsigned) paras, (unsigned FAR *) (sel))
#define _dos_freemem(sel)           freemem(sel)
#define MEMALLOCED                  -1
#define GOODFREE                    0
#elif defined(__ZTC__)
#define _dos_allocmem(paras, sel)   dos_alloc(paras)
#define _dos_freemem(sel)           dos_free(sel)
#define GOODFREE                    0
#endif


DWORD FAR PASCAL GlobalDosAlloc(dwBytes)
  DWORD dwBytes;
{
  WORD wPara;

#if defined(DOS)
#if defined(__ZTC__)
  if ((wPara = _dos_allocmem((WORD) ((dwBytes + 15) >> 4), &wPara)) != 0)
#else
  if ((_dos_allocmem((unsigned) ((dwBytes + 15) >> 4), (unsigned *) &wPara))
         == MEMALLOCED)
#endif
    return MAKELONG(wPara, wPara);
  else
    return 0L;

#else
  return 0L;
#endif
}

UINT FAR PASCAL GlobalDosFree(wSelector)
  UINT wSelector;
{
#if defined(DOS)
  return (_dos_freemem(wSelector) == GOODFREE) ? wSelector : 0;
#else
  return 0;
#endif
}


LPSTR FAR PASCAL GetDOSEnvironment(void)
{
#if defined(DOS)
  return (LPSTR) MK_FP( *((LPWORD) MK_FP(_psp, 0x2C)), 0);
#else
  return NULL;
#endif
}


