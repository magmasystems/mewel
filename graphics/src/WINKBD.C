/*===========================================================================*/
/*                                                                           */
/* File    : WKBDREAD.C                                                      */
/*                                                                           */
/* Purpose : Keyboard handling functions                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
/*
  Thomas Wagner's international keyboard driver does not work correctly
  with MS-DOS 5. As I am not able to find the bug, switching it totally off
  is the easiest solution -- Wolfgang Lorenz
*/
#undef INTERNATIONAL_MEWEL

#include "wprivate.h"
#include "window.h"
#include "winevent.h"


#if defined(OS2)
#define COOKED 0x04
#define RAW    0x08
static KBDINFO OrigKbdStatus;
#endif

#if defined(DOS) && defined(INTERNATIONAL_MEWEL)
#include "keydrv.h"
#endif

#if defined(__DPMI32__)
#include <wincon.h>
#define __NTCONSOLE__

static INT PASCAL NTShiftToMEWELShift(DWORD);

static INT CurrShiftCode = 0;
NTCONSOLEINFO NTConsoleInfo;
#endif


/*
  Prototypes for private functions
*/
static VOID PASCAL CheckEnhancedKybd(void);
static BOOL PASCAL IsPCAT(void);

/*
  Variables
*/
static BYTE IsEnhancedKybd = FALSE;
static BYTE _chCurrScanCode = 0;   /* for WM_CHAR translation */


