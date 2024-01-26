/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_ARGV_H
#define PM_ARGV_H

/* Create a NULL-terminated argv array suitable for passing to execv()
 * from 'cmdline' string.  Characters in the 'ignore' set are treated as white
 * space.  Caller must free with argv_destroy().
 */
char **argv_create(char *cmdline, char *ignore);

/* Destroy an argv array created by argv_create.
 */
void argv_destroy(char **argv);

/* Return the number of elements in the argv array (less the NULL terminator).
 */
int argv_length(char **argv);

/* Expand an argv array by one slot and add an entry.
 */
char **argv_append(char **argv, char *s);


#endif /* PM_ARGV_H */
