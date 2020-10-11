############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  This is for the Watcom 386 DOS Text version                             #
#                                                                          #
#                                                                          #
# To make a WATCOM 9/10 version of MEWEL, use the following command :      #
#                                                                          #
#  wmake [D=1] [PL=1] -f winlibwl.mak                                      #
#                                                                          #
# where D=1 for a debugging version                                        #
#       PL=1 for a Pharlap 386/TNT version                                 #
#                                                                          #
############################################################################

#
# Compiler flags
#
# For Optimization
#
# -3r             386 register calling conventions
# -3s             386 stack calling conventions
# -o{a,c,d,e,f[+],i,l,m,n,o,p,r,s,t,u,x} control optimization
#   a		-> relax aliasing constraints
#   c		-> disable <call followed by return> to <jump> optimization
#   d		-> disable all optimizations
#   e[=<num>]	-> expand user functions inline. <num> controls max size
#   f           -> generate traceable stack frames as needed
#   f+          -> always generate traceable stack frames
#   i		-> expand intrinsic functions inline
#   l		-> enable loop optimizations
#   m           -> generate inline 80x87 code for math functions
#   n		-> allow numerically unstable optimizations
#   o           -> continue compilation if low on memory
#   p           -> generate consistent floating-point results
#   r		-> reorder instructions for best pipeline usage
#   s		-> favor code size over execution time in optimizations
#   t		-> favor execution time over code size in optimizations
#   u		-> all functions must have unique addresses
#   x		-> equivalent to -omiler -s
# -s   disable stack checking
#
# For Debugging
# -d2 puts full debugging info in the object file
#

#
# Set this to the path where the MEWEL files are
#
MEWELPATH=\mewel

#
# Set this to the path where the WATCOM files are
#
WATCOMPATH=f:\watcom

#
# Which compiler to use...
#
CC=wcc386p

#
# Memory model (flat)
#
MODEL=-mf

#
# Debugging or optimization
#
#  Watcom 9.x has some optimization bugs, so you might want to try D=1
#
!ifdef D
DEBUG=-d2 -dDEBUG -od -I$(WATCOMPATH)\h
!else
OPT=-oit -s
!endif

#
# Text mode or GUI
#
#PLATFORM=-dMEWEL_TEXT


#
# Pharlap 386/TNT
#
!ifdef PL
EXTENDER=-dPLTNT -In:\tnt\include
!endif

#
# Compiler flags
#
CFLAGS=-3s -w2 -zq $(MODEL) $(PLATFORM) $(GUI) $(EXTENDER) $(INC) $(DEBUG) $(OPT)

# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.

.c.obj :
	$(CC) $(CFLAGS) $*.c
!ifdef PL
	womp /fp /dp $*.obj
!endif


#
# Watcom librarian
#
LIB = wlib

# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIBWL.LNK file to create
#          the library in a single shot.

.c.obj :
  $(CC) $(CFLAGS) $*.c


OBJS1 =      WACTIVE.OBJ  WADJUST.OBJ  WANYPOP.OBJ  WBITMAP.OBJ  WCHECKBX.OBJ &
WCLIPBRD.OBJ WCREATE.OBJ  WDLGDIR.OBJ  WDLGOPEN.OBJ WDRAWTXT.OBJ WDRVTYPE.OBJ &
WEDITPRO.OBJ WEDTDRAW.OBJ WEMALLOC.OBJ WENUMFNT.OBJ WENUMOBJ.OBJ WEXITWIN.OBJ &
WEXTTEXT.OBJ WFILEIO.OBJ  WFILLWIN.OBJ WFINDWIN.OBJ 

