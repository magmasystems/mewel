/* Windows-compatible -I option */

#include "int.h"

/* How to convert directory + file name to path name */

#if defined(MSDOS) || defined(unix)
#define	PATHFMT	"%s/%s"
#elif defined(VAXC)
#define	PATHFMT "%s%s"
#else
"define whatever is appropriate for your own system"
#endif

/* Storage to maintain the list of #include directories */

struct IPATH {
    char   *name;			/* directory name */
    struct IPATH *next;			/* next name, if any */
};

struct IPATH *ipath = 0;		/* search list head */

/* search-install - install directory name in #include search path */

search_install(name)
char   *name;
{
    struct IPATH **pp;

    /* Skip to end of list. */

    for (pp = &ipath; *pp; pp = &((*pp)->next))
	 /* void */ ;

    /* Install directory name at end of list */

    *pp = (struct IPATH *) emalloc(sizeof(struct IPATH));
    (*pp)->name = strsave(name);
    (*pp)->next = 0;
}

/* search_include - locate #include file */

search_include(name, path)
char   *name;				/* input */
char   *path;				/* output */
{
    register struct IPATH *p;

    for (p = ipath; p; p = p->next) {
	sprintf(path, PATHFMT, p->name, name);
	if (access(path, 0) == 0)
	    return (1);
    }
    return (0);
}

