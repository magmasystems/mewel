#ifndef __XMSLIB_H
#define __XMSLIB_H

/* X M S L I B . h  -- Interface to XMS (2.0+) memory functions
 * ---------------------------------------------------------------
 *
 * $Revision:   1.0  $
 *     $Date:   28 Feb 1992  8:55:10  $
 *      $Log:   F:/XMSLIB/VCS/XMSLIB.H_V  $
 * 
 *    Rev 1.0   28 Feb 1992  8:55:10
 * Initial revision.
 * 
 * ---------------------------------------------------------------
 */

/* ------------------------  Pragmas -------------------------- */

/* ---------------- Include files (interface) ----------------- */
#ifndef MEWEL
#include "xtypes.h" // for WORD and DWORD
#endif

/* ----------------------- Definitions ------------------------ */
typedef DWORD    XMSHANDLE;       // Handle to XMS memory
#define XMSHNULL ((XMSHANDLE)0)   // `NULL' handle

#define XMSDUMP_CTRL  1           // Bit flag: dump XMS control blocks
#define XMSDUMP_DATA  2           // Bit flag: dump XMS data
#define XMSDUMP_ALL   (XMSDUMP_CTRL|XMSDUMP_DATA)

/* ------------------- Function prototypes -------------------- */

#if defined ( __cplusplus )
  extern "C" {                    // Turn off C++ name mangling
#endif

XMSHANDLE XMSalloc      (DWORD uBytes);
int       XMSclose      (void);
void      XMSdump       (WORD uDumpFlags);
WORD      XMSerrorCode  (void);
int       XMSfree       (XMSHANDLE xhXM);
int       XMSget        (void _far * pDest, XMSHANDLE xhXM);
int       XMSgetExt     (void _far * pDest, XMSHANDLE xhXM, DWORD uSrcOfs,
                         DWORD uBytes);
DWORD     XMSgetLen     (XMSHANDLE xhXM);
WORD      XMSgetVersion (void);
int       XMSinstalled  (void);
int       XMSopen       (WORD uKbytes);
int       XMSput        (XMSHANDLE xhXM, const void _far * pSrc, DWORD uBytes);

#if defined ( __cplusplus )
  }
#endif

/* ------------------------- Globals -------------------------- */

/* ------------------------------------------------------------ */

#endif  // __XMSLIB_H
