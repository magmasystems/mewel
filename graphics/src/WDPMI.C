/*===========================================================================*/
/*                                                                           */
/* File    : WDPMI.C                                                         */
/*                                                                           */
/* Purpose : DPMI functions for 16 and 32 bits                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1994-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#ifndef TEST
typedef void *LPVOID;
typedef int  BOOL;
#endif

#undef FP_SEG
#undef FP_OFF
#define FP_SEG(fp) ((unsigned short)(((unsigned long)(fp)) & 0xFFFF0000L) >> 16)
#define FP_OFF(fp) ((unsigned short)(((unsigned long)(fp)) & 0x0000FFFFL))

#if defined(__BORLANDC__)
#pragma warn -par
#pragma warn -rvl
#endif

/*-----------------------------------------------------------*/

typedef struct tagDPMIRegs
{
  DWORD  EDI;
  DWORD  ESI;
  DWORD  EBP;
  DWORD  Res;
  DWORD  EBX;
  DWORD  EDX;
  DWORD  ECX;
  DWORD  EAX;
  USHORT Flags;
  USHORT ES;
  USHORT DS;
  USHORT FS;
  USHORT GS;
  USHORT IP;
  USHORT CS;
  USHORT SP;
  USHORT SS;
} DPMIREGS;


/*
typedef struct tagTRegs
{
  USHORT AX, BX, CX, DX, BP, SI, DI, DS, ES, cFlags;
} TREGISTERS;

BYTE   PeekFarByte(LPVOID);
USHORT PeekFarWord(LPVOID);
DWORD  PeekFarLong(LPVOID);
BOOL   FreeMappedDPMIPtr(LPVOID);
BOOL   GetMappedDPMIPtr(LPVOID *ProtPtr, LPVOID RealPtr, int Size);
BOOL   RealIntr(int IntNo, TREGISTERS *Regs);
*/


#if defined(__DPMI32__)

BOOL RealIntr(int IntNo, TREGISTERS *Regs)
{
  DPMIREGS DPMIregs;

  {
    /*
      DS:SI is the address of Regs
    */
    asm push ds
    asm mov  esi, Regs

    /*
      ES:DI is the address of DPMIRegs
    */
    asm lea  edi, DPMIregs

    /*
      Set the direction flag and clear out AX. AX will be used to zero out
      the top part of the 32-bits registers.
    */
    asm cld
    asm xor  eax, eax

    asm add  esi, 12         /* Regs[12]->EDI */
    asm movsw
    asm stosw

    asm sub esi, 4           /* Regs[10]->ESI */
    asm movsw
    asm stosw

    asm sub esi, 4           /* Regs[8]->EBP */
    asm movsw
    asm stosw

    asm stosw
    asm stosw               /* 0->Res */

    asm sub esi, 8           /* Regs[2]->EBX */
    asm movsw
    asm stosw

    asm add esi, 2           /* Regs[6]->EDX */
    asm movsw
    asm stosw

    asm sub esi, 4           /* Regs[4]->ECX */
    asm movsw
    asm stosw

    asm sub esi, 6           /* Regs[0]->EAX */
    asm movsw
    asm stosw

    asm add esi, 16         /* Regs[18]->Flags */
    asm movsw

    asm sub esi, 4          /* Regs[16]->ES */
    asm movsw

    asm sub esi, 4          /* Regs[14]->DS */
    asm movsw

    asm mov ecx, 6         /* 0-> FS, GS, IP, CS, SP, SS */
    asm rep stosw

    asm lea edi, DPMIregs
    asm mov ebx, IntNo     /* Set BH to zero (and BL) */
    asm xor ecx, ecx       /* No stack words to copy */
    asm mov eax, 0300h     /* DPMI code to simulate intr */
    asm int 31h            /* DPMI Services */

    asm mov eax, 0
    asm jc ExitPoint       /* Error? - yes */

    asm mov edi, Regs
    asm lea esi, DPMIregs
    asm cld

    asm add esi, 28         /* AX */
    asm movsw

    asm sub esi, 14         /* BX */
    asm movsw

    asm add esi, 6          /* CX */
    asm movsw

    asm sub esi, 6          /* DX */
    asm movsw

    asm sub esi, 14         /* BP */
    asm movsw

    asm sub esi, 6          /* SI */
    asm movsw

    asm sub esi, 6          /* DI */
    asm movsw

    asm add esi, 34         /* DS */
    asm movsw

    asm sub esi, 4          /* ES */
    asm movsw

    asm sub esi, 4          /* Flags */
    asm movsw

    asm mov eax, 1
ExitPoint:
    asm pop ds
  } 
  return _EAX;
}