/****************************************************************************/
/*                                                                          */
/* Function : KBDInit(void)                                                 */
/*                                                                          */
/* Purpose  : System-dependent function to initialize the keyboard handling */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL KBDInit(void)
{
#if defined(__NTCONSOLE__)
  NTConsoleInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(NTConsoleInfo.hStdInput, &NTConsoleInfo.dwConsoleMode);
#elif defined(OS2)
  KBDINFO status;
  status.cb = sizeof(status);
  KbdGetStatus(&status, 0);
  OrigKbdStatus = status;
  status.fsMask &= ~COOKED;
  status.fsMask |= RAW;
  KbdSetStatus(&status, 0);
#elif defined(INTERNATIONAL_MEWEL)
  IsEnhancedKybd = (BYTE) install_kb_driver();
#elif defined(DOS)
  CheckEnhancedKybd();
  Int9Init();
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : KBDTerminate(void)                                            */
/*                                                                          */
/* Purpose  : System-dependent function to restore the kbd before exiting.  */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL KBDTerminate(void)
{
#if defined(OS2)
#if 0
  /*
    10/17/90 (maa) - we seem to get a hang here
  */
  KbdSetStatus(&OrigKbdStatus, 0);
#endif
#elif defined(INTERNATIONAL_MEWEL)
  disable_kb_driver();
#elif defined(DOS)
  Int9Terminate();
#endif
}


#if defined(OS2) || defined(INTERNATIONAL_MEWEL) || defined(DOS)
/****************************************************************************/
/*                                                                          */
/* Function : KBDRead(void)                                                 */
/*                                                                          */
/* Purpose  : Main routine to fetch a character from the keyboard. Poll the */
/*            keyboard without waiting.                                     */
/*                                                                          */
/* Returns  : The virtual keycode of the key which was pressed, or 0 if no  */
/*            key was pressed.                                              */
/****************************************************************************/
VKEYCODE FAR PASCAL KBDRead(void)
{
  INT chCode, scanCode, shiftCode;

  static INT oldShiftCode = 0;

  /*
    We have common code for the DOS and OS/2 versions which provides
    special decoding of the keyboard to account for the keypad and
    for foreign characters.
  */

#ifdef OS2
  KBDKEYINFO keydata;

  /*
    Did the user call EnableHardwareInput(FALSE) ?
  */
  if (TEST_PROGRAM_STATE(STATE_HARDWARE_OFF))
    return 0;

  /*
    Read the keystroke and extract the scan code, char code, and shift state.
  */
#if defined(NOTHREADS)
  KbdCharIn(&keydata, IO_NOWAIT, 0);        /* Don't wait for a keystroke */
#else
  KbdCharIn(&keydata, IO_WAIT, 0);          /* Wait for a keystroke */
#endif
  if (keydata.fbStatus == 0)
  {
    /*
      See if the CAPSLOCK, NUMLOCK or SCROLLLOCK state has changed
    */
    shiftCode = KBDGetShift();
    if ((shiftCode & (NUM_SHIFT | CAP_SHIFT | SCL_SHIFT)) != oldShiftCode)
    {
      if      ((shiftCode & NUM_SHIFT) != (oldShiftCode & NUM_SHIFT))
        chCode = VK_NUMLOCK;
      else if ((shiftCode & CAP_SHIFT) != (oldShiftCode & CAP_SHIFT))
        chCode = VK_CAPITAL;
      else if ((shiftCode & SCL_SHIFT) != (oldShiftCode & SCL_SHIFT))
        chCode = VK_SCROLL;
      oldShiftCode = shiftCode;
      return chCode;
    }
    return 0;
  }
  scanCode   = (int) (_chCurrScanCode = keydata.chScan);
  shiftCode  = keydata.fsState;
  chCode     = keydata.chChar;
#endif /* OS2 */


#if defined(__NTCONSOLE__)
  INPUT_RECORD eventBuf;
  DWORD        dwPendingEvent;

  GetNumberOfConsoleInputEvents(NTConsoleInfo.hStdInput, &dwPendingEvent);
  if (dwPendingEvent == 0)
  {
    /*
      See if the CAPSLOCK, NUMLOCK or SCROLLLOCK state has changed
    */
    shiftCode = KBDGetShift();
    if ((shiftCode & (NUM_SHIFT | CAP_SHIFT | SCL_SHIFT)) != oldShiftCode)
    {
      if      ((shiftCode & NUM_SHIFT) != (oldShiftCode & NUM_SHIFT))
        chCode = VK_NUMLOCK;
      else if ((shiftCode & CAP_SHIFT) != (oldShiftCode & CAP_SHIFT))
        chCode = VK_CAPITAL;
      else if ((shiftCode & SCL_SHIFT) != (oldShiftCode & SCL_SHIFT))
        chCode = VK_SCROLL;
      oldShiftCode = shiftCode;
      return chCode;
    }
    return 0;
  }

  PeekConsoleInput(NTConsoleInfo.hStdInput, &eventBuf, 1, &dwPendingEvent);
  if (eventBuf.EventType != KEY_EVENT)
    return 0;

  ReadConsoleInput(NTConsoleInfo.hStdInput, &eventBuf, 1, &dwPendingEvent);
  if (!dwPendingEvent || !eventBuf.Event.KeyEvent.bKeyDown)
    return 0;

  scanCode   = (int) (_chCurrScanCode = eventBuf.Event.KeyEvent.wVirtualScanCode);
  shiftCode  = NTShiftToMEWELShift(eventBuf.Event.KeyEvent.dwControlKeyState);
  CurrShiftCode = shiftCode;
  chCode     = eventBuf.Event.KeyEvent.uChar.AsciiChar;

  /*
    Translate special codes into MEWEL keys
  */
  if (chCode == 0 && (CurrShiftCode & (ALT_SHIFT | CTL_SHIFT | SHIFT_SHIFT)))
  {
    static USHORT ausShKeyToMEWELKey[89] =
    {
         0,      0,      0,      0,      0,      0,      0,      0, /* 00-07 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 08-15 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 16-23 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 24-31 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 32-39 */
         0,      0,     15,      0,      0,      0,      0,      0, /* 40-47 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 48-55 */
         0,      0,      0,     84,     85,     86,     87,     88, /* 56-63 */
        89,     90,     91,     92,     93,      0,      0,     71, /* 64-71 */
         0,      0,      0,      0,      0,      0,      0,     79, /* 72-79 */
         0,      0,      0,      0,      0,      0,      0,    135, /* 80-87 */
       136                                                          /* 88    */
    };

    static USHORT ausCtlKeyToMEWELKey[89] =
    {
         0,      0,      0,      0,      0,      0,      0,      0, /* 00-07 */
         0,      0,      0,      0,      0,      0,    127,    148, /* 08-15 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 16-23 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 24-31 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 32-39 */
         0,      0,      0,      0,      0,      0,      0,      0, /* 40-47 */
         0,      0,      0,      0,      0,      0,      0,    150, /* 48-55 */
         0,      0,      0,     94,     95,     96,     97,     98, /* 56-63 */
        99,    100,    101,    102,    103,      0,      0,    119, /* 64-71 */
       141,    132,    142,    115,    143,    116,    144,    117, /* 72-79 */
       145,    118,    146,    147,      0,      0,      0,    137, /* 80-87 */
       138                                                          /* 88    */
     };

    static USHORT ausAltKeyToMEWELKey[89] =
    {
         0,      0,    120,    121,    122,    123,    124,    125, /* 00-07 */
       126,    127,    128,    129,    130,    131,     14,    165, /* 08-15 */
        16,     17,     18,     19,     20,     21,     22,     23, /* 16-23 */
        24,     25,      0,      0,      0,      0,     30,     31, /* 24-31 */
        32,     33,     34,     35,     36,     37,     38,      0, /* 32-39 */
         0,      0,      0,      0,     44,     45,     46,     47, /* 40-47 */
        48,     49,     50,      0,      0,      0,      0,     55, /* 48-55 */
         0,      0,      0,    104,    105,    106,    107,    108, /* 56-63 */
       109,    110,    111,    112,    113,      0,      0,    151, /* 64-71 */
       152,    153,     74,    155,     76,    157,     78,    159, /* 72-79 */
       160,    161,    162,    163,      0,      0,      0,    139, /* 80-87 */
       140                                                          /* 88    */
     };

    USHORT *pusTable;
    if      (CurrShiftCode & ALT_SHIFT)
      pusTable = ausAltKeyToMEWELKey;
    else if (CurrShiftCode & CTL_SHIFT)
      pusTable = ausCtlKeyToMEWELKey;
    else if (CurrShiftCode & SHIFT_SHIFT)
      pusTable = ausShKeyToMEWELKey;
    if (pusTable[scanCode])
      scanCode = (INT) pusTable[scanCode];

  }

  /*
    Force chCode to be 0 if the ALT+letter key is pressed.
  */
  else if ((CurrShiftCode & ALT_SHIFT))
  {
    /*
      We pressed ALT+letter... translate into MAKEKEY(scanCode, ALT)
    */
    if (chCode > ' ')
      chCode = 0;
  }
#endif /* NTCONSOLE */


#if defined(DOS) && !defined(__NTCONSOLE__)
  union REGS r;
  extern BYTE BrkFound;

  /*
    Handle CTRL-BREAK situation by returning a virtual keycode
  */
  if (BrkFound)
  {
    BrkFound = FALSE;
    return VK_CTRL_BREAK;
  }

  /*
    Did the user call EnableHardwareInput(FALSE) ?
  */
  if (TEST_PROGRAM_STATE(STATE_HARDWARE_OFF))
    return 0;

#ifdef INTERNATIONAL_MEWEL
  if (!kbdrv_keyavail())
    return 0;
  return kbdrv_getkey();
#else
  chCode = keyready(IsEnhancedKybd);   /* Check for a keypress    */
  shiftCode = KBDGetShift();
  if (chCode == 0)
  {
    /*
      See if the CAPSLOCK, NUMLOCK or SCROLLLOCK state has changed
    */
    if ((INT) (shiftCode & (NUM_SHIFT | CAP_SHIFT | SCL_SHIFT)) != (INT) oldShiftCode)
    {
      if      ((shiftCode & NUM_SHIFT) != (oldShiftCode & NUM_SHIFT))
        chCode = VK_NUMLOCK;
      else if ((shiftCode & CAP_SHIFT) != (oldShiftCode & CAP_SHIFT))
        chCode = VK_CAPITAL;
      else if ((shiftCode & SCL_SHIFT) != (oldShiftCode & SCL_SHIFT))
        chCode = VK_SCROLL;
      oldShiftCode = shiftCode;
      return chCode;
    }
    return 0;
  }

  /*
    Read the keystroke and extract the scan code, char code, and shift state.
  */
  r.h.ah = (BYTE) (IsEnhancedKybd ? 0x10 : 0x00);
  if (TEST_PROGRAM_STATE(STATE_JUST_POLL_EVENTS))
    r.h.ah++;    /* use the 'just poll' service of INT 16H */
#if defined(__DPMI16__)
  _AX = r.x.ax;
  geninterrupt(0x16);
  r.x.ax = _AX;
#else
  INT86(0x16, &r, &r);
#endif
  scanCode  = _chCurrScanCode = (BYTE) (r.h.ah);
  chCode    = r.h.al;
#endif
#endif /* DOS */


  /*
    See if a function key was pressed. A function key returns an ASCII
    code of either 0 or 224, or it can be one of the keys on the
    keypad.

->  Note for European 102-key keyboard users. Your 102nd key is a
    backslash (\), and generates a scan-code of 86. Unfortunately, so
    does the SHIFT-F3 key. At the expense of not being able to use the
    SHIFT-F3 key, you can add " && scanCode != 86" to the if statement
    below in order to recognize the backslash. IE -
      (scanCode >= 55 && scanCode != 57 && scanCode != 86))
                                        =================
  */
  if (chCode == 0 || (chCode == 224 && scanCode != 0) || 
      (scanCode >= 55 && scanCode != 57)) /* 57 = SPACEBAR */
  {
    if (scanCode == 86 && chCode != 0)
      goto bye;

    shiftCode &= 0x00FF;
    if (shiftCode & SHIFT_SHIFT)
      shiftCode |= SHIFT_SHIFT;    /* turn both left and right shifts on */

    /*
      If the user pressed the GREY plus, minus, star, divide, or enter
      keys, then just return the ASCII codes.
    */
#ifdef MODSOFT
    /*
     The original code did not properly process all combinations of arrows,
     grey arrow, arrows and grey arrows with NUMLOCK on, enter and grey
     enter without and with NUMLOCK, etc.  I don't know if my code is
     correct in all cases, but it seems to work for me...
    */
    if ((chCode != 0) && (chCode != 224))
#else
    if (scanCode == 74 ||    /* GREY MINUS (scan=74,ch=-) */
        scanCode == 78 ||    /* GREY PLUS  (scan=78,ch=+) */
        scanCode == 55 ||    /* GREY STAR  (scan=55,ch=*) */
        scanCode == 224 &&
                (chCode=='\r' || chCode=='/'))  /* GREY / or GREY ENTER */
      ;
    else
    /*
      If a key on the numeric keypad was pressed while NUMLOCK was on,
      and the CTRL, ALT or SHIFT keys weren't pressed, then return the
      number.
    */
    if ((shiftCode & (NUM_SHIFT | 0x0F)) == NUM_SHIFT && /* only numlock on */
	 chCode != 224 && chCode != 0 &&  /* not a grey cursor movement key */
	(scanCode>=71 && scanCode<=83 || scanCode==224 || scanCode==55))
#endif
      ;
    else
      chCode = MAKEKEY(scanCode, (shiftCode & ~TOGGLE_SHIFT_MASK));
  }
#ifdef KBD_OEM2ANSI
  else
  {
    static char MEWELOem2AnsiTbl[]=
    {
       199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236, 196, 197,
      201, 230, 198, 244, 246, 242, 251, 249, 255, 214, 220, 162, 163, 165, 182/*Pt->Paragraph*/, 131,
      225, 237, 243, 250, 241, 209, 170, 186, 191, 169/*©*/, 172, 189, 188, 161, 171, 187, 
      /* Graphic Symbols are not available in ANSI. */
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   151,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      'à', 223, 'â', 'ã', 'ä', 'å', 181, 'ç', 'è', 'é', 'ê', 'ë', 'ì', 'í', 'î', 'ï',
      'ð', 241, 'ò', 'ó', 'ô', 'õ', 247, '÷', 176, 183, 183, 'û', 'ü', 178, 128, 32
    };
    if (chCode > 127)
      chCode = MEWELOem2AnsiTbl[chCode-128];
  }
#endif /* KBD_OEM2ANSI */

bye:
  SysEventInfo.chLastScan = (UINT) scanCode;
  return (VKEYCODE) chCode;
}
#endif

#ifdef KBD_OEM2ANSI
#if defined(MSC) && !defined(WATCOM) && defined(MEWEL_GUI)
/* This function overrides the standard translatiion function in Microsofts
   graphic library. Microsoft assumes that the program uses IBM-ASCII. Since
   the Microsoft Font Files are coded in ANSI, this function was neccessary
   to translate the ASCII strings of the program to the ANSI strings of the
   font file. Since we use ANSI strings in the program, we don't need this
   function to do anything.
*/
near _ChrToAnsi(int x);
#pragma alloc_text(_GR_TEXT, _ChrToAnsi)
near _ChrToAnsi(x)
int x;
{
   return x;
}
#endif  /* MSC */
#endif /* KBD_OEM2ANSI */



#if defined(DOS) && !defined(INTERNATIONAL_MEWEL) && !defined(__NTCONSOLE__)
/*===========================================================================*/
/*                                                                           */
/* Function: CheckEnhancedKybd(void)                                         */
/*                                                                           */
/* Purpose : Sets IsEnhancedKeybd TRUE if enhanced keyboard is present.      */
/*                                                                           */
/*===========================================================================*/
static VOID PASCAL CheckEnhancedKybd(void)
{
  union REGS r;
  unsigned shift;
  
  IsEnhancedKybd = FALSE;

  /* 
    If we do not have a PC/AT, then no extended keyboard is present
  */
  if (!IsPCAT())
    return;

  /* 
    Get the shift status and save it
  */
  shift = KBDGetShift();

  /*
    Put a bogus value into al and try the extended bios call to get 
    the shift status. If the value of al hasn't changed, then the
    enhanced bios is not present.
  */
  r.h.al = (BYTE) ~r.h.al;
  r.h.ah = (BYTE) 0x12;
#if defined(__DPMI16__)
  _AX = r.x.ax;
  geninterrupt(0x16);
  r.h.al = _AL;
#else
  INT86(0x16, &r, &r);
#endif
  if (r.h.al != (BYTE) shift)
    return;
    
  /* 
    Now we know that the enhanced bios is present.
    We now check if the enhanced keyboard is attached.
  */
#if !42993
  /*
    4/29/93 (maa)
      Televoice says that certain 88-key keyboard can generate extended
      key sequences. (Like the Maxiswitch kbd)
  */
  if (*GetBIOSAddress(0x96) & 0x10)
#endif
    IsEnhancedKybd = TRUE;
}
#endif /* DOS && !INTERNATIONAL_MEWEL */


#if defined(OS2) || defined(DOS)
/*===========================================================================*/
/*                                                                           */
/* Function: KBDGetShift(void)                                               */
/*                                                                           */
/* Purpose : Returns the BIOS shift status of the keyboard                   */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL KBDGetShift(void)
{
#if defined(__NTCONSOLE__)
  return CurrShiftCode;

#elif defined(OS2)
  KBDINFO kbdstatus;
  kbdstatus.cb = sizeof(kbdstatus);
  KbdGetStatus(&kbdstatus, 0);
  return kbdstatus.fsState;

#elif defined(DOS)
#if defined(__DPMI16__)
  _AH = (BYTE) (IsEnhancedKybd ? 0x12 : 0x02);
  geninterrupt(0x16);
  return _AL;
#else
  union REGS r;
  r.h.ah = (BYTE) (IsEnhancedKybd ? 0x12 : 0x02);
  INT86(0x16, &r, &r);
  return r.h.al;
#endif
#endif
}
#endif



/*
  MEWEL WM_CHAR to Windows WM_CHAR translator
*/
VOID FAR PASCAL _MEWELtoWindows_WMCHAR(pwParam, plParam)
  WPARAM *pwParam;
  LPARAM *plParam;
{
  DWORD lParam, lParamNew;

  /*
    wParam is the value of the key. This should stay the same.
  */
  lParam = *plParam;

  /*
    lParam is devided up as follows :
      LOWORD(lParam) - repeat count
      LOBYTE(HIWORD(lParam)) - OEM-dependent scan code
      HIBYTE(HIWORD(lParam)) - 
        bit 24      - 1 if extended key, 0 if not
            25-26   - not used
            27-28   - reserved
            29      - 1 if ALT key pressed
            30      - previous key state, 1 if key already down
            31      - transition state, 1 if key is being released
  */
  lParamNew = 1L;                       /* repeat count in LOWORD is 1 */
  lParamNew |= (((LONG) _chCurrScanCode) << 16); /* scan code */
  if (LOWORD(lParam) & ALT_SHIFT)       /* ALT key indicator */
    lParamNew |= 0x20000000L;
  if (HIBYTE(*pwParam) != 0)            /* extended key indicator */
    lParamNew |= 0x01000000L;

  *plParam = lParamNew;
}


static BOOL PASCAL IsPCAT(void)
{
#if defined(EXTENDED_DOS)
  return TRUE;
#else
  return ((BYTE) * (LPSTR) 0xF000FFFE) == (BYTE) 0xFC;
#endif
}


FARADDR PASCAL GetBIOSAddress(offset)
  WORD offset;
{
  FARADDR lpMem;

#if defined(__DPMI16__) || defined(__DPMI32__)
  static USHORT selBios = 0;

  if (selBios == 0)
  {
    _AX = 2;
    _BX = 0x40;
    geninterrupt(0x31);
    selBios = _AX;
  }
  lpMem = (FARADDR) MK_FP(selBios, offset);


#elif defined(DOS286X)
  SEL selBios;
  DosGetBIOSSeg(&selBios);
  lpMem = (FARADDR) MK_FP(selBios, offset);

#elif defined(ERGO)
  SEL selBios;
  ParaToSel((USHORT) 0x40, &selBios);
  lpMem = (FARADDR) MK_FP(selBios, offset);

#elif defined(DOS386)
  lpMem = (FARADDR) MK_FP(_x386_zero_base_selector, (0x400 + offset));

#elif defined(PL386)
#if defined(__HIGHC__) || defined(__WATCOMC__)
  MK_FARP(lpMem, 0x34, (0x400 + offset));
#else
  lpMem = (FARADDR) MK_FP(0x34, (0x400 + offset));
#endif

#elif defined(WC386)
  lpMem = (FARADDR) (0x00000400 + offset);

#elif defined(__GNUC__)
  lpMem = (FARADDR) (0xE0000400 + offset);

#else
  lpMem = (FARADDR) (0x00400000 + offset);

#endif

  return lpMem;
}


INT FAR PASCAL GetKeyboardType(fnKeybInfo)
  int fnKeybInfo;
  /*
    0 = determine the keyboard type
    1 = determine the keyboard subtype  (not used)
    2 = determine the number of function keys
  */
{
#if defined(DOS)
  /*
    The types we deal with are :
      1  PC/XT (83 keys, 10 funcs)
      3  PC/AT (84 keys, 10 funcs)
      4  Enhanced (101 or 102 keys, 12 funcs)
  */

  /*
    If we do not have an AT-class machine, then we have a PC/XT keyboard
    (type 1) with 10 function keys.
  */
  if (!IsPCAT())
    return (fnKeybInfo == 2) ? 10 : 1;
  else if (IsEnhancedKybd)
    return (fnKeybInfo == 2) ? 12 : 4;
  else
    return (fnKeybInfo == 2) ? 10 : 3;

  /*
    Not successful? Return 0.
  */
#else /* ! DOS */
  return 0;
#endif
}


#if defined(__NTCONSOLE__)
static INT PASCAL NTShiftToMEWELShift(dwFlags)
  DWORD dwFlags;
{
  INT dwShift = 0;
  INT i;

  static INT aiShift[][2] =
  {
    { (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED),  ALT_SHIFT   },
    { (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED), CTL_SHIFT   },
    { SHIFT_PRESSED,                            SHIFT_SHIFT },
    { NUMLOCK_ON,                               NUM_SHIFT   },
    { SCROLLLOCK_ON,                            SCL_SHIFT   },
    { CAPSLOCK_ON,                              CAP_SHIFT   },
    /*
      What about INS_SHIFT?
    */
  };

  for (i = 0;  i < sizeof(aiShift)/sizeof(aiShift[0]);  i++)
    if (dwFlags & aiShift[i][0])
      dwShift |= aiShift[i][1];
  return dwShift;
}
#endif

