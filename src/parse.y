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

%{

#define YYSTYPE char *  /*  The generic type returned by all parse matches */
#undef YYDEBUG          /* no debug code plese */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>       /* for HUGE_VAL and trunc */
#include <assert.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "client.h"
#include "error.h"
#include "string.h"
#include "wrappers.h"

/* prototypes for parse handler routines */
static char *makeNode(char *s2, char *s3, char *s4);
static char *makeAlias(char *s2, char *s3);
static char *makeDevice(char *s2, char *s3, char *s4, char *s5);
static char *makePlugNameLine(char *s2);

static char *makeScriptSec(char *s2, int com);

static Interp *makeMapLine(char *s2, char *s3);
static List makeMapSecHead(Interp *s1);
static List makeMapSec(List s1, Interp *s2);
static char *makeStmt(StmtType mode, char *s2, List s3);

static char *makeSpecSize(char *s2);
static char *makeSpecOn(char *s2);
static char *makeSpecOff(char *s2);
static char *makeSpecType(char *s2);
static char *makeSpecName(char *s2);
static Spec *check_Spec();
static char *makeClientPort(char *s2);
static char *makeDevTimeout(char *s2);
static char *makePingPeriod(char *s2);
static char *makeTCPWrappers(bool val);

extern int yylex();
void yyerror();
static void _errormsg(char *msg);
static void _warnmsg(char *msg);
static long _strtolong(char *str);
static double _strtodouble(char *str);
static void _doubletotv(struct timeval *tv, double val);

 Spec *current_spec = NULL;  /* Holds a Spec as it is built */
 Script current_script = NULL; /* Holds a script as it is built */
%}

/* BISON Declatrations */

%token TOK_TCP_WRAPPERS
%token TOK_PORT

%token TOK_SPEC_NAME
%token TOK_SPEC_TYPE
%token TOK_OFF_STRING
%token TOK_ON_STRING
%token TOK_PLUG_COUNT
%token TOK_TIMEOUT
%token TOK_DEV_TIMEOUT
%token TOK_PING_PERIOD

%token TOK_EXPECT
%token TOK_MAP
%token TOK_MATCHPOS
%token TOK_SEND
%token TOK_DELAY

%token TOK_SCRIPT
%token TOK_LOGIN
%token TOK_LOGOUT
%token TOK_STATUS
%token TOK_STATUS_ALL
%token TOK_STATUS_SOFT
%token TOK_STATUS_SOFT_ALL
%token TOK_STATUS_TEMP
%token TOK_STATUS_TEMP_ALL
%token TOK_STATUS_BEACON
%token TOK_STATUS_BEACON_ALL
%token TOK_BEACON_ON
%token TOK_BEACON_OFF
%token TOK_ON
%token TOK_ON_ALL
%token TOK_OFF
%token TOK_OFF_ALL
%token TOK_CYCLE
%token TOK_CYCLE_ALL
%token TOK_RESET
%token TOK_RESET_ALL
%token TOK_PING
%token TOK_SPEC
%right TOK_DEVICE

%token TOK_PLUG_NAME
%token TOK_NODE
%token TOK_ALIAS

%token TOK_STRING_VAL
%token TOK_NUMERIC_VAL
%token TOK_YES
%token TOK_NO
%token TOK_BEGIN
%token TOK_END

%token TOK_UNRECOGNIZED

/* deprecated in 1.0.16 */
%token TOK_B_NODES
%token TOK_E_NODES
%token TOK_B_GLOBAL
%token TOK_E_GLOBAL
%token TOK_OLD_PORT

%%
/* Grammar Rules for the powerman.conf config file */

configuration_file : spec_list config_list
;
/**************************************************************/
/* config_list                                                */
/**************************************************************/

config_list     : config_list config_item
                | config_item
;
config_item     : client_port 
                | TCP_wrappers 
                | device
                | node
                | alias
                | deprecated
