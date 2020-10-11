/*===========================================================================*/
/*                                                                           */
/* File    : MOUAPI.C                                                        */
/*                                                                           */
/* Purpose : Medium-level routines to control the mouse.                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MOUSE
#if defined(__DPMI32__)
#define INCLUDE_CURSORS
#endif

#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif


typedef unsigned MOUSEHANDLE;


/*
   To avoid the duplication of Mouse routines throughout the low-level
   output stuff, some routines were added here. Also, the MouseHide()
   and MouseShow() routines now maintain a counter, so the hardware is
   only accessed when really necessary. This should speed things up a tad.
*/
static UINT mouse_hidden = 0;

/*
  These constants and variables deal with the double click and repeat
  intervals.
*/
#define DEFAULT_CLICK  (18/2)     /* 1/2 second = 9 clock ticks */
int CLICK_INTERVAL  = DEFAULT_CLICK;
#ifdef  SOFTCOST
int TYPEMATIC_DELAY = (18 / 4);   /* 1/4 second delay for mouse typematic */
#else
int TYPEMATIC_DELAY = (18);       /* 1 second delay for mouse typematic */
#endif
int TYPEMATIC_RATE  = (18 / 7);   /* 1/6 second rate for mouse typematic */


/*
  This structure records the mouse coordinates in pixel values for
  graphics mode.
*/
POINT _ptMouseInPixels = { 0, 0 };


extern VOID FAR PASCAL GMouseProcessMotion(void);
extern int  FAR PASCAL GMouseInit(unsigned FAR *);
extern VOID FAR PASCAL GMouseTerminate(void);
extern VOID FAR PASCAL GMouseGetStatus(int *button, MWCOORD *col, MWCOORD *row);
extern VOID FAR PASCAL GMouseShowCursor(void);
extern VOID FAR PASCAL GMouseHideCursor(void);

#if defined(DOS16M)
#define USE_MOUSEINTERRUPT
#endif

#if defined(DOS) && !defined(EXTENDED_DOS) && !defined(MSC)
#define USE_MOUSEINTERRUPT
#endif

#if defined(USE_MOUSEINTERRUPT)
#include "winevent.h"

int near _gotMouseInt = 0;
int near _mickeyHoriz = 0;
int near _mickeyVert  = 0;
int near _buttonState = 0;
int near _rowPosition = 0;        /* Never Used */
int near _colPosition = 0;        /* Never Used */

typedef struct tagMouseQueue
{
  int x, y;
  int button;
} MOUSEQUEUEELEMENT, *PMOUSEQUEUEELEMENT;

static QUEUE MouseQueue = 
{
  0,
  0,
  16,
  0,
  sizeof(MOUSEQUEUEELEMENT),
  NULL,
  0, 0, 0
};

extern void FAR cdecl MouseHandler();

#if defined(__DPMI16__)
static VOID PASCAL DP16PrepMouseHandler(void);
#else
#define DP16PrepMouseHandler()
#endif

#endif



INT FAR PASCAL MouseInitialize(void)
{
  int rc;

  rc = MouOpen((LPSTR) NULL, (unsigned FAR *) &MouseHandle);
  MEWELSysMetrics[SM_MOUSEPRESENT] = (MouseHandle != 0);
#if defined(DOS16M) || defined(USE_MOUSEINTERRUPT)
  if (!rc)
  {
#if defined(DOS16M)
    MOUSE_SetSubroutine(0x1F, (void (FAR *)()) MouseHandler);
#else
    /*
      A mask of 0x1E traps left and right button events. A mask of 0x1F
      would trap movements as well.
    */
    InitQueue(&MouseQueue);
    DP16PrepMouseHandler();
    MOUSE_SetSubroutine(0x1E, (void (FAR *)()) MouseHandler);
#endif
  }
#endif
  mouse_hidden = 0;
  CLR_PROGRAM_STATE(STATE_MOUSEHIDDEN);
  return rc;
}


