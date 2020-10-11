/*  %kw # %v    %n    %d    %t # */
/* Version # 14    METQUERY.C    13-Apr-94    11:07:08 # */
/* ***************************************************************** *
 *  METQUERY.C - Procedure to Qualify "GrafixCard and "GrafixInput"  *
 *  Copyright (c) 1987-1992   Metagraphics Software Corporation      * 
 * ***************************************************************** */

/*
This procedure, in conjunction with QueryGraphics() and QueryMouse(), is
used to dynamically determine and the graphics hardware and input
device configuration.

MetQuery() interrogates the system to discover all the
installed graphics adapters and input devices, and presents them in
the form of a text screen menu.

MetQuery() allows the operator to choose the desired graphics adapter,
resolution, and input device in the following ways;

1) from the program startup command line using letter:number codes.
2) from the environment variable METAPARM using letter:number codes.
3) from the text screen menu using the arrow keys

If you desire to trim the size of the program down, you can set the
define GRQtrimMenu to the following;

define GRQtrimMenu 0 for fancy text screen menu selection code
define GRQtrimMenu 1 for just simple text line display of selection
define GRQtrimMenu >1 for no display at all

The command line option /? lists all the display adapters supported, along
with the letter:number codes used to select from command line.

The command line option /; disables any further text screen displays


MetQuery() can be included or linked with your program.
Variables "GrafixCard" and "GrafixInput" should be globally 
defined and accessable to both your main routine and MetQuery:

   #include  "metawndo.h"
   int   GrafixCard,GrafixInput;

   void main( int argc, char *argv[] )
   {
            (...)
        MetQuery(argc,argv);                 
	    (...)
        InitGraphics(GrafixCard);
            (...)
        InitMouse(GrafixInput);
            (...)
   }

   #include "metquery.c"

=========================================================================== */
#ifdef ZortechCPP 
#define GRQtrimMenu     1           /* type problems with Zortech */
#endif

/* define 0 for fancy text menu selection code */
/* define 1 for just simple text line display of selection */
/* define >1 for no display at all */
#ifndef  GRQtrimMenu
#define GRQtrimMenu  0
#endif

#ifndef  cMET       /* If compiled separately, not "included" */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <ctype.h>
#include "metawndo.h"      /* master MetaWINDOW include file */
extern int GrafixCard, GrafixInput;
#endif /* separate */

#ifdef MetawareC
#include "dos.h"
#endif

#ifdef __386__
#define REGWRD w
#define INTCALL int386
#else
#define REGWRD x
#ifdef MetawareC
#define INTCALL _int86
#else
#define INTCALL int86
#endif /* Metaware */
#endif /* 386 */

/* command line and environment var switch characters */
#define  SWCHAR(x)    ( x == '-' || x == '/' )

/* maximum line length */
#define  GRQMAXLEN    127

/* QueryGraphics() return values */
#define  MQPRESENT     	0       		/* Device is present							*/
#define  MQNOTPRES    	1       		/* Device is not present					*/
#define  MQMAYBE        -1				/* can't tell if device present or not */
#define  MQBADDEV    	2       		/* Device code is unknown 				   */

typedef struct   _device
{
   char  *devslash;
   int    devnum;
   char  *devdesc;
   int    devhres, devvres;
   long   devcolors;
   int    devstat;
} device;

