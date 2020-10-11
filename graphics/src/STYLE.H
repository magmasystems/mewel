/***************************************************************************/
/*                                                                         */
/*     STYLE.H - defines which you can use for .RC files                   */
/*                                                                         */
/***************************************************************************/
#define MEWEL            1

#ifdef RC_INVOKED

/* Basic window types */
#define WS_OVERLAPPED	    0x00000000L
#define WS_POPUP	    0x80000000L
#define WS_CHILD	    0x40000000L

/* Clipping styles */
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_CLIP             WS_CLIPCHILDREN | WS_CLIPSIBLINGS

/* Generic window states */
#define WS_VISIBLE	    0x10000000L
#define WS_DISABLED	    0x08000000L

/* Main window states */
#define WS_MINIMIZE	    0x20000000L
#define WS_MAXIMIZE	    0x01000000L

/* Main window styles */
#define WS_CAPTION	    0x00C00000L     /* WS_BORDER | WS_DLGFRAME	*/
#define WS_BORDER	    0x00800000L
#define WS_DLGFRAME	    0x00400000L
#define WS_VSCROLL	    0x00200000L
#define WS_HSCROLL	    0x00100000L
#define WS_SYSMENU	    0x00080000L
#define WS_THICKFRAME	    0x00040000L
#define WS_MINIMIZEBOX	    0x00020000L
#define WS_MAXIMIZEBOX	    0x00010000L

/* Control window styles */
#define WS_GROUP	    0x00020000L
#define WS_TABSTOP	    0x00010000L

/* MEWEL specific styles */
#define WS_SYSCOLOR         0x00002000L
#define WS_SHADOW           0x00004000L
#define WS_HIDDEN           0x00008000L