OBJS2 =      WGDICHAR.OBJ WGDICLIP.OBJ WGDIFONT.OBJ WGDIRECT.OBJ &
WGDIRGB.OBJ  WGDIRGN.OBJ  WGDISET.OBJ  WGDISHAP.OBJ WGDISTUB.OBJ WGETCLAS.OBJ &
WGRAYSTR.OBJ WINACCEL.OBJ WINANSI.OBJ  WINATOM.OBJ  WINBLIT.OBJ  WINBORDR.OBJ &
WINCARET.OBJ WINCATCH.OBJ WINCLASS.OBJ WINCLICK.OBJ WINCOLOR.OBJ &
WINCOMBO.OBJ WINCREAT.OBJ WINDOS.OBJ   WINDLG.OBJ   WINDC.OBJ    WINDEBUG.OBJ 

OBJS3 =      WINDEFER.OBJ WINDRAW.OBJ  WINDSTRY.OBJ WINEDIT.OBJ  WINENABL.OBJ &
WINENUM.OBJ  WINEVENT.OBJ WINEXEC.OBJ  WINEXTRA.OBJ WINFLASH.OBJ WINHOOK.OBJ  &
WINICON.OBJ  WININIT.OBJ  WINKBD.OBJ   WINLBDIR.OBJ WINMDI.OBJ   &
WINMENU.OBJ  WINMENU2.OBJ WINMODAL.OBJ WINMONO.OBJ  WINMOUSE.OBJ WINMOVE.OBJ  &
WINPOINT.OBJ WINPROP.OBJ  WINQUEUE.OBJ WINRECT.OBJ  WINRES.OBJ   WINRES2.OBJ  &
WINRES3.OBJ

OBJS4 =      WINSCROL.OBJ WINSET.OBJ   WINSHOW.OBJ  WINSIZE.OBJ  WINSOUND.OBJ &
WINTASK.OBJ  WINTERM.OBJ  WINTIME.OBJ  WINTIMER.OBJ WINVID.OBJ   &
WINVIRT.OBJ  WINZOOM.OBJ  WISCHAR.OBJ  WKBSTACK.OBJ WLASTPOP.OBJ WLIBRARY.OBJ &
WLINETO.OBJ  WLISTBOX.OBJ WLNKLIST.OBJ WLOADACC.OBJ WLOADBMP.OBJ WLOADDLG.OBJ &
WLOADMNU.OBJ WLOADSTR.OBJ WLPTODP.OBJ  WLSTRING.OBJ WMEMGLOB.OBJ WMEMLOCL.OBJ &
WMENUHI.OBJ  WMESSAGE.OBJ WMNUCHNG.OBJ WMNUDRAW.OBJ WMNUINFO.OBJ WMOUAPI.OBJ  &
WMSGBOX.OBJ  WNOTIFY.OBJ

OBJS5 =      WOBJECT.OBJ  WOPENFIL.OBJ WPATBLT.OBJ  WPROFILE.OBJ WPUSHBUT.OBJ &
WREFORM.OBJ  WREFRESH.OBJ WRUBBER.OBJ  WSAVEDC.OBJ  WSCROLDC.OBJ WSCRLBAR.OBJ &
WSELECTR.OBJ WSETRECT.OBJ WSETWPOS.OBJ WSHADOW.OBJ  WSPRINTF.OBJ WSRCHPTH.OBJ &
WSTATIC.OBJ  WSTDPROC.OBJ WSTFOCUS.OBJ WSTRDISP.OBJ WSYSMENU.OBJ WTEXTMET.OBJ &
WTEXTOUT.OBJ WVISMAP.OBJ  WZORDER.OBJ

OBJS6 =      WCOMFILE.OBJ WCOMFIND.OBJ WSPY.OBJ     WLOADBMP.OBJ WDOS386.OBJ  &
WKEYSTAT.OBJ WWINORG.OBJ  WWINEXT.OBJ  WVIEWORG.OBJ WVIEWEXT.OBJ WFILETIT.OBJ &
WTABTEXT.OBJ WBITBLT.OBJ  WUNREGCL.OBJ WGETPOS.OBJ

OBJS7 =      WINBREAK.OBJ WININT24.OBJ

OBJS8 =      WINDATA.OBJ  WDESKTOP.OBJ WMODULE.OBJ