/* output device table */ 
static
device  o_dev[] = {

/* ATI */
   { "ai:20",  ATI800x600,       "ATI, VGA Wonder", 800, 600, 16,MQMAYBE},       
   { "ai:21",  ATI1024x768,      "ATI, VGA Wonder+", 1024, 768, 16,MQMAYBE},       
   { "ai:30",  ATI640x480X,      "ATI, VGA Wonder", 640, 480, 256,MQMAYBE},
   { "ai:31",  ATI800x600X,      "ATI, VGA Wonder XL", 800, 600, 256,MQMAYBE},       
   { "ai:32",  ATI1024x768X,     "ATI, VGA Wonder XL", 1024, 768, 256,MQMAYBE},
   { "ai:40",  ATI640x480Y,      "ATI, VGA Wonder XL", 640, 480, 32767,MQMAYBE},
   { "ai:50",  ATIUP640x480X,    "ATI, Graphics Ultra Pro", 640, 480, 256,MQMAYBE},
   { "ai:51",  ATIUP800x600X,    "ATI, Graphics Ultra Pro", 800, 600, 256,MQMAYBE},       
   { "ai:52",  ATIUP1024x768X,   "ATI, Graphics Ultra Pro", 1024, 768, 256,MQMAYBE},
   { "ai:53",  ATIUP1280x1024X,  "ATI, Graphics Ultra Pro", 1280,1024, 256,MQMAYBE},
/* ATT */
   { "at:1",   ATT640x400,       "AT&T graphics", 640, 400, 0,MQMAYBE},
   { "at:2",   DEB640x200,       "AT&T, DEB graphics", 640, 200, 16,MQMAYBE},     
   { "at:3",   DEB640x400,       "AT&T, DEB graphics", 640, 400, 16,MQMAYBE},    
/* COMPAQ */
   { "cp:1",   ATT640x400,       "Compaq Plasma", 640, 400, 0,MQMAYBE},    
   { "cp:10",  CPQ800x600,       "Compaq Advanced VGA", 800, 600, 16,MQMAYBE},    
   { "cp:11",  CPQ640x480X,      "Compaq Advanced VGA", 640, 480, 256,MQMAYBE},    
   { "cp:12",  CPQ1024x768X,     "Compaq QVision  VGA", 1024, 768, 256,MQMAYBE},    
/* CORNERSTONE */
   { "co:12",  DP2048x1538,      "Cornerstone, DP 150", 2048, 1538, 0,MQMAYBE},
   { "co:13",  DP2048x1538A,     "Cornerstone, DP 150/noJP1", 2048, 1664, 0,MQMAYBE},
/* DIAMOND */
   { "ts:20",  TS4800x600,       "Diamond Speedstar", 800,600, 16,MQMAYBE}, 
   { "ts:21",  TS41024x768,      "Diamond Speedstar", 1024, 768, 16,MQMAYBE},       
   { "ts:30",  TS4640x350X,      "Diamond Speedstar", 640, 350, 256,MQMAYBE},       
   { "ts:31",  TS4640x480X,      "Diamond Speedstar", 640, 480, 256,MQMAYBE},       
   { "ts:32",  TS4800x600X,      "Diamond Speedstar", 800, 600, 256,MQMAYBE},       
   { "ts:34",  TS41024x768X,     "Diamond Speedstar", 1024, 768, 256,MQMAYBE},       
   { "ts:40",  TS4640x480Y,      "Diamond Speedstar", 640, 480, 32767,MQMAYBE},       
   { "ts:41",  TS4800x600Y,      "Diamond Speedstar", 800, 600, 32767,MQMAYBE},       
/* The Diamond Stealth uses the S3 chip set and BIOS calls */
	{ "s3:10",	S3I640x480X,		"Diamond Stealth (S3)",  640, 480, 256,MQMAYBE},
	{ "s3:11",	S3I800x600X,		"Diamond Stealth (S3)",  800, 600, 256,MQMAYBE},
	{ "s3:12",	S3I1024x768X,		"Diamond Stealth (S3)",  1024, 768, 256,MQMAYBE},
/* EVEREX */
   { "ev:1",   EVX800x600,      "Everex, EVGA EV-673", 800,600, 16,MQMAYBE},       
/* GENOA */
   { "g:1",    GEN640x480,       "Genoa, SuperEGA 4880", 640, 480, 16,MQMAYBE}, 
   { "g:2",    GEN800x600,       "Genoa, SuperEGA 4880", 800, 600, 16,MQMAYBE}, 
	{ "g:3",    GEN800x600V,      "Genoa, SuperVGA 5/6000", 800,600, 16,MQMAYBE}, 
   { "g:4",    GEN1024x768,      "Genoa, SuperVGA 5000",1024, 768, 16,MQMAYBE}, 
   { "g:5",    GEN640x350X,      "Genoa, SuperVGA 5000", 640, 350, 256,MQMAYBE},
   { "g:6",	   GEN640x480X,      "Genoa, SuperVGA 5000", 640, 480, 256,MQMAYBE},
   { "g:7",    GEN61024x768,     "Genoa, SuperVGA 6000",1024, 768, 16,MQMAYBE}, 
   { "g:8",	   GEN6640x480X,     "Genoa, SuperVGA 6000", 640, 480, 256,MQMAYBE}, 
/* HERCULES */              
   { "h:1",    HER720x348,       "Hercules, Monographics", 720, 348, 0,MQMAYBE},
/* Hewlett Packard */
/* The HP Ultra VGA uses the S3 chip set and BIOS calls */
	{ "s3:10",	S3I640x480X,		"HP Ultra VGA",  640, 480, 256,MQMAYBE},
	{ "s3:11",	S3I800x600X,		"HP Ultra VGA",  800, 600, 256,MQMAYBE},
	{ "s3:12",	S3I1024x768X,		"HP Ultra VGA",  1024, 768, 256,MQMAYBE},

   /* IBM Industry Standard Modes */
/* CGA */
   { "i:1",    CGA640x200,       "IBM, CGA", 640, 200, 2,MQMAYBE},    
/* EGA */
   { "i:10",   EGA320x200,       "IBM, EGA", 320, 200, 16,MQMAYBE},
   { "i:11",   EGA640x200,       "IBM, EGA", 640, 200, 16,MQMAYBE},
   { "i:12",   EGA640x350,       "IBM, EGA", 640, 350, 16,MQMAYBE},
   { "i:13",   EGAMono,          "IBM, EGA", 640, 350, 0,MQMAYBE},
   { "f:40",   EGA640x350MP,     "IBM, EGA (planar mode)", 640,350, 16,MQMAYBE},
   { "f:41",   EGA640x3502,      "IBM, EGA", 640, 350, 2,MQMAYBE},
/* VGA */
   { "i:20",   VGA320x200,       "IBM, VGA", 320, 200, 16,MQMAYBE},
   { "i:21",   VGA640x200,       "IBM, VGA", 640, 200, 16,MQMAYBE},
   { "i:22",   VGA640x350,       "IBM, VGA", 640, 350, 16,MQMAYBE},
   { "i:23",   VGA640x480,       "IBM, VGA", 640, 480, 16,MQMAYBE}, 
   { "i:24",   VGA640x4802,      "IBM, VGA/MCGA", 640, 480, 2,MQMAYBE}, 
   { "i:25",   VGA320x200X,      "IBM, VGA", 320, 200, 256,MQMAYBE},    
/* 3270 */
   { "i:5",    TTS720x350,       "IBM, 3270", 720, 350, 0,MQMAYBE},                
/* Micro Display Systems */
   { "m:10",   MDS736x1008,      "MDS, Genius", 736, 1008, 0,MQMAYBE},        
/* NSI */
   { "ns:1",   NSI800x600,       "NSI, Smart EGA/Plus", 800, 600, 16,MQMAYBE},   
/* Oak OTI-067 chipset */
   { "ok:1",   OAK800x600,       "OAK VGA", 800, 600, 16,MQMAYBE},       
   { "ok:2",   OAK1024x768,      "OAK VGA", 1024, 768, 16,MQMAYBE},       
   { "ok:3",   OAK640x480X,      "OAK VGA", 640, 480, 256,MQMAYBE},
/* Orchid */
/* The Orchid designer is identical to the STB Extra/EM */
   { "st:20",  STB800x600,       "Orchid, Designer VGA", 800, 600, 16,MQMAYBE},       
   { "st:21",  STB1024x768,      "Orchid, Designer VGA", 1024, 768, 16,MQMAYBE},       
   { "st:30",  STB640x350X,      "Orchid, Designer VGA", 640, 350, 256,MQMAYBE},       
   { "st:31",  STB640x480X,      "Orchid, Designer VGA", 640, 480, 256,MQMAYBE},
/* The Orchid Pro designer II uses a Tseng Labs 4000 chip set and BIOS calls */
   { "ts:20",  TS4800x600,       "Orchid, ProDesigner II", 800,600, 16,MQMAYBE}, 
   { "ts:21",  TS41024x768,      "Orchid, ProDesigner II", 1024, 768, 16,MQMAYBE},       
   { "ts:30",  TS4640x350X,      "Orchid, ProDesigner II", 640, 350, 256,MQMAYBE},       
   { "ts:31",  TS4640x480X,      "Orchid, ProDesigner II", 640, 480, 256,MQMAYBE},       
   { "ts:32",  TS4800x600X,      "Orchid, ProDesigner II", 800, 600, 256,MQMAYBE},       
   { "ts:34",  TS41024x768X,     "Orchid, ProDesigner II", 1024, 768, 256,MQMAYBE},       
/* The Orchid Fahrenheit uses the S3 chip set and BIOS calls */
	{ "s3:10",	S3I640x480X,		"Orchid Fahrenheit 1280",  640, 480, 256,MQMAYBE},
	{ "s3:11",	S3I800x600X,		"Orchid Fahrenheit 1280",  800, 600, 256,MQMAYBE},
	{ "s3:12",	S3I1024x768X,		"Orchid Fahrenheit 1280",  1024, 768, 256,MQMAYBE},
/* Paradise */
   { "p:1",    PAR640x480,       "Paradise, Autoswitch EGA/480", 640, 480, 16,MQMAYBE},  
   { "p:2",    PAR800x600,       "Paradise, VGA ", 800, 600, 16,MQMAYBE},  
   { "p:3",    PAR1024x768,      "Paradise, VGA 1024", 1024, 768, 16,MQMAYBE},  
   { "p:4",    PAR640x480X,      "Paradise, VGA 1024", 640, 480, 256,MQMAYBE},  
/* Quadram */
   { "q:1",    STB800x600,       "Quadram, VGA Spectra", 800, 600, 16,MQMAYBE},       
   { "st:21",  STB1024x768,      "Quadram, VGA Spectra", 1024, 768, 16,MQMAYBE},       
   { "q:2",    STB640x350X,      "Quadram, VGA Spectra", 640, 350, 256,MQMAYBE},       
   { "q:3",    STB640x480X,      "Quadram, VGA Spectra", 640, 480, 256,MQMAYBE},       
/* Radius */
/* The Raduis Multiview is identical to the V7 VRAM II */
   { "v7:10",  V72640x480X,      "Radius Multiview", 640, 480, 256,MQMAYBE},
   { "v7:6",   V7800x600,        "Radius Multiview", 800, 600,  16,MQMAYBE},
   { "v7:11",  V72800x600X,      "Radius Multiview", 800, 600, 256,MQMAYBE},
   { "v7:8",   V71024x768,       "Radius Multiview", 1024, 768, 16,MQMAYBE},
   { "v7:12",  V721024x768X,     "Radius Multiview", 1024, 768, 256,MQMAYBE},
   { "v7:9",   V71024x768x2,     "Radius Multiview", 1024, 768, 0,MQMAYBE},
/* S3 Inc. */
	{ "s3:10",	S3I640x480X,		"S3 Incorporated",  640, 480, 256,MQMAYBE},
	{ "s3:11",	S3I800x600X,		"S3 Incorporated",  800, 600, 256,MQMAYBE},
	{ "s3:12",	S3I1024x768X,		"S3 Incorporated",  1024, 768, 256,MQMAYBE},
/* Sigma Designs */
   { "si:1",   SIG1664x1200,     "Sigma Design, L*View", 1664, 1200, 0,MQMAYBE},
/*!{ "si:3",   SIG1664x1200A,    "Sigma Design, L*View (A000)", 1664, 1200, 0,MQMAYBE}, */
/* STB */
   { "st:20",  STB800x600,       "STB, VGA Extra/EM", 800, 600, 16,MQMAYBE},       
   { "st:21",  STB1024x768,      "STB, VGA Extra/EM", 1024, 768, 16,MQMAYBE},       
   { "st:30",  STB640x350X,      "STB, VGA Extra/EM", 640, 350, 256,MQMAYBE},       
   { "st:31",  STB640x480X,      "STB, VGA Extra/EM", 640, 480, 256,MQMAYBE},       
/* The STB VGA EM-16 PLUS uses a Tseng Labs 4000 chip set and BIOS calls */
   { "ts:20",  TS4800x600,       "STB VGA EM-16 Plus", 800,600, 16,MQMAYBE}, 
   { "ts:21",  TS41024x768,      "STB VGA EM-16 Plus", 1024, 768, 16,MQMAYBE},       
   { "ts:30",  TS4640x350X,      "STB VGA EM-16 Plus", 640, 350, 256,MQMAYBE},       
   { "ts:31",  TS4640x480X,      "STB VGA EM-16 Plus", 640, 480, 256,MQMAYBE},       
   { "ts:32",  TS4800x600X,      "STB VGA EM-16 Plus", 800, 600, 256,MQMAYBE},       
   { "ts:34",  TS41024x768X,     "STB VGA EM-16 Plus", 1024, 768, 256,MQMAYBE},       
/* TECMAR */
   { "tc:1",   TEC800x600,       "Tecmar, VGA/AD", 800, 600, 16,MQMAYBE},       
   { "tc:2",   TEC1024x768,      "Tecmar, VGA/AD", 1024, 768, 16,MQMAYBE},       
   { "tc:3",   TEC640x350X,      "Tecmar, VGA/AD", 640, 350, 256,MQMAYBE},       
   { "tc:4",   TEC640x480X,      "Tecmar, VGA/AD", 640, 480, 256,MQMAYBE},       
/* Toshiba */
   { "to:1",   TOS640x400,       "Toshiba, 3100", 640, 400, 0,MQMAYBE},               
/* Trident Imapact */
   { "tr:1",   TRI800x600,       "Trident Impact", 800,600, 16,MQMAYBE}, 
   { "tr:2",   TRI1024x768,      "Trident Impact", 1024, 768, 16,MQMAYBE},       
   { "tr:3",   TRI640x480X,      "Trident Impact", 640, 480, 256,MQMAYBE},       
   { "tr:4",   TRI800x600X,      "Trident Impact", 800, 600, 256,MQMAYBE},       
   { "tr:5",   TRI1024x768X,     "Trident Impact", 1024, 768, 256,MQMAYBE},       
/* Tseng Labs */
  { "tl:0",   EVA640x480,        "Tseng Labs, EVA-480", 640, 480, 16,MQMAYBE},   
/* Tseng Labs 4000 chip set */
   { "ts:20",  TS4800x600,       "Tseng 4000", 800,600, 16,MQMAYBE}, 
   { "ts:21",  TS41024x768,      "Tseng 4000", 1024, 768, 16,MQMAYBE},       
   { "ts:30",  TS4640x350X,      "Tseng 4000", 640, 350, 256,MQMAYBE},       
   { "ts:31",  TS4640x480X,      "Tseng 4000", 640, 480, 256,MQMAYBE},       
   { "ts:32",  TS4800x600X,      "Tseng 4000", 800, 600, 256,MQMAYBE},       
   { "ts:34",  TS41024x768X,     "Tseng 4000", 1024, 768, 256,MQMAYBE},       
   { "ts:40",  TS4640x480Y,      "Tseng 4000", 640, 480, 32767,MQMAYBE},       
   { "ts:41",  TS4800x600Y,      "Tseng 4000", 800, 600, 32767,MQMAYBE},       
/* VESA */
   { "ve:20",  VESA800x600,         "VESA SuperVGA", 800,600, 16,MQMAYBE}, 
   { "ve:21",  VESA1024x768,        "VESA SuperVGA", 1024,768, 16,MQMAYBE}, 
   { "ve:31",  VESA640x480X,        "VESA SuperVGA", 640,480, 256,MQMAYBE}, 
   { "ve:32",  VESA800x600X,        "VESA SuperVGA", 800,600, 256,MQMAYBE}, 
   { "ve:33",  VESA1024x768X,       "VESA SuperVGA", 1024,768, 256,MQMAYBE}, 
   { "ve:34",  VESA1280x1024X,      "VESA SuperVGA", 1280,1024, 256,MQMAYBE}, 
   { "ve:40",  VESA640x480Y,        "VESA SuperVGA", 640,480, 32767,MQMAYBE}, 
   { "ve:41",  VESA640x480YY,       "VESA SuperVGA", 640,480, 65535L,MQMAYBE}, 
   { "ve:42",  VESA800x600Y,        "VESA SuperVGA", 800,600, 32767,MQMAYBE}, 
   { "ve:43",  VESA800x600YY,       "VESA SuperVGA", 800,600, 65535L,MQMAYBE}, 
   { "ve:44",  VESA1024x768Y,       "VESA SuperVGA", 1024,768, 32767,MQMAYBE}, 
   { "ve:45",  VESA1024x768YY,      "VESA SuperVGA", 1024,768, 65535L,MQMAYBE}, 
   { "ve:46",  VESA1280x1024Y,      "VESA SuperVGA", 1280,1024, 32767,MQMAYBE}, 
   { "ve:47",  VESA1280x1024YY,     "VESA SuperVGA", 1280,1024, 65535L,MQMAYBE},
   { "ve:71",  VESA640x480Z,        "VESA SuperVGA", 640,480, 16777216L,MQMAYBE}, 
   { "ve:72",  VESA800x600Z,        "VESA SuperVGA", 800,600, 16777216L,MQMAYBE}, 
/* IBM 8514 */ 
   { "ve:53",  V8514A1024x768x8I,   "8514A (interlaced)",    1024, 768,256,MQMAYBE},
   { "ve:55",  V8514A1024x768x8N,   "8514A (non-interlaced)",1024, 768,256,MQMAYBE},
/* V7 VEGA VGA */
   { "v7:5",   V7800x600,        "V7, VEGA VGA", 800, 600, 16,MQMAYBE},
/* V7 FastWrite */
   { "v7:6",   V7800x600,        "V7, FastWrite VGA", 800, 600, 16,MQMAYBE},
   { "v7:7",   V7640x480X,       "V7, FastWrite VGA", 640, 480,256,MQMAYBE},       
   { "v7:8",   V71024x768,       "V7, FastWrite VGA", 1024, 768, 16,MQMAYBE},
   { "v7:9",   V71024x768x2,     "V7, FastWrite VGA", 1024, 768, 0,MQMAYBE},
/* V7 VRAM */
   { "v7:6",   V7800x600,        "V7, VRAM VGA", 800, 600, 16,MQMAYBE},
   { "v7:7",   V7640x480X,       "V7, VRAM VGA", 640, 480,256,MQMAYBE},       
   { "v7:8",   V71024x768,       "V7, VRAM VGA", 1024, 768, 16,MQMAYBE},
   { "v7:9",   V71024x768x2,     "V7, VRAM VGA", 1024, 768, 0,MQMAYBE},
/* V7 1024i */
   { "v7:6",  V7800x600,        "V7, VGA 1024i", 800, 600,  16,MQMAYBE},
   { "v7:7",  V7640x480X,       "V7, VGA 1024i", 640, 480, 256,MQMAYBE},
   { "v7:8",  V71024x768,       "V7, VGA 1024i", 1024, 768, 16,MQMAYBE},
   { "v7:9",   V71024x768x2,    "V7, VGA 1024i", 1024, 768, 0,MQMAYBE},
/* V7 VRAM II */
   { "v7:10",  V72640x480X,      "V7, VRAM II", 640, 480, 256,MQMAYBE},
   { "v7:6",   V7800x600,        "V7, VRAM II", 800, 600,  16,MQMAYBE},
   { "v7:11",  V72800x600X,      "V7, VRAM II", 800, 600, 256,MQMAYBE},
   { "v7:8",   V71024x768,       "V7, VRAM II", 1024, 768, 16,MQMAYBE},
   { "v7:12",  V721024x768X,     "V7, VRAM II", 1024, 768, 256,MQMAYBE},
   { "v7:9",   V71024x768x2,     "V7, VRAM II", 1024, 768, 0,MQMAYBE},
/* WYSE */                       
   { "w:1",    WYS640x400,       "Wyse, WY-700", 640, 400, 0,MQMAYBE},
   { "w:2",    WYS1280x400,      "Wyse, WY-700", 1280, 400, 0,MQMAYBE},
   { "w:3",    WYS1280x800,      "Wyse, WY-700", 1280, 800, 0,MQMAYBE},
                                 
   { "", 0, "*** WARNING: No graphics adaptors located. ***", 0, 0, 0}
};
/* The last entry should always be 0,  no device located  */


