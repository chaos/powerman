/*
 * $Id$
 *
 * When we autoconfiscate powerman this file will be auto-generated.
 */

#include "error.h"

/* for list.c, cbuf.c, and wrappers */
#define WITH_LSD_FATAL_ERROR_FUNC 1
#define WITH_LSD_NOMEM_ERROR_FUNC 1

/* for hostlist.c */
#define _err(fmt, args...)      err(FALSE, fmt, ## args)
