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
#include "buffer.h"
#include "list.h"
#include "wrappers.h"
#include "error.h"
#include "config.h"
extern void yyerror();

#define MAX_INCLUDE_DEPTH 10
static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static int linenum[MAX_INCLUDE_DEPTH];
static char *filename[MAX_INCLUDE_DEPTH];
static int include_stack_ptr = 0;
static char string_buf[MAX_BUF];
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
/* Below are a bazillion codes you may encounter and want to handle. */
/* Includeing a bunch of the telnet protocol and its options.        */
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
    "\\xEOF"                      *string_buf_ptr++ = 236;
    "\\SUSP"                      *string_buf_ptr++ = 237;
    "\\ABORT"                     *string_buf_ptr++ = 238;
    "\\EOR"                       *string_buf_ptr++ = 239;
    "\\SE"                        *string_buf_ptr++ = 240;
    "\\NOP"                       *string_buf_ptr++ = 241;
    "\\DM"                        *string_buf_ptr++ = 242;
    "\\BRK"                       *string_buf_ptr++ = 243;
    "\\IP"                        *string_buf_ptr++ = 244;
    "\\AO"                        *string_buf_ptr++ = 245;
    "\\AYT"                       *string_buf_ptr++ = 246;
    "\\EC"                        *string_buf_ptr++ = 247;
    "\\EL"                        *string_buf_ptr++ = 248;
    "\\GA"                        *string_buf_ptr++ = 249;
    "\\SB"                        *string_buf_ptr++ = 250;
    "\\WILL"                      *string_buf_ptr++ = 251;
    "\\WONT"                      *string_buf_ptr++ = 252;
    "\\DO"                        *string_buf_ptr++ = 253;
    "\\DONT"                      *string_buf_ptr++ = 254;
    "\\IAC"                       *string_buf_ptr++ = 255;
    "\\TELOPT_BINARY"             *string_buf_ptr++ =   0;
    "\\TELOPT_ECHO"               *string_buf_ptr++ =   1;
    "\\TELOPT_RCP"                *string_buf_ptr++ =   2;
    "\\TELOPT_SGA"                *string_buf_ptr++ =   3;
    "\\TELOPT_NAMS"               *string_buf_ptr++ =   4;
    "\\TELOPT_STATUS"             *string_buf_ptr++ =   5;
    "\\TELOPT_TM"                 *string_buf_ptr++ =   6;
    "\\TELOPT_RCTE"               *string_buf_ptr++ =   7;
    "\\TELOPT_NAOL"               *string_buf_ptr++ =   8;
    "\\TELOPT_NAOP"               *string_buf_ptr++ =   9;
    "\\TELOPT_NAOCRD"             *string_buf_ptr++ =  10;
    "\\TELOPT_NAOHTS"             *string_buf_ptr++ =  11;
    "\\TELOPT_NAOHTD"             *string_buf_ptr++ =  12;
    "\\TELOPT_NAOFFD"             *string_buf_ptr++ =  13;
    "\\TELOPT_NAOVTS"             *string_buf_ptr++ =  14;
    "\\TELOPT_NAOVTD"             *string_buf_ptr++ =  15;
    "\\TELOPT_NAOLFD"             *string_buf_ptr++ =  16;
    "\\TELOPT_XASCII"             *string_buf_ptr++ =  17;
    "\\TELOPT_LOGOUT"             *string_buf_ptr++ =  18;
    "\\TELOPT_BM"                 *string_buf_ptr++ =  19;
    "\\TELOPT_DET"                *string_buf_ptr++ =  20;
    "\\TELOPT_SUPDUP"             *string_buf_ptr++ =  21;
    "\\TELOPT_SUPDUPOUTPUT"       *string_buf_ptr++ =  22;
    "\\TELOPT_SNDLOC"             *string_buf_ptr++ =  23;
    "\\TELOPT_TTYPE"              *string_buf_ptr++ =  24;
    "\\TELOPT_EOR"                *string_buf_ptr++ =  25;
    "\\TELOPT_TUID"               *string_buf_ptr++ =  26;
    "\\TELOPT_OUTMRK"             *string_buf_ptr++ =  27;
    "\\TELOPT_TTYLOC"             *string_buf_ptr++ =  28;
    "\\TELOPT_3270REGIME"         *string_buf_ptr++ =  29;
    "\\TELOPT_X3PAD"              *string_buf_ptr++ =  30;
    "\\TELOPT_NAWS"               *string_buf_ptr++ =  31;
    "\\TELOPT_TSPEED"             *string_buf_ptr++ =  32;
    "\\TELOPT_LFLOW"              *string_buf_ptr++ =  33;
    "\\TELOPT_LINEMODE"           *string_buf_ptr++ =  34;
    "\\TELOPT_XDISPLOC"           *string_buf_ptr++ =  35;
    "\\TELOPT_OLD_ENVIRON"        *string_buf_ptr++ =  36;
    "\\TELOPT_AUTHENTICATION"     *string_buf_ptr++ =  37;
    "\\TELOPT_ENCRYPT"            *string_buf_ptr++ =  38;
    "\\TELOPT_NEW_ENVIRON"        *string_buf_ptr++ =  39;
    \\[0-9][0-9][0-9]             *string_buf_ptr++ =  strtol(&yytext[1], NULL, 8);
    \\(.|\n)                      *string_buf_ptr++ = yytext[1];
    [^\\\n\"]+               {
        char *yptr = yytext;
    
        while ( *yptr )
            *string_buf_ptr++ = *yptr++;
    }
}
begin[ \t]+global                 {   return TOK_B_GLOBAL; }
tcpwrappers                       {   return TOK_TCP_WRAPPERS; }
device[ \t]+timeout               {   return TOK_DEV_TIMEOUT; }
ping[ \t]+period                  {   return TOK_PING_PERIOD; }
client[ \t]+listener[ \t]+port    {   return TOK_CLIENT_PORT; }
begin[ \t]+protocol[ \t]+specification {   return TOK_B_SPEC; }
specification[ \t]+name           {   return TOK_SPEC_NAME; }
specification[ \t]+type           {   return TOK_SPEC_TYPE; }
off[ \t]+string                   {   return TOK_OFF_STRING; }
on[ \t]+string                    {   return TOK_ON_STRING; }
all[ \t]+string                   {   return TOK_ALL_STRING; }
plug[ \t]+count                   {   return TOK_PLUG_COUNT; }
expect                            {   return TOK_EXPECT; }
map                               {   return TOK_MAP; }
send                              {   return TOK_SEND; }
delay                             {   return TOK_DELAY; }
begin[ \t]+LOGIN_SCRIPT           {   return TOK_B_LOGIN; }
end[ \t]+LOGIN_SCRIPT             {   return TOK_E_LOGIN; }
begin[ \t]+LOGOUT_SCRIPT          {   return TOK_B_LOGOUT; }
end[ \t]+LOGOUT_SCRIPT            {   return TOK_E_LOGOUT; }
begin[ \t]+STATUS_SCRIPT          {   return TOK_B_STATUS; }
end[ \t]+STATUS_SCRIPT            {   return TOK_E_STATUS; }
begin[ \t]+STATUS_SOFT_SCRIPT     {   return TOK_B_STATUS_SOFT; }
end[ \t]+STATUS_SOFT_SCRIPT       {   return TOK_E_STATUS_SOFT; }
begin[ \t]+ON_SCRIPT              {   return TOK_B_ON; }
end[ \t]+ON_SCRIPT                {   return TOK_E_ON; }
begin[ \t]+ON_ALL_SCRIPT          {   return TOK_B_ON_ALL; }
end[ \t]+ON_ALL_SCRIPT            {   return TOK_E_ON_ALL; }
begin[ \t]+OFF_SCRIPT             {   return TOK_B_OFF; }
end[ \t]+OFF_SCRIPT               {   return TOK_E_OFF; }
begin[ \t]+OFF_ALL_SCRIPT         {   return TOK_B_OFF_ALL; }
end[ \t]+OFF_ALL_SCRIPT           {   return TOK_E_OFF_ALL; }
begin[ \t]+CYCLE_SCRIPT           {   return TOK_B_CYCLE; }
end[ \t]+CYCLE_SCRIPT             {   return TOK_E_CYCLE; }
begin[ \t]+CYCLE_ALL_SCRIPT       {   return TOK_B_CYCLE_ALL; }
end[ \t]+CYCLE_ALL_SCRIPT         {   return TOK_E_CYCLE_ALL; }
begin[ \t]+RESET_SCRIPT           {   return TOK_B_RESET; }
end[ \t]+RESET_SCRIPT             {   return TOK_E_RESET; }
begin[ \t]+RESET_ALL_SCRIPT       {   return TOK_B_RESET_ALL; }
end[ \t]+RESET_ALL_SCRIPT         {   return TOK_E_RESET_ALL; }
begin[ \t]+PING_SCRIPT            {   return TOK_B_PING; }
end[ \t]+PING_SCRIPT              {   return TOK_E_PING; }
begin[ \t]+STATUS_TEMP_SCRIPT     {   return TOK_B_STATUS_TEMP; }
end[ \t]+STATUS_TEMP_SCRIPT       {   return TOK_E_STATUS_TEMP; }
begin[ \t]+STATUS_BEACON_SCRIPT   {   return TOK_B_STATUS_BEACON; }
end[ \t]+STATUS_BEACON_SCRIPT     {   return TOK_E_STATUS_BEACON; }
begin[ \t]+BEACON_ON_SCRIPT       {   return TOK_B_BEACON_ON; }
end[ \t]+BEACON_ON_SCRIPT         {   return TOK_E_BEACON_ON; }
begin[ \t]+BEACON_OFF_SCRIPT      {   return TOK_B_BEACON_OFF; }
end[ \t]+BEACON_OFF_SCRIPT        {   return TOK_E_BEACON_OFF; }
end[ \t]+protocol[ \t]+specification   {   return TOK_E_SPEC; }
device                            {   return TOK_DEVICE; }
end[ \t]+global                   {   return TOK_E_GLOBAL; }
begin[ \t]+nodes                  {   return TOK_B_NODES; }
plug[ \t]+name                    {   return TOK_PLUG_NAME; }
node                              {   return TOK_NODE; }
end[ \t]+nodes                    {   return TOK_E_NODES; }
include                        BEGIN(lex_incl);
<lex_incl>[ \t]*             {    /* eat white space */ }
<lex_incl>[\n]               { linenum[include_stack_ptr]++; }
<lex_incl>[^ \t\n]+                { /* got include file name */
    int len;
    
    len = strlen(yytext);
    yytext[len - 1] = '\0';
    
    if ( include_stack_ptr >= MAX_INCLUDE_DEPTH )
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
    if ( --include_stack_ptr < 0 ) {
        yyterminate();
    } else {
        /* do I need an fclose(yyin); here? */
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( include_stack[include_stack_ptr] );
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

