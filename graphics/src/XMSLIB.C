/* X M S L I B . c -- Extended memory (XMS 2.0+) support functions
 * ---------------------------------------------------------------
 *
 * $Revision:   1.0  $
 *     $Date:   28 Feb 1992  8:54:56  $
 *      $Log:   F:/XMSLIB/VCS/XMSLIB.C_V  $
 * 
 *    Rev 1.0   28 Feb 1992  8:54:56
 * Initial revision.
 * 
 * ---------------------------------------------------------------
 */
//#define _M_DEBUG
//#define _M_VERBOSE
/* ------------------------  Pragmas -------------------------- */

/* ---------------------- Include files ----------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined ( _M_DEBUG )
  #include <ctype.h>
#endif
#include <dos.h>

#include "xmslib.h"

/* -------------------- Local definitions --------------------- */
#define _CPYR_ " Copyright (C) 1992 M.G.A.Wilson"

#if defined ( M_I86SM )                               // Microsoft, small
  #define APICALL   call far ptr [mpXMSAPI]
  #define EAPICALL  call far ptr es:[mpXMSAPI]
#elif (defined ( __SMALL__ ))                         // Borland, small
  #define APICALL   call dword ptr mpXMSAPI
  #define EAPICALL  call dword ptr es:mpXMSAPI
#elif (defined ( __LARGE__ ) || defined ( M_I86LM ))  // Large
  #define APICALL   call [mpXMSAPI]
  #define EAPICALL  call es:[mpXMSAPI]
#else
  #error Unsupported compiler or memory model.
#endif

#define H_640K  0   // XMS move handle for 640K conventional memory

typedef struct tagXMSMOVE   // Extended Memory Move Structure
{                           // ------------------------------
  DWORD ulLength;           // Number of bytes to transfer
  WORD  uSrcHdl;            // Handle to source block
  DWORD ulSrcOfs;           // Offset into source block
  WORD  uDestHdl;           // Handle to destination block
  DWORD ulDestOfs;          // Offset into destination block
}
XMSMOVE_T;                  // Handle(s) == H_640K for conventional memory

typedef struct tagXMSCTRL   // Extended Memory Control structure
{                           // ---------------------------------
  WORD    uHandle;          // Handle to the actual XMS block
  DWORD   uHiWater;         // `High water' mark: offset of 1st free byte
  WORD    uNumK;            // Number of Kbytes in the block (1..64)
  BOOLEAN fCompacted;       // TRUE if the free blocks have been compacted
}
XMSCTRL_T;

typedef struct tagXMSBLOCKHEADER // Extended Memory Allocation Block Header
{                                // ---------------------------------------
  DWORD uLength;                 // Number of data bytes in the block
  DWORD uData;                   // Data, or number of free data bytes, if
}                                // <uLength> is 0
XMSBLOCKHEADER_T;

/* ------------------- Function prototypes -------------------- */

/* ------------------------- Globals -------------------------- */
static const char mszIdent[] = { "@(#)XMSLIB v1.00" _CPYR_ };

static DWORD mpXMSAPI;              // XMS API entry point
static WORD muXMSerrCode;           // Error code returned by API call

static WORD muCtrlBlks    = 0,      // Number of XMS control blocks
            muCurrCtrlBlk = 0;      // Current XMS control block
static XMSCTRL_T * mpXMCtrl = NULL; // Pointer to (array of) ctrl blocks


/* ADLER */
/*
  Changes to the original code to allow memory greater than 64K
  to be allocated. Instead of using a 16 bit handle (and leaving
  the remaining 16 bits for an offset), we use an 8-bit handle
  and leave 24-bits for the offset. This means that we can allocate
  up to 2^24 = 16MB in a single allocation.
*/

/*
  We can adjust the maximum offset by changing BITS_IN_XMS_HANDLE. 
  The maximum allocation size is 2 ^ (32 - BITS_IN_XMS_HANDLE).
*/
#if defined(USE_SINGLE_XMS_HANDLE)

static WORD XMSHandle = 0;

#define MAKE_XMS_HANDLE(handle, uOfs) \
  ((DWORD) (uOfs))
#define PARSE_XMS_HANDLE(handle, h, off) \
  h   = XMSHandle;
  off = (handle)

#else

#define BITS_IN_XMS_HANDLE  8
#define BITS_IN_OFFSET      (32 - BITS_IN_XMS_HANDLE)

#define USE_INDEX_AS_HANDLE