;
deprecated      : TOK_B_NODES   { 
    _warnmsg("'begin nodes' no longer needed"); 
}               | TOK_E_NODES   { 
    _warnmsg("'end nodes' no longer needed"); 
}               | TOK_B_GLOBAL  { 
    _warnmsg("'begin global' no longer needed"); 
}               | TOK_E_GLOBAL  { 
    _warnmsg("'end global' no longer needed"); 
} 
;
TCP_wrappers    : TOK_TCP_WRAPPERS         { 
    _warnmsg("'tcpwrappers' without yes|no"); 
    makeTCPWrappers(TRUE); 
}               | TOK_TCP_WRAPPERS TOK_YES { 
    makeTCPWrappers(TRUE);  
}               | TOK_TCP_WRAPPERS TOK_NO  { 
    makeTCPWrappers(FALSE); 
}
;
client_port     : TOK_PORT TOK_NUMERIC_VAL {
    $$ = (char *)makeClientPort($2);
}               | TOK_OLD_PORT TOK_NUMERIC_VAL {
    _warnmsg("'client listener' not needed");
    $$ = (char *)makeClientPort($2);
}
;
device          : TOK_DEVICE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL 
                  TOK_STRING_VAL %prec TOK_DEVICE {
    $$ = (char *)makeDevice($2, $3, $4, $5);
}
;
node            : TOK_NODE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL {
    $$ = (char *)makeNode($2, $3, $4);
}
;
alias           : TOK_ALIAS TOK_STRING_VAL TOK_STRING_VAL {
    $$ = (char *)makeAlias($2, $3);
}
;
/**************************************************************/
/* specifications                                             */
/**************************************************************/
spec_list       : spec_list spec
                | spec
;
spec            : TOK_SPEC TOK_BEGIN 
                  spec_name_line 
                  spec_item_list 
                  spec_stmt_list 
                  plug_name_sec 
                  TOK_END {
    $$ = (char *)check_Spec();
}
;
spec_name_line  : TOK_SPEC_NAME TOK_STRING_VAL {
    $$ = (char *)makeSpecName($2);
}
;
spec_item_list  : spec_item_list spec_item 
                | spec_item
;
spec_item       : spec_type_line 
                | off_string_line 
                | on_string_line 
                | spec_size_line 
                | dev_timeout
;
spec_type_line  : TOK_SPEC_TYPE TOK_STRING_VAL {
    $$ = (char *)makeSpecType($2);
}
;
off_string_line : TOK_OFF_STRING TOK_STRING_VAL {
    $$ = (char *)makeSpecOff($2);
}
;
on_string_line  : TOK_ON_STRING TOK_STRING_VAL {
    $$ = (char *)makeSpecOn($2);
}
;
spec_size_line  : TOK_PLUG_COUNT TOK_NUMERIC_VAL {
    $$ = (char *)makeSpecSize($2);
}
;
dev_timeout     : TOK_DEV_TIMEOUT TOK_NUMERIC_VAL {
    $$ = (char *)makeDevTimeout($2);
}
;
dev_timeout     : TOK_PING_PERIOD TOK_NUMERIC_VAL {
    $$ = (char *)makePingPeriod($2);
}
;
spec_stmt_list : spec_stmt_list spec_script 
                | spec_script 
;
spec_script     : TOK_SCRIPT TOK_LOGIN TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_LOG_IN);
}
                | TOK_SCRIPT TOK_LOGOUT TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_LOG_OUT);
}
                | TOK_SCRIPT TOK_STATUS TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_PLUGS);
} 
                | TOK_SCRIPT TOK_STATUS_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_PLUGS_ALL);
} 
                | TOK_SCRIPT TOK_STATUS_SOFT TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_NODES);
}
                | TOK_SCRIPT TOK_STATUS_SOFT_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_NODES_ALL);
}
                | TOK_SCRIPT TOK_STATUS_TEMP TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_TEMP);
}
                | TOK_SCRIPT TOK_STATUS_TEMP_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_TEMP_ALL);
}
                | TOK_SCRIPT TOK_STATUS_BEACON TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_BEACON);
}
                | TOK_SCRIPT TOK_STATUS_BEACON_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_STATUS_BEACON);
}
                | TOK_SCRIPT TOK_BEACON_ON TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_BEACON_ON);
}
                | TOK_SCRIPT TOK_BEACON_OFF TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_BEACON_OFF);
}
                | TOK_SCRIPT TOK_ON TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_ON);
}
                | TOK_SCRIPT TOK_ON_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_ON_ALL);
}
                | TOK_SCRIPT TOK_OFF TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_OFF);
}
                | TOK_SCRIPT TOK_OFF_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_OFF_ALL);
}
                | TOK_SCRIPT TOK_CYCLE TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_CYCLE);
}
                | TOK_SCRIPT TOK_CYCLE_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_POWER_CYCLE_ALL);
}
                | TOK_SCRIPT TOK_RESET TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_RESET);
}
                | TOK_SCRIPT TOK_RESET_ALL TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_RESET_ALL);
}
                | TOK_SCRIPT TOK_PING TOK_BEGIN stmt_list TOK_END {
    $$ = (char *)makeScriptSec($2, PM_PING);
}
;
stmt_list     : stmt_list stmt
                | stmt
