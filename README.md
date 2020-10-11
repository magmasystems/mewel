# MEWEL
A windowing system for DOS

## MEWEL/GUI 4.0

## Notes on the 4.0 Libs

Libraries included in MEWELGUI.ZIP
  MEWELMGM.LIB - Microsoft large model, GUI (for MSC graphics.lib)
  MEWELBGI.LIB - Borland   large model, GUI (for BGI)

You must use the 4.0 resource compiler with 4.0 libraries. Your app will crash and burn if you use RES files produced by the 3.40 resource compiler.

## Demo program

WMEWLDEM.ZIP contains a demo program which runs under DOS text, DOS GUI, and Windows. The file MAKEFILE.BC is for GUI/BGI and MAKEFILE.MSC is for GUI/MSC. WMEWLWIN.PRJ is the Borland project file for the Windows version.

## COMPILING FOR THE GUI VERSION  (*IMPORTANT*)

Notice that when you compile a program for the MEWEL GUI, you *must* put /DMEWEL_GUI in the compile line (under both BC and MSC). Otherwise, some of the internal constants will default to MEWEL's text mode settings.

## Troubleshooting

The largest number of tech support calls we get is by far related to using an improper MEWEL.INI file.

Make sure that you are using the proper MEWEL.INI file. The disk contains several different INI files. Choose the one which fits the graphics engine you are using, and customize it for your environment, and rename it MEWEL.INI. Put the MEWEL.INI in your current directory, or anywhere in your DOS path. 

* MEWELMSC.INI - Microsoft graphics engine
* MEWELBGI.INI - Borland BGI
* MEWELGX.INI  - Genus GX and any compiler
* MEWELMET.INI - MetaWindows and any compiler
* MEWELWAT.INI - Watcom compiler
* MEWELHIC.INI - High C compiler and MetaWindows

The display.drv variable in the MEWEL.INI file is extremely important for NGI, MSC and MetaWindows. Make sure you have it pointing to the proper directory.

## Genus GX, MetaWindows, Watcom or MetaWare High C

You need the MEWEL source code to create libraries for the MetaWindows or
the Genus GX graphics libraries, and the Watcom or High C compilers.

When you dearchive the MEWEL source code, you'll find several makefiles.
They are :

* winlibmg.mak - make libs the for Microsoft compiler for either the MSC, GX, or MetaWindows graphics engine.

* winlibbg.mak - make libs the for Borland compiler for either the BGI, GX, or MetaWindows graphics engine.

* winlibwg.mak - make libs the for the Watcom compiler

* winlibhg.mak - make libs for the High C compiler

Please read the beginning of each makefile for various options.
