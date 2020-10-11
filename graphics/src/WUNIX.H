/*===========================================================================*/
/*                                                                           */
/* File    : WUNIX.H                                                         */
/*                                                                           */
/* Purpose : Header file for UNIX specific declarations.                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems    All Rights Reserved   */
/*===========================================================================*/
#ifndef _WUNIX_H
#define _WUNIX_H

/* UNIX options */
#define WZV
#define MSC

#define pascal
#define cdecl
#define far
#define near

#ifdef DOS
#undef DOS
#endif

#if defined(MOTIF) && !defined(XWINDOWS)
#define XWINDOWS
#endif

/* Platform Dependencies */
#if !defined(i386) || defined(__GNUC__)
#define WORD_ALIGNED
#define ALIGN    sizeof(UINT)
#define MISALIGN (ALIGN - 1)
#endif

/*
  This is used to compensate for structure packing in bitmap, cursor,
  and icon resources.
*/
#define UNIX_STRUCTURE_PACKING

#endif  /* _WUNIX_H */
