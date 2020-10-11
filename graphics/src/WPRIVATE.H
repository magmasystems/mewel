/*===========================================================================*/
/*                                                                           */
/* File    : WPRIVATE.H                                                      */
/*                                                                           */
/* Purpose : Private include file for MEWEL internals                        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#if !defined(_WPRIVATE_H)

#define _WPRIVATE_H

#if defined(__TSC__)
#define DOS        1
#define MSC        1
#define MEWEL_GUI  1
#endif

#if defined(_MSC_VER) || defined(__HIGHC__)
#define MSC        1
#if defined(_MSC_VER)
#if (_MSC_VER >= 700)
#pragma warning(disable:4102 4103 4118 4005 4100)
#endif
#if (_MSC_VER >= 800)
#pragma warning(disable:4746)
#endif
#endif
#endif

#if defined(__ZTC__)
#define MSDOS      1
#endif

#if defined(__WATCOMC__)
#define WC386
#endif

#if defined(PLTNT)
#define PL386
#endif

/*
  MEWEL_32BITS is defined when we are dealing with a 32-bit compiler,
  such as the ones on UNIX, VMS and 32-bits DOS extenders.
*/
#if defined(UNIX) || defined(VAXC) || defined(DOS386) || defined(WC386) || defined(MAC) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
#define MEWEL_32BITS
#endif

/*
  Private include file for UNIX
*/
#if defined(UNIX) || defined(unix)
#include "wunix.h"
#endif

/*
  For ports of MEWEL to native GUIs (like Motif, DecWindows, Mac, Amiga),
  define the constant USE_NATIVE_GUI
*/
#if defined(MOTIF) || defined(DECWINDOWS) || defined(MAC) || defined(AMIGA) || defined(OPENLOOK)
#define USE_NATIVE_GUI   1
#endif

#if defined(MOTIF)
#define XWINDOWS
#define USE_PALETTES
#endif

#if defined(XWINDOWS)
#define USE_WINDOWS_COMPAT_KEYS
#endif

#if defined(OS2) && defined(INCLUDE_OS2)
#define INCL_DOS
#define INCL_SUB
#define INCL_MOU
#include <os2def.h>
#include <bse.h>
#endif /* OS2 */


#ifndef NO_STD_INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if !defined(VAXC)
# include <fcntl.h>
# if defined(__TURBOC__)
#  include <alloc.h>
#  include <mem.h>
#  include <dir.h>
#  include <conio.h>
# elif defined(__ZTC__)
#  include <direct.h>
# else
#  if !defined(MAC)
#  include <malloc.h>
#  endif /* MAC */
#  if !defined(__WATCOMC__)
#   include <memory.h>
#  endif
#  if defined(__GNUC__) && !defined(UNIX)
#    include <dir.h>
#  endif
#  if (defined(__BORLANDC__) || defined(MSC) || defined(__WATCOMC__) || defined(__HIGHC__)) && !defined(UNIX)
#   include <direct.h>
#   if defined(DOS)
#    include <conio.h>
#   endif
#  endif
# endif

# if defined(UNIX)
#  include <sys/types.h>
#  include <sys/stat.h>
#  if defined(INCLUDE_CURSES) && !defined(XWINDOWS)
#   include "mcurses.h"
#   if defined(USE_TERM_H)
#    include <term.h>
#   endif
#  endif
# elif defined(VAXC)
# include <types.h>
# include <stat.h>
# else /* !UNIX && !VAXC */
#  if !defined(MAC)
#  include <dos.h>
#  include <io.h>
#  include <sys\types.h>
#  include <sys\stat.h>
#  endif /* MAC */
# endif

#else  /* defined(VAX) */
# if defined(INCLUDE_CURSES) && !defined(XWINDOWS)
#  include "mcurses.h"
# endif
#endif

#endif  /* NO_STD_INCLUDES */


/*
  Stuff for Pharlap's TNT DOS Extender
*/
#if defined(PLTNT)

#if defined(__WATCOMC__)
#define _M_IX86 200   /* don't let pltypes.h think we are using MSVC/32 */
#endif

#include <pharlap.h>
#include <pldos32.h>

#if !defined(__WATCOMC__)
#define find_t  _find_t
#define int86   _int86
#define int86x  _int86x
#define intdos  _intdos
#define intdosx _intdosx
#define REGS    _REGS
#define SREGS   _SREGS
#define segread(s)
#endif

#undef FP_OFF
#define FP_OFF(s)  ((USHORT)(((ULONG)(s)) & 0xFFFF))
#endif


/*
  GNU C++ compiler
*/
#if defined(__GNUC__) || defined(__DPMI32__) || defined(UNIX)
#define FP_OFF(s)  ((USHORT)(((ULONG)(s)) & 0xFFFF))
#define segread(s)
#endif


