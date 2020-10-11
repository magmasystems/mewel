/*===========================================================================*/
/*                                                                           */
/* File    : ACCEL.H                                                         */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#ifndef ACCEL_H
#define ACCEL_H

typedef struct accelerator
{
  UINT  key;
  int   id;
} ACCELERATOR, *PACCELERATOR;

#define MAXACCELS  64

#ifdef PROTOTYPES
ACCELERATOR *FAR PASCAL _AcceleratorSearch(ACCELERATOR *, UINT);
#else
ACCELERATOR *FAR PASCAL _AcceleratorSearch();
#endif

extern ACCELERATOR *CurrAccelTable;
extern ACCELERATOR *AccelTable[MAXACCELS+1];

#endif /* ACCEL_H */

