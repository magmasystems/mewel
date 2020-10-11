/*===========================================================================*/
/*                                                                           */
/* File    : WINPRINT.H                                                      */
/*                                                                           */
/* Purpose : Prototypes for the MEWEL printing routines.                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#define _MEWEL_PRINTER_H

extern int FAR PASCAL PrTextOut(HDC hdc, int x, int y, LPSTR indata, int len);
extern int FAR PASCAL PrBitBlt(HDC hdc, int left, int top, int width, int lines, LPSTR indata);
extern int FAR PASCAL PrDeviceMode(HDC hdc, HWND hwnd, HANDLE instance, LPSTR devname, LPSTR output);
extern int FAR PASCAL PrGetTextPars(HDC hdc, int *width, int *lppage);

void FAR PASCAL InstallPrinterSupport (void);

