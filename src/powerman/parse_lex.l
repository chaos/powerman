/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

%x lex_incl lex_str

%{
#if HAVE_CONFIG_H
#include "config.h"
#endif
/* N.B. must define YYSTYPE before including parse_tab.h or type will be int. */
#define YYSTYPE char *
extern YYSTYPE yylval;
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hostlist.h"
#include "parse_tab.h"
#include "list.h"
#include "xmalloc.h"
#include "error.h"
#include "parse_util.h"
extern void yyerror();

#define YY_NO_INPUT /* silence compiler warnings about unused input() */
#define MAX_INCLUDE_DEPTH 10
static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static int linenum[MAX_INCLUDE_DEPTH];
static char *filename[MAX_INCLUDE_DEPTH];
static int include_stack_ptr = 0;
static char string_buf[8192];
static char *string_buf_ptr;
static List line_ptrs;

%}

/* Lex Options */
%option nounput

%%

%{
    /* yyin gets initialized in parse.y */
%}

#[^\n]*\n {
    linenum[include_stack_ptr]++;
}

[ \t\r]+ {
    /* Eat up white space */
}

[\n]   {
    linenum[include_stack_ptr]++;
}

([0-9]+)|([0-9]+"."[0-9]*)|("."[0-9]+)  {
    yylval = yytext;
    return TOK_NUMERIC_VAL;
}

\$ {
    return TOK_MATCHPOS;
}

\" {
    string_buf_ptr = string_buf;
    BEGIN(lex_str);
}

<lex_str>{
    \" {
        BEGIN(INITIAL);
        *string_buf_ptr = '\0';
        yylval = xstrdup (string_buf);
        list_append(line_ptrs, yylval);
        return TOK_STRING_VAL;
    }
    "\\a" {
        *string_buf_ptr++ = '\a';
    }
    "\\b" {
        *string_buf_ptr++ = '\b';
    }
    "\\e" {
        *string_buf_ptr++ = '\e';
    }
    "\\f" {
        *string_buf_ptr++ = '\f';
    }
    "\\n" {
        *string_buf_ptr++ = '\n';
    }
    "\\r" {
        *string_buf_ptr++ = '\r';
    }
    "\\t" {
        *string_buf_ptr++ = '\t';
    }
    "\\v" {
        *string_buf_ptr++ = '\v';
    }
    "\n" {
        yyerror();
    }
    \\[0-9][0-9][0-9] {
        *string_buf_ptr++ = strtol(&yytext[1], NULL, 8);
    }
    \\(.|\n) {
        *string_buf_ptr++ = yytext[1];
    }
    [^\\\n\"]+ {
        char *yptr = yytext;

        while ( *yptr )
            *string_buf_ptr++ = *yptr++;
    }
}

listen          return TOK_LISTEN;
tcpwrappers     return TOK_TCP_WRAPPERS;
plug_log_level  return TOK_PLUG_LOG_LEVEL;
timeout         return TOK_DEV_TIMEOUT;
pingperiod      return TOK_PING_PERIOD;
specification   return TOK_SPEC;
expect          return TOK_EXPECT;
setplugstate    return TOK_SETPLUGSTATE;
setresult       return TOK_SETRESULT;
foreachnode     return TOK_FOREACHNODE;
foreachplug     return TOK_FOREACHPLUG;
ifoff           return TOK_IFOFF;
ifon            return TOK_IFON;
send            return TOK_SEND;
delay           return TOK_DELAY;
login           return TOK_LOGIN;
logout          return TOK_LOGOUT;
status          return TOK_STATUS;
status_all      return TOK_STATUS_ALL;
on              return TOK_ON;
on_ranged       return TOK_ON_RANGED;
on_all          return TOK_ON_ALL;
off             return TOK_OFF;
off_ranged      return TOK_OFF_RANGED;
off_all         return TOK_OFF_ALL;
cycle           return TOK_CYCLE;
cycle_ranged    return TOK_CYCLE_RANGED;
cycle_all       return TOK_CYCLE_ALL;
reset           return TOK_RESET;
reset_ranged    return TOK_RESET_RANGED;
reset_all       return TOK_RESET_ALL;
ping            return TOK_PING;
status_temp     return TOK_STATUS_TEMP;
status_temp_all return TOK_STATUS_TEMP_ALL;
status_beacon   return TOK_STATUS_BEACON;
status_beacon_all return TOK_STATUS_BEACON_ALL;
beacon_on       return TOK_BEACON_ON;
beacon_on_ranged return TOK_BEACON_ON_RANGED;
beacon_off      return TOK_BEACON_OFF;
beacon_off_ranged return TOK_BEACON_OFF_RANGED;
device          return TOK_DEVICE;
plug[ \t]+name  return TOK_PLUG_NAME;
node            return TOK_NODE;
yes             return TOK_YES;
no              return TOK_NO;
success         return TOK_SUCCESS;
\{              return TOK_BEGIN;
\}              return TOK_END;
=               return TOK_EQUALS;
script          return TOK_SCRIPT;
alias           return TOK_ALIAS;

include         BEGIN(lex_incl);

<lex_incl>[ \t]* {
    /* eat white space */
}

<lex_incl>[\n] {
    linenum[include_stack_ptr]++;
}

<lex_incl>[^ \t\n]+ {
    /* got include file name */
    int len;

    len = strlen(yytext);
    yytext[len - 1] = '\0';

    if ( include_stack_ptr >= MAX_INCLUDE_DEPTH - 1 )
        err_exit(false, "Includes nested too deeply" );

    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
    linenum[include_stack_ptr] = 1;

    yyin = fopen( yytext + 1, "r" );
    if ( yyin == NULL )
        err_exit(true, "%s", yytext + 1);
    filename[include_stack_ptr] = xstrdup(yytext + 1);

    yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
    BEGIN(INITIAL);
}

<<EOF>> {
    if (include_stack_ptr == 0) {
        yyterminate();
    } else {
        /* do I need an fclose(yyin); here? */
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( include_stack[--include_stack_ptr] );
    }
}

. {
    return TOK_UNRECOGNIZED;
}

%%

/* provide yywrap() to avoid need to link to -lfl */
int
yywrap()
{
    return 1;
}

int
scanner_line(void)
{
    return linenum[include_stack_ptr];
}

void
scanner_line_destroy(char *ptr)
{
  xfree(ptr);
}

char *
scanner_file(void)
{
    return filename[include_stack_ptr];
}

void
scanner_init(char *filename0)
{
    int i;

    for (i = 0; i < MAX_INCLUDE_DEPTH; i++) {
        linenum[i] = 1;
        filename[i] = NULL;
    }
    filename[0] = xstrdup(filename0);
    line_ptrs = list_create((ListDelF)scanner_line_destroy);
}

void
scanner_fini(void)
{
    int i;

    for (i = 0; i < MAX_INCLUDE_DEPTH; i++) {
        if (filename[i] != NULL)
            xfree(filename[i]);
    }

  list_destroy(line_ptrs);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