#ifdef USE_INDEX_AS_HANDLE
#define MAKE_XMS_HANDLE(handle, uOfs) \
  (((DWORD) (muCurrCtrlBlk+1) << BITS_IN_OFFSET) | (uOfs))
#define PARSE_XMS_HANDLE(handle, h, off) \
  h   = mpXMCtrl[ (int) ((((handle) >> BITS_IN_OFFSET) & 0x00FF)-1) ].uHandle; \
  off = ((handle) & 0x00FFFFFFL)
#else
#define MAKE_XMS_HANDLE(handle, uOfs) \
  (((DWORD) (hXM) << BITS_IN_OFFSET) | (uOfs))
#define PARSE_XMS_HANDLE(handle, h, off) \
  h   = (WORD) (((handle) >> BITS_IN_OFFSET) & 0x00FF); \
  off = ((handle) & 0x00FFFFFFL)
#endif

static WORD XMSBlockSize = 256;  /* number of Kbytes of the XMS block */

#endif

static WORD XMS_KBytesReserved = 0;

/* END ADLER */


/* ------------------------------------------------------------

   The control structure maintained in conventional RAM looks like this:


   ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
 Hi³  Control block N  ³   Each control block contains a handle to an
   ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´   XMS memory block (16-bit word) and the current
          .  .  .          `high water' mark in the control block, that
   ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´   is, the offset of the first byte where
   ³  Control block 1  ³   allocated data can be placed.
   ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
 Lo³  Control block 0  ³   The number of blocks is fixed by XMSopen()
   ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ


   The structure of a partially-used block in XMS memory looks like this:

   ÚÄÂÄÄÄÄÂÄÂÄÄÄÄÄÄÄÄÂÄÂÄÄÂÄÂÄÄÄÄÄÄÄÂÄÂÄÄÄÄÄÄÄÄÄÄÄÄÂÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
   ³a³XXXX³b³XXXXXXXX³0³cX³d³XXXXXXX³e³XXXXXXXXXXXX³0³00000000000000000000³
   ÀÄÁÄÄÄÄÁÄÁÄÄÄÄÄÄÄÄÁÄÁÄÄÁÄÁÄÄÄÄÄÄÄÁÄÁÄÄÄÄÄÄÄÄÄÄÄÄÁÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

   Here, letters `a',`b' etc. represent the start of allocation units. Each
   contains a 16-bit word holding the LENGTH OF DATA FOLLOWING THE WORD. The
   total length of the unit is therefore (length)+2 bytes. If a length is
   marked as 0, then the block is free and can be (re)used for storage.

   If it has never been used, then the first word of the data will also be 0,
   and all space up to the end of the XMS block will thterefore be available.

   If the block has been previously used and then freed, the length of the
   (old) block will be stored in the first word of the data. A check for a
   fit of the newly-allocated block can then be made (which may generate a
   new, smaller freed block if the fit is not exact).

   ------------------------------------------------------------ */

// X M S g e t A P I a d d r e s s
// -------------------------------
// Locates the call gate to the XMS application programming interface
// and stores it in the global <mpXMSAPI>
//
static void _near XMSgetAPIaddress (void)
{
  _asm  {
    mov ax,4310h
    int 2Fh
    mov word ptr [mpXMSAPI],bx
    mov word ptr [mpXMSAPI + 2],es
  }
}

// X M S g e t F r e e M e m
// -------------------------
//
static WORD _near XMSgetFreeMem (WORD * puLargestBlk)
{
  WORD uTotal, uLargest;

  _asm {
    mov   ah,8
    APICALL
    mov   [muXMSerrCode],bx
    mov   uLargest,ax
    mov   uTotal,dx
  }
  *puLargestBlk = uLargest;
  return uTotal;
}

// X M S a l l o c B l o c k
// -------------------------
// Allocate a block of <uKbytes> K bytes from the XMS memory
//
// Returns:
//  Handle to the block, or 0 if none could be allocated
//
static WORD _near XMSallocBlock (WORD uKbytes)
{
  WORD hXM = 0; // The handle

  _asm {
    mov   ah,9
    mov   dx,uKbytes
    APICALL
    mov   [muXMSerrCode],bx
    or    ax,ax
    jz    AllocOver
    mov   hXM,dx
  }

AllocOver:
  return hXM;
  }

// X M S f r e e B l o c k
// -----------------------
// Free a block of XMS memory accessed through handle <hXM>
//
// Returns:
//  1 if successfully freed, 0 if not
//
static int _near XMSfreeBlock (WORD hXM)
{
  int fSuccess;

  _asm {
    mov   ah,0Ah
    mov   dx,hXM
    APICALL
    mov   [muXMSerrCode],bx
    mov   fSuccess,ax
    }

  return fSuccess;
}

