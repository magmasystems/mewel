#ifndef __XTYPES_H
#define __XTYPES_H

/* X T Y P E S . h  -- Type and constant definitions
 * ---------------------------------------------------------------
 *
 * $Revision:   1.0  $
 *     $Date:   28 Feb 1992  8:55:28  $
 *      $Log:   F:/XMSLIB/VCS/XTYPES.H_V  $
 * 
 *    Rev 1.0   28 Feb 1992  8:55:28
 * Initial revision.
 * 
 * ---------------------------------------------------------------
 */

/* ------------------------  Pragmas -------------------------- */

/* ---------------- Include files (interface) ----------------- */

/* ----------------------- Definitions ------------------------ */

typedef unsigned short WORD;      // 16-bit value
typedef unsigned long  DWORD;     // 32-bit value

typedef enum  // Boolean values
  {
  FALSE,      // 0
  TRUE        // 1
  }
BOOLEAN;

/* ------------------- Function prototypes -------------------- */

/* ------------------------- Globals -------------------------- */

/* ------------------------------------------------------------ */

#endif  // __XTYPES_H
