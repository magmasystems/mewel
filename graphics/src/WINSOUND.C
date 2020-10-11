/*===========================================================================*/
/*                                                                           */
/* File    : SOUND.C                                                         */
/*                                                                           */
/* Purpose : Sound routines                                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/
#define INCLUDE_CURSES
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_SUB

#include "wprivate.h"
#include "window.h"

#if defined(__WATCOMC__) || defined(__HIGHC__) || defined(PLTNT)
#include <conio.h>       /* for inp(), outp() */
#if defined(PLTNT) && !defined(__WATCOMC__)
#define inp   _inp
#define outp  _outp
#endif
#endif
#if defined(__GNUC__) && defined(DOS)
#include <pc.h>
#define inp   inportb
#define outp  outportb
#endif

/*****************************************************************************/
/* SoundNote()                                                              */
/*                                                                          */
/*      Purpose: Allows musical tones to be generated from the              */
/*               speaker.  The control parameters are in terms              */
/*               of notes, octaves and durations.  This function            */
/*               can be made silent with the audible routine.               */
/*                                                                          */
/*      Inputs:  The parameter note controls the note to be emitted.        */
/*               It can be set to one of the following pitch values:        */
/*                                                                          */
/*                  RESTNOTE               - Rest (silence for duration)    */
/*                  CNOTE, CsNOTE          - C, C sharp                     */
/*                  DNOTE, DsNOTE          - D, D sharp                     */
/*                  ENOTE, FNOTE, FsNOTE   - E, F, F sharp                  */
/*                  GNOTE, GsNOTE          - G, G sharp                     */
/*                  ANOTE, AsNOTE, BNOTE   - A, A sharp, B                  */
/*                                                                          */
/*                              -also-                                      */
/*                  Dbnote                 - D flat equals C sharp          */
/*                  Ebnote                 - E flat equals D sharp          */
/*                  Gbnote                 - G flat equals F sharp          */
/*                  Abnote                 - A flat equals G sharp          */
/*                  Bbnote                 - B flat equals A sharp          */
/*                                                                          */
/*                  PAUSENOTE              - Program idle (non-maskable)    */
/*                                                                          */
/*               The parameter octave is the octave in which to play        */
/*               the note, this ranges from MIN_OCTAVE to MAX_OCTAVE        */
/*               with the higher octaves almost inaudible.  An octave       */
/*               value of MID_OCTAVE and note value of CNOTE will           */
/*               give middle C.  The parameter duration is the              */
/*               duration of the note in milliseconds.                      */
/*                                                                          */
/*****************************************************************************/

#if !defined(UNIX) && !defined(VAXC)

void FAR PASCAL SoundNote(note, octave, duration)
  int      note, octave;
  unsigned duration;
{
  static unsigned scale[13] =
  {
    0,34334,36376,38539,40830,43258,45830,48556,51443,54502,57743,61176,64814
  };
 
  if (note == PAUSENOTE)
    note = RESTNOTE;

  if (!note)
    SoundTone((unsigned) 64000, 0, duration);
  else if (note >= CNOTE && note <= BNOTE && 
           octave >= MIN_OCTAVE && octave <= MAX_OCTAVE)
  {
    int frequency = scale[note] >> (7-octave);
    SoundTone(frequency, duration, 1);
  }
}


 
/*
  SoundTone(freq, duration, pause)
    freq     - cycles per second
    duration - period of square waves in milliseconds
    pause    - period of quiet calibrated in milliseconds
*/
void FAR PASCAL SoundTone(freq, duration, pause)
  unsigned freq,
           duration,
           pause;
{ 
  unsigned char save_8255;
  unsigned div;

#ifdef DOS
  /*
    Set channel 2 of the timer to act as a frequency divider
  */
  outp(0x43, 0xB6);

  /*
    Send low-order byte of tone to the control port, then the high byte
  */
  div = (unsigned) ((unsigned long) 0x1234DE / (unsigned long) freq);
  outp(0x42, (unsigned char) (div & 0x00FF));
  outp(0x42, (unsigned char) ((div >> 8) & 0x00FF));

  /* 
    Turn the speaker on
  */
  save_8255 = (unsigned char) inp(0x61);
  outp(0x61, save_8255 | 0x03);

  DosSleep(duration);       /* keep the sound going  */
  outp(0x61, save_8255);    /* turn off the speaker  */
  if (pause)
    DosSleep(pause);        /* insure adequate quiet */
#endif

#ifdef OS2
  DosBeep(freq, duration);
  if (pause)
    DosSleep((ULONG) pause);
#endif
}


void FAR PASCAL MessageBeep(i)
  int i;
{
  (void) i;
  SoundTone(900, 100, 5); 		/* Standard error tone */
}

void FAR PASCAL SoundClick(void)
{
  int i;
  for (i = 1;  i <= 6;  i++)
    SoundTone(1072, 5, 1);
}
 
#endif


#if defined(UNIX) || defined(VAXC)

#if defined(XWINDOWS)
#if defined(beep)
#undef beep
#endif
/*
  beep() should be XBell(display, iPercent)
*/
#define beep()  XBell(XSysParams.display, 100)
#endif


void FAR PASCAL SoundNote(note, octave, duration)
  int      note, octave;
  unsigned duration;
{
  beep();
}

void FAR PASCAL SoundTone(freq, duration, pause)
  unsigned freq,
           duration,
           pause;
{ 
  beep();
}

void FAR PASCAL MessageBeep(i)
  int i;
{
  SoundTone(900, 100, 5); 		/* Standard error tone */
}

void FAR PASCAL SoundClick()
{
  SoundTone(1072, 5, 1);
}

#endif /* UNIX */
 

#ifdef TEST

main()
{
  int i;
  int octave;

  printf("Doing beep....");  getch();
  MessageBeep(0);
  printf("Doing click...");  getch();
  SoundClick();

  printf("Doing scales...");  getch();
  for (octave = MIN_OCTAVE;  octave < MAX_OCTAVE;  octave++)
    for (i = 1;  i <= 12;  i++)
      SoundNote(i, octave, 250);
}

#endif
