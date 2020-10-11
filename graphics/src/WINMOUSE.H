/*===========================================================================*/
/*                                                                           */
/* File    : WINMOUSE.H                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#define ERROR_MOUSE_NO_DEVICE	385

#define MOVE_WITH_NO_BUTTONS    0x01
#define MOVE_WITH_BUTTON1       0x02
#define BUTTON1_DOWN            0x04
#define MOVE_WITH_BUTTON2       0x08
#define BUTTON2_DOWN            0x10
#define MOVE_WITH_BUTTON3       0x20
#define BUTTON3_DOWN            0x40

#ifndef MOU_NOWAIT
#define MOU_NOWAIT              0
#define MOU_WAIT                1
#endif


#ifdef DOS
typedef struct _MOUEVENTINFO
{
  UINT   fs;      /* State of mouse at the time the event was reported*/
  DWORD  time;    /* time since boot in milliseconds      */
  UINT   row;     /* absolute/relative row position       */
  UINT   col;     /* absolute/relative column position    */
} MOUEVENTINFO;
typedef MOUEVENTINFO *PMOUEVENTINFO;
#endif

#ifndef OS2
struct EventInfo
{
  UINT Mask;
  DWORD Time;
  UINT Row;
  UINT Col;
};
#endif

#ifdef DOS
typedef struct _PTRLOC
{
  UINT row;     /* pointer row coordinate screen position*/
  UINT col;     /* pointer column coordinate screen position*/
} PTRLOC;
typedef PTRLOC *PPTRLOC;
#endif

#ifndef OS2
struct PtrLoc
{
  UINT RowPos;
  UINT ColPos;
};
#endif

#ifdef DOS
typedef struct _MOUQUEINFO
{
  UINT cEvents;    /* current number of event queue elements*/
  UINT cmaxEvents; /* MaxNumQueElements value           */
} MOUQUEINFO;
typedef MOUQUEINFO *PMOUQUEINFO;
#endif

#ifndef OS2
struct QueInfo
{
  UINT Events;
  UINT QSize;
};
#endif

#ifndef OS2
typedef struct mouseparams
{
  int m1,m2,m3,m4;
} MOUSEPARAMS;
#endif