all          :  $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS8)
	echo "Everything compiled"
        del winlibwl.lib
	wlib /p=128 winlibwl.lib @winlibwl.lnk
	echo "Library is built"


WACTIVE.OBJ  :  WACTIVE.C

WADJUST.OBJ  :  WADJUST.C

WANYPOP.OBJ  :  WANYPOP.C

WBITMAP.OBJ  :  WBITMAP.C

WCHECKBX.OBJ :  WCHECKBX.C

WCLIPBRD.OBJ :  WCLIPBRD.C

WCREATE.OBJ  :  WCREATE.C

WDLGDIR.OBJ  :  WDLGDIR.C

WDLGOPEN.OBJ :  WDLGOPEN.C

WDRAWTXT.OBJ :  WDRAWTXT.C

WDRVTYPE.OBJ :  WDRVTYPE.C

WEDITPRO.OBJ :  WEDITPRO.C

WEMALLOC.OBJ :  WEMALLOC.C

WENUMFNT.OBJ :  WENUMFNT.C

WENUMOBJ.OBJ :  WENUMOBJ.C

WEXITWIN.OBJ :  WEXITWIN.C

WEXTTEXT.OBJ :  WEXTTEXT.C

WFILEIO.OBJ  :  WFILEIO.C

WFILETIT.OBJ :  WFILETIT.C

WFILLWIN.OBJ :  WFILLWIN.C

WFINDWIN.OBJ :  WFINDWIN.C

WGDICHAR.OBJ :  WGDICHAR.C

WGDICLIP.OBJ :  WGDICLIP.C

WGDIFONT.OBJ :  WGDIFONT.C

WGDIRECT.OBJ :  WGDIRECT.C

WGDIRGB.OBJ  :  WGDIRGB.C

WGDIRGN.OBJ  :  WGDIRGN.C

WGDISET.OBJ  :  WGDISET.C

WGDISHAP.OBJ :  WGDISHAP.C

WGDISTUB.OBJ :  WGDISTUB.C

WGETCLAS.OBJ :  WGETCLAS.C

WGETPOS.OBJ  :  WGETPOS.C

WGRAYSTR.OBJ :  WGRAYSTR.C

WINACCEL.OBJ :  WINACCEL.C

WINALT.OBJ   :  WINALT.C

WINANSI.OBJ  :  WINANSI.C

WINATOM.OBJ  :  WINATOM.C

WINBLIT.OBJ  :  WINBLIT.C

WINBORDR.OBJ :  WINBORDR.C

WINBREAK.OBJ :  WINBREAK.C

WINCARET.OBJ :  WINCARET.C

WINCATCH.OBJ :  WINCATCH.C

WINCLASS.OBJ :  WINCLASS.C

WINCLICK.OBJ :  WINCLICK.C

WINCOLOR.OBJ :  WINCOLOR.C

WINCOMBO.OBJ :  WINCOMBO.C

WINCREAT.OBJ :  WINCREAT.C

WINDLG.OBJ   :  WINDLG.C

WINDC.OBJ    :  WINDC.C

WINDEBUG.OBJ :  WINDEBUG.C

WINDEFER.OBJ :  WINDEFER.C

WINDOS.OBJ   :  WINDOS.C

WINDRAW.OBJ  :  WINDRAW.C

WINDSTRY.OBJ :  WINDSTRY.C

WINEDIT.OBJ  :  WINEDIT.C

WINENABL.OBJ :  WINENABL.C

WINENUM.OBJ  :  WINENUM.C

WINEVENT.OBJ :  WINEVENT.C

WINEXEC.OBJ  :  WINEXEC.C

WINEXTRA.OBJ :  WINEXTRA.C

WINFLASH.OBJ :  WINFLASH.C

WINHOOK.OBJ  :  WINHOOK.C

WINICON.OBJ  :  WINICON.C

WININIT.OBJ  :  WININIT.C

WININT24.OBJ :  WININT24.C

WINKBD.OBJ   :  WINKBD.C

WINLBDIR.OBJ :  WINLBDIR.C