/* Common Window Styles */
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW	    (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW	    (WS_CHILD)

/* Extended Window Styles */
#define WS_EX_DLGMODALFRAME  0x00000001L
#define WS_EX_NOPARENTNOTIFY 0x00000004L

#define WS_EX_TOPMOST	     0x00000008L
#define WS_EX_ACCEPTFILES    0x00000010L
#define WS_EX_TRANSPARENT    0x00000020L

/* Obsolete style names */
#define WS_TILED	    WS_OVERLAPPED
#define WS_ICONIC	    WS_MINIMIZE
#define WS_SIZEBOX	    WS_THICKFRAME
#define WS_TILEDWINDOW	    WS_OVERLAPPEDWINDOW

/* Static Control Styles */
/* MEWEL-specific */
#define SS_TEXT             0x00000200L
#define SS_FRAME            0x00000400L
#define SS_BOX              0x00000800L

#define SS_LEFT 	    0x00000000L
#define SS_CENTER	    0x00000001L
#define SS_RIGHT	    0x00000002L
#define SS_ICON 	    0x00000003L
#define SS_BLACKRECT	    0x00000004L | SS_BOX
#define SS_GRAYRECT	    0x00000005L | SS_BOX
#define SS_WHITERECT	    0x00000006L | SS_BOX
#define SS_BLACKFRAME	    0x00000007L | SS_FRAME
#define SS_GRAYFRAME	    0x00000008L | SS_FRAME
#define SS_WHITEFRAME	    0x00000009L | SS_FRAME
#define SS_SIMPLE	    0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_NOPREFIX         0x00000080L


/* Button Control Styles */
#define BS_PUSHBUTTON	    0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX	    0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON	    0x00000004L
#define BS_3STATE	    0x00000005L
#define BS_AUTO3STATE	    0x00000006L
#define BS_GROUPBOX	    0x00000007L
#define BS_USERBUTTON	    0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW	    0x0000000BL
#define BS_LEFTTEXT	    0x00000020L

/* MEWEL-specific */
#define BS_INVERTED         0x00000080L
#define BS_HELP             0x00000040L

/* Edit control styles */
#define ES_LEFT 	    0x00000000L
#define ES_CENTER	    0x00000001L
#define ES_RIGHT	    0x00000002L
#define ES_MULTILINE	    0x00000004L
#define ES_UPPERCASE	    0x00000008L
#define ES_LOWERCASE	    0x00000010L
#define ES_PASSWORD         0x00000020L
#define ES_AUTOVSCROLL	    0x00000040L
#define ES_AUTOHSCROLL	    0x00000080L
#define ES_NOHIDESEL	    0x00000100L
#define ES_OEMCONVERT	    0x00000400L
#define ES_READONLY	    0x00000800L
#define ES_WANTRETURN       0x00001000L

/* Scroll bar styles */
#define SBS_HORZ		    0x0000L
#define SBS_VERT		    0x0001L
#define SBS_TOPALIGN		    0x0002L
#define SBS_LEFTALIGN		    0x0002L
#define SBS_BOTTOMALIGN		    0x0004L
#define SBS_RIGHTALIGN		    0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN	    0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX		    0x0008L
/* MEWEL-specific */
#define SBS_CTL                     0x0010L

/* Listbox styles */
#define LBS_NOTIFY	      0x0001L
#define LBS_SORT	      0x0002L
#define LBS_NOREDRAW	      0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL	      0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_STANDARD	      (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Combo box styles */
#define CBS_SIMPLE	      0x0001L
#define CBS_DROPDOWN	      0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L

/* Dialog styles */
#define DS_ABSALIGN	    0x01L
#define DS_SYSMODAL	    0x02L
#define DS_LOCALEDIT        0x20L
#define DS_SETFONT          0x40L
#define DS_MODALFRAME       0x80L
#define DS_NOIDLEMSG        0x100L




#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define YELLOW          6
#define WHITE           7
#define INTENSE_BLACK   8
#define INTENSE_BLUE    9
#define INTENSE_GREEN   10
#define INTENSE_CYAN    11
#define INTENSE_RED     12
#define INTENSE_MAGENTA 13
#define INTENSE_YELLOW  14
#define INTENSE_WHITE   15


#define IDOK              1
#define IDCANCEL          2
#define IDABORT		  3
#define IDRETRY		  4
#define IDIGNORE	  5
#define IDYES		  6
#define IDNO		  7
#define IDHELP            8



#define VK_HOME            0x4700
#define VK_SH_HOME         0x4703
#define VK_CTRL_HOME       0x7704
#define VK_ALT_HOME        0x9708

#define VK_END             0x4f00
#define VK_SH_END          0x4f03
#define VK_CTRL_END        0x7504
#define VK_ALT_END         0x9f08

#define VK_PGUP            0x4900
#define VK_SH_PGUP         0x4903
#define VK_CTRL_PGUP       0x8404
#define VK_ALT_PGUP        0x9908

#define VK_PGDN            0x5100
#define VK_SH_PGDN         0x5103
#define VK_CTRL_PGDN       0x7604
#define VK_ALT_PGDN        0xa108

#define VK_LEFT            0x4B00
#define VK_SH_LEFT         0x4B03
#define VK_CTRL_LEFT       0x7304
#define VK_ALT_LEFT        0x9b08

#define VK_RIGHT           0x4d00
#define VK_SH_RIGHT        0x4d03
#define VK_CTRL_RIGHT      0x7404
#define VK_ALT_RIGHT       0x9d08

#define VK_UP              0x4800
#define VK_SH_UP           0x4803
#define VK_CTRL_UP         0x8d04
#define VK_ALT_UP          0x9808

#define VK_DOWN            0x5000
#define VK_SH_DOWN         0x5003
#define VK_CTRL_DOWN       0x9104
#define VK_ALT_DOWN        0xa008

#define VK_INS             0x5200
#define VK_INSERT          0x5200
#define VK_SH_INS          0x5203
#define VK_CTRL_INS        0x9204
#define VK_ALT_INS         0xa208

#define VK_DEL             0x5300
#define VK_DELETE          0x5300
#define VK_SH_DEL          0x5303
#define VK_CTRL_DEL        0x9304
#define VK_ALT_DEL         0xa308

#define VK_FIVE            0x4c00
#define VK_SH_FIVE         0x4c03
#define VK_CTRL_FIVE       0x8f04
#define VK_ALT_FIVE        0x4c08

#define VK_BACKSPACE       8
#define VK_SH_BACKSPACE    0x0e03
#define VK_CTRL_BACKSPACE  127
#define VK_ALT_BACKSPACE   0x0e08

#define VK_TAB             0x0009
#define VK_SH_TAB          0x0f03
#define VK_CTRL_TAB        0x9404
#define VK_ALT_TAB         0xa508

#define VK_ENTER           '\r'
#define VK_SH_ENTER        '\r'
#define VK_CTRL_ENTER      '\n'
#define VK_ALT_ENTER       0xa608

#define VK_GREYPLUS        0x4e00
#define VK_SH_GREYPLUS     0x4e03
#define VK_CTRL_GREYPLUS   0x9004
#define VK_ALT_GREYPLUS    0x4e08

#define VK_GREYMINUS       0x4a00
#define VK_SH_GREYMINUS    0x4a03
#define VK_CTRL_GREYMINUS  0x8e04
#define VK_ALT_GREYMINUS   0x4a08

#define VK_GREYSTAR        0x3700
#define VK_SH_GREYSTAR     0x3703
#define VK_CTRL_GREYSTAR   0x9604
#define VK_ALT_GREYSTAR    0x3708

#define VK_GREYDIVIDE      '/'
#define VK_SH_GREYDIVIDE   '/'
#define VK_CTRL_GREYDIVIDE 0x9504
#define VK_ALT_GREYDIVIDE  0xa408

#define VK_ALT_1           0x7808
#define VK_ALT_2           0x7908
#define VK_ALT_3           0x7a08
#define VK_ALT_4           0x7b08
#define VK_ALT_5           0x7c08
#define VK_ALT_6           0x7d08
#define VK_ALT_7           0x7e08
#define VK_ALT_8           0x7f08
#define VK_ALT_9           0x8008
#define VK_ALT_0           0x8108
#define VK_ALT_MINUS       0x8208
#define VK_ALT_PLUS        0x8308

#define VK_ALT_A           0x1e08
#define VK_ALT_B           0x3008
#define VK_ALT_C           0x2e08
#define VK_ALT_D           0x2008
#define VK_ALT_E           0x1208
#define VK_ALT_F           0x2108
#define VK_ALT_G           0x2208
#define VK_ALT_H           0x2308
#define VK_ALT_I           0x1708
#define VK_ALT_J           0x2408
#define VK_ALT_K           0x2508
#define VK_ALT_L           0x2608
#define VK_ALT_M           0x3208
#define VK_ALT_N           0x3108
#define VK_ALT_O           0x1808
#define VK_ALT_P           0x1908
#define VK_ALT_Q           0x1008
#define VK_ALT_R           0x1308
#define VK_ALT_S           0x1f08
#define VK_ALT_T           0x1408
#define VK_ALT_U           0x1608
#define VK_ALT_V           0x2f08
#define VK_ALT_W           0x1108
#define VK_ALT_X           0x2d08
#define VK_ALT_Y           0x1508
#define VK_ALT_Z           0x2c08

#define VK_F1              0x3b00
#define VK_F2              0x3c00
#define VK_F3              0x3d00
#define VK_F4              0x3e00
#define VK_F5              0x3f00
#define VK_F6              0x4000
#define VK_F7              0x4100
#define VK_F8              0x4200
#define VK_F9              0x4300
#define VK_F10             0x4400
#define VK_F11             0x8500
#define VK_F12             0x8600
#define VK_SH_F1           0x5403
#define VK_SH_F2           0x5503
#define VK_SH_F3           0x5603
#define VK_SH_F4           0x5703
#define VK_SH_F5           0x5803
#define VK_SH_F6           0x5903
#define VK_SH_F7           0x5a03
#define VK_SH_F8           0x5b03
#define VK_SH_F9           0x5c03
#define VK_SH_F10          0x5d03
#define VK_SH_F11          0x8703
#define VK_SH_F12          0x8803
#define VK_CTRL_F1         0x5e04
#define VK_CTRL_F2         0x5f04
#define VK_CTRL_F3         0x6004
#define VK_CTRL_F4         0x6104
#define VK_CTRL_F5         0x6204
#define VK_CTRL_F6         0x6304
#define VK_CTRL_F7         0x6404
#define VK_CTRL_F8         0x6504
#define VK_CTRL_F9         0x6604
#define VK_CTRL_F10        0x6704
#define VK_CTRL_F11        0x8904
#define VK_CTRL_F12        0x8a04
#define VK_ALT_F1          0x6808
#define VK_ALT_F2          0x6908
#define VK_ALT_F3          0x6a08
#define VK_ALT_F4          0x6b08
#define VK_ALT_F5          0x6c08
#define VK_ALT_F6          0x6d08
#define VK_ALT_F7          0x6e08
#define VK_ALT_F8          0x6f08
#define VK_ALT_F9          0x7008
#define VK_ALT_F10         0x7108
#define VK_ALT_F11         0x8b08
#define VK_ALT_F12         0x8c08

#define  VK_ESC           27

/**************************** CONTROL KEYS < ' ' *****************************/
#define  VK_CTRL_A        1
#define  VK_CTRL_B        2
#define  VK_CTRL_C        3
#define  VK_CTRL_D        4
#define  VK_CTRL_E        5
#define  VK_CTRL_F        6
#define  VK_CTRL_G        7
#define  VK_CTRL_H        8
#define  VK_CTRL_I        9
#define  VK_CTRL_J        10
#define  VK_CTRL_K        11
#define  VK_CTRL_L        12
#define  VK_CTRL_M        13
#define  VK_CTRL_N        14
#define  VK_CTRL_O        15
#define  VK_CTRL_P        16
#define  VK_CTRL_Q        17
#define  VK_CTRL_R        18
#define  VK_CTRL_S        19
#define  VK_CTRL_T        20
#define  VK_CTRL_U        21
#define  VK_CTRL_V        22
#define  VK_CTRL_W        23
#define  VK_CTRL_X        24
#define  VK_CTRL_Y        25
#define  VK_CTRL_Z        26

/*
  For MS Windows compatibility
*/
#define VK_ADD            '+'
#define VK_DIVIDE         '/'
#define VK_MULTIPLY       '*'
#define VK_SUBTRACT	  '-'
#define VK_PRIOR          VK_PGUP
#define VK_NEXT           VK_PGDN
#define VK_BACK           VK_BACKSPACE
#define VK_ESCAPE         VK_ESC
#define VK_BACKTAB        VK_SH_TAB
#define VK_RETURN         '\r'
#define VK_NEWLINE        '\n'
#define VK_NUMLOCK        0x2020
#define VK_SCROLL         0x1010
#define VK_CAPITAL        0x4040

#endif /* RC_INVOKED */

