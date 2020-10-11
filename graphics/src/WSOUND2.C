/*===========================================================================*/
/*                                                                           */
/* File    : WSOUND2.C                                                       */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible sound functions                */
/*           These are mostly stubs.                                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


int WINAPI OpenSound(void)
{
  return TRUE;
}

void WINAPI CloseSound(void)
{
}

int WINAPI StartSound(void)
{
  return TRUE;
}

int WINAPI StopSound(void)
{
  return TRUE;
}

int WINAPI SetVoiceQueueSize(int nVoice, int cbQueue)
{
  (void) nVoice;
  (void) cbQueue;
  return TRUE;
}

int WINAPI SetVoiceNote(int nVoice, int value, int length, int cDots)
{
  (void) nVoice;
  (void) value;
  (void) length;
  (void) cDots;
  return TRUE;
}

int WINAPI SetVoiceAccent(int nVoice, int nTempo, int nVolume, int fnMode, int nPitch)
{
  (void) nVoice;
  (void) nTempo;
  (void) nVolume;
  (void) fnMode;
  (void) nPitch;
  return TRUE;
}

int WINAPI SetVoiceEnvelope(int nVoice, int nShape, int nRepeat)
{
  (void) nVoice;
  (void) nShape;
  (void) nRepeat;
  return TRUE;
}

int WINAPI SetVoiceSound(int nVoice, DWORD dwFreq, int nDuration)
{
  (void) nVoice;
#if defined(__DPMI32__)
  (void) dwFreq;  (void) nDuration;
#else
  SoundTone(32000/LOWORD(dwFreq), nDuration, 5);
#endif
  return TRUE;
}

int WINAPI SetVoiceThreshold(int nVoice, int cNotesThreshhold)
{
  (void) nVoice;
  (void) cNotesThreshhold;
  return TRUE;
}

int FAR* WINAPI GetThresholdEvent(void)
{
  return NULL;
}

int WINAPI GetThresholdStatus(void)
{
  return TRUE;
}

int WINAPI SetSoundNoise(int nSource, int nDuration)
{
  (void) nSource;
  (void) nDuration;
  return TRUE;
}

int WINAPI CountVoiceNotes(int n) /* SOUND.11 */
{
  (void) n;
  return 0;
}

int WINAPI SyncAllVoices(void) /* SOUND.12 */
{
  return 0;
}

int WINAPI WaitSoundState(int n) /* SOUND.13 */
{
  (void) n;
  return 0;
}

