############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  Borland C++ GUI version for BGI (or GX or MetaWindows)                  #
#                                                                          #
#  Usage :                                                                 #
#                                                                          #
#     make [-DMW] [-DGX] [-DPL] [-DD] -f winlibbg.mak                      #
#                                                                          #
#   where  -DD builds a debugging version                                  #
#          -DGX builds a version for Genus GX                              #
#          -DMW builds a version for MetaWindows                           #
#          -DPL builds a version for Pharlap 286 Extender                  #
#                                                                          #
#   The default is for an optimized, real-mode version for BGI.            #
#                                                                          #
#                                                                          #
############################################################################

#
# Specify the compiler to use
#
CC=gcc

#
# Optimization : -G favors speed over size, -O improves jumps & loops
#
DEBUG=-g -O0
#OPT=-O2
LIBFILE=libmewgr.a

DEFINES=-DDOS -DMEWEL -DMEWEL_GUI

#
# Specify your include path
#
INCPATH=-Ic:/mewel -Id:/djgpp/contrib/bcc2grx/include -Id:/djgpp/contrib/libgrx/include

CFLAGS=$(OPT) $(DEBUG) -c $(DEFINES) $(INCPATH) -xc -ansi -funsigned-char -fwritable-strings

#
# Linker and librarian flags
#
LIB = ar



#
# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.
#
.c.o :
	$(CC) $(CFLAGS) $*.c

.asm.o :
	$(ASM) $(ASMFLAGS) $*;


OBJSASM =    DELAY.o    KEY.o      LCGACPY.o  LMEMCHR.o  LMEMMOVE.o \
LWMEMSET.o LMEMCPY.o  LSTRING.o

OBJSINTR =   WINALT.o   WINBREAK.o WININT24.o

OBJSEXEC =   WINSPAWN.o CHECKPAT.o EXEC.o

OBJSREAL =   WINDOS.o   XMSLIB.o

objs1 =    wactive.o  wadjust.o  wanypop.o  wbitblt.o  wbitmap.o  \
wcheckbx.o wclipbrd.o wcreate.o  wdlgdir.o  wdlgopen.o wdrawtxt.o \
wdrvtype.o weditpro.o wedtdraw.o wemalloc.o wenumobj.o wexitwin.o \
wexttext.o wfileio.o  wfillwin.o wfindwin.o 

objs2 =    wgdichar.o wgdiclip.o wgdifont.o wgdirgb.o  wgdirgn.o  \
wgdiset.o  wgdidda.o  wgdistub.o wgetclas.o wgraystr.o winaccel.o \
winansi.o  winatom.o  winblit.o  wincaret.o wincatch.o winclass.o \
winclick.o wincolor.o wincombo.o wincreat.o windlg.o   windc.o    \
windebug.o 

objs3 =    windefer.o windraw.o  windstry.o winedit.o  winenabl.o \
winenum.o  winevent.o winexec.o  winextra.o winflash.o winhook.o  \
wguiicon.o wininit.o  winkbd.o   winlbdir.o winmdi.o   winmenu.o  \
winmenu2.o winmodal.o winmono.o  wguimous.o winmove.o  winpoint.o \
winprop.o  winqueue.o winrect.o  winres.o   

objs4 =    winscrol.o winset.o   winshow.o  winsize.o  winsound.o \
wintask.o  winterm.o  wintime.o  wintimer.o winvid.o   winzoom.o  \
wischar.o  wkbstack.o wlibrary.o wlistbox.o wlnklist.o wloadacc.o \
wloaddlg.o wloadmnu.o wloadstr.o wlptodp.o  wlstring.o wmemglob.o \
wmemlocl.o wmenuhi.o  wmessage.o wmnuchng.o wmnuinfo.o wmouapi.o  \
wmsgbox.o  

objs5 =    wobject.o  wopenfil.o wpatblt.o  wprofile.o wpushbut.o \
wreform.o  winres2.o  wrubber.o  wsavedc.o  wscroldc.o wscrlbar.o \
wselectr.o wsetrect.o wsetwpos.o wsprintf.o wsrchpth.o wstatic.o  \
wstdproc.o wstfocus.o wsysmenu.o wtabtext.o wtextmet.o wtextout.o \
wzorder.o  wviewext.o wvieworg.o wviewscl.o wwinext.o  wwinorg.o  \
wwinscl.o

objs6 =    wcomfile.o wcomfind.o wspy.o     wrefresh.o wloadbmp.o \
wnotify.o  wgraphic.o wgui.o     wguicurs.o wguipcx.o  wlastpop.o \
wguibmp.o  wguifont.o wgdiflod.o wcrecurs.o wgdipoly.o wgdiplin.o \
wgdigpix.o wgdispix.o wgdielli.o wgdipie.o  wregion.o  wreadini.o \
wgdiarc.o  wstretch.o wkeystat.o wgdifill.o wgdiline.o wgditext.o \
wguimenu.o wcreatdc.o wgdipal.o  wgdifram.o wgdigdib.o wgdisdib.o \
winprint.o wgdiddib.o wgdistrd.o wguidata.o wfiletit.o wcreicon.o \
warricon.o

objs7 =    wrgnfill.o wrgnfram.o wrgninv.o  wrgnpain.o wrgnval.o  \
wplacemt.o wunregcl.o wgdisbit.o wgdiinv.o  wgetpos.o  whfileio.o \
wmfcstub.o wguienum.o wchofont.o wdos386.o


all          :  $(objs1) $(objs2) $(objs3) $(objs4) $(objs5) $(objs6) $(objs7)
	$(LIB) -ru $(LIBFILE) $(objs1) $(objs2) $(objs3) $(objs4) $(objs5) $(objs6) $(objs7)