;
stmt            : TOK_EXPECT TOK_STRING_VAL {
    $$ = (char *)makeStmt(STMT_EXPECT, $2, NULL);
}
                | TOK_EXPECT TOK_STRING_VAL map_sec {
    $$ = (char *)makeStmt(STMT_EXPECT, $2, (List)$3);
}
                | TOK_SEND TOK_STRING_VAL {
    $$ = (char *)makeStmt(STMT_SEND, $2, NULL);
}
                | TOK_DELAY TOK_NUMERIC_VAL {
    $$ = (char *)makeStmt(STMT_DELAY, $2, NULL);
}
;
map_sec         : map_sec map_line {
    $$ = (char *)makeMapSec((List)$1, (Interp *)$2);
}
                | map_line {
    $$ = (char *)makeMapSecHead((Interp *)$1);
}
;
map_line : TOK_MAP TOK_MATCHPOS TOK_NUMERIC_VAL TOK_STRING_VAL {
    $$ = (char *)makeMapLine($3, $4);
}
;
plug_name_sec   : plug_name_sec plug_name_line 
        | /* empty */
;
plug_name_line  : TOK_PLUG_NAME TOK_STRING_VAL {
    $$ = (char *)makePlugNameLine($2);
}
;

%%

void scanner_init(char *filename);
void scanner_fini(void);
int scanner_line(void);
char *scanner_file(void);

/*
 * Entry point into the yacc/lex parser.
 */
int parse_config_file (char *filename)
{
    extern FILE *yyin; /* part of lexer */

    scanner_init(filename);

    yyin = fopen(filename, "r");
    if (!yyin)
        err_exit(TRUE, "%s", filename);
    yyparse();
    fclose(yyin);

    scanner_fini();
    return 0;
} 

static char *makeTCPWrappers(bool val)
{
    conf_set_use_tcp_wrappers(val);
    return NULL;
}

static char *makeClientPort(char *s2)
{
    int port = _strtolong(s2);

    if (port < 1024)
        _errormsg("bogus client listener port");
    conf_set_listen_port(port); 
    return s2;
}

static void _spec_missing(char *msg)
{
    err_exit(FALSE, "protocol specification missing %s: %s::%d\n",
            msg, scanner_file(), scanner_line());
}

/*
 * We've got a whole Spec now so do some sanity checks.
 */
static Spec *check_Spec()
{
    Spec *this_spec;
    char *name;

    name = current_spec->name;
    if( current_spec->type == NO_DEV )
        _spec_missing("type");
    if( current_spec->off == NULL )
        _spec_missing("off string");
    if( current_spec->on == NULL )
        _spec_missing("on string");
    if( (current_spec->size == 0) )
        _spec_missing("size");
    /* Store the spec in list internal to config.c */
    conf_add_spec(current_spec);
    this_spec = current_spec;
    current_spec = NULL;
    return this_spec;
}

static char *makeSpecName(char *s2)
{
    if( current_spec != NULL )
        _errormsg("name must be first");
    current_spec = conf_spec_create(s2);
    return s2;
}

static char *makeSpecType(char *s2)
{
    int n;

    if( current_spec->type != NO_DEV )
        _errormsg("duplicate type");
    if( (n = strncmp(s2, "TCP", 3)) == 0)
        current_spec->type = TCP_DEV;
    else if ( (n = strncmp(s2, "serial", 6)) == 0)
        current_spec->type = SERIAL_DEV;
    return s2;
}

static char *makeSpecOff(char *s2)
{
    if( current_spec->off != NULL )
        _errormsg("duplicate off string");
    current_spec->off = Strdup(s2);
    return s2;
}

