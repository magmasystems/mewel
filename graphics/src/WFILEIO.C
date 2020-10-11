/*===========================================================================*/
/*                                                                           */
/* File    : WFILEIO.C                                                       */
/*                                                                           */
/* Purpose : Implements the Windows compatible file i/o functions            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if !defined(O_BINARY)
#define O_BINARY   0x0000
#endif


#ifndef _lread
UINT FAR PASCAL _lread(fd, lpBuf, nBytes)
  int  fd;
  VOID _huge *lpBuf;
  UINT nBytes;
{
#if !defined(DOS) || defined(MEWEL_32BITS)

  return read(fd, lpBuf, nBytes);

#elif (MODEL_DATA_PTR == MODEL_FAR)
#if defined(ZORTECH)
  return read(fd, (PSTR) lpBuf, nBytes);
#elif defined(MSC)
  unsigned nRead;
  if (_dos_read(fd, (LPSTR) lpBuf, nBytes, &nRead) == 0)
    return nRead;
  else
    return (UINT) HFILE_ERROR;
#else
    return read(fd, (LPSTR) lpBuf, nBytes);
#endif

#else
  /*
    Medium model
  */
  union  REGS  r;
  struct SREGS s;

  r.h.ah = 0x3F;
  r.x.bx = fd;
  r.x.cx = nBytes;
  r.x.dx = FP_OFF(lpBuf);
  s.ds   = FP_SEG(lpBuf);
  int86x(0x21, &r, &r, &s);

  if (r.x.ax > 0)
    return r.x.ax;
  else
    return HFILE_ERROR;
#endif
}
#endif


#ifndef _lwrite
UINT FAR PASCAL _lwrite(fd, lpBuf, nBytes)
  int fd;
  CONST VOID _huge *lpBuf;
  UINT nBytes;
{
#if !defined(DOS) || defined(MEWEL_32BITS)

  return write(fd, lpBuf, nBytes);

#elif (MODEL_DATA_PTR == MODEL_FAR)
#if defined(ZORTECH)
    return write(fd, (PSTR) lpBuf, nBytes);
#elif defined(MSC)
  unsigned nRead;
  if (_dos_write(fd, (LPSTR) lpBuf, nBytes, &nRead) == 0)
    return nRead;
  else
    return (UINT) HFILE_ERROR;
#else
    return write(fd, (LPSTR) lpBuf, nBytes);
#endif

#else
  /*
    Medium model
  */
  union REGS   r;
  struct SREGS s;

  r.h.ah = 0x40;
  r.x.bx = fd;
  r.x.cx = nBytes;
  r.x.dx = FP_OFF(lpBuf);
  s.ds   = FP_SEG(lpBuf);
  int86x(0x21, &r, &r, &s);

  if (r.x.ax > 0)
    return r.x.ax;
  else
    return HFILE_ERROR;
#endif
}
#endif


