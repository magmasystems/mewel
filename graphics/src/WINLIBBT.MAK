###############################################################################
#                                                                             #
#  MAKEFILE for the MEWEL windowing system.                                   #
#                                                                             #
#  Borland C++ version                                                        #
#                                                                             #
#  Usage :                                                                    #
#                                                                             #
# make [-DD] [-DTEXT] [-DMW] [-DGX] [-DDP16] [-DDLL] [-DRTL] -f winlibb.mak   #
#                                                                             #
#   where  -DD builds a debugging version                                     #
#                                                                             #
#          -DTEXT builds a version for Text Mode                              #
#          -DGX builds a version for Genus GX GUI                             #
#          -DMW builds a version for MetaWindows GUI                          #
#                                                                             #
#          -DDP16 builds a version for DPMI16                                 #
#          -DDP32 builds a version for DPMI32                                 #
#          -DDLL  builds a DLL version                                        #
#          -DRTL DLL references BC40RTL.DLL                                   #
#                                                                             #
#   The default is for an optimized, real-mode version for BGI.               #
#                                                                             #
#                                                                             #
###############################################################################
#
# These are the hard-coded directory names which you need to change
#
# MEWEL include files
MEWELINC=\mewel
# MEWEL library destination
MEWELLIB=\mewel
# Root directory of the Borland compiler
BCPATH=\bc45

# TLINK Response file
LNKFILE = winlibb.lnk
# BCC Response file
RSPFILE = winlibb.rsp

# Model is M for Medium, L for Large, S for Small
MODEL = -ml
!if $d(DP32)
MODEL =
!endif

#
# Specify the compiler to use
#
!ifdef DP32
  CC=bcc32
!else
  CC=bcc
!endif

#
# Optimization : -G favors speed over size, -O improves jumps & loops
#
!ifdef D
  DEBUG=-v
!else
  OPT=-G -O
!endif

#
# Include path
#
INC=-I$(MEWELINC) -I$(BCPATH)\include

# 
# If you define either MW or GX or TEXT, then a library will be built for
# MetaWindows (by MetaGraphics), GX (by Genus) or DOS Text rather than for BGI.
#   ie :  make -DGX -f winlibb.mak
#
LIBFILE = mewelbgi
!ifndef TEXT
!ifdef (DP32)
  GUILIB=bgi32
!else
  GUILIB=bgi16
!endif
!endif

!ifdef TEXT
LIBFILE=meweltxt
!endif

!ifdef MW
ENGINE=-DMETA
METAINC=\metawin
METALIB=\metawin
INC=$(INC) -I$(METAINC)
LIBFILE=mewelmwb
!if $d(DP32)
GUILIB=$(METALIB)\met_xb3o
!else
GUILIB=$(METALIB)\met_xd2o
!endif
!endif

!ifdef GX
ENGINE=-DGX
INC=$(INC) -Ic:\genus3
GUILIB=
!ifdef GXTEXT
ENGINE=$(ENGINE) -DUSE_GX_TEXT
!endif
LIBFILE=mewelgxb
!endif

#
# Specify overlay or DPMI-extender flags
#
OVL=-Yo
PREPOVL=-Y
!if $d(DP32) || $d(DP16)
  OVL=
!ifdef DLL
!ifdef DP16
      PREPOVL=-WXD -2
!else
      PREPOVL=-WXDE
!endif
!else
    PREPOVL=-WX
!ifdef DP16
      PREPOVL=$(PREPOVL) -2
!endif
!endif
!endif

!ifndef DLL
STKCHK=-N
!endif

DEFINES=-DDOS 
!ifndef TEXT
DEFINES = $(DEFINES) -DMEWEL_GUI
!endif

CFLAGS= $(PREPOVL) $(ENGINE) $(OPT) $(DEBUG) -c -d -f- -k -K $(STKCHK) $(MODEL) $(DEFINES) $(INC) -H

!ifdef ZAPP
CFLAGS=$(CFLAGS) -DZAPP
!endif

!ifdef ACAD
CFLAGS=$(CFLAGS) -DACAD
!endif

