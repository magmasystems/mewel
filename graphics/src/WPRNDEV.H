/*===========================================================================*/
/*                                                                           */
/* File    : WPRNDEV.H                                                       */
/*                                                                           */
/* Purpose : Header file for MEWEL printer driver routines. For internal     */
/*           use by MEWEL routines.                                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/*===========================================================================*/

/*
  Device context
*/

typedef struct tagPrDevice PRDEVICE, FAR *LPPRDEVICE;

#ifdef MSC
typedef int (FAR PASCAL *prnenaproc)(LPPRDEVICE, LPSTR, LPSTR);
typedef int (FAR PASCAL *prndisproc)(LPPRDEVICE);
typedef int (FAR PASCAL *prndevcapproc)(LPPRDEVICE, int);
typedef int (FAR PASCAL *prntxtparproc) (LPPRDEVICE, LPINT, LPINT);
typedef int (FAR PASCAL *prndmodeproc)(LPPRDEVICE, HWND, HANDLE, LPSTR, LPSTR);
typedef int (FAR PASCAL *prnctlproc)(LPPRDEVICE, int, int, LPSTR, LPSTR);
typedef int (FAR PASCAL *prntextproc)(LPPRDEVICE, int, int, LPSTR, int);
typedef int (FAR PASCAL *prnbltproc)(LPPRDEVICE, int, int, int, int, LPSTR);
typedef int (FAR PASCAL *prnoutproc) (LPPRDEVICE, LPSTR, int);
typedef int (FAR PASCAL *prnabtproc) (HDC, short);
#else
typedef int FAR PASCAL (FAR *prnenaproc)(LPPRDEVICE, LPSTR, LPSTR);
typedef int FAR PASCAL (FAR *prndisproc)(LPPRDEVICE);
typedef int FAR PASCAL (FAR *prndevcapproc)(LPPRDEVICE, int);
typedef int FAR PASCAL (FAR *prntxtparproc) (LPPRDEVICE, LPINT, LPINT);
typedef int FAR PASCAL (FAR *prndmodeproc)(LPPRDEVICE, HWND, HANDLE, LPSTR, LPSTR);
typedef int FAR PASCAL (FAR *prnctlproc)(LPPRDEVICE, int, int, LPSTR, LPSTR);
typedef int FAR PASCAL (FAR *prntextproc)(LPPRDEVICE, int, int, LPSTR, int);
typedef int FAR PASCAL (FAR *prnbltproc)(LPPRDEVICE, int, int, int, int, LPSTR);
typedef int FAR PASCAL (FAR *prnoutproc) (LPPRDEVICE, LPSTR, int);
typedef int FAR PASCAL (FAR *prnabtproc) (HDC, short);
#endif

struct tagPrDevice
{
  prndmodeproc   lpfnDevModeProc;   /* DeviceMode */
  prndevcapproc  lpfnDevCapsProc;   /* DeviceCapabilities */
  prntxtparproc  lpfnTextParsProc;  /* TextPars */
  prndisproc     lpfnDisableProc;   /* Disable */
  prnctlproc     lpfnControlProc;   /* Control */
  prntextproc    lpfnTextOutProc;   /* TextOut */
  prnbltproc     lpfnBitbltProc;    /* BitBlt */

  prnabtproc     lpfnAbortProc;     /* Abort function (app defined) */
  prnoutproc     lpfnOutFunc;       /* Output function (device dependent) */

  LPSTR          driver_pars;       /* pointer to driver parameters */
  char           portname[82];      /* output "port" passed to CreateDC */
  char           drivername[32];    /* complete driver name */

  WORD           fState;            /* driver state */
#define PRSTATE_OPEN     0x0001     /* driver is open */
#define PRSTATE_IS_IC    0x0002     /* information context */
#define PRSTATE_OUT_FILE 0x0100     /* output is to file */
  char           outdosname[82];    /* Output file path for FILE: */
  int            outhandle;         /* File handle or BIOS printer number */
};

/* Defined in wprntdc.c */
extern LPPRDEVICE FAR PASCAL hPrDCToPrDevice(HDC);

/* Printer output routines used by printer drivers. Defined in wprntout.c */

extern int FAR PASCAL PrOutDummy (LPPRDEVICE, LPSTR, int);
extern int FAR PASCAL PrClosePort (LPPRDEVICE);
extern int FAR PASCAL PrOpenPort (LPPRDEVICE);