// X M S m o v e
// -------------
// Transfer data to or from XMS memory
//
// Argument:
//  <pXMSmv>  Pointer to XMS move structure (see XMS spec)
//
// Returns:
//  1 for success, 0 on error
//
static int _near XMSmove (XMSMOVE_T _far * pXMSmv)
{
  int fSuccess;
  unsigned uSegment = FP_SEG(pXMSmv),
           uOffset  = FP_OFF(pXMSmv);
  _asm {
    mov   ax,ds
    mov   es,ax
    push  ds
    push  si
    mov   si,uOffset
    mov   ax,uSegment
    mov   ds,ax
    mov   ah,0Bh
    EAPICALL
    mov   es:[muXMSerrCode],bx
    pop   si
    pop   ds
    mov   fSuccess,ax
    }
  return fSuccess;
}

// X M S a l l o c W r i t e
// -------------------------
// Write control information into XMS memory to mark an allocated block
//
// Arguments:
//  <hXM>     Native (XMS-derived) handle to XMS memory block
//  <uOfs>    Offset within the XMS block
//  <uBytes>  Number of data bytes which the block will hold
//
// Returns:
//  Handle to memory, or XMSHNULL on error
//
static XMSHANDLE _near XMSallocWrite (WORD hXM, DWORD uOfs, DWORD uBytes)
{
  XMSBLOCKHEADER_T XMShdr;
  XMSMOVE_T XMSmv;
  XMSHANDLE xhXM = XMSHNULL;

  XMShdr.uLength  = uBytes;
  XMShdr.uData    = 0;

  XMSmv.ulLength  = sizeof(XMSBLOCKHEADER_T);
  XMSmv.uSrcHdl   = H_640K;
  XMSmv.ulSrcOfs  = (DWORD)((char _far *)&XMShdr);
  XMSmv.uDestHdl  = hXM;
  XMSmv.ulDestOfs = (DWORD)uOfs;

  if (XMSmove(&XMSmv))
  {
    xhXM = MAKE_XMS_HANDLE(hXM, uOfs);
  }
  return xhXM;
}

// C o a l e s c e F r e e U n i t s
// ---------------------------------
// Join contiguous freed blocks in an allocation unit into a single free block
//
// Argument:
//  <pXMCtrl> Pointer to memory control block
//
static void _near _fastcall CoalesceFreeUnits (XMSCTRL_T * pXMCtrl)
  {
  if (!pXMCtrl->fCompacted)
    {
    XMSBLOCKHEADER_T XMShdrThis,
                     XMShdrNext;
    XMSMOVE_T XMSmv;
    DWORD uOfs = 0,
          uRefOfs;

#if (defined ( _M_DEBUG ) && defined ( _M_VERBOSE ))
  printf("\n--> COALESCING BLOCK WITH HANDLE %04x\n", pXMCtrl->uHandle);
#endif

    XMSmv.ulLength  = sizeof(XMSBLOCKHEADER_T);

    while (uOfs < pXMCtrl->uHiWater)
      {
      DWORD uFreeLen = 0;
      BOOLEAN fBlockEnd = FALSE;

      XMSmv.uSrcHdl   = pXMCtrl->uHandle;
      XMSmv.uDestHdl  = H_640K;
      XMSmv.ulSrcOfs  = uRefOfs = uOfs;
      XMSmv.ulDestOfs = (DWORD)((char _far *)&XMShdrThis);

      if (!XMSmove(&XMSmv))
        return;

      XMSmv.ulDestOfs = (DWORD)((char _far *)&XMShdrNext);

      if (XMShdrThis.uLength == 0)
        {
        DWORD uExtraLen = 0;

        // This block is free: iterate through the following blocks,
        // accumulating the total free length...
        do
          {
          if (XMShdrThis.uData == 0)  // Hit end of chain
            {
            uFreeLen = 0;             // Original now marks end of chain
            fBlockEnd = TRUE;
            break;
            }

          uFreeLen += (XMShdrThis.uData + uExtraLen);
          uOfs     += (XMShdrThis.uData + sizeof(DWORD));

          if (uOfs >= pXMCtrl->uHiWater)
            {
            fBlockEnd = TRUE;
            break;
            }

          XMSmv.ulSrcOfs = uOfs;

          if (!XMSmove(&XMSmv))       // Read next header
            return;

          XMShdrThis = XMShdrNext;

          uExtraLen = sizeof(DWORD);  // Count 2 byte length on all but 1st block
          }
        while (XMShdrThis.uLength == 0);

        // Now update the *original* free block with the coalesced length
        XMShdrNext.uLength = 0;
        if (fBlockEnd == TRUE && uRefOfs == 0)
          // Special case -- all blocks empty
          XMShdrNext.uData = 0;
        else
          XMShdrNext.uData = uFreeLen;
        XMSmv.uSrcHdl   = H_640K;
        XMSmv.ulSrcOfs  = (DWORD)((char _far *)&XMShdrNext);
        XMSmv.uDestHdl  = pXMCtrl->uHandle;
        XMSmv.ulDestOfs = uRefOfs;

        XMSmove(&XMSmv);

        if (fBlockEnd == TRUE)
          {
          // Update high water mark to new end of block
          pXMCtrl->uHiWater = uRefOfs;
          break;
          }
        }
      else
        uOfs += (XMShdrThis.uLength + sizeof(DWORD));
      }
    pXMCtrl->fCompacted = TRUE;
    }
  }