/* input device table */ 
static
device i_dev[] = {
   { "m:d",   MsDriver, "Mouse driver",0,0,-1,MQMAYBE},      
   { "m:1",   MsCOM1,   "Microsoft mouse, COM1",0,0,-1,MQMAYBE},      
   { "m:2",   MsCOM2,   "Microsoft mouse, COM2",0,0,-1,MQMAYBE},      
   { "l:1",   MoCOM1,   "Logitech/Mouse Systems mouse, COM1",0,0,-1,MQMAYBE},
   { "l:2",   MoCOM2,   "Logitech/Mouse Systems mouse, COM2",0,0,-1,MQMAYBE},
   { "j:1",   JoyStick,  "Joystick on game port",0,0,-1,MQMAYBE},
   { "",0,"*** No input devices or serial ports located ***",0,0,-1,MQMAYBE},
};
/* The last entry should always be 0 , no device located */


static int     GrQParse(int,char *[]);
static void    GrQSniff(void);

#if GRQtrimMenu == 0     
static void    GrQMenu(char *[]);
static int     view_list( device*, int, int);
static int     disp_item(int, int, device* , int, int, char*, int, int);
static char    getxkey( char*, char* );
static void    Qbeep(void);
static void    t_quit(int, char*);
static Word    time55( void );