#
# Assembler flags
#
ASM = tasm
ASMFLAGS = /dTC=1 /dLMODEL=1 /dMODL=large /mx
!ifdef D
ASMFLAGS=$(ASMFLAGS) /zi
!endif
!ifdef DP16
ASMFLAGS=$(ASMFLAGS) /dDOS286X=1
!endif

#
# Linker and librarian flags
#
LIB = tlib
!ifdef DP32
  LINK = tlink32
  LFLAGS = -L$(BCPATH)\lib -Tpd $(DEBUG)
!else
  LINK = tlink
  LFLAGS = -L$(BCPATH)\lib -Twd $(DEBUG)
!ifdef RTL
    LFLAGS = /c/C $(LFLAGS)
!endif
!endif


#
# For a 16-bit DLL version, pick a DEF file
#
!if $d(DP16) && $d(DLL)
!if $d(MW)
      DEFFILE16=mewelmet.def
!elif $d(TEXT)
      DEFFILE16=meweltxt.def
!else
      DEFFILE16=mewelg16.def
!endif
!endif


###############################################################################
#                                                                             #
# Base object files for ALL builds                                            #
#                                                                             #
###############################################################################
OBJSBASE = \
WACTIVE.OBJ  \
WADJUST.OBJ  \
WANYPOP.OBJ  \
WARRICON.OBJ \
WBITBLT.OBJ  \
WBITMAP.OBJ  \
WCHECKBX.OBJ \
WCHOCOLR.OBJ \
WCHOFONT.OBJ \
WCLIPBRD.OBJ \
WCOMFILE.OBJ \
WCOMFIND.OBJ \
WCREATDC.OBJ \
WCREATE.OBJ  \
WDESKTOP.OBJ \
WDLGCTID.OBJ \
WDLGDIR.OBJ  \
WDLGOPEN.OBJ \
WDRAWTXT.OBJ \
WDRVTYPE.OBJ \
WEDITPRO.OBJ \
WEDTDRAW.OBJ \
WEMALLOC.OBJ \
WENUMOBJ.OBJ \
WEXITWIN.OBJ \
WEXTTEXT.OBJ \
WFILEIO.OBJ  \
WFILETIT.OBJ \
WFILLWIN.OBJ \
WFINDWIN.OBJ \
WGDICHAR.OBJ \
WGDICLIP.OBJ \
WGDIFONT.OBJ \
WGDIRGB.OBJ  \
WGDIRGN.OBJ  \
WGDISET.OBJ  \
WGDISTUB.OBJ \
WGETCLAS.OBJ \
WGETPOS.OBJ  \
WGRAYSTR.OBJ \
WHFILEIO.OBJ \
WINACCEL.OBJ \
WINANSI.OBJ  \
WINATOM.OBJ  \
WINATOM2.OBJ \
WINBLIT.OBJ  \
WINCARET.OBJ \
WINCLASS.OBJ \
WINCLICK.OBJ \
WINCOLOR.OBJ \
WINCOMBO.OBJ \
WINCREAT.OBJ \
WINDATA.OBJ  \
WINDC.OBJ    \
WINDEFER.OBJ \
WINDLG.OBJ   \
WINDRAW.OBJ  \
WINDSTRY.OBJ \
WINEDIT.OBJ  \
WINENABL.OBJ \
WINENUM.OBJ  \
WINEVENT.OBJ \
WINEXEC.OBJ  \
WINEXTRA.OBJ \
WINFLASH.OBJ \
WINHOOK.OBJ  \
WININIT.OBJ  \
WINKBD.OBJ   \
WINLBDIR.OBJ \
WINMDI.OBJ   \
WINMENU.OBJ  \
WINMENU2.OBJ \
WINMODAL.OBJ \
WINMONO.OBJ  \
WINMOVE.OBJ  \
WINQUEUE.OBJ \
WINPOINT.OBJ \
WINPRINT.OBJ \
WINPROP.OBJ  \
WINRECT.OBJ  \
WINRES.OBJ   \
WINRES2.OBJ  \
WINRES3.OBJ  \
WINSCROL.OBJ \
WINSET.OBJ   \
WINSHOW.OBJ  \
WINSIZE.OBJ  \
WINSOUND.OBJ \
WINTASK.OBJ  \
WINTERM.OBJ  \
WINTIME.OBJ  \
WINTIMER.OBJ \
WINVID.OBJ   \
WINZOOM.OBJ  \
WISCHAR.OBJ  \
WKBSTACK.OBJ \
WKEYSTAT.OBJ \
WLASTPOP.OBJ \
WLISTBOX.OBJ \
WLNKLIST.OBJ \
WLOADACC.OBJ \
WLOADBMP.OBJ \
WLOADDLG.OBJ \
WLOADMNU.OBJ \
WLOADSTR.OBJ \
WLPTODP.OBJ  \
WLSTRING.OBJ \
WMAPPOIN.OBJ \
WMENUBMP.OBJ \
WMENUHI.OBJ  \
WMESSAGE.OBJ \
WMFCSTUB.OBJ \
WMNUCHNG.OBJ \
WMNUINFO.OBJ \
WMODULE.OBJ  \
WMOUAPI.OBJ  \
WMSGBOX.OBJ  \
WMULDIV.OBJ  \
WNOTIFY.OBJ  \
WOBJECT.OBJ  \
WOPENFIL.OBJ \
WPATBLT.OBJ  \
WPLACEMT.OBJ \
WPRNTDLG.OBJ \
WPROCINS.OBJ \
WPROFILE.OBJ \
WPUSHBUT.OBJ \
WREFORM.OBJ  \
WREFRESH.OBJ \
WRGNFILL.OBJ \
WRGNFRAM.OBJ \
WRGNINV.OBJ  \
WRGNPAIN.OBJ \
WRGNVAL.OBJ  \
WRUBBER.OBJ  \
WSAVEDC.OBJ  \
WSCROLDC.OBJ \
WSCRLBAR.OBJ \
WSETRECT.OBJ \
WSETWPOS.OBJ \
WSHOWPOP.OBJ \
WSPRINTF.OBJ \
WSPY.OBJ     \
WSRCHPTH.OBJ \
WSTATIC.OBJ  \
WSTDPROC.OBJ \
WSTFOCUS.OBJ \
WSTUBSB1.OBJ \
WSTUBSG1.OBJ \
WSTUBSG2.OBJ \
WSTUBSG3.OBJ \
WSTUBSK1.OBJ \
WSTUBSS1.OBJ \
WSTUBSU1.OBJ \
WSTUBSU2.OBJ \
WSTUBSU3.OBJ \
WSTUBSU4.OBJ \
WSTUBSU5.OBJ \
WSTUBSU6.OBJ \
WSTUBSU7.OBJ \
WSYSMENU.OBJ \
WTABTEXT.OBJ \
WTEXTMET.OBJ \
WTEXTOUT.OBJ \
WUNREGCL.OBJ \
WVIEWEXT.OBJ \
WVIEWORG.OBJ \
WVIEWSCL.OBJ \
WWINEXT.OBJ  \
WWINORG.OBJ  \
WWINSCL.OBJ  \
WZORDER.OBJ

