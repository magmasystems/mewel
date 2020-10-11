/*===========================================================================*/
/*                                                                           */
/* File    : WKEYSTAT.C                                                      */
/*                                                                           */
/* Purpose : Tests the keyboard to see if a character is ready               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCLUDE_MOUSE

#include "wprivate.h"
#include "window.h"

/*==========================================================================*/
/*                              GetKeyState                                 */
/* Usage:      See Microsoft Windows Documentation.                         */
/* Input:      See Microsoft Windows Documentation.                         */
/* Output:     See Microsoft Windows Documentation.                         */
/* Desc:       This functions provides the necessary Windows compatibility  */
/*             for use by the VLB window class.                             */
/* Modifies:   None                                                         */
/* Notes:      None                                                         */
/*==========================================================================*/

static struct
{
  UINT vkKey;
  UINT fsShift;
  UINT nKeyState;
} KbdStateInfo[] =
{
  { VK_MENU,    ALT_SHIFT,   0x8000 },
  { VK_CONTROL, CTL_SHIFT,   0x8000 },
  { VK_SHIFT,   SHIFT_SHIFT, 0x8000 },
  { VK_SCROLL,  SCL_SHIFT,   0x0001 },
  { VK_NUMLOCK, NUM_SHIFT,   0x0001 },
  { VK_CAPITAL, CAP_SHIFT,   0x0001 },
  { VK_INSERT,  INS_SHIFT,   0x0001 },
};


INT FAR PASCAL GetKeyState(nVirtKey)
  INT  nVirtKey;
{
  UINT bShiftStatus;
  INT  i;

  if (nVirtKey==VK_LBUTTON || nVirtKey==VK_RBUTTON || nVirtKey==VK_MBUTTON)
  {
#if defined(DOS)
    INT   status;
    MWCOORD x, y;
    MOUSE_GetStatus(&status, &x, &y);
    if ((nVirtKey == VK_LBUTTON && (status & 0x01)) ||
        (nVirtKey == VK_RBUTTON && (status & 0x02)))
#elif defined(OS2)
    UINT pfWait = FALSE;
    MOUEVENTINFO ei;
    MouReadEventQue((PMOUEVENTINFO) &ei, (LPUINT) &pfWait, MouseHandle);
    if ((nVirtKey == VK_LBUTTON && 
           (ei.fs & (BUTTON1_DOWN | MOVE_WITH_BUTTON1))) ||
        (nVirtKey == VK_RBUTTON && 
           (ei.fs & (BUTTON2_DOWN | MOVE_WITH_BUTTON2))))
#else
    if (0)
#endif
      return 0x8000;
    else
      return 0;
  }

  bShiftStatus = KBDGetShift();

  for (i = 0;  i < sizeof(KbdStateInfo)/sizeof(KbdStateInfo[0]);  i++)
    if (KbdStateInfo[i].vkKey == (UINT) nVirtKey)
      return (KbdStateInfo[i].fsShift & bShiftStatus)
                                          ? KbdStateInfo[i].nKeyState : 0;


  return ((INT) SysEventInfo.chLastKey == nVirtKey);
}


int WINAPI GetAsyncKeyState(nVirtKey)
  INT  nVirtKey;
{
  return GetKeyState(nVirtKey);
}


