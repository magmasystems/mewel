/*
    The following defines are used as error message IDs for the
    PrUsr_Error routine. Change to match your resource ID layout.
*/

#define PR_EMSG_HEADER     0x0800   /* Error message header */
#define PR_EMSG_NOMEM      0x0810   /* No memory for printer buffers */
#define PR_EMSG_BADCONF    0x0811   /* Bad configuration */
#define PR_EMSG_ESCLONG    0x0812   /* Escape sequence too long */
#define PR_EMSG_ESCBAD     0x0813   /* Incorrect escape syntax */
#define PR_EMSG_CREATERR   0x0814   /* Can't create file */
#define PR_EMSG_WRERROR    0x0815   /* Error writing file */
#define PR_EMSG_EACCESS    0x0816   /* Invalid path or access denied */

#define PR_QMSG_IOERR      0x0820   /* Printer I/O Error */
#define PR_QMSG_OFFLINE    0x0821   /* Printer offline */
#define PR_QMSG_NOPAPER    0x0822   /* Printer out of paper */
#define PR_QMSG_OVERWRITE  0x0823   /* Output file exists, overwrite? */

/*
   ID_RESOL is used in PRDRIVER.C. See the notes in this file for details.
*/

#define ID_RESOL  100

/*
   ID_FNAME and ID_FNAMET are used for the sample file name dialog only.
*/

#define ID_FNAMET          0x1800
#define ID_FNAME           0x1801

#ifndef RC_INVOKED
extern char tmp_str [256]; /* temporary string buffer */

#ifdef WAGNER_FFAX

extern short error (HWND wind, short msgid, ...);
extern short query (HWND wind, short msgid, ...);
extern int set_save_file (HWND wind, char *fname);

#define PrUsr_Error error
#define PrUsr_Query query
#define PrUsr_GetOutFileName set_save_file

extern char winini [];

#define PrUsr_GetProf(a,b,c,d,e) GetPrivateProfileString(a,b,c,d,e,winini)
#define PrUsr_GetProfInt(a,b,c) GetPrivateProfileInt(a,b,c,winini)
#define PrUsr_WriteProf(a,b,c) WritePrivateProfileString(a,b,c,winini)

#else

extern int _cdecl PrUsr_Error (HWND wind, short msgid, ...);
extern int _cdecl PrUsr_Query (HWND wind, short msgid, ...);
extern int FAR PASCAL PrUsr_GetOutFileName (HWND win, LPSTR fname);

#define PrUsr_GetProf(a,b,c,d,e) GetProfileString(a,b,c,d,e)
#define PrUsr_GetProfInt(a,b,c) GetProfileInt(a,b,c)
#define PrUsr_WriteProf(a,b,c) WriteProfileString(a,b,c)

#endif
#endif
