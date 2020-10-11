/*===========================================================================*/
/*                                                                           */
/* File    :  WININT24.C                                                     */
/*                                                                           */
/* Purpose :  Critical Error Handler                                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

#define MSG(x) pszCritErrorMsg[(x) - INTL_INT24_FIRST]
static char *pszCritErrorMsg[INTL_INT24_COUNT] =
{
  "Write protect error",
  "Unknown unit",
  "Drive not ready",
  "Unknown command",
  "Bad CRC error",
  "Bad request structure length",
  "Seek error",
  "Unknown media",
  "Sector not found",
  "Printer out of paper",
  "Write fault",
  "Read fault",
  "General failure",
  "Sharing violation",
  "Lock violation",
  "Invalid disk change",
  "FCB unavailable",
  "Sharing buffer overflow",
  "Code page mismatch",
  "Out of input",
  "Insufficant disk space",
  "%s on drive %c:!",
  "%s on device %s!"
};

UINT    Int24Err = 0;
PSTR    Int24ErrMsg;

#ifdef TEST
#define   lstrncpy   strncpy
#endif

extern void FAR PASCAL Int24Install(void);

#if defined(__ZTC__)
#include <int.h>
#if __cplusplus
extern "C"
  {
#endif
extern int cerror_open(void);
extern int cerror_close(void);
extern void far cerrorIntTrap(void);
int FAR Int24Handler(int far *ax, int far *di, int es, int di, int si, int dx, int cx, int bx, int bp);
#if __cplusplus
  }
#endif

#elif defined(__TSC__)
void FAR Int24Handler(unsigned deverror, unsigned errcode, unsigned FAR *devhdr);
#elif defined(MSC)
void FAR CDECL Int24Handler(unsigned deverror, unsigned errcode, unsigned FAR *devhdr);
#elif defined(__TURBOC__)
int     Int24Handler(int errval, int ax, int bp, int si);
#endif


static UINT _wErrorMode = FALSE;


UINT FAR PASCAL SetErrorMode(UINT wMode)
{
  UINT wOld = _wErrorMode;
  _wErrorMode = wMode;
  return wOld;
}


void FAR PASCAL Int24Install(void)
{
#ifdef INTERNATIONAL_MEWEL
  /*
    Read the error messages out of the resource file
  */
  static BOOL fMsgRead = FALSE;
  if (!fMsgRead) {
    /*
      We must open and close the resource file by hand, because
      MewelCurrOpenResourceFile contains -1 at this time
    */
    HANDLE hInst = OpenResourceFile(NULL);
    if (hInst != -1) {
      int i;
      for (i = 0;  i < INTL_INT24_COUNT;  i++) {
        char acBuffer[80];
        if (LoadString(hInst, INTL_INT24_FIRST + i, acBuffer, 80) > 0) {
          pszCritErrorMsg[i] = strdup(acBuffer);
        }
      }
      CloseResourceFile(hInst);
    }
    fMsgRead = TRUE;
  }
#endif /* INTERNATIONAL_MEWEL */

#if defined(DOS386) || defined(__DPMI16__) || defined(__DPMI32__)
#elif defined(__ZTC__)
  cerror_open();
#elif defined(MSC)
  _harderr(Int24Handler);
#elif defined(__TURBOC__)
  harderr(Int24Handler);
#endif
}

void FAR CDECL Int24Restore(void)
{
#if defined(__ZTC__) && !defined(DOS386)
  cerror_close();
#endif
}



#if defined(__ZTC__)
int FAR Int24Handler(int far *pax, int far *pdi, int es, int di, int si, int dx, int cx, int bx, int bp)
{
  static char msg[80];
  char *em;
  int  errval = *pdi;
  
  em = (errval >= 0 && errval < INTL_INT24_COUNT-2) ? pszCritErrorMsg[errval]
                                                    : MSG(GENERAL_FAILURE);
  if (*pax >= 0)
    sprintf(msg, MSG(INTL_ON_DRIVE), em, (*pax & 0xFF) + 'A');
  else
  {
    char device[9];
    unsigned FAR *devhdr;

    devhdr = (unsigned FAR *) MK_FP(bp, si);
    lstrncpy((char FAR *) device, (char FAR *) &devhdr[5], 8);
    device[8] = '\0'; 
    sprintf(msg, MSG(INTL_ON_DEVICE), em, device);
  }

  /*
     The error message is nicely formatted in the variable msg[].
     The next line should be application-dependent code to output the
     error message....
  */
  Int24ErrMsg = msg;

  /*
    Set a global flag which says that we have an INT 24 error
  */
  if (_wErrorMode == FALSE)
    SET_INT24_ERR(errval);
  *pax = 0;
  return 0;
}


#elif defined(MSC)
#if defined(__TSC__)
void FAR Int24Handler(deverror, errcode, devhdr)
#else
void FAR CDECL Int24Handler(deverror, errcode, devhdr)
#endif
  unsigned deverror, errcode;
  unsigned FAR *devhdr;
{
  static char msg[80];
  char *em;

  em = (errcode < INTL_INT24_COUNT-2) ? pszCritErrorMsg[errcode]
                                      : MSG(INTL_GENERAL_FAILURE);
  if (!(deverror & 0x8000))
    sprintf(msg, MSG(INTL_ON_DRIVE), em, (deverror & 0xFF) + 'A');
  else
  {
    char device[9];
    lstrncpy((char FAR *) device, (char FAR *) &devhdr[5], 8);
    device[8] = '\0'; 
    sprintf(msg, MSG(INTL_ON_DEVICE), em, device);
  }

  /*
     The error message is nicely formatted in the variable msg[].
     The next line should be application-dependent code to output the
     error message....
  */
  Int24ErrMsg = msg;

  /*
    Set a global flag which says that we have an INT 24 error
  */
  if (_wErrorMode == FALSE)
    SET_INT24_ERR(errcode);

#if !defined(EXTENDED_DOS)
  _hardretn(-1);
#endif
}


#elif defined(__TURBOC__)
int     Int24Handler(int errval, int ax, int bp, int si)
{
  static char msg[80];
  char *em;
  
  em = (errval >= 0 && errval < INTL_INT24_COUNT-2) ? pszCritErrorMsg[errval]
                                                    : MSG(INTL_GENERAL_FAILURE);
  if (ax >= 0)
    sprintf(msg, MSG(INTL_ON_DRIVE), em, (ax & 0xFF) + 'A');
  else
  {
    char device[9];
    unsigned FAR *devhdr;
    devhdr = (unsigned FAR *) MK_FP(bp, si);
    lstrncpy((char FAR *) device, (char FAR *) &devhdr[5], 8);
    device[8] = '\0'; 
    sprintf(msg, MSG(INTL_ON_DEVICE), em, device);
  }

  /*
     The error message is nicely formatted in the variable msg[].
     The next line should be application-dependent code to output the
     error message....
  */
  Int24ErrMsg = msg;

  /*
    Set a global flag which says that we have an INT 24 error
  */
  if (_wErrorMode == FALSE)
    SET_INT24_ERR(errval);
#if !defined(DOS286X) && !defined(__DPMI16__) && !defined(__DPMI32__)
  hardretn(0);
#endif
  return 0;
}
#endif




/*#define TEST*/
#ifdef TEST
main()
{
  FILE *f;
  
  Int24Install();
  CLR_INT24_ERR();

  f = fopen("A:FOO", "r");
  if (IS_INT24_ERR())
    printf("We got an int24 error with code %d\n", GET_INT24_ERR());
  printf("This is right after the call to fopen - f is %x\n", f);
  Int24Restore();
}
#endif