VOID FAR PASCAL MouseTerminate(void)
{
  if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
    GMouseTerminate();
  else
    MouseHide();
  /*
    Reset the mouse back to its defaults.
  */
  mouse_hidden = 0;  /* reset the status counter to 0 */
  MOUSE_Init(MOUSE_TERMINATE);
#if defined(DOS16M) || defined(USE_MOUSEINTERRUPT)
  MOUSE_SetSubroutine(0x00, 0L);
#endif
}



INT FAR PASCAL ShowCursor(BOOL bShow)
{
  static int cursorCounter = 0; /* change to -1 if no mouse detected */

  if (bShow)
    ++cursorCounter;
  else
    --cursorCounter;

  if (cursorCounter < 0)
  {
    if (!bShow)
      MouseHide();
  }
  else
  {
    MouseShow();
  }
  return cursorCounter;
}


INT FAR PASCAL MouseShow(void)
{
  /*
    If the mouse is currently hidden, decrement the hidden count. If
    the hidden count is still above 0, then return.
  */
  if (mouse_hidden)
    if (--mouse_hidden)
      return TRUE;

  if (MouseHandle)
  {
    if (!TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
      MOUSE_ShowCursor();
    else
      GMouseShowCursor();
    CLR_PROGRAM_STATE(STATE_MOUSEHIDDEN);
  }

  return TRUE;
}

INT FAR PASCAL MouseHide(void)
{
  /*
    Already hidden? Don't do anything
  */
  if (mouse_hidden++)
    return TRUE;

  /*
    Hide the mouse
  */
  if (MouseHandle)
  {
    if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
      GMouseHideCursor();
    else
      MOUSE_HideCursor();
    SET_PROGRAM_STATE(STATE_MOUSEHIDDEN);
  }

  return TRUE;
}

BOOL FAR PASCAL MouseHideIfInRange(int firstrow, int lastrow)
{
   int   button;
   MWCOORD row, col;

   /*
     Do the test only if we have a mouse and it's not already hidden
   */
   if (MouseHandle && !mouse_hidden)
   {
     /*
       Get the position of the mouse
     */
     MOUSE_GetStatus(&button, &col, &row);
#ifdef MEWEL_TEXT
     if (VID_IN_GRAPHICS_MODE())
       row /= VideoInfo.yFontHeight;
     else if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
       GMouseGetStatus(&button, &col, &row);
     else
       row >>= 3;
#endif

     /*
       Hide it if it is within the range
     */
     if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
     {
       if (row >= firstrow-1 && row <= lastrow+1)
       {
         MouseHide();
         return TRUE;
       }
     }
     else
     if (row >= firstrow && row <= lastrow)
     {
       MouseHide();
       return TRUE;
     }
   }
   return FALSE;
}


BOOL FAR PASCAL IsMouseInstalled(void)
{
  return (BOOL) ((MouseHandle) ? TRUE : FALSE);
}

INT FAR PASCAL SetDoubleClickTime(ticks)
  int ticks;
{
  return (CLICK_INTERVAL = ((ticks > 0) ? ticks : DEFAULT_CLICK));
}
INT FAR PASCAL GetDoubleClickTime(void)
{
  return CLICK_INTERVAL;
}
VOID FAR PASCAL MouseSetCursorChar(chMouse)
  INT chMouse;
{
  if (!TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
    MOUSE_SetCursorMode(0, (chMouse == NORMAL_CURSOR) ? 0x77FF : 0x7700, 
                           0x7700 | chMouse);
}

VOID FAR PASCAL GetCursorPos(lpPoint)
  LPPOINT lpPoint;
{
  int tmp; 
  MWCOORD x, y;
  MouseGetStatus(&tmp, &x, &y);
  lpPoint->x = x;
  lpPoint->y = y;
}

VOID FAR PASCAL GetPixelCursorPos(lpPoint)
  LPPOINT lpPoint;
{
  lpPoint->x = _ptMouseInPixels.x;
  lpPoint->y = _ptMouseInPixels.y;
}

VOID FAR PASCAL SetCursorPos(x, y) 
  int x, y;  /* uses character, not pixel, coordinates */
{
#ifndef MEWEL_GUI
  y *= (VID_IN_GRAPHICS_MODE()) ? VideoInfo.yFontHeight : 8;
  x *= (VID_IN_GRAPHICS_MODE()) ? FONT_WIDTH : 8;
#endif
  MOUSE_SetPos(x, y);
}

VOID FAR PASCAL ClipCursor(LPRECT lpRect)
{ 
  if (lpRect)
    MOUSE_ClipCursor(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
  else
    MOUSE_ClipCursor(0, 0, VideoInfo.width-1, VideoInfo.length-1);
}


BOOL FAR PASCAL IsMouseLeftButtonDown(void)
{
  UINT pfWait = MOU_NOWAIT;
  MOUEVENTINFO evMouse;

  MouReadEventQue((PMOUEVENTINFO) &evMouse, (LPUINT) &pfWait, MouseHandle);
  return (evMouse.fs & BUTTON1_DOWN) != 0;
}

BOOL FAR PASCAL SwapMouseButton(bSwap)
  BOOL bSwap;
{
  if (!MouseHandle)
    return FALSE;
  MEWELSysMetrics[SM_SWAPBUTTON] = bSwap;
  return TRUE;
}


INT FAR PASCAL GetMouseMessage(lpMsg)
  LPMSG lpMsg;
{
  static unsigned lastmask = 0;
  static DWORD ldblclick_time = 0L, rdblclick_time = 0L;
  static DWORD ltypeamatic_time = 0L;

  MOUEVENTINFO ei;  /* ptr to event structure  */
  UINT     pfWait = FALSE;
  DWORD    time;
  UINT     message;

  
  message = lpMsg->message = 0;

  /*
    No mouse?
  */
  if (!MouseHandle)
    return FALSE;

  /*
    Has EnableHardwareInput(FALSE) been called?
  */
  if (TEST_PROGRAM_STATE(STATE_HARDWARE_OFF))
    return FALSE;

  /*
    Using the graphical mouse from the Toolbox?
  */
  if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
    GMouseProcessMotion();

  /*
    If we have a double-click message pending, then fill the lpMsg structure
    with the double-click message and the mouse position info. Reset a
    number of status variables too!
  */
  if (SysEventInfo.DblClickPendingMsg)
  {
    message = SysEventInfo.DblClickPendingMsg;
    SysEventInfo.DblClickPendingMsg = 0;
    ldblclick_time = rdblclick_time = ltypeamatic_time = 0L;
    MouReadEventQue(&ei, (LPUINT) &pfWait, MouseHandle);
    goto fill_event;
  }

#ifdef DOS16M
  /*
     It takes a long time to do a mode switch to and from protected mode.
     If we continually poll the mouse, then keyboard events take a *long*
     time to get through. Therefore, we use an interrupt procedure to
     detect if anything has happened to the mouse.
  */
  if (!_gotMouseInt)
    return 0;
  else
    _gotMouseInt--;
#endif

  /*
    Call the mouse driver to get the mouse position and button status.
  */
  MouReadEventQue(&ei, (LPUINT) &pfWait, MouseHandle);
  time = BIOSGetTime();

  /*
    See if the user depressed or released the left mouse button...
  */
  if ((lastmask & BUTTON1_DOWN) != (ei.fs & BUTTON1_DOWN))
  {
    /*
      Determine the WM_LBUTTONxxx message
    */
    message = (lastmask & BUTTON1_DOWN) ? WM_LBUTTONUP : WM_LBUTTONDOWN;

    if (message == WM_LBUTTONUP)
    {
      /*
        If the right button was down, then it's a middle-button msg.
      */
      if ((lastmask & BUTTON2_DOWN))
        message = WM_MBUTTONUP;
      ltypeamatic_time = 0L;
    }
    else  /* WM_LBUTTONDOWN */
    {
      /*
        See if the right button is also down. If so, it's a middle-button msg.
      */
      if (ei.fs & BUTTON2_DOWN)
        message = WM_MBUTTONDOWN;
      /*
        A button-down event signifies that the user might have clicked.
        If this button-down event occured in the time allocated for
        another click, then it's a double-click. If not, then
        record the time of this button-up event in preparation for
        checking for a double-click.
      */
      if (ldblclick_time && time <= ldblclick_time)
      {
        SysEventInfo.DblClickPendingMsg = 
          (message == WM_MBUTTONDOWN) ? WM_MBUTTONDBLCLK : WM_LBUTTONDBLCLK;
        return FALSE;
      }
      else
        ldblclick_time = time + CLICK_INTERVAL;
      ltypeamatic_time = time + TYPEMATIC_DELAY;
    }
  }

  else if ((lastmask & BUTTON2_DOWN) != (ei.fs & BUTTON2_DOWN))
  {
    message = (lastmask & BUTTON2_DOWN) ? WM_RBUTTONUP : WM_RBUTTONDOWN;
    if (message == WM_RBUTTONUP)
    {
      if ((lastmask & BUTTON1_DOWN))
        message = WM_MBUTTONUP;
    }
    else 
    {
      if (ei.fs & BUTTON1_DOWN)
        message = WM_MBUTTONDOWN;
      if (rdblclick_time && time <= rdblclick_time)
      {
        SysEventInfo.DblClickPendingMsg = 
            (message == WM_MBUTTONDOWN) ? WM_MBUTTONDBLCLK : WM_RBUTTONDBLCLK;
        return FALSE;
      }
      else
        rdblclick_time = time + CLICK_INTERVAL;
    }
  }

  else if ((lastmask & BUTTON3_DOWN) != (ei.fs & BUTTON3_DOWN))
  {
    message = (lastmask & BUTTON3_DOWN) ? WM_MBUTTONUP : WM_MBUTTONDOWN;
  }

  /*
    No button transitions. See if the mouse was moved...
  */
  else if ((ei.fs &
 (MOVE_WITH_BUTTON1|MOVE_WITH_BUTTON2|MOVE_WITH_BUTTON3|MOVE_WITH_NO_BUTTONS)))
  {
    message = WM_MOUSEMOVE;
    ldblclick_time = rdblclick_time = 0L;
    SysEventInfo.DblClickPendingMsg = 0;
    if ((ei.fs & MOVE_WITH_NO_BUTTONS))  /* 3/6/90 - Allow typeamatic, even */
    {                                    /*  if we moved the mouse from its */
      ltypeamatic_time = 0L;             /*  initial point. This will help  */
                                         /*  with autoscrolling a listbox   */
    }                                    /*  when the mouse is above/below  */
  }                                      /*  the listbox border.            */

  /*
    Save the current mouse status
  */
  lastmask = ei.fs;

  /*
    Fill the MSG structure with the mouse event
  */
fill_event:
  if (message==0 && ltypeamatic_time && (time=BIOSGetTime()) >= ltypeamatic_time)
  {
    ltypeamatic_time = time + TYPEMATIC_RATE;
    message = WM_MOUSEREPEAT;
  }

  if ((lpMsg->message = message) != 0)
  {
    UINT shiftDOS = KBDGetShift();
    WPARAM wParam   = 0;

    /*
      Map the DOS shift state into a Window's compatible MK_xxx code.
    */
    if (shiftDOS & (LEFT_SHIFT | RIGHT_SHIFT))
      wParam |= MK_SHIFT;
    if (shiftDOS & CTL_SHIFT)
      wParam |= MK_CONTROL;

    /*
      OR in the MK_xxx codes which corrspond to button states.
    */
    if (ei.fs & BUTTON1_DOWN)
      wParam |= MK_LBUTTON;
    if (ei.fs & BUTTON2_DOWN)
      wParam |= MK_RBUTTON;
    if (ei.fs & BUTTON3_DOWN)
      wParam |= MK_MBUTTON;

    lpMsg->wParam = wParam;
    lpMsg->lParam = MAKELONG(ei.col, ei.row);
    return TRUE;
  }
  else 
  {
    return FALSE;
  }
}



UINT FAR PASCAL MouOpen(name, handle)
  LPSTR name;
  unsigned FAR *handle;
{
  (void) name;

  if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
  {
    return GMouseInit(handle);
  }

  if (MOUSE_Init(MOUSE_INIT))
  {
    *handle = (UINT) -1;            /* if yes, give dummy handle */
    MOUSE_SetSpeed(8, 8);           /* set mickey/pixel ratio to 1:1 */
    MOUSE_SetCursorMode(0, 0x77FF, 0x7700);
    MOUSE_ShowCursor();
    return 0;                       /* and return no error */
  }
  else
  {
    *handle = 0;
    return(ERROR_MOUSE_NO_DEVICE); /* else, return no mouse err */
  }
}


UINT FAR PASCAL MouReadEventQue(event, pfWait, handle)
  PMOUEVENTINFO event;  /* ptr to event structure  */
  UINT FAR *pfWait;  /* FALSE if we should wait */
  unsigned handle;  /* mouse handle      */
{
  static int lastrow;       /* position at last call   */
  static int lastcol;
  static int lastpixrow;    /* pixel position at last call   */
  static int lastpixcol;
  MWCOORD  row, col;
  int    button;

  (void) handle;
  (void) pfWait;

  event->time = (DWORD) -1L;            /* put in dummy time figure */
  event->fs   = 0;                      /* first, blank button info */

  MouseGetStatus(&button, &col, &row);

  /*
    Did we move the mouse?
  */
  if (row - lastrow || col - lastcol ||
#ifdef MEWEL_GUI
      (
#else
      (VID_IN_GRAPHICS_MODE() && 
#endif
      (_ptMouseInPixels.y - lastpixrow || _ptMouseInPixels.x - lastpixcol)))
  {
    if (button & 1)
      event->fs |= (MEWELSysMetrics[SM_SWAPBUTTON]) ? MOVE_WITH_BUTTON2 : MOVE_WITH_BUTTON1;
    if (button & 2)
      event->fs |= (MEWELSysMetrics[SM_SWAPBUTTON]) ? MOVE_WITH_BUTTON1 : MOVE_WITH_BUTTON2;
    else if (!(button & 1))             /* if no buttons, but motion    */
      event->fs |= MOVE_WITH_NO_BUTTONS;  /* then set apporpriate bit */
  }

  if (button & 1)
    event->fs |= (MEWELSysMetrics[SM_SWAPBUTTON]) ? BUTTON2_DOWN : BUTTON1_DOWN;
  if (button & 2)
    event->fs |= (MEWELSysMetrics[SM_SWAPBUTTON]) ? BUTTON1_DOWN : BUTTON2_DOWN;

  event->row = lastrow = row;         /* return row    */
  event->col = lastcol = col;         /* return column */
  lastpixrow = _ptMouseInPixels.y;
  lastpixcol = _ptMouseInPixels.x;

  return 0;                           /* return no error */
}


VOID PASCAL MouseGetStatus(pButton, pX, pY)
  INT   *pButton;
  MWCOORD *pX, *pY;
{
#if defined(USE_MOUSEINTERRUPT)
  if (!MouseQueue.semQueueEmpty)
  {
    PMOUSEQUEUEELEMENT pMQE =
           (PMOUSEQUEUEELEMENT) QueueGetData(&MouseQueue, MouseQueue.qtail);
    MouseQueue.qtail = QueueNextElement(&MouseQueue, MouseQueue.qtail);
    if (MouseQueue.qhead == MouseQueue.qtail)
      DOSSEMSET(&MouseQueue.semQueueEmpty);
    DOSSEMCLEAR(&MouseQueue.semQueueFull);
    *pButton = pMQE->button;
    *pX = pMQE->x;
    *pY = pMQE->y;
  }
  else
#endif

  if (TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
    GMouseGetStatus(pButton, pX, pY);
  else
    MOUSE_GetStatus(pButton, pX, pY);

#ifdef MEWEL_GUI
  _ptMouseInPixels.x = *pX;
  _ptMouseInPixels.y = *pY;
#else
  if (VID_IN_GRAPHICS_MODE())
  {
    /*
      Record the current pixel-based coordinates
    */
    _ptMouseInPixels.x = *pX;
    _ptMouseInPixels.y = *pY;
    *pY /= VideoInfo.yFontHeight;
    *pX /= FONT_WIDTH;  /* convert Mickeys to screen */
  }
  else if (!TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
  {
    *pY /= 8;
    *pX /= 8;  /* convert Mickeys to screen */
  }
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : These functions are not used in the current incarnation of    */
/*            MEWEL.                                                        */
/*                                                                          */
/****************************************************************************/

#ifdef NOTUSED
UINT FAR PASCAL MouSetDevStatus(status, handle)
  unsigned FAR *status;
  unsigned handle;
{
  return 0;
}
#endif

#ifdef NOTUSED
UINT FAR PASCAL MouGetNumQueEl(info, handle)
  PMOUQUEINFO info;
  unsigned handle;
{
  return info->cEvents = 1;
}
#endif

#ifdef NOTUSED
UINT FAR PASCAL MouSetPtrPos(loc, handle)
  PPTRLOC  loc;
  unsigned handle;
{
  MOUSE_SetPos(loc->col, loc->row);
  return 0;
}
#endif

#ifdef NOTUSED
FAR PASCAL MouseWaitForClick(event, button_mask, interval)
  PMOUEVENTINFO event;
  unsigned button_mask; 
  int      interval;
{
  unsigned long finaltime = BIOSGetTime() + interval;

  while (BIOSGetTime() <= finaltime)
  {
    MouReadEventQue(event, (LPUINT) NULL, MouseHandle);

    /* See if the button was released. If so, we clicked; return YES */
    if ((event->fs & button_mask) == 0)
      return TRUE;
    /* If we moved the mouse, then we did not click */
    if (event->fs & (MOVE_WITH_NO_BUTTONS|MOVE_WITH_BUTTON1|MOVE_WITH_BUTTON2))
      return FALSE;
  }

  /* We timed out, so we don't have a click */
  return FALSE;
}
#endif


/*#undef TEST*/
#ifdef TEST

unsigned short MouseHandle = 0;
DWORD          fProgramState = 0L;

struct videoinfo VideoInfo =
{
   25,
   80,
   0xB800,
   0,
   0x0000,
   8
};

struct cursorinfo CursorInfo =
{
  0, 0, 0, 0,
  0                                    /* fState */
};


main()
{
  MSG event;
  static int nLeft = 0;
  static BYTE chMouse[4] = { ' ', 4, 29, 18 };

#if 0
  WinUseGraphicsMouse();
#endif
  if (MouOpen((char FAR *) NULL, (unsigned FAR *) &MouseHandle))
    exit(1); 
    
  for (;;)
  { 
    if (kbhit())  break; 

    GetMouseMessage(&event);

    if (!event.message)
      continue;

    if (!TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
      MOUSE_HideCursor();
    
    switch (event.message)
    {
      case WM_LBUTTONDOWN :
        printf("LBUTTON_DOWN  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        nLeft++;
        MOUSE_SetCursorMode(0, 0x7700, 0x7700 | chMouse[nLeft%4]);
        break;

      case WM_RBUTTONDOWN :
        printf("RBUTTON_DOWN  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_MBUTTONDOWN :
        printf("MBUTTON_DOWN  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_LBUTTONUP :
        printf("LBUTTON_UP  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_RBUTTONUP :
        printf("RBUTTON_UP  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_MBUTTONUP :
        printf("MBUTTON_UP  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_MOUSEMOVE :
#if 0
        printf("MOUSE_MOVE  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
#endif
        break;

      case WM_LBUTTONDBLCLK :
        printf("LBUTTON_DBLCLK  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_MBUTTONDBLCLK :
        printf("MBUTTON_DBLCLK  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      case WM_RBUTTONDBLCLK :
        printf("RBUTTON_DBLCLK  <%d,%d> [%x]\n",
     HIWORD(event.lParam), LOWORD(event.lParam), event.wParam);
        break;

      default :
        break;
    }

    if (!TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE))
      MOUSE_ShowCursor();
  } 
  MouseTerminate();
}


/* BIOSGetTime - return the current PC time */
unsigned long FAR PASCAL BIOSGetTime()
{
#if !defined(DOS286X) && !defined(DOS16M) && !defined(ERGO)
  /* Set up a pointer into BIOS Data Area */
  volatile unsigned long far *biostod = (long far *) 0x046C;
  return *biostod;
#else
  /* Can trash BIOS TOD Rollover flag -- DO NOT USE! */
  unsigned long currtime;
  union REGS in, out;

  in.h.ah = 0;
  INT86(0x1A, &in, &out);
  return currtime = ((unsigned long) out.x.cx << 16) | out.x.dx;
#endif
}

FAR PASCAL KBDGetShift()
{
  union REGS r;
  r.x.ax = 0x0200;
  INT86(0x16, &r, &r);
  return r.x.ax;
}

INT FAR PASCAL VidHideCursor(void)
{
  return 0;
}
INT FAR PASCAL VidSetCursorMode(int isblock, int nWidth)
{
  return 0;
}
#endif /* TEST */

void FAR PASCAL GMouseProcessMotion(void)        {}
int  FAR PASCAL GMouseInit(unsigned FAR *handle) { (void) handle; return 0; }
void FAR PASCAL GMouseTerminate(void)            {}
VOID FAR PASCAL GMouseGetStatus(int *button, MWCOORD *col, MWCOORD *row) { (void) button;  (void) col;  (void) row;}
VOID FAR PASCAL GMouseShowCursor(void)           {}
VOID FAR PASCAL GMouseHideCursor(void)           {}


#ifdef DOS16M
#if 0
void cdecl FAR MouseHandler()
{
  _gotMouseInt++;
}
#endif
#endif /* DOS16M */


#if defined(USE_MOUSEINTERRUPT)
VOID FAR PASCAL MouseProcessInterrupt(void)
{
  MOUSEQUEUEELEMENT mqe;

  mqe.x      = _colPosition;
  mqe.y      = _rowPosition;
  mqe.button = _buttonState;
  QueueAddData(&MouseQueue, (PSTR) &mqe);
  DOSSEMCLEAR(&MouseQueue.semQueueEmpty);

  _gotMouseInt--;
}
#endif


#if defined(__DPMI16__) && defined(USE_MOUSEINTERRUPT)

unsigned short oldSS;
unsigned short oldSP;
unsigned short ourStackEnd;
unsigned short ourStack[128];


/*
  In 16-bit modes, data and code are offsets from different
  segments so we need a trick that allows us to get to the
  original data segment in order to modify the main program's
  global variables.  The trick used here is to declare a
  function that reserves enough space to store two bytes (the
  value of DS).  We never call this function, we use it as a
  placeholder for data in the code segment.  The #pragma option
  turns off standard stack frame so the the first location at
  storeDS is the two bytes reserved by the __emit__ statements.
*/
#pragma option -k-
static VOID storeDS(void)
{
  /*
    Reserve two bytes for DS.  NEVER CALL THIS FUNCTION!
  */
  __emit__(0x00);
  __emit__(0x00);
}

static VOID PASCAL DP16PrepMouseHandler(void)
{
  /*
    It's illegal to write to the code segment under DPMI16, so
    we need to create an alias for the code segment.  By
    default, this alias is a data selector.
  */
  asm
  {
    push ds
    push es

    /*
      Keep data segment for global variables in es.
    */
    push ds
    pop  es

    /* 
      Create data selector alias for code selector. Assumes success.
    */
    mov ax, 0Ah
    mov bx, SEG storeDS
    int 31h

    /*
      Store the global variable DS.
    */
    mov ds, ax
    mov ds:[storeDS], es

    /*
      Free alias selector.
    */
    mov ax, 0Ah
    mov bx, ds
    int 31h

    pop es
    pop ds
  }
}


VOID FAR CDECL MouseHandler(void)
{
  asm
  {
    push ds

    /*
      In 16-bit modes, we grab the data segment from the space
      reserved by the __emit__ statements in the storeDS function.
    */
    mov ds, cs:[storeDS]

    mov  _rowPosition, dx
    mov  _colPosition, cx
    mov  _buttonState, bx
    mov  _mickeyHoriz, si
    mov  _mickeyVert,  di
    mov  _gotMouseInt, 1

    /*
      Save the current stack so that we can switch over to the C stack
    */
    mov  oldSS, ss
    mov  oldSP, sp

    /*
      Switch over to the C stack
    */
    cli
    mov  ax, cs
    mov  ss, ax
    mov  sp, ourStackEnd
    sti
  }

  MouseProcessInterrupt();

  asm
  {
    cli
    mov  ss,oldSS
    mov  sp,oldSP
    sti

    pop ds
  }
}
#endif


