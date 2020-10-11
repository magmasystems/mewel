/*===========================================================================*/
/*                                                                           */
/* File    : WDRVTYPE.C                                                      */
/*                                                                           */
/* Purpose : Implements the GetDriveType() function                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static INT PASCAL IsDriveValid(int);
static INT PASCAL IsDriveFixed(int);
static INT PASCAL IsDriveNetwork(int);

UINT FAR PASCAL GetDriveType(iDrive)
  INT iDrive;   /* 0 = A, 1 = B, 2 = C, ... */
{
#if defined(__DPMI32__)
  #undef DOS
  extern UINT  FAR PASCAL GetDriveTypeA(LPSTR);
  char szRoot[8];
  sprintf(szRoot, "%c:\\", iDrive + 'A');
  return GetDriveTypeA(szRoot);
#elif defined(DOS)
  iDrive++;     /* DOS expects 1 = A, 2 = B, 3 = C, ... */
  if (!IsDriveValid(iDrive))
    return 0;
  if (IsDriveNetwork(iDrive))
    return DRIVE_REMOTE;
  if (IsDriveFixed(iDrive))
    return DRIVE_FIXED;
  return DRIVE_REMOVABLE;
#else
  return DRIVE_FIXED;
#endif
}


#if defined(DOS)

static INT PASCAL IsDriveValid(iDrive)
  int iDrive;
{
  union REGS r;

  r.x.ax = 0x4408;
  r.h.bl = (char) iDrive;
  intdos(&r, &r);
  return (r.x.ax != 0x000F);
}


static INT PASCAL IsDriveFixed(iDrive)
  int iDrive;
{
  union REGS r;

  r.x.ax = 0x4408;
  r.h.bl = (char) iDrive;
  intdos(&r, &r);
  return r.x.ax;
}


static INT PASCAL IsDriveNetwork(iDrive)
  int iDrive;
{
  union REGS r;

  r.x.ax = 0x4409;
  r.h.bl = (char) iDrive;
  intdos(&r, &r);
  return (!r.x.cflag && (r.x.dx & 0x1000));
}

#endif



#ifdef TEST
main()
{
  int  ch;

  for (ch = 'A';  ch <= 'Z';  ch++)
    if (IsDriveValid(ch - 'A' + 1))
    {
      if (IsDriveNetwork(ch - 'A' + 1))
        printf("Drive %c is on a network.\n", ch);
      if (IsDriveFixed(ch - 'A' + 1))
        printf("Drive %c is fixed.\n", ch);
      else
        printf("Drive %c is removable.\n", ch);
    }
}
#endif