// X M S a l l o c P l a n B
// -------------------------
// Allocator called when we've hit the high water mark in the last XMS block
//
// Argument:
//  <uBytes>  Number of data bytes for which storage is required
//
// Returns:
//  Handle to memory, or XMSHNULL on error
//
static XMSHANDLE _near XMSallocPlanB (DWORD uBytes)
  {
  XMSCTRL_T * pXMCtrl = mpXMCtrl;

  for (muCurrCtrlBlk = 0;
       muCurrCtrlBlk < muCtrlBlks;    // For each XMS block...
       muCurrCtrlBlk++, pXMCtrl++)
    {
    DWORD uOfs = 0;    // Start at the bottom...
    XMSBLOCKHEADER_T XMShdr;
    XMSMOVE_T XMSmv;

    CoalesceFreeUnits(pXMCtrl);

    XMSmv.ulLength  = sizeof(XMSBLOCKHEADER_T);
    XMSmv.uSrcHdl   = pXMCtrl->uHandle;
    XMSmv.uDestHdl  = H_640K;
    XMSmv.ulDestOfs = (DWORD)((char _far *)&XMShdr);

    do
      {
      XMSmv.ulSrcOfs  = uOfs;
      XMSmove(&XMSmv);

      if (XMShdr.uLength == 0)
        {
        // This block is reusable
        if (XMShdr.uData >= uBytes + sizeof(DWORD) + sizeof(DWORD))
          {
#if (defined ( _M_DEBUG ) && defined ( _M_VERBOSE ))
          printf("Reusing block of %04lx bytes at %04lx : New blocks %04lx and %04lx\n",
                  XMShdr.uData, uOfs, uBytes, XMShdr.uData - uBytes - sizeof(DWORD));
#endif
          // Got space in middle of block -- write a new free block beyond it
          XMShdr.uData   -= (uBytes + sizeof(DWORD));
          XMSmv.uDestHdl  = XMSmv.uSrcHdl;
          XMSmv.uSrcHdl   = H_640K;
          XMSmv.ulSrcOfs  = XMSmv.ulDestOfs;
          XMSmv.ulDestOfs = uOfs + uBytes + sizeof(DWORD);
          XMSmove(&XMSmv);

          return XMSallocWrite(pXMCtrl->uHandle, uOfs, uBytes);
          }
        else
          {
          // Not big enough for allocation
          if (XMShdr.uData == 0)
            {
            // Block has never been used; work out length
            DWORD uLen = 0xFFFFFFFFL - uOfs;

            if (uLen < uBytes + sizeof(DWORD) - 1)
              // No space in this block; try next
              break;

            // Got space at end of block
            pXMCtrl->uHiWater = uOfs + uBytes + sizeof(DWORD);
            return XMSallocWrite(XMSmv.uSrcHdl, uOfs, uBytes);
            }

          // Skip to next
          uOfs += XMShdr.uData;
          }
        }
      else
        // This block ain't reusable: skip to next
        uOfs += XMShdr.uLength;

      uOfs += sizeof(DWORD);
      }
    while (uOfs < pXMCtrl->uHiWater);
    }
  muCurrCtrlBlk = 0;
  return XMSHNULL;
  }

// -----------------------------------------------------------------------
// -----   E x t e r n a l l y - v i s i b l e   f u n c t i o n s   -----
// -----------------------------------------------------------------------

