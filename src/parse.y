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

/*
 * A PreScript is a list of PreStmts.
 */
typedef struct {
    StmtType type;              /* delay/expect/send */
    char *str;                  /* expect string, send fmt, setstatus plug */
    struct timeval tv;          /* delay value */
    int mp1;                    /* setstatus plug, setplugname plug match pos */
    int mp2;                    /* settatus stat, setplugname node match pos */
    List stmts;                 /* subblock */
} PreStmt;
typedef List PreScript;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    char *name;                 /* specification name, e.g. "icebox" */
    DevType type;               /* device type, e.g. TCP_DEV */
    char *off;                  /* off string, e.g. "OFF" */
    char *on;                   /* on string, e.g. "ON" */
    struct timeval timeout;     /* timeout for this device */
    struct timeval ping_period; /* ping period for this device 0.0 = none */
    List plugs;                 /* list of plug names (e.g. "1" thru "10") */
    PreScript prescripts[NUM_SCRIPTS];  /* array of PreScripts */
} Spec;                                 /*   script may be NULL if undefined */

/* powerman.conf */
static void makeNode(char *nodestr, char *devstr, char *plugstr);
static void makeAlias(char *namestr, char *hostsstr);
static Stmt *makeStmt(PreStmt *p);
static void destroyStmt(Stmt *stmt);
static void makeDevice(char *devstr, char *specstr, char *hoststr, 
                        char *portstr);
static void makeClientPort(char *s2);

/* device config */
static PreStmt *makePreStmt(StmtType type, char *str, char *tvstr, 
                      char *mp1str, char *mp2str, List prestmts);
static void destroyPreStmt(PreStmt *p);
static Spec *makeSpec(char *name);
static Spec *findSpec(char *name);
static int matchSpec(Spec * spec, void *key);
static void destroySpec(Spec * spec);
static void _clear_current_spec(void);
static void makeScript(int com, List stmts);

/* utility functions */
static void _errormsg(char *msg);
static void _warnmsg(char *msg);
static long _strtolong(char *str);
static double _strtodouble(char *str);
static void _doubletotv(struct timeval *tv, double val);

extern int yylex();
void yyerror();


static List device_specs = NULL;      /* list of Spec's */
static Spec current_spec;             /* Holds a Spec as it is built */
%}

/* script names */
%token TOK_LOGIN TOK_LOGOUT TOK_STATUS TOK_STATUS_ALL
%token TOK_STATUS_SOFT TOK_STATUS_SOFT_ALL TOK_STATUS_TEMP TOK_STATUS_TEMP_ALL
%token TOK_STATUS_BEACON TOK_STATUS_BEACON_ALL TOK_BEACON_ON TOK_BEACON_OFF
%token TOK_ON TOK_ON_ALL TOK_OFF TOK_OFF_ALL TOK_CYCLE TOK_CYCLE_ALL TOK_RESET
%token TOK_RESET_ALL TOK_PING TOK_SPEC 

/* script statements */
%token TOK_EXPECT TOK_SETSTATUS TOK_SETPLUGNAME TOK_SEND TOK_DELAY
%token TOK_FOREACHPLUG TOK_FOREACHNODE

/* other device configuration stuff */
%token TOK_SPEC_NAME TOK_SPEC_TYPE TOK_OFF_STRING TOK_ON_STRING
%token TOK_MAX_PLUG_COUNT TOK_TIMEOUT TOK_DEV_TIMEOUT TOK_PING_PERIOD
%token TOK_PLUG_NAME TOK_SCRIPT 

/* device types */
%token TOK_TYPE_TCP TOK_TYPE_TELNET TOK_TYPE_SERIAL

/* powerman.conf stuff */
%token TOK_DEVICE TOK_NODE TOK_ALIAS TOK_TCP_WRAPPERS TOK_PORT

/* general */
%token TOK_MATCHPOS TOK_STRING_VAL TOK_NUMERIC_VAL TOK_YES TOK_NO
%token TOK_BEGIN TOK_END TOK_UNRECOGNIZED