#else
BOOL RealIntr(int IntNo, TREGISTERS *Regs)
{
  DPMIREGS DPMIregs;
  char     chInt = (char) (IntNo & 0x00FF);

  {
    /*
      DS:SI is the address of Regs
    */
    asm push ds
    asm lds  si, Regs

    /*
      ES:DI is the address of DPMIRegs
    */
    asm mov  ax, ss
    asm mov  es, ax
    asm lea  di, DPMIregs

    /*
      Set the direction flag and clear out AX. AX will be used to zero out
      the top part of the 32-bits registers.
    */
    asm cld
    asm xor  ax, ax

    asm add  si, 12         /* Regs[12]->EDI */
    asm movsw
    asm stosw

    asm sub si, 4           /* Regs[10]->ESI */
    asm movsw
    asm stosw

    asm sub si, 4           /* Regs[8]->EBP */
    asm movsw
    asm stosw

    asm stosw
    asm stosw               /* 0->Res */

    asm sub si, 8           /* Regs[2]->EBX */
    asm movsw
    asm stosw

    asm add si, 2           /* Regs[6]->EDX */
    asm movsw
    asm stosw

    asm sub si, 4           /* Regs[4]->ECX */
    asm movsw
    asm stosw

    asm sub si, 6           /* Regs[0]->EAX */
    asm movsw
    asm stosw

    asm add si, 16         /* Regs[18]->Flags */
    asm movsw

    asm sub si, 4          /* Regs[16]->ES */
    asm movsw

    asm sub si, 4          /* Regs[14]->DS */
    asm movsw

    asm mov cx, 6          /* 0-> FS, GS, IP, CS, SP, SS */
    asm rep stosw

    asm lea di, DPMIregs
    asm mov ax, 0300h      /* DPMI code to simulate intr */
    asm xor bx, bx         /* Set BH to zero (and BL) */
    asm mov bl, chInt      /* Save interrupt number */
    asm xor cx, cx         /* No stack words to copy */
    asm int 31h            /* DPMI Services */

    asm mov ax, 0
    asm jc ExitPoint     /* Error? - yes */

    asm les di, Regs
    asm mov ax, ss 
    asm mov ds, ax 
    asm lea si, DPMIregs
    asm cld

    asm add si, 28         /* AX */
    asm movsw

    asm sub si, 14         /* BX */
    asm movsw

    asm add si, 6          /* CX */
    asm movsw

    asm sub si, 6          /* DX */
    asm movsw

    asm sub si, 14         /* BP */
    asm movsw

    asm sub si, 6          /* SI */
    asm movsw

    asm sub si, 6          /* DI */
    asm movsw

    asm add si, 34         /* DS */
    asm movsw

    asm sub si, 4          /* ES */
    asm movsw

    asm sub si, 4          /* Flags */
    asm movsw

    asm mov ax, 1
ExitPoint:
    asm pop ds
  } 
  return _AX;
}
#endif


BOOL GetMappedDPMIPtr(LPVOID *ProtPtr, LPVOID RealPtr, int Size)
{
  /*
    Get an LDT descriptor & selector for it
    AX=0, CX=1
    The selector is returned in AX, and we will save it in BX
  */
  _EAX = 0;
  _ECX = 1;
  geninterrupt(0x31);
  asm jc Error
  asm xchg eax, ebx

  /* 
    Set descriptor to real address
  */
  _EDX = (((DWORD) RealPtr) >> 16) & 0x0000FFFFL;
  _EAX = 0;
  asm mov al, dh
  _ECX = 4;
  asm shr eax, cl
  asm shl edx, cl
  asm xchg eax, ecx
  _EAX = 7;
  geninterrupt(0x31);
  asm jc Error

  /* 
    Set descriptor to limit Size bytes
    AX = 8
    BX = Selector
    CX:DX is limit
  */
  _ECX = 0;
  _EDX = Size;
  _EAX = FP_OFF(RealPtr);
  _EDX += _EAX;
  asm jnc lbl1
  _EDX = -1L;
lbl1:
  _EAX = 8;
  geninterrupt(0x31);
  asm jc Error

  /* 
    Save selector:offset in ProtPtr
    The selector is still in BX
  */
  _EDX = (_BX << 16) | FP_OFF(RealPtr);
  * (DWORD *) ProtPtr = _EDX;

  return 1;

Error:
  return 0;
}