###############################################################################
#                                                                             #
# Target (real, DPMI16, DPMI32) specific object files                         #
#                                                                             #
###############################################################################

##################################
# Borland DPMI 16                #
##################################
!if $d(DP16)
OBJSTARG = DELAY.OBJ    \
	   KEY.OBJ      \
	   LCGACPY.OBJ  LMEMCHR.OBJ  LMEMCPY.OBJ  LMEMMOVE.OBJ LSTRING.OBJ  LWMEMSET.OBJ \
	   WINALT.OBJ   \
	   WINBREAK.OBJ \
	   WININT24.OBJ \
	   WBORRTM.OBJ 
##################################
# Borland DPMI 32                #
##################################
!elif $d(DP32)
OBJSTARG = WDOS386.OBJ  \
	   WDPMI.OBJ
##################################
# Real Mode                      #
##################################
!else
OBJSTARG = CHECKPAT.OBJ \
	   DELAY.OBJ \
	   EXEC.OBJ \
	   KEY.OBJ \
	   LCGACPY.OBJ LMEMCHR.OBJ LMEMCPY.OBJ LMEMMOVE.OBJ LSTRING.OBJ LWMEMSET.OBJ \
	   MOUSESUB.OBJ \
	   WINALT.OBJ \
	   WINBREAK.OBJ \
	   WINCATCH.OBJ \
	   WINDEBUG.OBJ WINDOS.OBJ \
	   WININT24.OBJ \
	   WINSPAWN.OBJ \
	   WLIBRARY.OBJ \
	   WMEMGLOB.OBJ WMEMLOCL.OBJ \
	   WSELECTR.OBJ \
	   XMSLIB.OBJ
