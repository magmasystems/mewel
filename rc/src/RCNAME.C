#include "int.h"
#include "rccomp.h"

struct RC_NAME {
	char *rc_name;
	int rc_code;
};
struct RC_NAME class_def[] = {
	"COMBO_CLASS",	13,
	"ICON_CLASS",	12,
	"SCROLLBAR_CLASS",	4,
	"USER_CLASS",	20,
	"STATIC_CLASS",	5,
	"PUSHBUTTON_CLASS",	6,
	"EDIT_CLASS",	2,
	"FRAME_CLASS",	10,
	"CHECKBOX_CLASS",	7,
	"BOX_CLASS",	11,
	"TEXT_CLASS",	9,
	"BUTTON_CLASS",	1,
	"LISTBOX_CLASS",	3,
	"RADIOBUTTON_CLASS",	8,
	"NORMAL_CLASS",	0,
};
struct RC_NAME rt_def[] = {
	"RT_BITMAP",	MAKEINTRESOURCE(2),
	"RT_FONTDIR",	MAKEINTRESOURCE(7),
	"RT_DIALOG",	MAKEINTRESOURCE(5),
	"RT_STRING",	MAKEINTRESOURCE(6),
	"RT_CURSOR",	MAKEINTRESOURCE(1),
	"RT_RCDATA",	MAKEINTRESOURCE(10),
	"RT_MENU",	MAKEINTRESOURCE(4),
	"RT_ACCELERATOR",	MAKEINTRESOURCE(9),
	"RT_GROUP_ICON",	MAKEINTRESOURCE(14),
	"RT_FONT",	MAKEINTRESOURCE(8),
	"RT_MSGBOX",	MAKEINTRESOURCE(15),
	"RT_GROUP_CURSOR",	MAKEINTRESOURCE(12),
	"RT_ICON",	MAKEINTRESOURCE(3),
};
struct RC_NAME res_def[] = {
	"RES_PRELOAD",	0x0040,
	"RES_WINCOMPATDLG",	0xFE,
	"RES_MOVEABLE",	0x0010,
	"RES_DISCARDABLE",	0x1000,
};
struct RC_NAME mf_def[] = {
	"MF_STRING",	0x0000,
	"MF_BITMAP",	0x0004,
	"MF_UNCHECKED",	0x0000,
	"MF_CHECKED",	0x0008,
	"MF_HELP",	0x4000,
	"MF_SHADOW",	0x0020,
	"MF_ENABLED",	0x0000,
	"MF_GRAYED",	0x0001,
	"MF_DISABLED",	0x0002,
	"MF_POPUP",	0x0010,
	"MF_MENUBREAK",	0x0020,
	"MF_MENUBARBREAK",0x0020,
	"MF_SEPARATOR",	0x1000,
};

#define	adim(x)	(sizeof(x)/sizeof(x[0]))

/* rc_name - look up name in one of the above tables */

static char *rc_name(rc_table, rc_tablen, rc_code)
struct RC_NAME *rc_table;
int     rc_tablen;
int     rc_code;
{
    register struct RC_NAME *p;

    for (p = rc_table; p < rc_table + rc_tablen; p++)
	if (p->rc_code == rc_code)
	    return (p->rc_name);
    return ("UNKNOWN");
}

/* class_name - look up class name */

char   *class_name(rc_code)
int     rc_code;
{
    return (rc_name(class_def, adim(class_def), rc_code));
}

/* rt_name - look up rt name */

char   *rt_name(rc_code)
int     rc_code;
{
    return (rc_name(rt_def, adim(rt_def), rc_code));
}

/* res_name - look up res name */

char   *res_name(rc_code)
int     rc_code;
{
    return (rc_name(res_def, adim(res_def), rc_code));
}

/* mf_name - look up mf name */

char   *mf_name(rc_code)
int     rc_code;
{
    return (rc_name(mf_def, adim(mf_def), rc_code));
}