BOOL FreeMappedDPMIPtr(LPVOID ProtPtr)
{
  _EBX = FP_SEG(ProtPtr);
  _EAX = 1;
  geninterrupt(0x31);

  _EAX = 0;
  asm jc Error
  _EAX = 1;

Error:
  return _EAX;
}

BYTE PeekFarByte(LPVOID lpAddr)
{
  /*
    Save lpAddr in BX cause we won't be able to address it after
    changing DS
  */
  _EBX = (DWORD) lpAddr;

  asm push ds
  asm mov edx, ebx
  _DS = (_EDX >> 16);
  asm movzx eax, bx
  asm mov bl, byte ptr[eax]
  asm pop ds
  asm movzx eax,bl
}

USHORT PeekFarWord(LPVOID lpAddr)
{
  /*
    Save lpAddr in BX cause we won't be able to address it after
    changing DS
  */
  _EBX = (DWORD) lpAddr;

  asm push ds
  asm mov edx, ebx
  _DS = (_EDX >> 16);
  asm movzx eax, bx
  asm mov bx, word ptr[eax]
  asm pop ds
  asm movzx eax,bx
}

DWORD PeekFarLong(LPVOID lpAddr)
{
  /*
    Save lpAddr in BX cause we won't be able to address it after
    changing DS
  */
  _EBX = (DWORD) lpAddr;

  asm push ds
  asm mov edx, ebx
  _DS = (_EDX >> 16);
  asm movzx eax, bx
  asm mov ebx,dword ptr [eax]
  asm pop ds
  asm mov eax,ebx
}

/*-----------------------------------------------------------*/

#ifdef TEST

#include <stdio.h>
#include <dos.h>
#include <graphics.h>
#include <conio.h>
#include <mem.h>

#pragma option -a-

typedef struct video
{
  char  crt_mode;        /* 0 */
  short crt_cols;        /* 1 */
  short crt_len;         /* 3 */
  short crt_start;       /* 5 */
  short cursor_pos[8];   /* 7 */
  short cursor_mode;     /* 23 */
  char  active_page;     /* 25 */
  short addr_6845;       /* 26 */
  char  crt_mode_set;    /* 28 */
  char  crt_pallette;    /* 29 */
} BIOSVIDEOINFO,  _FAR *LPBIOSVIDEOINFO;

#pragma option -a. /* restore default packing */

int main(int argc, char **argv)
{
  LPBIOSVIDEOINFO v;
  LPVOID pReal;
  int    rc;

  pReal = (LPVOID) 0x00400049L;

  if (GetMappedDPMIPtr(&((LPVOID) v), (LPVOID) pReal, (int) 1000))
  {
    printf("v->crt_mode    is %d\n", PeekFarByte(&v->crt_mode));
    printf("v->crt_cols    is %d\n", PeekFarWord(&v->crt_cols));
    printf("v->crt_len     is %d\n", PeekFarWord(&v->crt_len ));
    printf("v->crt_start   is %x\n", PeekFarWord(&v->crt_start));
    printf("v->active_page is %d\n", PeekFarByte(&v->active_page));
    printf("v->addr_6845   is %x\n", PeekFarWord(&v->addr_6845));

    FreeMappedDPMIPtr(v);
  }

  {
  int GraphDriver;
  int GraphMode;
  TREGISTERS tr;

  GraphDriver = 9;
  GraphMode = 2;
  initgraph( &GraphDriver, &GraphMode, "d:\\bc4\\bgi" );
  rc = graphresult();
  if (rc != grOk)
  {
    printf(" Graphics System Error: %s\n", grapherrormsg(rc));
    exit(1);
  }

  /*
    Init the mouse
  */
  memset((char *) &tr, 0, sizeof(tr));
  RealIntr(0x33, &tr);

  /*
    Set the horz and vert bounds
  */
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 7;
  tr.DX = 639;
  RealIntr(0x33, &tr);

  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 8;
  tr.DX = 479;
  RealIntr(0x33, &tr);

  /*
    Show the mouse
  */
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 1;
  RealIntr(0x33, &tr);


  getch();

  /*
    Hide the mouse
  */
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 2;
  RealIntr(0x33, &tr);

  closegraph();
  }

  return 0;
}

#endif

