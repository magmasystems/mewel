#include <dos.h>
#include <malloc.h>
#include <memory.h>

#ifndef MK_FP
#define MK_FP(seg,ofs)	((void far *) \
		   (((unsigned long)(seg) << 16) | (unsigned)(ofs)))
#endif


/*
  XMS Move Information
    This structure must be set up for the XMSMoveExtendedMemory call.

  Notes :
    If the SrcHandle is 0, then the SrcOffset is considered to be
  a standard segment:offset address. The same goes for the DestHandle.
*/
typedef struct tagXMSMoveStruct
{
  unsigned long  ulLength;
  unsigned short usSrcHandle;
  unsigned long  ulSrcOffset;
  unsigned short usDestHandle;
  unsigned long  ulDestOffset;
} XMSMOVESTRUCT, far *LPXMSMOVESTRUCT;


/*
  Pointer to the XMS driver
*/
typedef void (far *XMSDRIVER)(void);
static XMSDRIVER lpfnXMSDriver = (XMSDRIVER) 0;


int pascal XMSQueryInstalled(void)
{
  union  REGS  r;

  r.x.ax = 0x4300;
  int86(0x2F, &r, &r);
  return (r.h.al == 0x80);  /* return TRUE if installed, FALSE if not */
}


int pascal  XMSQueryDriverAddress(void)
{
  union  REGS  r;
  struct SREGS s;
  unsigned short rc;

  r.x.ax = 0x4310;
  int86x(0x2F, &r, &r, &s);
  lpfnXMSDriver = (XMSDRIVER) MK_FP(s.es, r.x.bx);

  {
    void (far *ssDriver)() = lpfnXMSDriver;
    _asm
    {
      mov  ah,00H
      call [ssDriver]
      mov  rc,ax
    }
  }

  return (int) rc;
}


int pascal  XMSQueryFreeExtendedMemory(void)
{
  unsigned short iLargestFree, nTotalFree;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,08H
    call [ssDriver]
    mov  iLargestFree,ax
    mov  nTotalFree,dx
  }
  return nTotalFree;   /* returns the number of 1K bytes */
}


int pascal  XMSAllocExtendedMemory(unsigned nKBytes)
{
  unsigned rc, handle;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,09H
    mov  dx,nKBytes
    call [ssDriver]
    mov  handle,dx
    mov  rc,ax
  }
  return (rc) ? handle : 0;
}


int pascal  XMSFreeExtendedMemory(unsigned handle)
{
  unsigned rc;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,0AH
    mov  dx,handle
    call [ssDriver]
    mov  rc,ax
  }
  return rc;
}


char far *pascal XMSLockExtendedMemory(unsigned handle)
{
  unsigned rc;
  unsigned iSeg, iOff;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,0CH
    mov  dx,handle
    call [ssDriver]
    mov  iSeg,dx
    mov  iOff,bx
    mov  rc,ax
  }

  return (rc) ? MK_FP(iSeg, iOff) : (char far *) 0L;
}


int pascal XMSUnlockExtendedMemory(unsigned handle)
{
  unsigned rc;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,0DH
    mov  dx,handle
    call [ssDriver]
    mov  rc,ax
  }

  return rc;
}


int pascal XMSReallocExtendedMemory(unsigned handle, unsigned nKBytes)
{
  unsigned rc;

  void (far *ssDriver)() = lpfnXMSDriver;
  _asm
  {
    mov  ah,0FH
    mov  bx,nKBytes
    mov  dx,handle
    call [ssDriver]
    mov  rc,ax
  }
  return rc;
}


int pascal XMSMoveExtendedMemory(LPXMSMOVESTRUCT lpMoveStruct)
{
  unsigned rc;
  void (far *ssDriver)() = lpfnXMSDriver;

  _asm
  {
    mov  ah,0BH
    push ds
    push si
    lds  si,lpMoveStruct
    call [ssDriver]
    pop  si
    pop  ds
    mov  rc,ax
  }
  return rc;
}


int pascal xmemcpy(unsigned short usDestHandle, unsigned long ulDestOffset,
            unsigned short usSrcHandle,  unsigned long ulSrcOffset,
            unsigned long ulLength)
{
  XMSMOVESTRUCT xms;

  xms.usSrcHandle  = usSrcHandle;
  xms.ulSrcOffset  = ulSrcOffset;
  xms.usDestHandle = usDestHandle;
  xms.ulDestOffset = ulDestOffset;
  xms.ulLength     = ulLength;
  return XMSMoveExtendedMemory(&xms);
}



#ifdef TEST
#include <stdio.h>
#include <stdlib.h>

main()
{
  int  iVersion;
  int  hMem;
  int  rc;
  unsigned char far *pConvMem;

  if (!XMSQueryInstalled())
  {
    printf("XMS not installed\n");
    exit(1);
  }
  iVersion = XMSQueryDriverAddress();
  printf("XMS version %x and driver address is %lp\n", iVersion, lpfnXMSDriver);
  printf("Total free extended memory is %dK bytes\n", XMSQueryFreeExtendedMemory());

  hMem = XMSAllocExtendedMemory(16);
  printf("Allocated 16K - the handle is %d\n", hMem);

  pConvMem = malloc(16 * 1024);
  _fmemset(pConvMem, 0xAA, 16 * 1024);

  rc = xmemcpy(hMem, 0L, 0, (unsigned long) pConvMem, 16 * 1024L);
  printf("Moved 16K of AA to extended memory - rc is %d\n", rc);
  _fmemset(pConvMem, 0, 16 * 1024);
  rc = xmemcpy(0, (unsigned long) pConvMem, hMem, 0L, 16 * 1024L);
  printf("Moved 16K of AA from extended memory - rc is %d\n", rc);

  for (rc = 0;  rc < 16 * 1024;  rc++)
    if (pConvMem[rc] != 0xAA)
    {
      printf("Found non-AA at offset %d\n", rc);
      break;
    }


  if (hMem)
    XMSFreeExtendedMemory(hMem);
  return 0;
}

#endif