/*
  ADLER
    Public interface to set the number of Kbytes to reserve
*/
void far pascal XMSReserveMemory(WORD uKbytes)
{
  XMS_KBytesReserved = uKbytes;
}

// X M S i n s t a l l e d
// -----------------------
// Returns:
//  1 if XMS memory is installed, else 0
//
int XMSinstalled (void)
  {
  int fInstalled;

  _asm {
    mov ax,4300h
    int 2Fh
    and ax,0080h
    mov cl,7
    shr al,cl
    mov fInstalled,ax
    }

  return fInstalled;
  }

// X M S g e t V e r s i o n
// -------------------------
// Gets the version of XMS memory driver (e.g. HIMEM.SYS) in use
//
// Returns:
//  BCD coded number (e.g. v3.21 -> 0x0321), or 0 if not installed or
//  the XMS library's XMSopen() function has not been called.
//
WORD XMSgetVersion (void)
  {
  WORD uVersion = 0;

  if (XMSinstalled() && mpXMSAPI != 0L)
    {
    _asm {
      mov   ah,0
      APICALL
      mov   uVersion,bx
      }
    }
  return uVersion;
  }

// X M S o p e n
// -------------
// `Open communications' with extended memory
//
// Argument:
//  <uKbytes> Number of Kbytes of extended memory to use.
//
// Returns:
//  1 for success
//  0 for failure
//
// Notes:
//  A value of 0 for <uKbytes> will attempt to use *all* available XMS memory.
//  The call will fail if the entire amount requested can not be obtained.
//  At least 64K extended memory must be available as a contiguous block.
//
int XMSopen(WORD uKbytes)
{
  if (muCtrlBlks == 0)
  {
    WORD uKlargestAvail,
         uLastK;

    XMSgetAPIaddress();
    XMSgetFreeMem(&uKlargestAvail);


    /*
      ADLER
        See if the app reserved any memory. If so, then use that
      value for uKbytes.
    */
    if (uKbytes == 0)
      uKbytes = uKlargestAvail - XMS_KBytesReserved;

#if defined(USE_SINGLE_XMS_HANDLE)
    muCtrlBlks = 1;
    if (uKbytes == 0 || uKlargestAvail < uKbytes)
      XMSBlockSize = uKlargestAvail;
    else
      XMSBlockSize = uKbytes;
#else
    if (uKbytes == 0)
    {
      if (uKlargestAvail >= XMSBlockSize)
        muCtrlBlks = (uKlargestAvail / XMSBlockSize);
    }
    else if (uKlargestAvail >= uKbytes)
      muCtrlBlks = (uKbytes / XMSBlockSize) + 1;
#endif

    if ((uLastK = (uKbytes % XMSBlockSize)) == 0)
      uLastK = XMSBlockSize;

    if (muCtrlBlks > 0)
    {
      muCurrCtrlBlk = 0;

      if ((mpXMCtrl = (XMSCTRL_T *)calloc(muCtrlBlks, sizeof(XMSCTRL_T)))
           != NULL)
      {
        // XMS control blocks are pre-filled with zeroes. Now do the
        // actual carving up of the extended memory...

        XMSCTRL_T * pXMCtrl = mpXMCtrl;
        WORD uCount;

        for (uCount = 0; uCount < muCtrlBlks; uCount++)
        {
          WORD uNumK   = (uCount == muCtrlBlks - 1 ? uLastK : XMSBlockSize);
          WORD uHandle = XMSallocBlock(uNumK);

          if (uHandle > 0)
          {
#if !defined(USE_SINGLE_XMS_HANDLE) && !defined(USE_INDEX_AS_HANDLE)
            if (uHandle > (1 << BITS_IN_XMS_HANDLE))
            {
#             include <conio.h>
              extern void cdecl WinTerminate(void);
              WinTerminate();
              printf("XMS error - handle 0x%x too big. Contact Magma Systems.\n");
              getch();
              exit(1);
            }
#endif

            pXMCtrl->uHandle    = uHandle;
            pXMCtrl->uNumK      = uNumK;
            pXMCtrl->fCompacted = TRUE;
            pXMCtrl++;
          }
          else
          {
            muCtrlBlks = uCount;
            break;
          }
        }
      }
      else
        muCtrlBlks = 0;
    }

    return(muCtrlBlks > 0);
  }

  // XMSopen() already called with no intervening XMSclose()
  return 0;
}

