/*===========================================================================*/
/*                                                                           */
/* File    : WPRNHELP.H                                                      */
/*                                                                           */
/* Purpose : Header file for MEWEL printer driver helper routines            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/*===========================================================================*/

/*
   PrHelp_Copyline

   Copyline copies a scan line from the source bitmap in inverted form 
   to a local buffer.
*/

extern int _cdecl PrHelp_Copyline (LPSTR dest, LPSTR source, int width, 
                                   int destoff, int *xoff);

/*
   dest        Destination buffer
   source      Source buffer
   width       Number of bytes to copy
   destoff     Byte offset in destination
   xoff        Receives offset of first significant (non-zero) byte

   Returns     Number of significant bytes counting from xoff
*/

/* ------------------------------------------------------------------

   PrHelp_CompTiff

   CompTiff compresses a scan line in TIFF encoding.
*/

extern int _cdecl PrHelp_CompTiff (LPSTR dest, LPSTR source, int width);

/*
   dest        Destination buffer
   source      Source buffer
   width       Source width in bytes

   Returns     Length of resulting string
*/

/* ------------------------------------------------------------------

   PrHelp_CompDeltaRow

   CompDeltaRow compresses a scan line in Delta Row encoding.
*/

extern int _cdecl PrHelp_CompDeltaRow (LPSTR dest, LPSTR source, int width, WORD seed);

/*
   dest        Destination buffer
   source      Source buffer
   width       Source width in bytes
   seed        Seed offset, Segment is assumed to be equal to source segment

   Returns     Length of resulting string
*/

/* ------------------------------------------------------------------

   PrHelp_Addline

   Addline copies a scan line from the source bitmap in inverted form to
   a local band buffer. Addline supports any number of "pins" per
   band, it assumes that the topmost "pin" is also the topmost bit (bit 7)
   in the first byte, and that the 9th pin again goes into the topmost
   bit (bit 7) of the next byte. The band buffer is cleared on the
   first call (scanline == 0), so it is not necessary to fill the band
   before printing the last partial buffer.
   If the "stripe length" is 0, then Addline performs an inverting copy
   similar to Copyline.
*/

extern void _cdecl PrHelp_Addline (LPSTR destbuf, LPSTR srcbuf, 
                                   int destwidth, int srcwidth, 
                                   int destoff, int scanline, int stripelen);
/*

   destbuf     Destination buffer
   srcbuf      Source buffer
   destwidth   Destination width in bytes (total)
   srcwidth    Source width in bytes
   destoff     Destination offset (bit position)
   scanline    Scan line number (0 is first)
   stripelen   Length of a stripe in bytes (8-pin: 1, 9-pin: 2, 
               24-pin: 3, etc.)

*/

/* ------------------------------------------------------------------

   PrHelp_Linelimits

   Linelimits scans a band buffer for blank runs, adjusting indexes 
   and pixel skip counts to the stripe length and printer positioning 
   capabilities.
*/

typedef struct {
               LPSTR bufp;
               int blen;
               int fstripe;
               int stripelen;
               int minrun;

               int begidx;
               int begpels;
               int moreidx;
               int morepels;
               } linelimdesc;

extern int _cdecl PrHelp_Linelimits (linelimdesc far *llpar);

/*
   bufp        Band buffer
   blen        Buffer width in bytes (total)
   fstripe     stripe adjustment factor for begidx. If 0, begidx will
               always be 0.
   stripelen   stripe adjustment factor for returned length
   minrun      minimum length of a blank run to be returned
   begidx      Receives byte index of first non-blank data stripe
   begpels     Receives pel skip count (adjusted with fstripe)
   moreidx     Is 0 if the whole line was processed, else contains
               index of next non-blank stripe after blank run.
   morepels    Same as moreidx, but pel skip count.

   Returns     Significant length of line starting at begidx (in bytes,
               adjusted to stripe boundary)

   If the printer can not position horizontally, <fstripe> should be
   set to zero. In this case, Linelimits simply returns the length
   of the band with blanks stripped off the end. Begidx, begpels,
   moreidx, and morepels will be 0.

   Else <fstripe> should contain the stripe adjustment factor, i.e.
   the stripe length multiplied by the x position division factor.
   When <fstripe> is nonzero, Linelimits will skip the first blank
   run and set <begidx> and <begpels> to the starting index and
   the starting pixel skip count (for x positioning). It then will
   scan through the line for a blank run of at least <minrun> bytes.
   If such a run is found, <moreidx> and <morepels> will be set to
   the index and pixel skip count of the next non-blank run, and
   the routine returns. The <moreidx> value must be added to the
   buffer pointer to continue scanning from there on the next entry,
   while the <morepels> value allows accumulating the skip count
   for positioning from the start of the line.
   The return value is the length of the significant portion of
   the line, i.e. the number of bytes between <begidx> and the end
   of the non-blank area.
*/