/* text screen drawing utilities */
void  tx_chout( Byte );
void  tx_print(char *);
void  tx_xy( Word, Word );
void  tx_scroll( Word, Word, Word, Word, Word, Word );
void  tx_attr( Word, Word, Word, Word );
void  tx_clear(void);
void  tx_frame( Word, Word, Word, Word);
void  tx_hbar( Word, Word, Word );
void  tx_title(char *,Word);
int   tx_mono_disp(void);
#endif 




void MetQuery( int argc, char *argv[] )
{
    int i;

#if GRQtrimMenu == 1    /* simple display */
    char     line[GRQMAXLEN];
#endif

   GrafixCard = GrafixInput = 0;

/*	parse out the command line or environment variable, if any */
    i = GrQParse(argc,argv);

/* sniff for devices using QueryGraphics() and QueryMouse() */
    GrQSniff();

/*	 if the /; option was present, don't do anything more */
    if( i )
        return;

#if GRQtrimMenu == 0
/* allow text screen menu selection */
    GrQMenu(argv);
#endif

#if GRQtrimMenu == 1
/* a simple text display of the selected adapter and input device */

      mwMetaVersion(line);
      printf("\n-[ %s ]-\n",line);
      /* find display adapter info */
      for( i = 0; o_dev[i].devnum; i++ )
         if( o_dev[i].devnum == GrafixCard)   
            break;

      printf ( "\nDISPLAY: %s -> %s, %4d x %4d", 
              o_dev[i].devslash, o_dev[i].devdesc, 
              o_dev[i].devhres,  o_dev[i].devvres );

      /* find input device info */
      for( i = 0; i_dev[i].devnum; i++ ) 
         if(i_dev[i].devnum == GrafixInput)     
            break;

      printf( "\n  MOUSE: %s -> %s", i_dev[i].devslash, i_dev[i].devdesc );
      getch();

#endif /* GrQtrimMenu == 1 */

/*
**  "GrafixCard" and "GrafixInput" are now set with valid InitGraphics and  
**   InitMouse values (or are equal to zero if no devices were found   
**   or specified).                                                    
*/

}




