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
#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#define YYSTYPE char *  /*  The generic type returned by all parse matches */
#undef YYDEBUG          /* no debug code plese */
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>       /* for HUGE_VAL and trunc */
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "wrappers.h"
#include "pluglist.h"
#include "device.h"
#include "device_serial.h"
#include "device_pipe.h"
#include "device_tcp.h"
#include "error.h"
#include "string.h"

/*
 * A PreScript is a list of PreStmts.
 */
#define PRESTMT_MAGIC 0x89786756
typedef struct {
    int magic;
    StmtType type;              /* delay/expect/send */
    char *str;                  /* expect string, send fmt, setplugstate plug */
    struct timeval tv;          /* delay value */
    int mp1;                    /* setplugstate plug match position */
    int mp2;                    /* setplugstate state match position */
    List prestmts;              /* subblock */
    List interps;               /* interpretations for setplugstate */
} PreStmt;
typedef List PreScript;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    char *name;                 /* specification name, e.g. "icebox" */
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
                      char *mp1str, char *mp2str, List prestmts, List interps);
static void destroyPreStmt(PreStmt *p);
static Spec *makeSpec(char *name);
static Spec *findSpec(char *name);
static int matchSpec(Spec * spec, void *key);
static void destroySpec(Spec * spec);
static void _clear_current_spec(void);
static void makeScript(int com, List stmts);
static void destroyInterp(Interp *i);
static Interp *makeInterp(InterpState state, char *str);
static List copyInterpList(List ilist);

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
%token TOK_ON TOK_ON_RANGED TOK_ON_ALL TOK_OFF TOK_OFF_RANGED TOK_OFF_ALL 
%token TOK_CYCLE TOK_CYCLE_RANGED TOK_CYCLE_ALL 
%token TOK_RESET TOK_RESET_RANGED TOK_RESET_ALL TOK_PING TOK_SPEC 

/* script statements */
%token TOK_EXPECT TOK_SETPLUGSTATE TOK_SEND TOK_DELAY
%token TOK_FOREACHPLUG TOK_FOREACHNODE TOK_IFOFF TOK_IFON

/* other device configuration stuff */
%token TOK_OFF_STRING TOK_ON_STRING
%token TOK_MAX_PLUG_COUNT TOK_TIMEOUT TOK_DEV_TIMEOUT TOK_PING_PERIOD
%token TOK_PLUG_NAME TOK_SCRIPT 

/* powerman.conf stuff */
%token TOK_DEVICE TOK_NODE TOK_ALIAS TOK_TCP_WRAPPERS TOK_PORT