// X M S c l o s e
// ---------------
// `Close communications' with XMS memory
//
// Returns:
//  1 for success
//  0 for failure
//
// Notes:
//  De-allocates *all* memory associated with XMSalloc()ed data
//
int XMSclose (void)
  {
  if (muCtrlBlks > 0)
    {
    WORD uCount = muCtrlBlks,
                   uFreed = 0;
    XMSCTRL_T * pXMCtrl   = mpXMCtrl + uCount;

    do
      {
      if (XMSfreeBlock(pXMCtrl->uHandle))
        uFreed++;
      pXMCtrl--;
      }
    while (uCount-- > 0);

    if (uFreed == muCtrlBlks)
      {
      free(mpXMCtrl);
      muCtrlBlks    = 0;
      muCurrCtrlBlk = 0;
      mpXMCtrl      = NULL;
      return 1;
      }
    }
  return 0;
  }

// X M S a l l o c
// ---------------
// Allocate space for data in extended memory
//
// Argument:
//  <uBytes>  Number of data bytes for which storage is required
//
// Returns:
//  Handle to memory, or XMSHNULL on error
//
// Notes:
//  Overhead is one (WORD) in XMS memory per entry
//
XMSHANDLE XMSalloc (DWORD uBytes)
  {
  XMSCTRL_T * pXMCtrl  = mpXMCtrl + muCurrCtrlBlk;
  XMSHANDLE   xhXM     = XMSHNULL;
  DWORD       uHiWater = pXMCtrl->uHiWater;
  WORD        uNumK    = pXMCtrl->uNumK;

  // Word-align

  if (uBytes & 0x01)
    uBytes++;

  // adler
  // since XMSCTRL_T.data is 4 bytes, then we must allocate a minimum of
  // 4 bytes.
  if (uBytes < sizeof(DWORD))
    uBytes = sizeof(DWORD);

  // Check for wrap at 64K boundary

  while (
#if 1 /* ADLER - no wrapping at 256K !!! */
         (uBytes + uHiWater + sizeof(DWORD) > (DWORD) uNumK * 1024L))
#else
         (uBytes + uHiWater + sizeof(DWORD) < uBytes) ||
         ((uNumK < XMSBlockSize) &&
         (uBytes + uHiWater + sizeof(DWORD) > uNumK * 1024)))
#endif
    {
#if defined(_M_DEBUG)
    printf("XMS wrapped at 64K\n");
#endif
    if (++muCurrCtrlBlk == muCtrlBlks)
      return XMSallocPlanB(uBytes);

    pXMCtrl++;
    uHiWater = pXMCtrl->uHiWater;
    uNumK    = pXMCtrl->uNumK;
    }

  xhXM = XMSallocWrite(pXMCtrl->uHandle, uHiWater, uBytes);

  if (xhXM != XMSHNULL)
    pXMCtrl->uHiWater = uHiWater + uBytes + sizeof(DWORD);
  else
    muCurrCtrlBlk = 0;


#if defined(_M_DEBUG)
if (xhXM == XMSHNULL)
{
WinTerminate();
printf("XMSAlloc NULL\n\n\nuHiWater %ld\nuNumK %d\nuBytes %ld\nmuCtrlBlks %d\nuHandle %u\n",
        uHiWater, uNumK, uBytes, muCtrlBlks, pXMCtrl->uHandle);
exit(1);
}
#endif

  return xhXM;
  }

// X M S f r e e
// -------------
// Free an allocation unit obtained with XMSalloc()
//
// Arguments:
//  <xhXM>    Handle to XMS memory
//
// Returns:
//  1 on success, 0 on failure
//
int XMSfree(XMSHANDLE xhXM)
{
  XMSBLOCKHEADER_T XMShdr;
  XMSMOVE_T XMSmv;

  XMSmv.ulLength  = sizeof(XMSBLOCKHEADER_T);
  PARSE_XMS_HANDLE(xhXM, XMSmv.uSrcHdl, XMSmv.ulSrcOfs);
  XMSmv.uDestHdl  = H_640K;
  XMSmv.ulDestOfs = (DWORD)((char _far *)&XMShdr);

  if (XMSmove(&XMSmv))
  {
    XMShdr.uData   = XMShdr.uLength;  // Old len stored in 1st two data bytes
    XMShdr.uLength = 0;               // Length now marked as zero = reusable

    XMSmv.uDestHdl  = XMSmv.uSrcHdl;
    XMSmv.ulDestOfs = XMSmv.ulSrcOfs; // Write over length bytes
    XMSmv.uSrcHdl   = H_640K;
    XMSmv.ulSrcOfs  = (DWORD)((char _far *)&XMShdr);

    if (XMSmove(&XMSmv))
    {
      WORD uBlock;
      XMSCTRL_T * pXMCtrl = mpXMCtrl;

      for (uBlock = 0;  uBlock < muCtrlBlks;  uBlock++)
      {
        if (pXMCtrl->uHandle == XMSmv.uDestHdl)
        {
          pXMCtrl->fCompacted = FALSE;
          break;
        }
        pXMCtrl++;
      }
      return 1;
    }
  }

  return 0;
}