static int GrQParse( int argc, char *argv[] )
/*
** Parse the command line or METAPARM environment variable for options.
** Sets global GrafixCard and GrafixInput to selection if any. 
** Returns True if the /; option present, else returns False
*/
{

   int   i, len, choice, pass;
   char  *p, *s, *d; 
   char  line[GRQMAXLEN];
   line[0] = '\0';

	/* concatenate command line args */
   for( i =  1; argc > 1; argc-- ) 
      strcat( line, argv[i++] );

/* is there a command line choice ? */
   choice = False;
   p = line;
   for( i = 0; *p && i < GRQMAXLEN; i++ ) {
      if( SWCHAR ( *p ) )
            choice = True;
      p++;  
   }
   *p = '\0';   /* force to MAXLEN if over */

/* if no cmd line , is there an environment var? */
   if( !choice ) {
      p = getenv("METAPARM");   
      if( p )   
         strcpy( line, p );
   }

   /* force to lower case, remove spaces */
   p = s = line;
   while( *s ) {
      *p = (char) tolower( (int) *s );
      if( *p != ' ' )
            p++;
      s++;
   }
   *p = '\0';

/* process each option on the line */
   pass = False;  /* set bypass menu to false */
   p = line;
   while( *p ) {
   
      /* scan for a '/' and/or '-' */
      while( *p && ! SWCHAR( *p ) )
         p++;

      if( *p == '\0' )
         break;

      if( *(p+1) == ';' ) 
         pass = True;

/* table look up for each output device */
      for( i = 0; o_dev[i].devnum; i++) {

         s = o_dev[i].devslash;  /* s is the device in list to check against */
         d = p + 1;              /* d is the requested device to look for */
         len = strlen(s);        /* length of current device */
         if( ! strncmp(s,d,len) ) { /* when 0, we matched */
            GrafixCard = o_dev[i].devnum;
            break;
         }
      }

/* table look up for each input device */
      for( i = 0; i_dev[i].devnum; i++) {
         s = i_dev[i].devslash;
         d = p + 1;
         len = strlen(s);        /* length of current device */
         if( ! strncmp(s,d,len) ) { /* when 0, we matched */
            GrafixInput = i_dev[i].devnum;
            break;
         }
      }  /* end of for( i ) */

      p++;

   }    /* end of while( *p ) */

   return pass;

}  /* end of GrQParse() */


static void GrQSniff()
/*
**  use QueryGraphics() and QueryMouse() to try and see whats there
*/
{
   int   i, vidchoice;


   vidchoice = 0;

   /* for each device in the table */
   i = -1;
   while(o_dev[++i].devnum) {

      o_dev[i].devstat = mwQueryGraphics(o_dev[i].devnum);

      /* note the best one available */
      if( o_dev[i].devstat < 1 ) {
         if( o_dev[i].devnum == VGA640x480 )
            vidchoice += 8;
         if( o_dev[i].devnum == EGA640x350 )
            vidchoice += 4;
         if( o_dev[i].devnum == HER720x348 )
            vidchoice += 2;
         if( o_dev[i].devnum == CGA640x200 )
            vidchoice += 1;
      }
   }

   /* if a card wasn't specified */
   if( GrafixCard == 0 ) {
      /* pick the best one available (or CGA by default) */
      GrafixCard = CGA640x200;
      if( vidchoice >= 2 )
         GrafixCard = HER720x348;
      if( vidchoice >= 4 )
         GrafixCard = EGA640x350;
      if( vidchoice >= 8 )
         GrafixCard = VGA640x480;
   }

   /* for each device in table */
   i = -1;
   while(i_dev[++i].devnum)
      /* query it */
      i_dev[i].devstat= mwQueryMouse(i_dev[i].devnum);

   /* if a mouse wasn't specified */
   if( GrafixInput == 0 ) {

      /* find the first one available in the list */
      i = -1;
      while(i_dev[++i].devnum)
         if( i_dev[i].devstat < 1 ) {
            GrafixInput = i_dev[i].devnum;
            break;
         }
   }

}


#if GRQtrimMenu == 0

/* maximun number of lines on the screen */
int GrQLines = 25;

/* menu selection title string, you can define your own */
#ifndef  GRQTITLE 
#define GRQTITLE    "Confirm Graphic Devices" 
#endif

/* terminated or exit message, you can define your own */
#ifndef GRQQUITSTR   
#define GRQQUITSTR  "\nMetaWINDOW terminated.\n"
#endif

/* The menu header. If not defined, will use the prog name & path */
/* #define GRQHDRSTR  "your msg here" */

/* time out delay in system clock ticks (appx 18.2 per sec), 0 for forever */
#ifndef  GRQDELAY
#define GRQDELAY     180
#endif

/* this struc stores the attrib values for mono vs color screens */
struct   
{  Word  normal;     /* Normal text attribute */
   Word  inverse;
   Word  hilite;
   Word  installed;  /* color if device installed */
   Word  notHere;    /* color if device not installed */
   Word  maybe;      /* color if device maybe installed */
   Word  BadDev;     /* color if unknown device code */
} tx_attr_val;

/* Attrib values for set_attr - color display */
#define BLUEBK       0x1000
#define WHITEBK      0x7000
#define RED          0x0400                    
#define YELLOW       0x0E00
#define GREEN        0x0200
#define WHITE        0x0700
#define BWHITE       0x0F00

/* Attrib values for set_attr - mono display */
#define MNORMAL      0x0700
#define MINVERSE     0x7000
#define MHILITE      0x0F00
#define FLASH        0x8000


/* KEYS */
#define kESC         0x1B
#define kCR          0x0D
#define Home         71
#define End          79
#define PgDn         81
#define PgUp         73
#define UP_arr       72
#define LF_arr       75
#define DN_arr       80
#define RT_arr       77

/* misc */
#define TOP          -2
#define BOT          -3
#define SUP          True
#define SDOWN        False

