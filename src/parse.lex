/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
 *  
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/
/* Lex Definitions */

%x lex_incl lex_str

%{
/* N.B. must define YYSTYPE before including ioa_tab.h or type will be int. */
#define YYSTYPE char *
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse_tab.h"
#include "powerman.h"
#include "list.h"
#include "wrappers.h"
#include "error.h"
#include "parse_util.h"
extern void yyerror();

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

#[^\n]*\n { linenum[include_stack_ptr]++; }  

[ \t]+ {  /* Eat up white space */ }

[\n]   {  linenum[include_stack_ptr]++; }

([0-9]+)|([0-9]+"."[0-9]*)|("."[0-9]+)  { yylval = yytext; return TOK_NUMERIC_VAL; }

\$  { return TOK_MATCHPOS; }

\"                             string_buf_ptr = string_buf; BEGIN(lex_str);
<lex_str>{ 
    \"                           {   /* end of string */
        int len;

        BEGIN(INITIAL); 
        *string_buf_ptr = '\0';
        len = strlen(string_buf);
        yylval = Malloc(len + 1);
        list_append(line_ptrs, yylval);
        strncpy(yylval, string_buf, len);
        yylval[len] = '\0';
        return TOK_STRING_VAL; 
}
    "\\a"                         *string_buf_ptr++ = '\a';
    "\\b"                         *string_buf_ptr++ = '\b';
    "\\e"                         *string_buf_ptr++ = '\e';
    "\\f"                         *string_buf_ptr++ = '\f';
    "\\n"                         *string_buf_ptr++ = '\n';
    "\\r"                         *string_buf_ptr++ = '\r';
    "\\t"                         *string_buf_ptr++ = '\t';
    "\\v"                         *string_buf_ptr++ = '\v';
    "\n"                          yyerror();
    \\[0-9][0-9][0-9]             *string_buf_ptr++ =  strtol(&yytext[1], NULL, 8);
    \\(.|\n)                      *string_buf_ptr++ = yytext[1];
    [^\\\n\"]+               {
        char *yptr = yytext;
    
        while ( *yptr )
            *string_buf_ptr++ = *yptr++;
    }
}
client[ \t]+listener[ \t]+port    {   return TOK_OLD_PORT;      /* old */ }
begin[ \t]+global                 {   return TOK_B_GLOBAL;      /* old */ }
end[ \t]+global                   {   return TOK_E_GLOBAL;      /* old */ }
begin[ \t]+nodes                  {   return TOK_B_NODES;       /* old */ }
end[ \t]+nodes                    {   return TOK_E_NODES;       /* old */ }

port                              {   return TOK_PORT ; }
tcpwrappers                       {   return TOK_TCP_WRAPPERS; }
timeout                           {   return TOK_DEV_TIMEOUT; }
pingperiod                        {   return TOK_PING_PERIOD; }
specification                     {   return TOK_SPEC; }
expect                            {   return TOK_EXPECT; }
setplugstate                      {   return TOK_SETPLUGSTATE; }
foreachnode                       {   return TOK_FOREACHNODE; }
foreachplug                       {   return TOK_FOREACHPLUG; }
ifoff                             {   return TOK_IFOFF; }
ifon                              {   return TOK_IFON; }
send                              {   return TOK_SEND; }
delay                             {   return TOK_DELAY; }
login                             {   return TOK_LOGIN; }
logout                            {   return TOK_LOGOUT; }
status                            {   return TOK_STATUS; }
status_all                        {   return TOK_STATUS_ALL; }
status_soft                       {   return TOK_STATUS_SOFT; }
status_soft_all                   {   return TOK_STATUS_SOFT_ALL; }
on                                {   return TOK_ON; }
on_all                            {   return TOK_ON_ALL; }
off                               {   return TOK_OFF; }
off_all                           {   return TOK_OFF_ALL; }
cycle                             {   return TOK_CYCLE; }
cycle_all                         {   return TOK_CYCLE_ALL; }
reset                             {   return TOK_RESET; }
reset_all                         {   return TOK_RESET_ALL; }
ping                              {   return TOK_PING; }
status_temp                       {   return TOK_STATUS_TEMP; }
status_temp_all                   {   return TOK_STATUS_TEMP_ALL; }
status_beacon                     {   return TOK_STATUS_BEACON; }
status_beacon_all                 {   return TOK_STATUS_BEACON_ALL; }
beacon_on                         {   return TOK_BEACON_ON; }
beacon_off                        {   return TOK_BEACON_OFF; }
device                            {   return TOK_DEVICE; }
plug[ \t]+name                    {   return TOK_PLUG_NAME; }
node                              {   return TOK_NODE; }
yes                               {   return TOK_YES; }
no                                {   return TOK_NO; }
\{                                {   return TOK_BEGIN; }
\}                                {   return TOK_END; }
=                                 {   return TOK_EQUALS; }
script                            {   return TOK_SCRIPT; }
alias                             {   return TOK_ALIAS; }
include                        BEGIN(lex_incl);
<lex_incl>[ \t]*             {    /* eat white space */ }
<lex_incl>[\n]               { linenum[include_stack_ptr]++; }
<lex_incl>[^ \t\n]+                { /* got include file name */
    int len;
    
    len = strlen(yytext);
    yytext[len - 1] = '\0';
    
    if ( include_stack_ptr >= MAX_INCLUDE_DEPTH - 1 )
        err_exit(FALSE, "Includes nested too deeply" );
    
    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
    
    yyin = fopen( yytext + 1, "r" );
    if ( yyin == NULL )
        err_exit(TRUE, "%s", yytext + 1);
    filename[include_stack_ptr] = Strdup(yytext + 1);
    
    yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
    BEGIN(INITIAL);
}
<<EOF>>                        {
    if (include_stack_ptr == 0) {
        yyterminate();
    } else {
        /* do I need an fclose(yyin); here? */
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( include_stack[--include_stack_ptr] );
    }
}
.                              {   
/* perhaps I should replace this with a call to yyerror() */
    return TOK_UNRECOGNIZED; 
}

%%

int
scanner_line(void)
{
    return linenum[include_stack_ptr];
}

void
scanner_line_destroy(char *ptr)
{
  Free(ptr);
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
    filename[0] = Strdup(filename0);
    line_ptrs = list_create((ListDelF)scanner_line_destroy);
}

void
scanner_fini(void)
{
    int i;

    for (i = 0; i < MAX_INCLUDE_DEPTH; i++) {
        if (filename[i] != NULL)
            Free(filename[i]);
    }

  list_destroy(line_ptrs);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