// X M S e r r o r C o d e
// -----------------------
// Get the error code associated with the most recent XMS API call
//
WORD XMSerrorCode (void)
  {
  return muXMSerrCode;
  }

// X M S g e t L e n
// -----------------
// Get the length of the space associated with an XMS handle
//
// Arguments:
//  <xhXM>    Handle to XMS memory
//
// Returns:
//  Length of data, or 0 on error
//
DWORD XMSgetLen (XMSHANDLE xhXM)
  {
  WORD  hXM;
  DWORD uOfs;
  WORD  uBlock = 0;
  DWORD uLen   = 0;
  XMSCTRL_T * pXMCtrl = mpXMCtrl;

  PARSE_XMS_HANDLE(xhXM, hXM, uOfs);

  while (uBlock < muCtrlBlks && pXMCtrl->uHandle != hXM)
  {
    uBlock++;
    pXMCtrl++;
  }

  if (uBlock < muCtrlBlks)
  {
    XMSMOVE_T XMSmv;

    XMSmv.ulLength  = sizeof(DWORD);
    XMSmv.uSrcHdl   = hXM;
    XMSmv.ulSrcOfs  = uOfs;
    XMSmv.uDestHdl  = H_640K;
    XMSmv.ulDestOfs = (DWORD)((char _far *)&uLen);

    XMSmove(&XMSmv);
  }
  return uLen;
}

// X M S g e t E x t
// -----------------
// Get data from XMS memory to conventional memory; extended version
//
// Arguments:
//  <pDest>   Pointer to source buffer
//  <xhXM>    Handle to XMS memory
//  <uSrcOfs> Offset of data in the stored block (0 = start)
//  <uBytes>  Number of bytes to copy
//
// Returns:
//  1 on success, 0 on failure
//
int XMSgetExt (void _far * pDest, XMSHANDLE xhXM, DWORD uSrcOfs, DWORD uBytes)
{
  XMSMOVE_T XMSmv;

  XMSmv.ulLength  = uBytes;

  PARSE_XMS_HANDLE(xhXM, XMSmv.uSrcHdl, XMSmv.ulSrcOfs);
  XMSmv.ulSrcOfs  += sizeof(DWORD) + uSrcOfs;

  XMSmv.uDestHdl  = H_640K;
  XMSmv.ulDestOfs = (DWORD)((char _far *)pDest);

  return XMSmove(&XMSmv);
}

// X M S g e t
// -----------
// Get data from XMS memory to conventional memory
//
// Arguments:
//  <pDest>   Pointer to source buffer
//  <xhXM>    Handle to XMS memory
//
// Returns:
//  1 on success, 0 on failure
//
// Notes:
//  Automatically sets number of bytes copied out to the number copied in.
//  Caller *must* ensure that buffer at <pDest> is big enough.
//
int XMSget (void _far * pDest, XMSHANDLE xhXM)
{
  XMSMOVE_T XMSmv;

  XMSmv.ulLength  = XMSgetLen(xhXM);

  PARSE_XMS_HANDLE(xhXM, XMSmv.uSrcHdl, XMSmv.ulSrcOfs);
  XMSmv.ulSrcOfs  += sizeof(DWORD);

  XMSmv.uDestHdl  = H_640K;
  XMSmv.ulDestOfs = (DWORD)((char _far *)pDest);

  return XMSmove(&XMSmv);
}

// X M S p u t
// -----------
// Put data in XMS memory from conventional memory
//
// Arguments:
//  <xhXM>    Handle to XMS memory
//  <pSrc>    Pointer to source buffer
//  <uBytes>  Bytes to copy
//
// Returns:
//  1 on success, 0 on failure
//
int XMSput (XMSHANDLE xhXM, const void _far * pSrc, DWORD uBytes)
{
  XMSMOVE_T XMSmv;

  if (uBytes == 0)
    uBytes = XMSgetLen(xhXM);
  else if (uBytes & 0x01)
    uBytes++;

  XMSmv.ulLength  = uBytes;
  XMSmv.uSrcHdl   = H_640K;
  XMSmv.ulSrcOfs  = (DWORD)((char _far *)pSrc);

  PARSE_XMS_HANDLE(xhXM, XMSmv.uDestHdl, XMSmv.ulDestOfs);
  XMSmv.ulDestOfs += sizeof(DWORD);

  return XMSmove(&XMSmv);
}