!endif

###############################################################################
#                                                                             #
# Graphics mode (GUI, Text) specific object files                             #
#                                                                             #
###############################################################################

##################################
# Text Mode                      #
##################################
!ifdef TEXT
OBJSMODE = RECT.OBJ     \
	   VBTSHLP.OBJ  \
	   WENUMFNT.OBJ \
	   WGDIRECT.OBJ WGDISHAP.OBJ \
	   WINBORDR.OBJ WINICON.OBJ  WINMOUSE.OBJ WINVIRT.OBJ  \
	   WLINETO.OBJ  \
	   WMETAFIL.OBJ \
	   WMNUDRAW.OBJ \
	   WSHADOW.OBJ  WSTRDISP.OBJ \
	   WVISMAP.OBJ

   ##################################
   # Borland DPMI 16                #
   ##################################
!if $d(DP16)
   OBJSMODE = $(OBJSMODE) \
	      WVGAWRT.OBJ

   ##################################
   # Real Mode                      #
   ##################################
!elif!$d(DP32)
   OBJSMODE = $(OBJSMODE)  \
	      WVGAMAP.OBJ  \
	      WVGAWRT.OBJ
!endif
##################################
# GUI Mode                       #
##################################
!else
OBJSMODE = WCHOFONT.OBJ WCRECURS.OBJ WCREICON.OBJ \
	   WGDIARC.OBJ \
	   WGDIBMP2.OBJ WGDIBRUS.OBJ \
	   WGDIDDA.OBJ WGDIDDIB.OBJ \
	   WGDIELLI.OBJ \
	   WGDIFILL.OBJ WGDIFLOD.OBJ WGDIFRAM.OBJ \
	   WGDIGDIB.OBJ WGDIGPIX.OBJ \
	   WGDIINV.OBJ \
	   WGDILINE.OBJ \
	   WGDIPIE.OBJ WGDIPLIN.OBJ WGDIPOLY.OBJ \
	   WGDISBIT.OBJ WGDISDIB.OBJ WGDISPIX.OBJ WGDISTRD.OBJ \
	   WGDITEXT.OBJ WGETTEMP.OBJ \
	   WGRAPHIC.OBJ \
	   WGUI.OBJ WGUIBMP.OBJ WGUICURS.OBJ WGUIDATA.OBJ WGUIENUM.OBJ WGUIFONT.OBJ \
	   WGUIICON.OBJ WGUIMENU.OBJ WGUIPCX.OBJ \
	   WMETAFIL.OBJ \
	   WREADINI.OBJ WREGION.OBJ \
	   WSOUND2.OBJ  WSTRETCH.OBJ

     ##################################
     # Real Mode and DPMI 16          #
     ##################################
!ifndef DP32
     OBJSMODE = $(OBJSMODE) \
		WGDIPAL.OBJ
!endif

     ##################################
     # MetaWindow                     #
     ##################################
!if $d(MW)
     OBJSMODE = $(OBJSMODE)  \
		WGUIMOUS.OBJ
	  ##################################
	  # Borland DPMI 16                #
	  ##################################
!ifdef DP16
	  OBJSMODE = $(OBJSMODE) \
		     MWGRAF.OBJ
