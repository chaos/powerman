#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "argv.h"

int
main(int argc, char *argv[])
{
	char **av;

	av = argv_create("foo bar baz", "");
	assert(argv_length(av) == 3);
	av = argv_append(av, "bonk");
	assert(argv_length(av) == 4);
	assert(strcmp(av[0], "foo") == 0);
	assert(strcmp(av[1], "bar") == 0);
	assert(strcmp(av[2], "baz") == 0);
	assert(strcmp(av[3], "bonk") == 0);
	assert(av[4] == NULL);
	argv_destroy(av);

	av = argv_create("a,b:c d", ",:");
	assert(argv_length(av) == 4);
	assert(strcmp(av[0], "a") == 0);
	assert(strcmp(av[1], "b") == 0);
	assert(strcmp(av[2], "c") == 0);
	assert(strcmp(av[3], "d") == 0);
	assert(av[4] == NULL);
	argv_destroy(av);

	exit(0);
}
