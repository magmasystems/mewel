/*===========================================================================*/
/*                                                                           */
/* File    : WINQUEUE.C                                                      */
/*                                                                           */
/* Purpose : Event-queue management routines                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/
#define INCLUDE_OS2

#include "wprivate.h"
#include "window.h"
#include "winevent.h"

QUEUE EventQueue = 
{
  0,
  0,
  MAXEVENTS,
  0,
  sizeof(MSG),
  NULL,
  0, 0, 0
};

/***************************************************************************/
/*                                                                         */
/*                         QUEUE ROUTINES                                  */
/*                                                                         */
/***************************************************************************/

VOID FAR PASCAL InitEventQueue(void)
{
  InitQueue(&EventQueue);
}

QUEUE *FAR PASCAL InitQueue(queue)
  QUEUE *queue;
{
  if (queue->qdata == NULL)  /* in case we come back here from WinExec */
    queue->qdata = emalloc(queue->maxelements * queue->element_width);
  DOSSEMCLEAR(&queue->semQueueFull);
  DOSSEMSET(&queue->semQueueEmpty);
  DOSSEMCLEAR(&queue->semQueueMutex);
  return queue;
}

VOID FAR PASCAL QueueAddData(queue, data)
  QUEUE *queue;
  BYTE  *data;
{
  memcpy(&queue->qdata[queue->qhead * queue->element_width], data, 
         queue->element_width);
  if (queue->nelements < queue->maxelements)
    queue->nelements++;
  queue->qhead = (queue->qhead + 1) % queue->maxelements;
}

int FAR PASCAL QueueNextElement(queue, i)
  QUEUE *queue;
  int   i;
{
  return (i + 1) % queue->maxelements;
}

int FAR PASCAL QueuePrevElement(queue, i)
  QUEUE *queue;
  int   i;
{
  return (i == 0) ? queue->maxelements-1 : (i - 1);
}

BYTE *FAR PASCAL QueueGetData(queue, i)
  QUEUE *queue;
  int   i;
{
  PSTR s = &queue->qdata[i * queue->element_width];
  return s;
}