!endif

     ##################################
     # Genus GX                       #
     ##################################
!elif $d(GX)
     OBJSMODE = $(OBJSMODE)  \
		WGUIMOUS.OBJ

     ##################################
     # Borland BGI                    #
     ##################################
!else
     OBJSMODE = $(OBJSMODE)  \
		WINMOUSE.OBJ
!endif
!endif

OBJSALL = $(OBJSBASE) $(OBJSTARG) $(OBJSMODE)

###############################################################################
# Build it!                                                                   #
###############################################################################
Everything : ResponseFile LinkFile Library

Library : $(OBJSALL)
!ifdef DLL
     @echo Creating DLL...
     $(LINK) $(LFLAGS) @$(LNKFILE)
     implib $(MEWELLIB)\$(LIBFILE).lib $(MEWELLIB)\mewel.dll
     @echo Done!
!else
     @echo Creating library...
     @del $(MEWELLIB)\$(LIBFILE).lib > NUL
     $(LIB) /C /P64 $(MEWELLIB)\$(LIBFILE) @$(LNKFILE)
     @echo Done!
!endif

###################################################
# Create response file                            #
###################################################
ResponseFile : 
     @echo Creating $(CC) response file...
     @copy &&~
     $(CFLAGS)
~ $(RSPFILE) > NUL

###################################################
# Create link file                                #
###################################################
LinkFile : 
     @del $(LNKFILE) > NUL
     @echo Creating $(LINK) response file...
     ##################################
     # DLL                            #
     ##################################
!ifdef DLL
	  ##################################
	  # Borland DPMI 16                #
	  ##################################
!ifdef DP16
	       ##################################
	       # RTL                            #
	       ##################################
!ifdef RTL
	       @copy &&~
		    c0dl $(OBJSALL)
		    $(MEWELLIB)\mewel.dll
		    nul
		    dpmi16 crtldll $(GUILIB)
		    $(DEFFILE16)
~ $(LNKFILE) > NUL
	       ##################################
	       #                                #
	       ##################################
!else
	       @copy &&~
		    c0dl $(OBJSALL)
		    $(MEWELLIB)\mewel.dll
		    nul
		    dpmi16 cwl mathwl $(GUILIB)
		    $(DEFFILE16)
~ $(LNKFILE) > NUL
!endif
	  ##################################
	  # Borland DPMI 32                #
	  ##################################
!else
	  @copy &&~
	       c0d32 $(OBJSALL)
	       $(MEWELLIB)\mewel.dll
	       nul
	       dpmi32 cw32i $(GUILIB)
	       mewelg32.def 
~ $(LNKFILE) > NUL
!endif
!else
     ##################################
     # Static Library                 #
     ##################################
	  @copy &&~
	  + $(OBJSALL:.OBJ= +)
~ $(LNKFILE) > NUL
!endif

######################################################
# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the .LNK file to create
#          the library in a single shot.
######################################################
.c.obj :
  $(CC) @$(RSPFILE) $*.c
!ifdef ADD
  $(LIB) /C $(LIBFILE) -+$*.obj
!endif

.asm.obj :
	$(ASM) $(ASMFLAGS) $*;