static void GrQMenu( char *argv[] )
/*
**  Allow menu selection of desired devices
*/
{
   int   i, inum, onum, x, y;
   unsigned int ktime;
   char  *p, z, w, line[GRQMAXLEN];

   /*	  setup text video attribute table  */

   if( tx_mono_disp() )
   {  /* on a monochrome system */
      tx_attr_val.normal  = MNORMAL;
      tx_attr_val.hilite  = MHILITE;
      tx_attr_val.inverse = MINVERSE;
      tx_attr_val.installed= MHILITE;
      tx_attr_val.maybe =    MHILITE;
      tx_attr_val.notHere=   MNORMAL;
      tx_attr_val.BadDev=    MNORMAL | FLASH;
   }
   else /* color system */
   {
      tx_attr_val.normal=     WHITE | BLUEBK;
      tx_attr_val.hilite=    BWHITE | BLUEBK;
      tx_attr_val.inverse=           WHITEBK;
      tx_attr_val.installed= BWHITE | BLUEBK;
      tx_attr_val.maybe=     BWHITE | BLUEBK;
      tx_attr_val.notHere=   YELLOW | BLUEBK;
      tx_attr_val.BadDev=     WHITE | BLUEBK;
   }

   while( True )  /* big loop for choice screen */
   {
      /* find display adapter info */
      for( onum = 0; o_dev[onum].devnum; onum++ )
         if( o_dev[onum].devnum == GrafixCard)   
            break;

      /* find input device info */
      for( inum = 0; i_dev[inum].devnum; inum++ ) 
         if(i_dev[inum].devnum == GrafixInput)     
            break;

      /* clear screen */
      tx_scroll( True, 0, 0, 79, 24, 0 );

      tx_frame( 1, 1, 78, GrQLines - 2 );
      tx_hbar( 1, 3, 78 );

#ifdef GRQHDRSTR    /* if defined, print out instead of program name */
      p = GRQHDRSTR;
#else
      p = argv[0];   /* the program name */
#endif
      y = 0;
      tx_title( p,y );

      y +=2;
      tx_title( GRQTITLE,y );
/*
** print metawindow lib version
*/
      sprintf( line, "-[ " );
      mwMetaVersion( &line[3] );
      i = strlen( line );
      sprintf( &line[i], " ]-" );
      tx_title( line, GrQLines - 1 );

      x = 4;   
      y = 5;

      tx_xy( x, y );
/*
** display graphics device
*/
      if( GrafixCard )
      {
         tx_print( "Display Device / Mode:" );
         if( o_dev[onum].devcolors == 0L )
             sprintf(line,"%s, %dx%d monochrome", o_dev[onum].devdesc,
                     o_dev[onum].devhres, o_dev[onum].devvres );
         else
             sprintf(line,"%s, %dx%d %ld color", o_dev[onum].devdesc,
                 o_dev[onum].devhres, o_dev[onum].devvres,
                 o_dev[onum].devcolors );

         tx_attr( tx_attr_val.hilite, x + 23 , y , strlen(line) + x + 25 );
         tx_xy( x + 23 , y );
         tx_print( line );

         if( o_dev[onum].devstat == MQNOTPRES ) {
            tx_xy( x + 23 , y + 1 );
            tx_attr( tx_attr_val.notHere, x + 23, y + 1, x + 44 );
            tx_print( "Device Not Installed!" );
         }

      }
      else
      {
         p = o_dev[onum].devdesc;
         tx_attr( tx_attr_val.hilite, x, y, strlen( p ) + x );
         tx_print( p );
      }
/*
** display input device
*/
      tx_xy( x, y += 2 );
      tx_print( "Input Device or Mouse:" );
      p = i_dev[inum].devdesc;
      tx_attr( tx_attr_val.hilite, x + 23, y, strlen(p) + x + 25 );
      tx_xy( x + 23, y );
      tx_print( p );

      if( i_dev[inum].devstat == MQNOTPRES ) {
         tx_xy( x + 23 , y + 1 );
         tx_attr( tx_attr_val.notHere, x + 23, y + 1, x + 44 );
         tx_print( "Device Not Installed!" );
      }


/*
** display options
*/
      tx_xy( x, y += 3 );
      tx_attr( tx_attr_val.hilite, x, y, x + 3 );
      tx_print("[D]");
      tx_print ("     -> choose an alternate Display.");

      tx_xy( x, y += 2 );
      tx_attr( tx_attr_val.hilite, x, y, x + 3 );
      tx_print("[I]");
      tx_print ("     -> choose an alternate Input device or mouse.");

      tx_xy( x, y += 2 );
      tx_attr( tx_attr_val.hilite, x, y, x + 5 );
      tx_print("[Esc]");
      tx_print ("   -> Escape to DOS.");

      tx_xy( x, y += 2 );
      tx_attr( tx_attr_val.hilite, x, y, x + 7 );
      tx_print("[other]");
      tx_print (" -> accept defaults");

      tx_xy( x, y += 3 );
      tx_print("Choice ?   [ ]");
      tx_xy( x+12, y );
/*
** wait for a key or timeout
*/  
      ktime = time55();
      while( GRQDELAY )
      {
         if( ( time55() - ktime ) > GRQDELAY )
         {
            tx_clear();
            tx_xy( 0, 0 );
            return;
         }
         if( kbhit() ) 
            break;
      }

/*
** get a key and process
*/
      getxkey( &z, &w );
      w = (char)toupper(w);
      switch( w )
      {

         case kESC:
                /* escape key */
                t_quit(9, GRQQUITSTR);
                break;

         case 'D': 
               i = view_list( &o_dev[0], onum, False );
               GrafixCard = o_dev[i].devnum;  
               break;

         case 'I':
               i = view_list( &i_dev[0], inum, True );
               GrafixInput = i_dev[i].devnum;  
               break;

         default:   /* any other key accepts choice */
               tx_clear();
               tx_xy(0,0);
               return;

      } /* end of switch */

   } /* end of while( True ) */ 

} /* end of GrQMenu() */