/* deprecated in 1.0.16 */
%token TOK_B_NODES TOK_E_NODES TOK_B_GLOBAL TOK_E_GLOBAL TOK_OLD_PORT

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
    conf_set_use_tcp_wrappers(TRUE);
}               | TOK_TCP_WRAPPERS TOK_YES { 
    conf_set_use_tcp_wrappers(TRUE);
}               | TOK_TCP_WRAPPERS TOK_NO  { 
    conf_set_use_tcp_wrappers(FALSE);
}
;
client_port     : TOK_PORT TOK_NUMERIC_VAL {
    makeClientPort($2);
}               | TOK_OLD_PORT TOK_NUMERIC_VAL {
    _warnmsg("'client listener' not needed");
    makeClientPort($2);
}
;
device          : TOK_DEVICE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL 
                  TOK_STRING_VAL {
    makeDevice($2, $3, $4, $5);
}
;
node            : TOK_NODE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL {
    makeNode($2, $3, $4);
}               | TOK_NODE TOK_STRING_VAL TOK_STRING_VAL {
    makeNode($2, $3, NULL);
}
;
alias           : TOK_ALIAS TOK_STRING_VAL TOK_STRING_VAL {
    makeAlias($2, $3);
}
;
/**************************************************************/
/* specifications                                             */
/**************************************************************/
spec_list       : spec_list spec
                | spec
;
spec            : TOK_SPEC TOK_STRING_VAL TOK_BEGIN 
                  spec_item_list 
                  TOK_END {
    makeSpec($2);
}
;
spec_item_list  : spec_item_list spec_item 
                | spec_item
;
spec_item       : spec_type
                | spec_off_regex
                | spec_on_regex
                | spec_timeout
                | spec_ping_period
                | spec_plug_list
                | spec_script_list
;
spec_type       : TOK_SPEC_TYPE TOK_TYPE_TCP {
    current_spec.type = TCP_DEV;
}               | TOK_SPEC_TYPE TOK_TYPE_TELNET {
    current_spec.type = TELNET_DEV;
}               | TOK_SPEC_TYPE TOK_TYPE_SERIAL {
    current_spec.type = SERIAL_DEV;
}
;
spec_off_regex  : TOK_OFF_STRING TOK_STRING_VAL {
    if (current_spec.off != NULL)
        _errormsg("duplicate off string");
    current_spec.off = Strdup($2);
}
;
spec_on_regex   : TOK_ON_STRING TOK_STRING_VAL {
    if (current_spec.on != NULL)
        _errormsg("duplicate on string");
    current_spec.on = Strdup($2);
}
;
spec_timeout    : TOK_DEV_TIMEOUT TOK_NUMERIC_VAL {
    _doubletotv(&current_spec.timeout, _strtodouble($2));
}
;
spec_ping_period: TOK_PING_PERIOD TOK_NUMERIC_VAL {
    _doubletotv(&current_spec.ping_period, _strtodouble($2));
}
;
string_list     : string_list TOK_STRING_VAL {
    list_append((List)$1, Strdup($2)); 
    $$ = $1; 
}               | TOK_STRING_VAL {
    $$ = (char *)list_create((ListDelF)Free);
    list_append((List)$$, Strdup($1)); 
}
;
spec_plug_list  : TOK_PLUG_NAME TOK_BEGIN string_list TOK_END {
    if (current_spec.plugs != NULL)
        _errormsg("duplicate plug list");
    current_spec.plugs = (List)$3;
}
;
spec_script_list  : spec_script_list spec_script 
                | spec_script 