######################################################
# Overlayed OBJS for Real Mode                       #
######################################################
!if !$(DP16) && !$(DP32)
WADJUST.OBJ  :  WADJUST.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WANYPOP.OBJ  :  WANYPOP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WARRICOB.OBJ :  WARRICON.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WBITMAP.OBJ  :  WBITMAP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCHECKBX.OBJ :  WCHECKBX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCHOCOLR.OBJ :  WCHOCOLR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCHOFONT.OBJ :  WCHOFONT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCLIPBRD.OBJ :  WCLIPBRD.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCOMFILE.OBJ :  WCOMFILE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCOMFIND.OBJ :  WCOMFIND.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCREATDC.OBJ :  WCREATDC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCRECURS.OBJ :  WCRECURS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WCREICON.OBJ :  WCREICON.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WDLGCTID.OBJ :  WDLGCTID.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WDLGDIR.OBJ  :  WDLGDIR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WDLGOPEN.OBJ :  WDLGOPEN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WDRAWTXT.OBJ :  WDRAWTXT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WDRVTYPE.OBJ :  WDRVTYPE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WENUMFNT.OBJ :  WENUMFNT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WENUMOBJ.OBJ :  WENUMOBJ.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WEXITWIN.OBJ :  WEXITWIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WFILEIO.OBJ  :  WFILEIO.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WFILETIT.OBJ :  WFILETIT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WFILLWIN.OBJ :  WFILLWIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WFINDWIN.OBJ :  WFINDWIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIARC.OBJ  :  WGDIARC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIBMP2.OBJ :  WGDIBMP2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIBRUS.OBJ :  WGDIBRUS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDICLIP.OBJ :  WGDICLIP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIELLI.OBJ :  WGDIELLI.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIFLOD.OBJ :  WGDIFLOD.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIFONT.OBJ :  WGDIFONT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIFRAM.OBJ :  WGDIFRAM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIGDIB.OBJ :  WGDIGDIB.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIGPIX.OBJ :  WGDIGPIX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIINV.OBJ  :  WGDIINV.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIMAP.OBJ  :  WGDIMAP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIPAL.OBJ  :  WGDIPAL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIPIE.OBJ  :  WGDIPIE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIPLIN.OBJ :  WGDIPLIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIPOLY.OBJ :  WGDIPOLY.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIRGN.OBJ  :  WGDIRGN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISBIT.OBJ :  WGDISBIT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISDIB.OBJ :  WGDISDIB.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISET.OBJ  :  WGDISET.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDIDDA.OBJ  :  WGDIDDA.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISPIX.OBJ :  WGDISPIX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISTRD.OBJ :  WGDISTRD.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGDISTUB.OBJ :  WGDISTUB.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGETCLAS.OBJ :  WGETCLAS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGETPOS.OBJ  :  WGETPOS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGETTEMP.OBJ :  WGETTEMP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGRAYSTR.OBJ :  WGRAYSTR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGUIBMP.OBJ  :  WGUIBMP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGUIENUM.OBJ :  WGUIENUM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGUIFONT.OBJ :  WGUIFONT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGUIPCX.OBJ  :  WGUIPCX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WHFILEIO.OBJ :  WHFILEIO.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINANSI.OBJ  :  WINANSI.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINATOM.OBJ  :  WINATOM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINATOM2.OBJ :  WINATOM2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINCATCH.OBJ :  WINCATCH.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINCLASS.OBJ :  WINCLASS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINCOMBO.OBJ :  WINCOMBO.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINDEBUG.OBJ :  WINDEBUG.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINDEFER.OBJ :  WINDEFER.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINDOS.OBJ   :  WINDOS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINDRAW.OBJ  :  WINDRAW.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINDSTRY.OBJ :  WINDSTRY.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINEDIT.OBJ  :  WINEDIT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINENABL.OBJ :  WINENABL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINENUM.OBJ  :  WINENUM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINEXEC.OBJ  :  WINEXEC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINEXTRA.OBJ :  WINEXTRA.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINFLASH.OBJ :  WINFLASH.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINHOOK.OBJ  :  WINHOOK.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WGUIICON.OBJ :  WGUIICON.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WININIT.OBJ  :  WININIT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINKBD.OBJ   :  WINKBD.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINLBDIR.OBJ :  WINLBDIR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINMDI.OBJ   :  WINMDI.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINMENU.OBJ  :  WINMENU.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINMENU2.OBJ :  WINMENU2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINMODAL.OBJ :  WINMODAL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINMONO.OBJ  :  WINMONO.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINPRINT.OBJ :  WINPRINT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINPROP.OBJ  :  WINPROP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINRES.OBJ   :  WINRES.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINRES2.OBJ  :  WINRES2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINRES3.OBJ  :  WINRES3.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINSCROL.OBJ :  WINSCROL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINSET.OBJ   :  WINSET.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINSHOW.OBJ  :  WINSHOW.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINSIZE.OBJ  :  WINSIZE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINSOUND.OBJ :  WINSOUND.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINTASK.OBJ  :  WINTASK.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINVID.OBJ   :  WINVID.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WINZOOM.OBJ  :  WINZOOM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WISCHAR.OBJ  :  WISCHAR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WKBSTACK.OBJ :  WKBSTACK.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLASTPOP.OBJ :  WLASTPOP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLIBRARY.OBJ :  WLIBRARY.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLISTBOX.OBJ :  WLISTBOX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLOADACC.OBJ :  WLOADACC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLOADBMP.OBJ :  WLOADBMP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLOADDLG.OBJ :  WLOADDLG.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLOADMNU.OBJ :  WLOADMNU.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLOADSTR.OBJ :  WLOADSTR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WLPTODP.OBJ  :  WLPTODP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMAPPOIN.OBJ  :  WMAPPOIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMENUBMP.OBJ  :  WMENUBMP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMENUHI.OBJ  :  WMENUHI.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMETAFIL.OBJ :  WMETAFIL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMFCSTUB.OBJ :  WMFCSTUB.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMNUCHNG.OBJ :  WMNUCHNG.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMNUINFO.OBJ :  WMNUINFO.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMODULE.OBJ  :  WMODULE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMSGBOX.OBJ  :  WMSGBOX.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WMULDIV.OBJ  :  WMULDIV.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WOPENFIL.OBJ :  WOPENFIL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPATBLT.OBJ  :  WPATBLT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPLACEMT.OBJ :  WPLACEMT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPRNTDLG.OBJ :  WPRNTDLG.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPROCINS.OBJ :  WPROCINS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPROFILE.OBJ :  WPROFILE.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WPUSHBUT.OBJ :  WPUSHBUT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WREADINI.OBJ :  WREADINI.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WREFORM.OBJ  :  WREFORM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRGNFILL.OBJ :  WRGNFILL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRGNFRAM.OBJ :  WRGNFRAM.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRGNINV.OBJ  :  WRGNINV.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRGNPAIN.OBJ :  WRGNPAIN.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRGNVAL.OBJ  :  WRGNVAL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WRUBBER.OBJ  :  WRUBBER.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSAVEDC.OBJ  :  WSAVEDC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSCROLDC.OBJ :  WSCROLDC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSCRLBAR.OBJ :  WSCRLBAR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSELECTR.OBJ :  WSELECTR.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSETRECT.OBJ :  WSETRECT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSETWPOS.OBJ :  WSETWPOS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSHOWPOP.OBJ :  WSHOWPOP.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSOUND2.OBJ  :  WSOUND2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSPRINTF.OBJ :  WSPRINTF.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSPY.OBJ     :  WSPY.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTATIC.OBJ  :  WSTATIC.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTFOCUS.OBJ :  WSTFOCUS.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTRETCH.OBJ :  WSTRETCH.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSB1.OBJ :  WSTUBSB1.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSG1.OBJ :  WSTUBSG1.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSG2.OBJ :  WSTUBSG2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSG3.OBJ :  WSTUBSG3.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSK1.OBJ :  WSTUBSK1.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSS1.OBJ :  WSTUBSS1.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU1.OBJ :  WSTUBSU1.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU2.OBJ :  WSTUBSU2.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU3.OBJ :  WSTUBSU3.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU4.OBJ :  WSTUBSU4.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU5.OBJ :  WSTUBSU5.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU6.OBJ :  WSTUBSU6.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSTUBSU7.OBJ :  WSTUBSU7.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WSYSMENU.OBJ :  WSYSMENU.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WTABTEXT.OBJ :  WTABTEXT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WTEXTMET.OBJ :  WTEXTMET.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WTEXTOUT.OBJ :  WTEXTOUT.C
  $(CC) @$(RSPFILE) $(OVL) $*.c

WUNREGCL.OBJ :  WUNREGCL.C
  $(CC) @$(RSPFILE) $(OVL) $*.c
!endif