/*
  Definition of functions which maniuplate segments and offsets and
  create FAR pointers from segment/offset pairs. Most compilers
  already support these macros, but we must take care of UNIX & VMS.
*/
#if !defined(WC386) && !defined(__HIGHC__)
#ifndef FP_SEG
#define FP_SEG(fp) (*((unsigned FAR *)&(fp)+1))
#endif
#endif

#if defined(__HIGHC__) || defined(PL386)
#define MK_FARP(fp,seg,ofs) \
((*(((unsigned short*)&(fp)) + sizeof(unsigned int)/sizeof(unsigned short))= \
(unsigned short)(seg)), (*(unsigned int *)&(fp) = (unsigned int)(ofs)))
#endif

#if !defined(WC386) && !defined(UNIX)
#ifndef MK_FP
#define MK_FP(seg,ofs)	((void FAR *) \
		   (((unsigned long)(seg) << 16) | (unsigned)(ofs)))
#endif
#endif


/*
  Keywords
*/

#if defined(VAXC)
#define unlink(s) delete(s)
#define tell(f)   lseek(f,0,1)
/* prototypes for vaxstr.c */
char *strdup(char *s);
char *strupr(char *s);
int  stricmp(char *s, char *t);
int  strnicmp(char *s, char *t,int n);
#endif


/*
  Function prototypes
*/
#if defined(UNIX) || defined(VAXC)
extern char *strupr();
extern char *getcwd();
#endif

#if defined(sunos)
/* Subsitute atexit() requires that we intercept all exit() calls. */
#define	exit	win_cleanup_exit
#endif

#if defined(sco) && !defined(PCTEST)
extern char *strupr(char *);
extern int  stricmp(char *, char *);
extern int  strnicmp(char *, char *, int);
#endif

#if defined(__ZTC__)
/*#define stricmp  lstricmp*/
#define tell(f) lseek(f, 0L, SEEK_CUR)
#define _dos_setdrive(a, b) dos_setdrive((unsigned) a, (unsigned *)b)
#define _dos_getdrive(a)    dos_getdrive((unsigned *)a)
#define _dos_getftime       dos_getftime
#endif

/*
  File processing differences
*/
#if defined(UNIX)
#define CH_SLASH   '/'
#define STR_SLASH  "/"
#define DRIVESPEC_LENGTH  0
#if !defined(O_BINARY)
#define O_BINARY   0
#endif
#elif defined(VAXC)
#define CH_SLASH   '/'
#define STR_SLASH  "/"
#define DRIVESPEC_LENGTH  0
#else  /* !UNIX */
#define CH_SLASH   '\\'
#define STR_SLASH  "\\"
#define DRIVESPEC_LENGTH  2
#endif

#ifndef MAXPATH
#if defined(UNIX) || defined(VAXC) || defined(OS2)
#define MAXPATH		256
#else
#define MAXPATH		128
#endif
#endif

/*
  The max length of a auto buffer
*/
#define MAXBUFSIZE      161   /* 160 columns + 1 for null terminator */

/*
  A good macro for getting the number of objects in an array
*/
#define ARRAY_DIM(x)  ( sizeof((x)) / sizeof((x)[0]) )


/*
  DOS and OS/2 use 2 bytes per screen cell, and Curses uses 1 byte
*/
#if defined(UNIX) || defined(VAXC)
#define SCREENCELLSIZE   sizeof(short)
#else
#define SCREENCELLSIZE   2
#endif

/*
  Test to see if a string pointer is a numeric resource id
*/
#if defined(MEWEL_32BITS)
#define ISNUMERICRESOURCE(s)   ((((DWORD) (s)) & 0xFFFF0000L) == 0L)
#else
#define ISNUMERICRESOURCE(s)   ((FP_SEG(s)) == 0)
#endif


/*
  Opsys-dependent functions
*/

/*
  CDPLUS should be defined if we want to use CTRL keys for accelerators
  on a PC-based UNIX. This affects winaccel.c and winmenu.c
*/
#if defined(UNIX)
#define CDPLUS
#endif


#if !defined(UNIX) && !defined(VAXC) && !defined(XWINDOWS)

/*
  null-define some Curses functions
*/
#define refresh()
#define endwin()
#define standout()
#define standend()
#define move()
#define LINES
#define COLS

#endif

/*
  Defines for highlighting for UNIX/VMS systems
*/

#if defined(DOS) || defined(OS2)
#define HIGHLITE_ON    standout
#elif defined(XWINDOWS)
#define HIGHLITE_ON()
#elif defined(VAXC)
#define HIGHLITE_ON() \
{ \
 VirtualScreenFlush(); \
 setattr(_REVERSE); \
}
#elif defined(aix)
#define HIGHLITE_ON()
#else
#define HIGHLITE_ON() \
{ \
 VirtualScreenFlush(); \
 standout(); \
}
#endif

#if defined(XWINDOWS)
#define HIGHLITE_OFF()
#elif defined(VAXC)
#define HIGHLITE_OFF()   clrattr(_REVERSE)
#elif defined(aix)
#define HIGHLITE_OFF()
#else
#define HIGHLITE_OFF()   standend()
#endif