#if defined ( _M_DEBUG)

// -----------------------------------------------------------------------
// -------------   D e b u g g i n g   f u n c t i o n s   ---------------
// -----------------------------------------------------------------------

// X M S d u m p C o n t r o l B l o c k
// -------------------------------------
static void _near XMSdumpControlBlock (const XMSCTRL_T * pXMCtrl)
  {
  printf("%04x  %04lx (%5lu)  %04x (%5u)\n", pXMCtrl->uHandle, pXMCtrl->uHiWater,
         pXMCtrl->uHiWater, pXMCtrl->uNumK, pXMCtrl->uNumK);
  }

// X M S d u m p E x t e n d e d M e m o r y B l o c k
// ---------------------------------------------------
static void _near XMSdumpExtendedMemoryBlock (const XMSCTRL_T * pXMCtrl)
  {
  XMSMOVE_T XMSmv;
  DWORD uOfs = 0;
  union
    {
    XMSBLOCKHEADER_T XMShdr;
    unsigned char    bBuffer[sizeof(DWORD) + 16];
    }
  Data;

  if (pXMCtrl->uHiWater == 0) // Has never been used
    return;

  XMSmv.ulLength  = sizeof(Data);
  XMSmv.uSrcHdl   = pXMCtrl->uHandle;
  XMSmv.uDestHdl  = H_640K;
  XMSmv.ulDestOfs = (DWORD)((char _far *)&Data);

  do
    {
    DWORD uRemaining = pXMCtrl->uHiWater - uOfs;

    if (uRemaining < sizeof(Data))
      XMSmv.ulLength = uRemaining;

    XMSmv.ulSrcOfs = uOfs;

    if (XMSmove(&XMSmv))
      {
      WORD i;
      DWORD  iMax = Data.XMShdr.uLength;

      if (iMax == 0)
        iMax = Data.XMShdr.uData;

      if (iMax > 16)
        iMax = 16;

      printf("%04lx: %04lx  ", uOfs, Data.XMShdr.uLength);
      for (i = 0; i < 16; i++)
        if (i < iMax)
          printf("%02x ", Data.bBuffer[i + sizeof(DWORD)]);
        else
          printf("   ");
      printf("  ");
      for (i = 0; i < iMax; i++)
        {
        unsigned char c = Data.bBuffer[i + sizeof(DWORD)];
        printf("%c", iscntrl(c) ? '.' : c);
        }
      printf("\n");
      }
    else
      {
      printf("ERROR %04x\n", XMSerrorCode());
      break;
      }

    if (Data.XMShdr.uLength == 0)
      uOfs += Data.XMShdr.uData;
    else
      uOfs += Data.XMShdr.uLength;

    uOfs += sizeof(DWORD);
    }
  while (uOfs < pXMCtrl->uHiWater &&
         (Data.XMShdr.uLength > 0 || Data.XMShdr.uData > 0));
  }

// X M S d u m p
// -------------
void XMSdump (WORD uDumpFlags)
  {
  XMSCTRL_T * pXMCtrl;
  WORD uCtrlBlk;

  printf("\n*** XMS Dump ***\n\n");

  if (uDumpFlags & XMSDUMP_CTRL)
    {
    printf("Control blocks:\n\n");
    printf("  Num:  Hdl  MaxH  (MaxD)  NoKH  (NoKD)\n");

    for (uCtrlBlk = 0, pXMCtrl = mpXMCtrl;
         uCtrlBlk < muCtrlBlks;
         uCtrlBlk++)
      {
      printf("%5u: ", uCtrlBlk);
      XMSdumpControlBlock(pXMCtrl++);
      }
    }

  if (uDumpFlags & XMSDUMP_DATA)
    {
    printf("\nExtended memory:");

    for (uCtrlBlk = 0, pXMCtrl = mpXMCtrl;
         uCtrlBlk < muCtrlBlks;
         uCtrlBlk++)
      {
      printf("\n\nBlock %5u:\n\n", uCtrlBlk);
      XMSdumpExtendedMemoryBlock(pXMCtrl++);
      }
    }
  printf("\n****************\n\n");
  }

#endif  // _M_DEBUG