/* general */
%token TOK_MATCHPOS TOK_STRING_VAL TOK_NUMERIC_VAL TOK_YES TOK_NO
%token TOK_BEGIN TOK_END TOK_UNRECOGNIZED TOK_EQUALS

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
}               | TOK_DEVICE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL {
    makeDevice($2, $3, $4, NULL);
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
spec_item       : spec_timeout
                | spec_ping_period
                | spec_plug_list
                | spec_script_list
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
}               | TOK_SCRIPT TOK_ON_RANGED stmt_block {
    makeScript(PM_POWER_ON_RANGED, (List)$3);
}               | TOK_SCRIPT TOK_ON_ALL stmt_block {
    makeScript(PM_POWER_ON_ALL, (List)$3);
}               | TOK_SCRIPT TOK_OFF stmt_block {
    makeScript(PM_POWER_OFF, (List)$3);
}               | TOK_SCRIPT TOK_OFF_RANGED stmt_block {
    makeScript(PM_POWER_OFF_RANGED, (List)$3);
}               | TOK_SCRIPT TOK_OFF_ALL stmt_block {
    makeScript(PM_POWER_OFF_ALL, (List)$3);
}               | TOK_SCRIPT TOK_CYCLE stmt_block {
    makeScript(PM_POWER_CYCLE, (List)$3);
}               | TOK_SCRIPT TOK_CYCLE_RANGED stmt_block {
    makeScript(PM_POWER_CYCLE_RANGED, (List)$3);
}               | TOK_SCRIPT TOK_CYCLE_ALL stmt_block {
    makeScript(PM_POWER_CYCLE_ALL, (List)$3);
}               | TOK_SCRIPT TOK_RESET stmt_block {
    makeScript(PM_RESET, (List)$3);
}               | TOK_SCRIPT TOK_RESET_RANGED stmt_block {
    makeScript(PM_RESET_RANGED, (List)$3);
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
    $$ = (char *)makePreStmt(STMT_EXPECT, $2, NULL, NULL, NULL, NULL, NULL);
}               | TOK_SEND TOK_STRING_VAL {
    $$ = (char *)makePreStmt(STMT_SEND, $2, NULL, NULL, NULL, NULL, NULL);
}               | TOK_DELAY TOK_NUMERIC_VAL {
    $$ = (char *)makePreStmt(STMT_DELAY, NULL, $2, NULL, NULL, NULL, NULL);
}               | TOK_SETPLUGSTATE TOK_STRING_VAL regmatch {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, $2, NULL, NULL, $3, NULL, NULL);
}               | TOK_SETPLUGSTATE TOK_STRING_VAL regmatch interp_list {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, $2, NULL, NULL, $3, NULL,
                             (List)$4);
}               | TOK_SETPLUGSTATE regmatch regmatch {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, $2, $3, NULL, NULL);
}               | TOK_SETPLUGSTATE regmatch regmatch interp_list {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, $2, $3, NULL,(List)$4);
}               | TOK_SETPLUGSTATE regmatch {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, NULL, $2, NULL, NULL);
}               | TOK_SETPLUGSTATE regmatch interp_list {
    $$ = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, NULL,$2,NULL,(List)$3);
}               | TOK_FOREACHNODE stmt_block {
    $$ = (char *)makePreStmt(STMT_FOREACHNODE, NULL, NULL, NULL, NULL, 
                             (List)$2, NULL);
}               | TOK_FOREACHPLUG stmt_block {
    $$ = (char *)makePreStmt(STMT_FOREACHPLUG, NULL, NULL, NULL, NULL, 
                             (List)$2, NULL);
}               | TOK_IFOFF stmt_block {
    $$ = (char *)makePreStmt(STMT_IFOFF, NULL, NULL, NULL, NULL, 
                             (List)$2, NULL);
}               | TOK_IFON stmt_block {
    $$ = (char *)makePreStmt(STMT_IFON, NULL, NULL, NULL, NULL, 
                             (List)$2, NULL);
}
;
interp_list       : interp_list interp { 
    list_append((List)$1, $2); 
    $$ = $1; 
}               | interp { 
    $$ = (char *)list_create((ListDelF)destroyInterp);
    list_append((List)$$, $1); 
}
;
interp            : TOK_ON TOK_EQUALS TOK_STRING_VAL {
    $$ = (char *)makeInterp(ST_ON, $3);
}                 | TOK_OFF TOK_EQUALS TOK_STRING_VAL {
    $$ = (char *)makeInterp(ST_OFF, $3);
}
;
regmatch          : TOK_MATCHPOS TOK_NUMERIC_VAL {
    $$ = $2;
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

/* makePreStmt(type, str, tv, mp1(plug), mp2(stat/node), prestmts, interps */
static PreStmt *makePreStmt(StmtType type, char *str, char *tvstr, 
                      char *mp1str, char *mp2str, List prestmts, 
                      List interps)
{
    PreStmt *new;

    new = (PreStmt *) Malloc(sizeof(PreStmt));

    new->magic = PRESTMT_MAGIC;
    new->type = type;
    new->mp1 = mp1str ? _strtolong(mp1str) : -1;
    new->mp2 = mp2str ? _strtolong(mp2str) : -1;
    if (str)
        new->str = Strdup(str);
    if (tvstr)
        _doubletotv(&new->tv, _strtodouble(tvstr));
    new->prestmts = prestmts;
    new->interps = interps;

    return new;
}

static void destroyPreStmt(PreStmt *p)
{
    assert(p->magic == PRESTMT_MAGIC);
    p->magic = 0;
    if (p->str)
        Free(p->str);
    p->str = NULL;
    if (p->prestmts)
        list_destroy(p->prestmts);
    p->prestmts = NULL;
    if (p->interps)
        list_destroy(p->interps);
    p->interps = NULL;
    Free(p);
}

static void _clear_current_spec(void)
{
    int i;

    current_spec.name = NULL;
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

static Interp *makeInterp(InterpState state, char *str)
{
    Interp *new = (Interp *)Malloc(sizeof(Interp));

    new->magic = INTERP_MAGIC; 
    new->str = Strdup(str); 
    new->re = NULL;  /* defer compilation until copyInterpList */
    new->state = state;

    return new;
}

static void destroyInterp(Interp *i)
{
    assert(i->magic == INTERP_MAGIC);
    i->magic = 0;
    Free(i->str);
    if (i->re) {
        regfree(i->re);
        Free(i->re);
    }
    Free(i);
}

static List copyInterpList(List il)
{
    reg_syntax_t cflags = REG_EXTENDED | REG_NOSUB;
    ListIterator itr;
    Interp *ip, *icpy;
    List new = list_create((ListDelF) destroyInterp);

    if (il != NULL) {

        itr = list_iterator_create(il);
        while((ip = list_next(itr))) {
            assert(ip->magic == INTERP_MAGIC);
            icpy = makeInterp(ip->state, ip->str);
            icpy->re = (regex_t *)Malloc(sizeof(regex_t));
            Regcomp(icpy->re, icpy->str, cflags);
            assert(icpy->magic == INTERP_MAGIC);
            list_append(new, icpy);
        }
        list_iterator_destroy(itr);
    }

    return new;
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
        regfree(&stmt->u.expect.exp);
        break;
    case STMT_DELAY:
        break;
    case STMT_SETPLUGSTATE:
        list_destroy(stmt->u.setplugstate.interps);
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
    case STMT_IFON:
    case STMT_IFOFF:
        list_destroy(stmt->u.foreach.stmts);
        break;
    default:
        break;
    }
    Free(stmt);
}

static Stmt *makeStmt(PreStmt *p)
{
    Stmt *stmt;
    PreStmt *subp;
    reg_syntax_t cflags = REG_EXTENDED;
    ListIterator itr;

    assert(p->magic == PRESTMT_MAGIC);
    stmt = (Stmt *) Malloc(sizeof(Stmt));
    stmt->type = p->type;
    switch (p->type) {
    case STMT_SEND:
        stmt->u.send.fmt = Strdup(p->str);
        break;
    case STMT_EXPECT:
        Regcomp(&stmt->u.expect.exp, p->str, cflags);
        break;
    case STMT_SETPLUGSTATE:
        stmt->u.setplugstate.stat_mp = p->mp2;
        if (p->str)
            stmt->u.setplugstate.plug_name = Strdup(p->str);
        else
            stmt->u.setplugstate.plug_mp = p->mp1;
        stmt->u.setplugstate.interps = copyInterpList(p->interps);
        break;
    case STMT_DELAY:
        stmt->u.delay.tv = p->tv;
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
        stmt->u.foreach.stmts = list_create((ListDelF) destroyStmt);
        itr = list_iterator_create(p->prestmts);
        while((subp = list_next(itr))) {
            assert(subp->magic == PRESTMT_MAGIC);
            list_append(stmt->u.foreach.stmts, makeStmt(subp));
        }
        list_iterator_destroy(itr);
        break;
    case STMT_IFON:
    case STMT_IFOFF:
        stmt->u.ifonoff.stmts = list_create((ListDelF) destroyStmt);
        itr = list_iterator_create(p->prestmts);
        while((subp = list_next(itr))) {
            assert(subp->magic == PRESTMT_MAGIC);
            list_append(stmt->u.ifonoff.stmts, makeStmt(subp));
        }
        list_iterator_destroy(itr);
        break;
    default:
        break;
    }
    return stmt;
}

static void _parse_hoststr(Device *dev, char *hoststr, char *flagstr)
{
    /* pipe device, e.g. "conman -j baytech0 |&" */
    if (strstr(hoststr, "|&") != NULL) {
        dev->data           = pipe_create(hoststr, flagstr);
        dev->destroy        = pipe_destroy;
        dev->connect        = pipe_connect;
        dev->disconnect     = pipe_disconnect;
        dev->finish_connect = NULL;
        dev->preprocess     = NULL;

    /* serial device, e.g. "/dev/ttyS0" */
    } else if (hoststr[0] == '/') {
        struct stat sb;

        if (stat(hoststr, &sb) == -1 || (!(sb.st_mode & S_IFCHR))) 
            _errormsg("serial device not found or not a char special file");

        dev->data           = serial_create(hoststr, flagstr);
        dev->destroy        = serial_destroy;
        dev->connect        = serial_connect;
        dev->disconnect     = serial_disconnect;
        dev->finish_connect = NULL;
        dev->preprocess     = NULL;

    /* tcp device, e.g. "cyclades0:2001" */
    } else {
        char *port = strchr(hoststr, ':');
        int n;

        if (port) {                 /* host='host:port', flags=NULL */
            *port++ = '\0';
        } else
            _errormsg("hostname is missing :port");

        n = _strtolong(port);       /* verify port number */
        if (n < 1 || n > 65535)
            _errormsg("port number out of range");
        dev->data           = tcp_create(hoststr, port, flagstr);
        dev->destroy        = tcp_destroy;
        dev->connect        = tcp_connect;
        dev->disconnect     = tcp_disconnect;
        dev->finish_connect = tcp_finish_connect;
        dev->preprocess     = tcp_preprocess;
    }
}

static void makeDevice(char *devstr, char *specstr, char *hoststr, 
                        char *flagstr)
{
    ListIterator itr;
    Device *dev;
    Spec *spec;
    int i;

    /* find that spec */
    spec = findSpec(specstr);
    if ( spec == NULL ) 
        _errormsg("device specification not found");

    /* make the Device */
    dev = dev_create(devstr);
    dev->specname = Strdup(specstr);
    dev->timeout = spec->timeout;
    dev->ping_period = spec->ping_period;

    _parse_hoststr(dev, hoststr, flagstr);

    /* create plugs (spec->plugs may be NULL) */
    dev->plugs = pluglist_create(spec->plugs);

    /* transfer remaining info from the spec to the device */
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
    Device *dev = dev_findbyname(devstr);

    if (dev == NULL) 
        _errormsg("unknown device");

    /* plugstr can be NULL - see comment in pluglist.h */
    switch (pluglist_map(dev->plugs, nodestr, plugstr)) {
        case EPL_DUPNODE:
            _errormsg("duplicate node");
        case EPL_UNKPLUG:
            _errormsg("unknown plug name");
        case EPL_DUPPLUG:
            _errormsg("plug already assigned");
        case EPL_NOPLUGS:
            _errormsg("more nodes than plugs");
        case EPL_NONODES:
            _errormsg("more plugs than nodes");
        default:
            break;
    }

    if (!conf_addnodes(nodestr))
        _errormsg("duplicate node name");
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