extern unsigned MouseHandle;

/*
  For the 386 DOS Extenders, we need to redefine the way that int86()
  is called.
*/
#if defined(__WATCOMC__) && defined(__386__)
#define ax  eax
#define bx  ebx
#define cx  ecx
#define dx  edx
#define si  esi
#define di  edi
#define int86  int386
#define int86x int386x
#endif

#ifdef ERGO
extern int ParaToSel(unsigned short, unsigned short *);
#endif

#if defined(XWINDOWS)
#include "xmewel.h"
#endif

/*
  Memory model stuff for internal usage
*/
#define MODEL_UNKNOWN  (-1)
#define MODEL_TINY     0
#define MODEL_SMALL    1
#define MODEL_MEDIUM   2
#define MODEL_COMPACT  3
#define MODEL_LARGE    4
#define MODEL_HUGE     5
#define MODEL_XLARGE   6
#define MODEL_NEAR     0
#define MODEL_FAR      1
#define MODEL_HUGE_PTR 2
#define MODEL_NAME     MODEL_UNKNOWN
#define MODEL_CODE_PTR MODEL_UNKNOWN
#define MODEL_DATA_PTR MODEL_UNKNOWN

#if defined(__TSC__) || defined (MSC) || defined(__ZTC__) || defined(__WATCOM__)
#       undef MODEL_NAME
#       if defined(M_I86TM)
#		define MODEL_NAME MODEL_TINY
#	endif
#	if defined(M_I86SM)
#		define MODEL_NAME MODEL_SMALL
#	endif
#	if defined(M_I86MM)
#		define MODEL_NAME MODEL_MEDIUM
#	endif
#	if defined(M_I86CM)
#		define MODEL_NAME MODEL_COMPACT
#	endif
#	if defined(M_I86LM)
#		define MODEL_NAME MODEL_LARGE
#	endif
#	if defined(M_I86HM)
#		define MODEL_NAME MODEL_HUGE
#	endif
#	if defined(M_I86XM)
#		define MODEL_NAME MODEL_XLARGE
#	endif
#	if defined(__386__)
#		define MODEL_NAME MODEL_XLARGE
#	endif
#endif

#if defined (__TURBOC__) || defined(__BORLANDC__)
#	undef MODEL_NAME
#	if defined(__TINY__)
#		define MODEL_NAME MODEL_TINY
#	endif
#	if defined(__SMALL__)
#		define MODEL_NAME MODEL_SMALL
#	endif
#	if defined(__MEDIUM__)
#		define MODEL_NAME MODEL_MEDIUM
#	endif
#	if defined(__COMPACT__)
#		define MODEL_NAME MODEL_COMPACT
#	endif
#	if defined(__LARGE__)
#		define MODEL_NAME MODEL_LARGE
#	endif
#	if defined(__HUGE__)
#		define MODEL_NAME MODEL_HUGE
#	endif
#endif

#if (MODEL_NAME != MODEL_UNKNOWN)
#	undef MODEL_CODE_PTR
#	undef MODEL_DATA_PTR
#	if (MODEL_NAME == MODEL_TINY)
#		define MODEL_CODE_PTR MODEL_NEAR
#		define MODEL_DATA_PTR MODEL_NEAR
#	endif
#	if (MODEL_NAME == MODEL_SMALL)
#		define MODEL_CODE_PTR MODEL_NEAR
#		define MODEL_DATA_PTR MODEL_NEAR
#	endif
#	if (MODEL_NAME == MODEL_MEDIUM)
#		define MODEL_CODE_PTR MODEL_FAR
#		define MODEL_DATA_PTR MODEL_NEAR
#	endif
#	if (MODEL_NAME == MODEL_COMPACT)
#		define MODEL_CODE_PTR MODEL_NEAR
#		define MODEL_DATA_PTR MODEL_FAR
#	endif
#	if (MODEL_NAME == MODEL_LARGE)
#		define MODEL_CODE_PTR MODEL_FAR
#		define MODEL_DATA_PTR MODEL_FAR
#	endif
#	if (MODEL_NAME == MODEL_HUGE)
#		define MODEL_CODE_PTR MODEL_FAR
#		define MODEL_DATA_PTR MODEL_HUGE_PTR
#	endif
#	if (MODEL_NAME == MODEL_XLARGE)
#		define MODEL_CODE_PTR MODEL_FAR
#		define MODEL_DATA_PTR MODEL_FAR
#	endif
#endif

/*
  Thomas Wagner's defines. A must for his internationalization scheme.
*/
#ifdef WAGNER
#define WAGNER_SPECIAL      1
#define WAGNER_GRAPHICS     1
#define INTERNATIONAL_MEWEL 1  /* enable internationalization */
#include "video.h"
#endif


/*
  Include the main include file, but only if it hasn't been included
  already.
*/
#ifndef _INC_WINDOWS
#include "windows.h"
#endif

#endif /* !defined(_WPRIVATE_H) */