static int view_list( device *devptr, int curDev, int mouse )
/*
** menu device selection - uses cursor keys to select
*/
{
   int   choosing, refrsh, x, y;
   int   page_head= 0;  /* top of page                   */
   int   page_len = GrQLines - 6; /* number of lines on page       */
   int   cur_select;    /* the initial selection number  */
   int   tail_sel;      /* how many selections are there */
   Word  cy;            /* the cursor position           */
   Word  ex;            /* the length of the bar (calculated) */  
   char  str[80], s, a, *p;
   int   i;
/*
** set menu x,y position
*/
   y = 4;
   if( mouse ) x = 10;
   else        x = 5;

/*
** clear screen
*/
   tx_scroll( True, 0, 0, 79, 24, 0 );

/*
** Make menu screen
*/
   tx_frame( 1, 1, 78, GrQLines - 2 );
   tx_hbar( 1, 3, 78 );

   p = "Cursor keys [\x18\x19\x1B\x1A] -> move, [\x11\xD9] -> select, [Esc] -> Abort";
   tx_attr( tx_attr_val.hilite, 9, GrQLines - 1, strlen(p) + 11 );
   tx_xy( 9, GrQLines - 1 );
   tx_print( p );
   if( mouse )
   {
      p = "Select Input Device";
      sprintf( str, "%-10s   %-40s", "Cmd Line", "Pointing Device");
   }
   else
   {
      p = "Select Display Device/Mode";
      sprintf( str, "%-10s   %-30s    %-12s  %-12s", "Cmd Line",
                   "Graphics Device", "Resolution", "Colors" );
   }
   tx_title( p,0 );
   tx_xy( x - 2, y - 2 );
   tx_print( str );

/*
** find the number of entries in menu
*/
   tail_sel = 0;
   while( ( devptr + tail_sel )->devnum != 0 )
      tail_sel++;
   tail_sel--; /* last entry is "none", don't display */

/*
** set max page length - fewer items will simply shrink 'page'
*/
   page_len = (tail_sel < page_len) ? tail_sel + 1 : page_len;

/*
** set current selection, ie default entry
*/
   cur_select = (curDev > tail_sel) ? 0 : curDev;
/*
** set the select bar xy position & length
*/
   ex = strlen( str ) + 1; 
   cy = y;
/*
** set the default
*/
   if(cur_select)
   {

      if( cur_select > ( page_len / 2 ) )
      {
         cy += page_len / 2;   /* set the cursor to the middle */ 
         page_head = cur_select - (page_len / 2);  /* set page_head */
      }
      else  cy += cur_select;
   }

   choosing = refrsh = True;

   while( choosing )
   {
/*
** rewrite entire page
*/
      if( refrsh )
      {
         for( i = 0; i < page_len; i++ )
         {
            if ( y + i == (signed)cy )  /* this is the current bar position */
               disp_item(x,y+i, devptr, page_head + i, tail_sel, str, True, ex );
            else
               disp_item(x,y+i, devptr, page_head + i, tail_sel, str, False, ex );
         }
         refrsh = False;
      }
      else  /* just moved bar */
         disp_item(x, cy, devptr, cur_select, tail_sel, str, True,ex );
/*
** get cursor out of way
*/
      tx_xy( 0, GrQLines - 1 );
      getxkey( &s, &a );
/*
** escape
*/  
      if( a == kESC )
         t_quit(9, GRQQUITSTR);
/*
** made a choice, sophie
*/
      if( a == kCR ) {
  
         if( devptr[cur_select].devstat < MQBADDEV )
            return cur_select;
      }

/*
** big select switch
*/
   /* erase old bar */
      disp_item(x,cy, devptr, cur_select, tail_sel, str, False,ex );
      switch( s )
      {
         case  UP_arr:
         case  LF_arr:
                  if( cur_select <= 0 )
                    break;
                  cur_select--;
                  if( cy > (unsigned)y )
                     cy--;
                  else  /* top of page; move up one line */
                     if( page_head > 0 )
                     {
                         page_head --;
                         tx_scroll( SDOWN, x, y, ex, y + page_len - 1, 1 );
                     }
                  break;
         case  DN_arr:
         case  RT_arr:
                  if( cur_select >= tail_sel )
                    break;

                  cur_select++;
                  if( cy < (unsigned)(page_len + y - 1) )
                     cy++;
                  else  /* bottom of page; move down one line */
                     if( page_head < tail_sel )
                     {
                        page_head++;
                        tx_scroll(SUP,x,y,ex,y+page_len-1,1);
                     }
                  break;
         case  Home:
                  if( cur_select <= 0 )
                    break;
/*
** set page, select, to 0 and set cursor bar
*/
                  page_head = cur_select = 0;
                  cy = y;
                  refrsh = True;
                  break;

         case  End:
                  if( cur_select >= tail_sel )
                     break;
/*
** set page, select, and set cursor bar
*/
                  page_head = tail_sel - page_len + 1;
                  cur_select = tail_sel;
                  cy = page_len + y - 1;
                  refrsh = True;
                  break;

         case  PgDn:
                  if(cur_select >= tail_sel)
                     break;

                  page_head += page_len;
                  cur_select += page_len;
                  if( page_head > tail_sel - page_len + 1 
                      || cur_select > tail_sel )
                  {
                     page_head = tail_sel - page_len + 1;
                     cur_select = tail_sel;
                     /* set cursor bar position */
                     cy = page_len + y - 1;
                  }
                  refrsh = True;
                  break;

         case  PgUp:
                  if( cur_select <= 0)
                    break;

                  page_head-=page_len;
                  cur_select-=page_len;
                  if( page_head < 0 || cur_select < 0 )
                  {
                      /* reset to top of page */
                     page_head = 0;
                     cy = y;
                     cur_select =0;
                  }
                  refrsh = True;
                  break;

         default:
                  Qbeep();
                  break;

      }  /* end case switch */

   }  /* end while choosing */   

   return False;

}   /* end of viewlist */



static int disp_item( int x, int y, device *devptr, int item,
                        int max_item, char *line, int bar, int ex )
/*
**  Display device line item.
**  bar is flag to do the bar for the selection.
*/
{
   Word  color2use;

   /* don't go off end of list */
   if( item > max_item)
      return BOT;  /* at end of list */ 

   if( item < 0 )
      return TOP;  /* top of list */ 

   devptr = devptr + item;

      switch(devptr->devstat)
      {
         /* got a hit, device is there */
         case MQPRESENT:
                  color2use= tx_attr_val.installed;
                  break;
         /* device is not installed */
         case MQNOTPRES:
                  color2use= tx_attr_val.notHere;
                  break;
         /* device is ? */
         case MQMAYBE:
                  color2use= tx_attr_val.maybe;
                  break;
         /* dont know this device */
         case MQBADDEV:
                  color2use= tx_attr_val.BadDev;
                  break;
         default:
                  color2use= tx_attr_val.BadDev + FLASH;
                  break;
      }

   /* see if its a graphics card */
   if( devptr->devcolors > -1L )
   { /* screen thing	*/

      if( devptr->devcolors == 0L )
          sprintf( line," -%-6s   %-33s %4d x %4d    monochrome",
                        devptr->devslash, devptr->devdesc,
                        devptr->devhres, devptr->devvres );
      else
          sprintf( line," -%-6s   %-33s %4d x %4d    %-10ld",
                        devptr->devslash, devptr->devdesc,
                        devptr->devhres, devptr->devvres, devptr->devcolors );

   }
   else  /* this is a mouse thing */
      sprintf(line," -%-6s   %-40s", devptr->devslash, devptr->devdesc );


   /* inverse makes normal disappear, so if bar and normal, */
   /* make normal foreground into black  */
   if(bar)
   {
      /* invert colors */
      color2use |= tx_attr_val.inverse;

      /* if background color == foreground, nothing will show up! */
      if( ( (color2use & 0x7000) >> 4) == (color2use & 0x0700) )
        color2use &= 0xF0FF;  /* make black foreground */
   }
   tx_attr( color2use, x, y, ex+1 );
   tx_xy( x, y );    /* move into position */
   tx_print(line);

   return False;
}


static char getxkey( char *s, char *a)
{
/*
** Get extended keys - returns scancode and ascii value
** in passed args. Sets the first value with the scan 
** code, the second with the ascii code. Returns 0 if
** ascii, scancode if not an ascii key.
*/ 
   *a = (char) getch(); 
   if(*a)       /* if ascii, null scan code */
      *s= '\0';
   else   /* if first code is null, there is a scan code waiting */
      *s= (char) getch();

   /* return the scancode */
   return *s;
}


void tx_title( char *msg, Word y )
/*
** print msg - centered and hilite
*/
{
   int   i,x;

   i = strlen( msg );
   x = ( 80 - i ) / 2;
   tx_attr( tx_attr_val.hilite, x, y, x + i - 1 );
   tx_xy( x, y );
   tx_print( msg );
   return;
}

void tx_print( char *msg )
/*
** Print a string of text, bypassing DOS
*/
{
   while( *msg )  
      tx_chout( (Byte) *msg++ );
   return;
}


#define HORZLINE     ((Byte)205)    /*  Extended ASCII box characters   */
#define VERTLINE     ((Byte)186)
#define LEFTTCORN    ((Byte)201)
#define RIGHTTCORN   ((Byte)187)
#define LEFTBCORN    ((Byte)200)
#define RIGHTBCORN   ((Byte)188)
#define LEFTTEE      ((Byte)204)
#define RIGHTTEE     ((Byte)185)