static char *makeSpecOn(char *s2)
{
    if( current_spec->on != NULL )
        _errormsg("duplicate on string");
    current_spec->on = Strdup(s2);
    return s2;
}

static char *makeSpecSize(char *s2)
{
    int size = _strtolong(s2);
    int i;

    if( current_spec->size != 0 )
        _errormsg("duplicate plug count");
    if (size < 1)
        _errormsg("invalid plug count");
    current_spec->size = size;
    current_spec->plugname = (char **)Malloc(size * sizeof(char *));
    for (i = 0; i < size; i++)
        current_spec->plugname[i] = NULL;
    return s2;
}

static char *makeDevTimeout(char *s2)
{
    _doubletotv(&current_spec->timeout, _strtodouble(s2));
    return s2;
}

static char *makePingPeriod(char *s2)
{
    _doubletotv(&current_spec->ping_period, _strtodouble(s2));
    return s2;
}

static char *makeStmt(StmtType mode, char *s2, List s3)
{
    PreStmt *specl;
    struct timeval tv;

    /*
     * Four kinds of thing could be arriving.  
     * - EXPECT with an interpretation (s2, s3)
     * - EXPECT without an interpretation (s2, NULL)
     * - SEND (s2, NULL)   (s2 may contain a "%s") 
     * - DELAY (s2, NULL)
     */
    if (mode == STMT_DELAY) {
        _doubletotv(&tv, _strtodouble(s2));
        specl = conf_prestmt_create(mode, NULL, &tv, s3);
    } else {
        specl = conf_prestmt_create(mode, s2, NULL, s3);
    }
    if (current_script == NULL)
        current_script = list_create((ListDelF) conf_prestmt_destroy);
    list_append(current_script, specl);
    return s2;
}

static List makeMapSec(List s1, Interp *s2)
{
    list_append(s1, s2);
    return s1;
}

static List makeMapSecHead(Interp *s1)
{
    List map;

    map = list_create((ListDelF) conf_interp_destroy);
    list_append(map, s1);
    return map;
}

static Interp *makeMapLine(char *s3, char *s4)
{
    Interp *interp;

    interp = conf_interp_create(s4);
    interp->match_pos = _strtolong(s3);
    if (interp->match_pos < 1 || interp->match_pos >= MAX_MATCH_POS)
        _errormsg("invalid match position value");
    return interp;
}

static char *makeScriptSec(char *s2, int com)
{
    if( current_spec->prescripts[com] != NULL )
        _errormsg("duplicate script");
    current_spec->prescripts[com] = current_script;
    current_script = NULL;
    return s2;
}
    


static char *makePlugNameLine(char *s2)
{
    int i;

    if( current_spec == NULL ) 
        _errormsg("plug name outside of specification");
    if( current_spec->plugname == NULL ) 
        _errormsg("plug name line not preceded by plugcount line");
    if( current_spec->size <= 0 ) 
        _errormsg("invalid size");
    for (i = 0; i < current_spec->size; i++)
        if (current_spec->plugname[i] == NULL)
            break;
    if(i == current_spec->size)
        _errormsg("too many plug names lines");
    current_spec->plugname[i] = Strdup(s2);
    return s2;
}

/*
 * We've hit a Device line.  
 * s2: proper name for the Device
 * s3: name of the Spec to use
 * s4: the internet host name of the Device or special file name
 * s5: the TCP port on which it is listening or serial flags (e.g. 9600,8n1)
 * From this information we can build a complete Device structure.
 */   
