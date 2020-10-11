/*
   COMPAT.H: EXEC function with memory swap -
             Borland/Microsoft compatibility header file.

   Public domain software by

        Thomas Wagner
        Ferrari electronic GmbH
        Beusselstrasse 27
        D-1000 Berlin 21
        Germany
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>

#if defined(__TURBOC__)
#define TURBO  1
#define MSC    0
#else
#define TURBO  0
#define MSC    1
#endif

#if !defined(CDECL)
#define CDECL  cdecl
#endif

#if defined(__TURBOC__)
#include <dir.h>
#include <alloc.h>
#include <sys\stat.h>
#define OS_MAJOR  (_version & 0xff)
#endif

#if defined(MSC) && !defined(__TURBOC__)
#if !defined(__ZTC__)
#include <malloc.h>
#endif
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>

#define fnsplit   _splitpath
#define fnmerge   _makepath

#if !defined(__TSC__) 

#if defined(__ZTC__)
#define _MAX_PATH  FILENAME_MAX
#define _MAX_FNAME 9
#define _MAX_DRIVE 3
#define _MAX_EXT   5
#define _MAX_DIR   128
#endif

#define MAXPATH   _MAX_PATH
#define MAXDRIVE  _MAX_DRIVE
#define MAXDIR    _MAX_DIR
#define MAXFILE   _MAX_FNAME
#define MAXEXT    _MAX_EXT

#if !defined(__ZTC__)
#define FA_RDONLY _A_RDONLY
#define FA_HIDDEN _A_HIDDEN
#define FA_SYSTEM _A_SYSTEM
#define FA_ARCH   _A_ARCH
#endif

#endif /* !defined (__TSC__) */

#define findfirst(a,b,c) _dos_findfirst(a,c,b)
#define ffblk find_t

#define OS_MAJOR  _osmajor

#define farmalloc(x)    ((void far *)halloc(x,1))
#define farfree(x)      hfree((void huge *)x)

#define stpcpy(d,s)     (strcpy (d, s), d + strlen (s))

#if !defined(__TSC__) && !defined(__ZTC__)
#undef MK_FP
#define MK_FP(seg,ofs)  ((void far *)(((unsigned long)(seg) << 16) | (unsigned short)(ofs)))
#endif

#if defined(__ZTC__)
#define stricmp(s, t)   strcmp(s, t)
#endif

#endif