;
spec_script     : TOK_SCRIPT TOK_LOGIN stmt_block {
    makeScript(PM_LOG_IN, (List)$3);
}               | TOK_SCRIPT TOK_LOGOUT stmt_block {
    makeScript(PM_LOG_OUT, (List)$3);
}               | TOK_SCRIPT TOK_STATUS stmt_block {
    makeScript(PM_STATUS_PLUGS, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_ALL stmt_block {
    makeScript(PM_STATUS_PLUGS_ALL, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_SOFT stmt_block {
    makeScript(PM_STATUS_NODES, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_SOFT_ALL stmt_block {
    makeScript(PM_STATUS_NODES_ALL, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_TEMP stmt_block {
    makeScript(PM_STATUS_TEMP, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_TEMP_ALL stmt_block {
    makeScript(PM_STATUS_TEMP_ALL, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_BEACON stmt_block {
    makeScript(PM_STATUS_BEACON, (List)$3);
}               | TOK_SCRIPT TOK_STATUS_BEACON_ALL stmt_block {
    makeScript(PM_STATUS_BEACON_ALL, (List)$3);
}               | TOK_SCRIPT TOK_BEACON_ON stmt_block {
    makeScript(PM_BEACON_ON, (List)$3);
}               | TOK_SCRIPT TOK_BEACON_OFF stmt_block {
    makeScript(PM_BEACON_OFF, (List)$3);
}               | TOK_SCRIPT TOK_ON stmt_block {
    makeScript(PM_POWER_ON, (List)$3);
}               | TOK_SCRIPT TOK_ON_ALL stmt_block {
    makeScript(PM_POWER_ON_ALL, (List)$3);
}               | TOK_SCRIPT TOK_OFF stmt_block {
    makeScript(PM_POWER_OFF, (List)$3);
}               | TOK_SCRIPT TOK_OFF_ALL stmt_block {
    makeScript(PM_POWER_OFF_ALL, (List)$3);
}               | TOK_SCRIPT TOK_CYCLE stmt_block {
    makeScript(PM_POWER_CYCLE, (List)$3);
}               | TOK_SCRIPT TOK_CYCLE_ALL stmt_block {
    makeScript(PM_POWER_CYCLE_ALL, (List)$3);
}               | TOK_SCRIPT TOK_RESET stmt_block {
    makeScript(PM_RESET, (List)$3);
}               | TOK_SCRIPT TOK_RESET_ALL stmt_block {
    makeScript(PM_RESET_ALL, (List)$3);
}               | TOK_SCRIPT TOK_PING stmt_block {
    makeScript(PM_PING, (List)$3);
}
;
stmt_block      : TOK_BEGIN stmt_list TOK_END {
    $$ = $2;
}
;
stmt_list       : stmt_list stmt { 
    list_append((List)$1, $2); 
    $$ = $1; 
}               | stmt { 
    $$ = (char *)list_create((ListDelF)destroyPreStmt);
    list_append((List)$$, $1); 
}
;
stmt            : TOK_EXPECT TOK_STRING_VAL {
    $$ = (char *)makePreStmt(STMT_EXPECT, $2, NULL, NULL, NULL, NULL);
}               | TOK_SEND TOK_STRING_VAL {
    $$ = (char *)makePreStmt(STMT_SEND, $2, NULL, NULL, NULL, NULL);
}               | TOK_DELAY TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_DELAY, NULL, $2, NULL, NULL, NULL);
}               | TOK_SETPLUGNAME TOK_MATCHPOS TOK_NUMERIC_VAL TOK_MATCHPOS 
                  TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_SETPLUGNAME, NULL, NULL, $5, $3, NULL);
}               | TOK_SETSTATUS TOK_STRING_VAL TOK_MATCHPOS TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_SETSTATUS, $2, NULL, NULL, $4, NULL);
}               | TOK_SETSTATUS TOK_MATCHPOS TOK_NUMERIC_VAL TOK_MATCHPOS 
                  TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_SETSTATUS, NULL, NULL, $3, $5, NULL);
}               | TOK_SETSTATUS TOK_MATCHPOS TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_SETSTATUS, NULL, NULL, NULL, $3, NULL);
}               | TOK_FOREACHNODE stmt_block {
    $$ = (char *)makePreStmt(STMT_FOREACHNODE, NULL, NULL, NULL, NULL, (List)$2);
}               | TOK_FOREACHPLUG stmt_block {
    $$ = (char *)makePreStmt(STMT_FOREACHPLUG, NULL, NULL, NULL, NULL, (List)$2);
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

    device_specs = list_create((ListDelF) destroySpec);
    _clear_current_spec();

    yyparse();
    fclose(yyin);

    scanner_fini();

    list_destroy(device_specs);

    return 0;
} 

static void _spec_missing(char *msg)
{
    err_exit(FALSE, "protocol specification missing %s: %s::%d\n",
            msg, scanner_file(), scanner_line());
}

/* makePreStmt(type, str, tv, mp1(plug), mp2(stat/node) */
static PreStmt *makePreStmt(StmtType type, char *str, char *tvstr, 
                      char *mp1str, char *mp2str, List prestmts)
{
    PreStmt *new;

    new = (PreStmt *) Malloc(sizeof(PreStmt));
    memset(new, 0, sizeof(PreStmt));

    new->type = type;
    new->mp1 = mp1str ? _strtolong(mp1str) : -1;
    new->mp2 = mp2str ? _strtolong(mp2str) : -1;
    if (str)
        new->str = Strdup(str);
    if (tvstr)
        _doubletotv(&new->tv, _strtodouble(tvstr));

    return new;
}

static void destroyPreStmt(PreStmt *p)
{
    if (p->str)
        Free(p->str);
    p->str = NULL;
    Free(p);
}

static void _clear_current_spec(void)
{
    int i;

    current_spec.name = NULL;
    current_spec.type = NO_DEV;
    current_spec.off = NULL;
    current_spec.on = NULL;
    current_spec.plugs = NULL;
    timerclear(&current_spec.timeout);
    timerclear(&current_spec.ping_period);
    for (i = 0; i < NUM_SCRIPTS; i++)
        current_spec.prescripts[i] = NULL;
}

static Spec *_copy_current_spec(void)
{
    Spec *new = (Spec *) Malloc(sizeof(Spec));
    int i;

    *new = current_spec;
    for (i = 0; i < NUM_SCRIPTS; i++)
        new->prescripts[i] = current_spec.prescripts[i];
    _clear_current_spec();
    return new;
}

static Spec *makeSpec(char *name)
{
    Spec *spec;

    current_spec.name = Strdup(name);
    if (current_spec.type == NO_DEV)
        _spec_missing("type");
    if(current_spec.off == NULL)
        _spec_missing("offstring");
    if(current_spec.on == NULL)
        _spec_missing("onstring");

    /* FIXME: check for manditory scripts here?  what are they? */

    spec = _copy_current_spec();
    assert(device_specs != NULL);
    list_append(device_specs, spec);

    return spec;
}

static void destroySpec(Spec * spec)
{
    int i;

    if (spec->name)
        Free(spec->name);
    if (spec->off)
        Free(spec->off);
    if (spec->on)
        Free(spec->on);
    if (spec->plugs)
        list_destroy(spec->plugs);
    for (i = 0; i < NUM_SCRIPTS; i++)
        if (spec->prescripts[i])
            list_destroy(spec->prescripts[i]);
    Free(spec);
}

static int matchSpec(Spec * spec, void *key)
{
    return (strcmp(spec->name, (char *) key) == 0);
}

static Spec *findSpec(char *name)
{
    return list_find_first(device_specs, (ListFindF) matchSpec, name);
}

static void makeScript(int com, List stmts)
{
    if (current_spec.prescripts[com] != NULL)
        _errormsg("duplicate script");
    current_spec.prescripts[com] = stmts;
}

/**
 ** Powerman.conf stuff.
 **/

static void destroyStmt(Stmt *stmt)
{
    assert(stmt != NULL);

    switch (stmt->type) {
    case STMT_SEND:
        Free(stmt->u.send.fmt);
        break;
    case STMT_EXPECT:
        break;
    case STMT_DELAY:
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
        list_destroy(stmt->u.foreach.stmts);
        break;
    default:
    }
    Free(stmt);
}

static Stmt *makeStmt(PreStmt *p)
{
    Stmt *stmt;
    PreStmt *subp;
    reg_syntax_t cflags = REG_EXTENDED;
    ListIterator itr;

    stmt = (Stmt *) Malloc(sizeof(Stmt));
    stmt->type = p->type;
    switch (p->type) {
    case STMT_SEND:
        stmt->u.send.fmt = Strdup(p->str);
        break;
    case STMT_EXPECT:
        Regcomp(&(stmt->u.expect.exp), p->str, cflags);
        break;
    case STMT_SETSTATUS:
        stmt->u.setstatus.stat_mp = p->mp2;
        if (p->str)
            stmt->u.setstatus.plug_name = Strdup(p->str);
        else
            stmt->u.setstatus.plug_mp = p->mp1;
        break;
    case STMT_SETPLUGNAME:
        stmt->u.setplugname.plug_mp = p->mp1;
        stmt->u.setplugname.node_mp = p->mp2;
        break;
    case STMT_DELAY:
        stmt->u.delay.tv = p->tv;
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
        stmt->u.foreach.stmts = list_create((ListDelF) destroyStmt);
        itr = list_iterator_create(p->stmts);
        while((subp = list_next(itr))) {
            list_append(stmt->u.foreach.stmts, makeStmt(subp));
        }
        list_iterator_destroy(itr);
        break;
    default:
    }
    return stmt;
}

static void makeDevice(char *devstr, char *specstr, char *hoststr, 
                        char *portstr)
{
    ListIterator itr;
    Device *dev;
    Spec *spec;
    char *plugname;
    reg_syntax_t cflags = REG_EXTENDED;
    int i;

    /* find that spec */
    spec = findSpec(specstr);
    if ( spec == NULL ) 
        _errormsg("device specification not found");

    /* make the Device */
    dev = dev_create(devstr, spec->type);
    dev->specname = Strdup(specstr);
    dev->timeout = spec->timeout;
    dev->ping_period = spec->ping_period;

    /* set up the host name and port */
    switch(dev->type) {
    case TCP_DEV :
    case TELNET_DEV :
        dev->u.tcp.host = Strdup(hoststr);
        dev->u.tcp.service = Strdup(portstr);
        break;
    case SERIAL_DEV :
        dev->u.serial.special = Strdup(hoststr);
        dev->u.serial.flags = Strdup(portstr);
        break;
    default :
        _errormsg("unimplemented device type");
    }

    /* create plugs */
    if (spec->plugs) {
        itr = list_iterator_create(spec->plugs);
        while((plugname = list_next(itr))) {
            Plug *plug = dev_plug_create(plugname);

            list_append(dev->plugs, plug);
        }
        list_iterator_destroy(itr);
    }

    /* transfer remaining info from the spec to the device */
    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    Regcomp( &(dev->on_re), spec->on, cflags | REG_NOSUB);
    Regcomp( &(dev->off_re), spec->off, cflags | REG_NOSUB);
    for (i = 0; i < NUM_SCRIPTS; i++) {
        PreStmt *p;

        if (spec->prescripts[i] == NULL) {
            dev->scripts[i] = NULL;
            continue; /* unimplemented script */
        }

        dev->scripts[i] = list_create((ListDelF) destroyStmt);

        /* copy the list of statements in each script */
        itr = list_iterator_create(spec->prescripts[i]);
        while((p = list_next(itr))) {
            list_append(dev->scripts[i], makeStmt(p));
        }
        list_iterator_destroy(itr);
    }

    dev_add(dev);
}

static void makeAlias(char *namestr, char *hostsstr)
{
    if (!conf_add_alias(namestr, hostsstr))
        _errormsg("bad alias");
}

static void makeNode(char *nodestr, char *devstr, char *plugstr)
{
    Plug *plug;
    Device *dev;

    if (!conf_addnode(nodestr))
        _errormsg("duplicate node");
    dev = dev_findbyname(devstr);       /* find the device by its name */
    if(dev == NULL) 
        _errormsg("unknown device");

    /* 
     * Legal plugs are specified in advance with 'plug name' line in the device
     * configuration file.  Node line references to plug names must match
     * these names.
     */
    if (plugstr) {
        plug = list_find_first(dev->plugs, (ListFindF) dev_plug_match_plugname, 
                               plugstr);
        if (plug == NULL)
            _errormsg("unknown plug name");
        if (plug->node)
            _errormsg("plug already assigned");
        plug->node = Strdup(nodestr);

    /* 
     * Some devices do not specify plug names in advance.  For these devices,
     * a Node line must not reference a plug name (that field is omitted).
     * Instead, we simply create a plug entry for each node configured for 
     * the device.  The names will be filled in later via 'setaddr' statements 
     * (see ibmrsa.dev).
     */
    } else {
        plug = dev_plug_create(NULL);
        plug->node = Strdup(nodestr);
        list_append(dev->plugs, plug);
    }
}

static void makeClientPort(char *s2)
{
    int port = _strtolong(s2);

    if (port < 1024)
        _errormsg("bogus client listener port");
    conf_set_listen_port(port); 
}

/**
 ** Utility functions
 **/

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