#ifndef _lcreat
INT FAR PASCAL _lcreat(lpPathName, iAttribute)
  LPCSTR lpPathName;
  INT    iAttribute;
{
#if defined(UNIX)
  /*
    iAttribute is 0 for normal, 1 for read-only, 2 for hidden, 3 for system
  */
  static INT iTransAttr[4] =
  {
    0666,                  /* 0 is normal */
    0644,                  /* 1 is read-only */
    0644,                  /* 2 is hidden */
    0644                   /* 3 is system */
  };
  return creat((LPSTR) lpPathName, iTransAttr[iAttribute]);

#elif !defined(DOS) || defined(MEWEL_32BITS)
  /*
    iAttribute is 0 for normal, 1 for read-only, 2 for hidden, 3 for system
  */
  static INT iTransAttr[4] =
  {
    S_IREAD | S_IWRITE,    /* 0 is normal */
    S_IREAD,               /* 1 is read-only */
    S_IREAD | S_IWRITE,    /* 2 is hidden */
    S_IREAD | S_IWRITE     /* 3 is system */
  };
  return creat(lpPathName, iTransAttr[iAttribute]);

#elif defined(MSC) && !defined(__ZTC__) && (MODEL_NAME == MODEL_LARGE)
  static INT iTransAttr[4] =
  {
    _A_NORMAL,             /* 0 is normal */
    _A_RDONLY,             /* 1 is read-only */
    _A_HIDDEN,             /* 2 is hidden */
    _A_SYSTEM              /* 3 is system */
  };
  INT   fd;
  if (_dos_creat(lpPathName, iTransAttr[iAttribute], &fd) == 0)
    return fd;
  else
    return HFILE_ERROR;

#elif defined(__TURBOC__) && (MODEL_DATA_PTR == MODEL_FAR)
  static INT iTransAttr[4] =
  {
    _A_NORMAL,             /* 0 is normal */
    _A_RDONLY,             /* 1 is read-only */
    _A_HIDDEN,             /* 2 is hidden */
    _A_SYSTEM              /* 3 is system */
  };
  return _creat(lpPathName, iTransAttr[iAttribute]);

#else
  union REGS   r;
  struct SREGS s;

  static INT iTransAttr[4] =
  {
    _A_NORMAL,             /* 0 is normal */
    _A_RDONLY,             /* 1 is read-only */
    _A_HIDDEN,             /* 2 is hidden */
    _A_SYSTEM              /* 3 is system */
  };

  r.h.ah = 0x3C;
  r.x.cx = iTransAttr[iAttribute];
  r.x.dx = FP_OFF(lpPathName);
  s.ds   = FP_SEG(lpPathName);
  int86x(0x21, &r, &r, &s);

  if (r.x.cflag)
    return HFILE_ERROR;
  else
    return r.x.ax;
#endif
}
#endif


#ifndef _lopen
INT FAR PASCAL _lopen(lpPathName, iAttribute)
  LPCSTR lpPathName;
  INT    iAttribute;
{
  /*
    Translate the Windows attribute into one which can be used by
    open().
  */
  switch (iAttribute & 0x03)
  {
    case READ  :
      iAttribute = (iAttribute & ~3) | O_RDONLY;
      break;
    case WRITE :
      iAttribute = (iAttribute & ~3) | O_WRONLY;
      break;
    case READ_WRITE :
      iAttribute = (iAttribute & ~3) | O_RDWR;
      break;
  }

#if !defined(DOS) || defined(MEWEL_32BITS)
  return open(lpPathName, iAttribute | O_BINARY);

#elif defined(MSC) && !defined(__ZTC__) && (MODEL_NAME == MODEL_LARGE)
  {
  INT   fd;
  if (_dos_open(lpPathName, (iAttribute | 0x8000), &fd) == 0)
    return fd;
  else
    return HFILE_ERROR;
  }

#elif defined(__TURBOC__) && (MODEL_DATA_PTR == MODEL_FAR)
  return open(lpPathName, iAttribute | O_BINARY);

#else
  {
  union REGS   r;
  struct SREGS s;

  r.h.ah = 0x3D;
  r.h.al = (BYTE) iAttribute;
  r.x.dx = FP_OFF(lpPathName);
  s.ds   = FP_SEG(lpPathName);
  int86x(0x21, &r, &r, &s);

  if (r.x.cflag)
    return HFILE_ERROR;
  else
    return r.x.ax;
  }
#endif
}
#endif


HFILE FAR PASCAL _lclose(fd)
  HFILE fd;
{
#if defined(MSC) && (_MSC_VER >= 700)
  if (_close(fd) == 0)
#else
  if (close(fd) == 0)
#endif
    return 0;
  else
    return HFILE_ERROR;
}


LONG FAR PASCAL _llseek(HFILE fd, LONG ulPos, int iMode)
{
#if defined(MSC) && (_MSC_VER >= 700)
  return _lseek(fd, ulPos, iMode);
#else
  return lseek(fd, ulPos, iMode);
#endif
}

