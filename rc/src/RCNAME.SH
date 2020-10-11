#!/bin/sh

# rc_name.sh - convert class names from #include file to C tables

awk '
	$2 ~ /_CLASS$/	{ class_def[$2] = $3 }
	$2 ~ /^RT_/	{ rt_def[$2] = $3 }
	$2 ~ /^RES_/	{ res_def[$2] = $3 }
	$2 ~ /^MF_/	{ mf_def[$2] = $3 }

	END {
		print "struct RC_NAME {"
		print "	char *rc_name;"
		print "	int rc_code;"
		print "};"

		print "struct RC_NAME class_def[] = {"
		for (i in class_def)
			print "	\"" i "\",	" class_def[i] ","
		print "};"

                print "struct RC_NAME rt_def[] = {"
                for (i in rt_def)
                        print "	\"" i "\",	" rt_def[i] ","
                print "};" 

                print "struct RC_NAME res_def[] = {"
                for (i in res_def)
                        print "	\"" i "\",	" res_def[i] ","
                print "};" 

                print "struct RC_NAME mf_def[] = {"
                for (i in mf_def)
                        print "	\"" i "\",	" mf_def[i] ","
                print "};" 
	}
' $*

cat <<EOF

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

EOF