WINMDI.OBJ   :  WINMDI.C

WINMENU.OBJ  :  WINMENU.C

WINMENU2.OBJ :  WINMENU2.C

WINMODAL.OBJ :  WINMODAL.C

WINMONO.OBJ  :  WINMONO.C

WINMOUSE.OBJ :  WINMOUSE.C

WINMOVE.OBJ  :  WINMOVE.C

WINQUEUE.OBJ :  WINQUEUE.C

WINPOINT.OBJ :  WINPOINT.C

WINPROP.OBJ  :  WINPROP.C

WINRECT.OBJ  :  WINRECT.C

WINRES.OBJ   :  WINRES.C

WINRES2.OBJ  :  WINRES2.C

WINSCROL.OBJ :  WINSCROL.C

WINSET.OBJ   :  WINSET.C

WINSHOW.OBJ  :  WINSHOW.C

WINSIZE.OBJ  :  WINSIZE.C

WINSOUND.OBJ :  WINSOUND.C

WINSPAWN.OBJ :  WINSPAWN.ASM

WINTASK.OBJ  :  WINTASK.C

WINTERM.OBJ  :  WINTERM.C

WINTIME.OBJ  :  WINTIME.C

WINTIMER.OBJ :  WINTIMER.C

WINVID.OBJ   :  WINVID.C

WINVIRT.OBJ  :  WINVIRT.C

WINZOOM.OBJ  :  WINZOOM.C

WISCHAR.OBJ  :  WISCHAR.C

WKBSTACK.OBJ :  WKBSTACK.C

WLIBRARY.OBJ :  WLIBRARY.C

WLINETO.OBJ  :  WLINETO.C

WLISTBOX.OBJ :  WLISTBOX.C

WLNKLIST.OBJ :  WLNKLIST.C

WLOADACC.OBJ :  WLOADACC.C

WLOADBMP.OBJ :  WLOADBMP.C

WLOADDLG.OBJ :  WLOADDLG.C

WLOADMNU.OBJ :  WLOADMNU.C

WLOADSTR.OBJ :  WLOADSTR.C

WLPTODP.OBJ  :  WLPTODP.C

WLSTRING.OBJ :  WLSTRING.C

WMEM.OBJ     :  WMEM.C

WMENUHI.OBJ  :  WMENUHI.C

WMESSAGE.OBJ :  WMESSAGE.C

WMOUAPI.OBJ  :  WMOUAPI.C

WMSGBOX.OBJ  :  WMSGBOX.C

WOBJECT.OBJ  :  WOBJECT.C

WOPENFIL.OBJ :  WOPENFIL.C

WPATBLT.OBJ  :  WPATBLT.C

WPROFILE.OBJ :  WPROFILE.C

WPUSHBUT.OBJ :  WPUSHBUT.C

WREFORM.OBJ  :  WREFORM.C

WRUBBER.OBJ  :  WRUBBER.C

WSAVEDC.OBJ  :  WSAVEDC.C

WSCROLDC.OBJ :  WSCROLDC.C

WSCRLBAR.OBJ :  WSCRLBAR.C

WSELECTR.OBJ :  WSELECTR.C

WSETRECT.OBJ :  WSETRECT.C

WSETWPOS.OBJ :  WSETWPOS.C

WSHADOW.OBJ  :  WSHADOW.C

WSPRINTF.OBJ :  WSPRINTF.C

WSTATIC.OBJ  :  WSTATIC.C

WSTDPROC.OBJ :  WSTDPROC.C

WSTFOCUS.OBJ :  WSTFOCUS.C

WSTRDISP.OBJ :  WSTRDISP.C

WSYSMENU.OBJ :  WSYSMENU.C

WTEXTMET.OBJ :  WTEXTMET.C

WTEXTOUT.OBJ :  WTEXTOUT.C

WUNREGCL.OBJ :  WUNREGCL.C

WVISMAP.OBJ  :  WVISMAP.C

WZORDER.OBJ  :  WZORDER.C