void tx_frame( Word sx, Word sy, Word ex, Word ey )
/*
** Draw a text frame
*/
{
   unsigned int   i;
   static Byte  HorzLine    = HORZLINE;
   static Byte  VertLine    = VERTLINE;
   static Byte  LeftTCorn   = LEFTTCORN;
   static Byte  RightTCorn  = RIGHTTCORN;
   static Byte  LeftBCorn   = LEFTBCORN;
   static Byte  RightBCorn  = RIGHTBCORN;

   /* horizontal lines */
   for( i = sx + 1; i < ex; i++ )
   {
      tx_xy(i,sy);  
      tx_chout(HorzLine);
      tx_xy(i,ey);  
      tx_chout(HorzLine);
   }

   /* vertical lines */
   for( i = sy + 1; i < ey; i++ )
   {
      tx_xy( sx, i );
      tx_chout(VertLine);
      tx_xy( ex, i );
      tx_chout(VertLine);
   }

   /* place corners */
   tx_xy(sx,sy);    
   tx_chout(LeftTCorn);

   tx_xy(ex,sy);    
   tx_chout(RightTCorn);

   tx_xy(sx,ey);    
   tx_chout(LeftBCorn);

   tx_xy(ex,ey);    
   tx_chout(RightBCorn);
   return;
}

void tx_hbar( Word sx, Word sy, Word ex )
/*
**  draw a horizontal bar with T connectors on each end
*/
{
   unsigned int   i;
   static Byte  HorzLine  = HORZLINE;
   static Byte  LeftTee   = LEFTTEE;
   static Byte  RightTee  = RIGHTTEE;

   for( i = sx + 1; i < ex; i++ )
   {
      tx_xy( i, sy);  
      tx_chout(HorzLine);
   }
   tx_xy( sx, sy );    
   tx_chout(LeftTee);
   tx_xy(ex,sy);    
   tx_chout(RightTee);
   return;
}

static void Qbeep()
/* ring da bell */
{
   printf("\007");
   return;
}


/*
** *************************************************   
** *  PC-DIRECT BIOS CALLS.                        *    
** *  These routines implement the standard text   *  
** *  functions for gotoxy, setting attribs,       *
** *  scrolling text, and outputing a character.   *
** *  A typical 80x25 text screen has coordinates  *
** *  of 0,0 (min) to 79,24 (max)                  * 
** *************************************************
*/

int tx_mono_disp()
/*
** INT 11 equipment determination.
** A fair number of monochome systems behave
** differently than the IBM mono system in their 
** implementation of attribute setting. This
** function decides whether current display is 
** mono or color so evasive action may be taken.
*/
{
   union REGS myregs;
   int   i;

   INTCALL( 0x11, &myregs, &myregs );
   i = myregs.REGWRD.ax & 0x0030;  /* bits 4&5 are display type */  
   if(i == 0x0030)   /* both set means mono display */
      return True;
   else
      return False;
}

void tx_xy( Word x, Word y ) 
/* 
** PC-BIOS gotoxy routine for text mode 
** where x,y are form 0 to max screen size.
** Typically 24 for y and 79 for x
*/
{
   union REGS myregs;

   myregs.REGWRD.ax = 0x0200;        /* setcur */
   myregs.REGWRD.bx = 0;             /* page 0 */
   myregs.REGWRD.dx = (y << 8) + x;
   INTCALL( 0x10, &myregs, &myregs );
   return;
}

void tx_attr( Word attrib, Word sx, Word sy, Word ex )
/* 
** PC-BIOS  set attrib routine for text mode. 
** set the specified text row sx,sy with
** length ex to an attrib (defined above)
** Must be done before text is written.
*/
{

   union REGS myregs;

   myregs.REGWRD.ax = 0x0600;           
   myregs.REGWRD.bx = attrib;
   myregs.REGWRD.cx = (sy << 8) + sx;   /* start pt  upper left  */
   myregs.REGWRD.dx = (sy << 8) + ex;   /* end point lower right */
   INTCALL(0x10,&myregs,&myregs);
   return;
}

void tx_scroll( Word dir, Word sx, Word sy, Word ex, Word ey, Word dist )
/*
** PC-BIOS scroll routine. 
** Moves a specified block of
** text either up or down. 
*/
{
   union REGS myregs;

   if(dir)
      myregs.REGWRD.ax = 0x0600 +dist;     /* scroll up function    */
   else
      myregs.REGWRD.ax = 0x0700 +dist;     /* scroll down function    */

   myregs.REGWRD.bx = tx_attr_val.normal;
   myregs.REGWRD.cx = (sy << 8) + sx;   /* start pt  upper left  */
   myregs.REGWRD.dx = (ey << 8) + ex;   /* end point lower right */
   INTCALL(0x10,&myregs,&myregs);
   return;
}

void tx_chout( Byte ch )
/*
** PC-BIOS Teletype output
** Bypass DOS, as ANSI.SYS driver
** will mess up INVERSE & HILITE
*/
{
   union REGS myregs;

   myregs.REGWRD.ax = 0x0E00 | ch;
   myregs.REGWRD.bx = 0;
   INTCALL(0x10,&myregs,&myregs);
   return;
}

Word time55()
/*
** Return the current 55 millisecond counter value
*/
{
   union REGS my_regs;
   my_regs.REGWRD.ax = 0;
   INTCALL( 0x1A, &my_regs, &my_regs );
   return  (Word)my_regs.REGWRD.dx;
}

void t_quit( int errlvl, char *msg )
/*
**  Exit routine for MetQuery
*/
{
   if( errlvl != 1 )   /* 1 means no MW error */
      tx_clear();

   printf("\n%s", msg);
   exit( errlvl );
}

void tx_clear (void)
{
      tx_scroll( True, 0, 0, 79, 24, 0 );
}

#endif  /* GRQtrimMenu = 0 */


void GrInitErr( int val )
{
   char  buf[40];
   short i;
   
   printf("\nMetaWINDOW SYSTEM ERROR: ");
   switch(val)
   {
      case 1:
            printf("\n\nInitGraphics() has already been called\n");
            break;
      case -1:
            printf("\n\nThe Metagraphics Driver ( METASHEL.EXE ) is not installed.\n");
            break;
      case -2:
            printf("The InitGraphics(%d) call is invalid.\n\nREASON: ",GrafixCard);
            if(GrafixCard == 0)
            {
               printf("MetaWINDOW was unable to find a default adaptor,");
               printf("\n        and you did not select one.");
            }
            else
            {
               i=0;
               while(GrafixCard != o_dev[i].devnum && o_dev[i].devnum != 0)
                  i++;
               strcpy(&buf[0],&o_dev[i].devdesc[0]);
               printf("Device%s\" is not",&buf[0]);
               mwMetaVersion(&buf[0]);
               printf("\n        supported in %s.\n",buf);
            } /* end of not 0 adaptor else if */
            break;
      case -3:
            printf("\n\nError allocating rowtables\n");
            break;
      case -4:
            printf("\n\nCan't map in devices physical memory (protected mode)\n");
            break;
      case -5:
            printf("\n\nDevice won't respond\n");
            break;
      case -6:
            printf("\n\nCan't load devices driver file\n");
            break;
      default:
            printf("\n\nUnknown error code %d\n",val);
            break;

   } /* end of bad Init switch */

   exit(1);
}

/*  End of File - METQUERY.C */