static char *makeDevice(char *s2, char *s3, char *s4, char *s5)
{
    Device *dev;
    Spec *spec;
    reg_syntax_t cflags = REG_EXTENDED;
    int i;
    Plug *plug;

    /* find that spec */
    spec = conf_find_spec(s3);
    if ( spec == NULL ) 
    _errormsg("device specification not found");

    /* make the Device */
    dev = dev_create(s2);
    dev->specname = Strdup(s3);
    dev->type = spec->type;
    dev->timeout = spec->timeout;
    dev->ping_period = spec->ping_period;
    /* set up the host name and port */
    switch(dev->type) {
    case TCP_DEV :
        dev->u.tcp.host = Strdup(s4);
        dev->u.tcp.service = Strdup(s5);
        break;
    case SERIAL_DEV :
        dev->u.serial.special = Strdup(s4);
        dev->u.serial.flags = Strdup(s5);
        break;
    default :
        _errormsg("unimplemented device type");
    }
    for (i = 0; i < spec->size; i++) {
        plug = dev_plug_create(spec->plugname[i]);
        list_append(dev->plugs, plug);
    }
    /* begin transfering info from the Spec to the Device */
    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    Regcomp( &(dev->on_re), spec->on, cflags);
    Regcomp( &(dev->off_re), spec->off, cflags);
    
    /* 
     * Each script needs to be transferred and any Interps need to 
     * be set up.  The conf_stmt_create() call transforms the EXPECT 
     * strings into compiled RegEx recognizers. 
     */
    for (i = 0; i < NUM_SCRIPTS; i++) {
        ListIterator script;
        PreStmt *specl;
        Stmt *stmt;
        Interp *interp;
        Interp *new;
        List map;
        ListIterator map_i;

        if (spec->prescripts[i] == NULL) {
            dev->scripts[i] = NULL;
            continue; /* unimplemented */
        }

        dev->scripts[i] = list_create((ListDelF) conf_stmt_destroy);
        script = list_iterator_create(spec->prescripts[i]);
        while( (specl = list_next(script)) ) {
            if( specl->map == NULL )
                map = NULL;
            else {
                map = list_create((ListDelF) conf_interp_destroy);
                map_i = list_iterator_create(specl->map);
                while( (interp = list_next(map_i)) ) {
                    new = conf_interp_create(interp->plug_name);
                    new->match_pos = interp->match_pos;
                    list_append(map, new);
                }
                list_iterator_destroy(map_i);
            }
            stmt = conf_stmt_create(specl->type, specl->string1, 
                                          map, specl->tv);
            list_append(dev->scripts[i], stmt);
        }
    }

    dev_add(dev);
    return s2;
}

static char *makeAlias(char *s2, char *s3)
{
    if (!conf_add_alias(s2, s3))
        _errormsg("bad alias");
    return s2;
}

/*
 * s1 : literal "node"
 * s2 : node name
 * s3 : device name
 * s4 : plug name
 */
static char *makeNode(char *s2, char *s3, char *s4)
{
    Plug *plug;
    Device *dev;

    if (!conf_addnode(s2))
        _errormsg("duplicate node");
    dev = dev_findbyname(s3);       /* find the device by its name */
    if(dev == NULL) 
        _errormsg("unknown device");

    /*
     * NOTE: Some devices do not define the plug names in advance - the plug 
     * names must come from the node definitions instead.  Example: ibmrsa.dev 
     */

    /* find the plug in the device's list of plug names */
    plug = list_find_first(dev->plugs, (ListFindF) dev_plug_match_plugname, s4);
    if (plug) {
        plug->node = Strdup(s2);
        return s2;
    }

    /* if not found, look for a plug on this device without a plug name */
    plug = list_find_first(dev->plugs, (ListFindF)dev_plug_match_noname, s4);
    if (plug == NULL)
        _errormsg("unknown plug name or device plugcount exceeded");

    plug->node = Strdup(s2);
    plug->name = Strdup(s4);
    return s2;
}

static double _strtodouble(char *str)
{
    char *endptr;
    double val = strtod(str, &endptr);

    if (val == 0.0 && endptr == str)
        _errormsg("error parsing double value");
    if ((val == HUGE_VAL || val == -HUGE_VAL) && errno == ERANGE)
        _errormsg("double value would cause overflow");
    return val;
}

static long _strtolong(char *str)
{
    char *endptr;
    long val = strtol(str, &endptr, 0);

    if (val == 0 && endptr == str)
        _errormsg("error parsing long integer value");
    if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)
        _errormsg("long integer value would cause under/overflow");
    return val;
}

static void _doubletotv(struct timeval *tv, double val)
{
    tv->tv_sec = (val * 10.0)/10; /* crude round-down without -lm */
    tv->tv_usec = ((val - tv->tv_sec) * 1000000.0);
}

static void _errormsg(char *msg)
{
    err_exit(FALSE, "%s: %s::%d", msg, scanner_file(), scanner_line());
}

static void _warnmsg(char *msg)
{
    err(FALSE, "warning: %s: %s::%d", msg, scanner_file(), scanner_line());
}

void yyerror()
{
    _errormsg("parse error");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
