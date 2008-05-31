/*
 * Test driver for pluglist module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include "list.h"
#include "pluglist.h"
#include "hostlist.h"
#include "error.h"
#include "xmalloc.h"

void usage(void)
{
	fprintf(stderr, "Usage: tpl [-f plug_to_find] [-p fixed_plugs] nodelist [pluglist]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	extern int optind;
	extern char *optarg;
	int c;
	PlugList pl = NULL;
	char *hwplugs = NULL;
	char *nodelist = NULL; 
	char *pluglist = NULL;
	char *findplug = NULL;

	err_init(basename(argv[0]));

	while ((c = getopt(argc, argv, "p:f:")) != EOF) {
		switch (c) {
		case 'p':
			hwplugs = optarg;
			break;
		case 'f':
			findplug = optarg;
			break;
		default:
			usage();
		}
	}
	if (argc - optind == 0)
		usage();
	nodelist = argv[optind++];
	if (argc - optind == 1)
		pluglist = argv[optind++];
	if (argc - optind != 0)
		usage();

	if (hwplugs) {
		hostlist_t hl = hostlist_create(hwplugs);
		hostlist_iterator_t itr = hostlist_iterator_create(hl);
		List l = list_create((ListDelF)xfree);
		char *plug;

		while ((plug = hostlist_next(itr)))
			list_append(l, xstrdup(plug));

		hostlist_iterator_destroy(itr);
		hostlist_destroy(hl);
		pl = pluglist_create(l);
		list_destroy(l);
	} else 
		pl = pluglist_create(NULL);

	switch (pluglist_map(pl, nodelist, pluglist)) {
		case EPL_DUPNODE: 
			fprintf(stderr, "duplicate node\n");
			break;
		case EPL_UNKPLUG: 
			fprintf(stderr, "unknown plug\n");
			break;
		case EPL_DUPPLUG:
			fprintf(stderr, "duplicate plug\n");
			break;
		case EPL_NOPLUGS:
			fprintf(stderr, "more nodes than plugs\n");
			break;
		case EPL_NONODES:
			fprintf(stderr, "more plugs than nodes\n");
			break;
		case EPL_SUCCESS: 
			break;
	}

	if (findplug) {
		Plug *plug = pluglist_find(pl, findplug);

		if (plug)
			printf("plug=%s node=%s\n", plug->name, 
					plug->node ? plug->node : "NULL");
		else
			printf("plug %s: not found\n", findplug);
	} else {
		PlugListIterator itr = pluglist_iterator_create(pl);
		Plug *plug;
		
		while ((plug = pluglist_next(itr))) {
			printf("plug=%s node=%s\n", plug->name, 
					plug->node ? plug->node : "NULL");
		}
		pluglist_iterator_destroy(itr);
	}

	exit(0);
}
