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
