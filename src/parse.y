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

	extern int yyline; /* For parse error reporting */
#define YYSTYPE char *  /*  The generic type returned by all parse matches */
#undef YYDEBUG          /* no debug code plese */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "powermand.h"
#include "action.h"
#include "server.h"
#include "listener.h"
#include "exit_error.h"
#include "log.h"
#include "pm_string.h"
#include "buffer.h"
#include "wrappers.h"

/* prototypes for parse handler routines */
static char *makeNode(char *s1, char *s2, char *s4, char *s5, char *s7);
static char *makeDevice(char *s2, char *s3, char *s4, char *s5);
static char *makeClusterName(char *s2);
static char *makePlugNameLine(char *s2);
static char *makeResetSec(char *s2);
static char *makePowerCycleSec(char *s2);
static char *makePowerOffSec(char *s2);
static char *makePowerOnSec(char *s2);
static char *makeUpdateNodesSec(char *s2);
static char *makeUpdatePlugsSec(char *s2);
static char *makeLogOutSec(char *s2);
static char *makeCheckLoginSec(char *s2);
static Interpretation *makeMapLine(char *s2, char *s3);
static List makeMapSecHead(Interpretation *s1);
static List makeMapSec(List s1, Interpretation *s2);
static char *makeScriptEl(Script_El_T mode, char *s2, char *s3, List s4);
static char *makeLogInSec(char *s2);
static char *makeInterp(char *s2);
static char *makeSpecSize(char *s2);
static char *makeSpecAll(char *s2);
static char *makeSpecOn(char *s2);
static char *makeSpecOff(char *s2);
static char *makeSpecType(char *s2);
static char *makeSpecName(char *s2);
static Spec *check_Spec();
static char *makeClientPort(char *s2);
static char *makeLogFile(char *s2, char *s3);
static char *makeDevTimeout(char *s2);
static char *makeUpdate(char *s2);
static char *makeInterDev(char *s2);
static char *makeTimeOut(char *s2);
static char *makeTCPWrappers();
static char *makeGlobalSec(char *s2);
extern void yyerror();

extern int yylex();
extern void yyerror();

 int yyline = 0;      /* For parse error reporting */
 char *conf = NULL;   /* Currently unused.  The return result of the parse */
 Globals *cheat;      /* A way of getting at the Globals structure */
 Spec *current_spec = NULL;  /* Holds a Spec as it is built */
 List current_script = NULL; /* Holds a script as it is built */
%}

/* BISON Declatrations */

%token TOK_B_GLOBAL
%token TOK_TCP_WRAPPERS
%token TOK_TIMEOUT
%token TOK_INTERDEV
%token TOK_UPDATE
%token TOK_DEV_TIMEOUT
%token TOK_LOG_FILE
%token TOK_CLIENT_PORT
%token TOK_B_SPEC
%token TOK_SPEC_NAME
%token TOK_SPEC_TYPE
%token TOK_OFF_STRING
%token TOK_ON_STRING
%token TOK_ALL_STRING
%token TOK_SIZE
%token TOK_STRING_INTERP_MODE
%token TOK_B_PM_LOG_IN
%token TOK_EXPECT
%token TOK_MAP
%token TOK_SEND
%token TOK_DELAY
%token TOK_E_PM_LOG_IN
%token TOK_B_PM_CHECK_LOGIN
%token TOK_E_PM_CHECK_LOGIN
%token TOK_B_PM_LOG_OUT
%token TOK_E_PM_LOG_OUT
%token TOK_B_PM_UPDATE_PLUGS
%token TOK_E_PM_UPDATE_PLUGS
%token TOK_B_PM_UPDATE_NODES
%token TOK_E_PM_UPDATE_NODES
%token TOK_B_PM_POWER_ON
%token TOK_E_PM_POWER_ON
%token TOK_B_PM_POWER_OFF
%token TOK_E_PM_POWER_OFF
%token TOK_B_PM_POWER_CYCLE
%token TOK_E_PM_POWER_CYCLE
%token TOK_B_PM_RESET
%token TOK_E_PM_RESET
%token TOK_E_SPEC
%token TOK_CLUSTER_NAME
%right TOK_DEVICE
%token TOK_E_GLOBAL
%token TOK_B_NODES
%token TOK_PLUG_NAME
%token TOK_NODE
%token TOK_E_NODES
%token TOK_STRING_VAL
%token TOK_TELNET
%token TOK_UNRECOGNIZED




%%
/* Grammar Rules */

configuration_file : configuration_sec node_sec {
	conf = $1;
}
;
configuration_sec : properties_sec devices_sec 
;
properties_sec : specifications_sec global_sec 
;
/* The above rules specify a four section grammar: */
/* 1) Spec list (device specifications) */
/* 2) global Config variables and properties */
/* 3) Devices list */
/* 4) Cluster (nodes list) */

/**************************************************************/
/* Section 1) Specifications                                  */
/*                                                            */
/*   As currently implemented, the "cheat" reference to a     */
/* Globals structure exists prior to entering the parser.     */
/* Thus Spec structures may be added to it as they are        */
/* defined.  When the code goes organic the specifications    */
/* list will have to be handed out of this section of code.   */
/*                                                            */
/**************************************************************/
specifications_sec : specifications_sec specification
                   | specification
;
specification : TOK_B_SPEC spec_name_line specification_item_list spec_script_list plug_name_sec TOK_E_SPEC {
	$$ = (char *)check_Spec();
}
;
spec_name_line : TOK_SPEC_NAME TOK_STRING_VAL {
	$$ = (char *)makeSpecName($2);
}
;
specification_item_list : specification_item_list specification_item 
		        | specification_item
;
specification_item : spec_type_line 
		   | off_string_line 
		   | on_string_line 
		   | all_string_line 
		   | spec_size_line 
                   | dev_timeout
		   | interp_line 
;
spec_type_line : TOK_SPEC_TYPE TOK_STRING_VAL {
	$$ = (char *)makeSpecType($2);
}
;
off_string_line : TOK_OFF_STRING TOK_STRING_VAL {
	$$ = (char *)makeSpecOff($2);
}
;
on_string_line : TOK_ON_STRING TOK_STRING_VAL {
	$$ = (char *)makeSpecOn($2);
}
;
all_string_line : TOK_ALL_STRING TOK_STRING_VAL {
	$$ = (char *)makeSpecAll($2);
}
;
spec_size_line : TOK_SIZE TOK_STRING_VAL {
	$$ = (char *)makeSpecSize($2);
}
;
dev_timeout : TOK_DEV_TIMEOUT TOK_STRING_VAL {
	$$ = (char *)makeDevTimeout($2);
}
;
interp_line : TOK_STRING_INTERP_MODE  TOK_STRING_VAL {
	$$ = (char *)makeInterp($2);
}
;
spec_script_list : spec_script_list spec_script 
                 | spec_script 
;
spec_script : TOK_B_PM_LOG_IN script_list TOK_E_PM_LOG_IN {
	$$ = (char *)makeLogInSec($2);
}
	| TOK_B_PM_CHECK_LOGIN script_list TOK_E_PM_CHECK_LOGIN {
	$$ = (char *)makeCheckLoginSec($2);
}
	| TOK_B_PM_LOG_OUT script_list TOK_E_PM_LOG_OUT {
	$$ = (char *)makeLogOutSec($2);
}
	| TOK_B_PM_UPDATE_PLUGS script_list TOK_E_PM_UPDATE_PLUGS {
	$$ = (char *)makeUpdatePlugsSec($2);
}
	| TOK_B_PM_UPDATE_NODES script_list TOK_E_PM_UPDATE_NODES {
	$$ = (char *)makeUpdateNodesSec($2);
}
	| TOK_B_PM_POWER_ON script_list TOK_E_PM_POWER_ON {
	$$ = (char *)makePowerOnSec($2);
}
	| TOK_B_PM_POWER_OFF script_list TOK_E_PM_POWER_OFF {
	$$ = (char *)makePowerOffSec($2);
}
	| TOK_B_PM_POWER_CYCLE script_list TOK_E_PM_POWER_CYCLE {
	$$ = (char *)makePowerCycleSec($2);
}
	| TOK_B_PM_RESET script_list TOK_E_PM_RESET {
	$$ = (char *)makeResetSec($2);
}
;
script_list : script_list script_el 
	    | script_el 
;
script_el : TOK_EXPECT TOK_STRING_VAL {
	$$ = (char *)makeScriptEl(EXPECT, $2, $2, NULL);
}
	| TOK_EXPECT TOK_STRING_VAL map_sec {
	$$ = (char *)makeScriptEl(EXPECT, $2, $2, (List)$3);
}
	| TOK_SEND TOK_STRING_VAL {
	$$ = (char *)makeScriptEl(SEND, $2, NULL, NULL);
}
	| TOK_DELAY TOK_STRING_VAL {
	$$ = (char *)makeScriptEl(DELAY, $2, NULL, NULL);
}
;
map_sec : map_sec map_line {
	$$ = (char *)makeMapSec((List)$1, (Interpretation *)$2);
}
	| map_line {
	$$ = (char *)makeMapSecHead((Interpretation *)$1);
}
;
map_line : TOK_MAP TOK_STRING_VAL TOK_STRING_VAL {
	$$ = (char *)makeMapLine($2, $3);
}
;
plug_name_sec : plug_name_sec plug_name_line 
	      | /* empty */
;
plug_name_line : TOK_PLUG_NAME TOK_STRING_VAL {
	       $$ = (char *)makePlugNameLine($2);
}
;
/**************************************************************/
/* Section 2) Global Config variables and properties */
global_sec : TOK_B_GLOBAL global_item_list TOK_E_GLOBAL {
	$$ = makeGlobalSec($2);
}
;
global_item_list : global_item_list global_item 
                 | global_item
;
global_item : cluster_name 
            | TCP_wrappers
	    | log_file 
	    | client_port 
            | timeout          /* select() loop pace */
            | interdev         /* delay batween device commands */
            | update           /* cluster update interval */
;
cluster_name : TOK_CLUSTER_NAME TOK_STRING_VAL {
	$$ = (char *)makeClusterName($2);
}
;
TCP_wrappers : TOK_TCP_WRAPPERS {
	$$ = (char *)makeTCPWrappers();
}
;
log_file : TOK_LOG_FILE TOK_STRING_VAL TOK_STRING_VAL {
	$$ = (char *)makeLogFile($2, $3);
}
;
client_port : TOK_CLIENT_PORT TOK_STRING_VAL {
	$$ = (char *)makeClientPort($2);
}
;
timeout : TOK_TIMEOUT TOK_STRING_VAL {
	$$ = (char *)makeTimeOut($2);
}
;
interdev : TOK_INTERDEV TOK_STRING_VAL {
	$$ = (char *)makeInterDev($2);
}
;
update : TOK_UPDATE TOK_STRING_VAL {
	$$ = (char *)makeUpdate($2);
}
;
/**************************************************************/
/* 3) Devices list */
devices_sec : devices_sec device_line
            | device_line
;
device_line : TOK_DEVICE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL %prec TOK_DEVICE {
	$$ = (char *)makeDevice($2, $3, $4, $5);
}
;
/**************************************************************/
/* 4) Cluster (nodes list) */
node_sec : TOK_B_NODES node_list TOK_E_NODES 
;
node_list : node_list node_line  
	  | node_line 
;
node_line : TOK_NODE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL {
	$$ = (char *)makeNode($2, $3, $4, $5, $6);
}
	| TOK_NODE TOK_STRING_VAL TOK_STRING_VAL TOK_STRING_VAL {
	$$ = (char *)makeNode($2, $3, $4, NULL, NULL);
}
;

%%


/* 
 *   Virtually everything that follows is in one-to-one correspondence
 * with something from the config file.  The names match.  It's easy to 
 * tell.  parse_config_file(0 is the root of the parser.  It's the
 * only function the outside world needs to see.
 */
int 
parse_config_file (void)
{
	yyparse();

	return 0;
} 

char *
makeGlobalSec(char *s2)
{
	if( cheat->cluster == NULL )
		exit_msg("Missing cluster name");
	if( cheat->listener->port == NO_PORT )
		exit_msg("Missing Listener port");
	return NULL;
}

char *
makeClusterName(char *s2)
{
	if( cheat->cluster != NULL )
		exit_msg("Cluster name %s already encountered", 
			get_String(cheat->cluster->name));
	cheat->cluster = make_Cluster(s2);
	return s2;
}

char *
makeTCPWrappers()
{
	cheat->TCP_wrappers = TRUE;
	return NULL;
}

char *
makeLogFile(char *s2, char *s3)
{
	int n;
	int level;

	assert( s2 != NULL );

/* gets the "log" fd, prints a timestamp */
/* log level 0 is the most restrictive */
	n = sscanf(s3, "%d", &level);
	if ( n != 1)
		level = 0;
	make_Log(s2, level); /* exits if already initialized */
	return s2;
}

char *
makeClientPort(char *s2)
{
	int n;
	int port;

	if( cheat->listener->port != NO_PORT )
		exit_msg("Listener port %d already encountered", cheat->listener->port);

	n = sscanf(s2, "%d", &port);
	if ( n == 1)
	{
		cheat->listener->port = port;
	}
	else
	{
		exit_msg("Unable to identify port %s", s2);
	}
	return s2;
}

char *
makeTimeOut(char *s2)
{


	set_tv( &(cheat->timeout_interval), s2);
	return s2;
}

char *
makeInterDev(char *s2)
{


	set_tv( &(cheat->interDev), s2);
	return s2;
}

char *
makeUpdate(char *s2)
{


	assert( cheat->cluster != NULL );
	set_tv( &(cheat->cluster->update_interval), s2);
	return s2;
}

/*
 * We've got a whole Spec now so do some sanity checks.
 */
Spec *
check_Spec()
{
	Spec *this_spec;
	int i;
	char *name;

/* 
 * perahps some commands are unimplemented.  Put empty lists in their
 * array spots so other routines don't faulter.
 */
	for (i = 0; i < current_spec->num_scripts; i++)
	{
		if( current_spec->scripts[i] == NULL )
		{
			current_spec->scripts[i] = list_create(free_Spec_El);
		}
	}
	name = get_String(current_spec->name);
	if( current_spec->type == NO_DEV )
		exit_msg("Missing type for specification %s", name);
	if( current_spec->off == NULL )
		exit_msg("Missing Off string for specification %s", name);
	if( current_spec->on == NULL )
		exit_msg("Missing On string for specification %s", name);
	if( current_spec->all == NULL )
		exit_msg("Missing All string for specification %s", name);
	if( (current_spec->type != PMD_DEV) && (current_spec->size == 0) )
		exit_msg("Missing Size field for specification %s", name);
	if( current_spec->mode == NO_MODE )
		exit_msg("Missing interpretation mode field for specification %s", name);

	list_append(cheat->specs, current_spec);
	this_spec = current_spec;
	current_spec = NULL;
/* I haven't set up a good way to sanity check struct timeval fields */
/* I don't yet use the return value. */
	return this_spec;
}

char *
makeSpecName(char *s2)
{
/* 
 *   N.B. The Specification's name line must be before all other
 * specification info.
 */
	if( current_spec != NULL )
		exit_msg("There is already a specification in progress");
	current_spec = make_Spec(s2);
	return s2;
}
char *
makeSpecType(char *s2)
{
	int n;


	if( current_spec->type != NO_DEV )
		exit_msg("Specification type already seen");
	if( (n = strncmp(s2, "TCP", 3)) == 0)
		current_spec->type = TCP_DEV;
	if( (n = strncmp(s2, "PMD", 3)) == 0)
		current_spec->type = PMD_DEV;
	if( (n = strncmp(s2, "TTY", 3)) == 0)
		current_spec->type = TTY_DEV;
	if( (n = strncmp(s2, "Telnet", 6)) == 0)
		current_spec->type = TELNET_DEV;
	if( (n = strncmp(s2, "SNMP", 4)) == 0)
		current_spec->type = SNMP_DEV;
	return s2;
}
char *
makeSpecOff(char *s2)
{
	if( current_spec->off != NULL )
		exit_msg("Off string %s for this specification already seen", 
				get_String(current_spec->off));
	current_spec->off = make_String(s2);
	return s2;
}
char *
makeSpecOn(char *s2)
{

	if( current_spec->on != NULL )
		exit_msg("On string %s for this specification already seen", 
				get_String(current_spec->on));
	current_spec->on = make_String(s2);
	return s2;
}
char *
makeSpecAll(char *s2)
{

	if( current_spec->all != NULL )
		exit_msg("All string %s for this specification already seen", 
				get_String(current_spec->all));
	current_spec->all = make_String(s2);
	return s2;
}
char *
makeSpecSize(char *s2)
{
	int size = 0;
	int n;
	int i;

	if( current_spec->type == PMD_DEV )
		exit_msg("PowerMan device types may not list a size");
	if( current_spec->size != 0 )
		exit_msg("Size field already seen for this specification");
	n = sscanf(s2, "%d", &size);
	if( n != 1) exit_msg("Failed to interpret size \"%s\"", s2);
	assert( size >= 0 );
	current_spec->size = size;
	/* PMD_DEV devices don't know their size yet */
	/* allow for size == 0 and set plugname to NULL */
	if( size == 0 )
		current_spec->plugname = NULL;
	else
	  {
		current_spec->plugname = (String *)Malloc(size * sizeof(String));
		for (i = 0; i < size; i++)
		  current_spec->plugname[i] = NULL;
	  }
	return s2;
}

char *
makeDevTimeout(char *s2)
{


	set_tv( &(current_spec->timeout), s2);
	return s2;
}

char *
makeInterp(char *s2)
{
	int n;
	int len = strlen("LITERAL");
	

	if( current_spec->mode != NO_MODE )
		exit_msg("Interpretation mode field already seen for this specification");
	
	if ( (n = strncmp(s2, "LITERAL", len)) == 0 )
	{
		current_spec->mode = LITERAL;
	}
	else if ( (n = strncmp(s2, "REGEX", len)) == 0 )
	{
		current_spec->mode = REGEX;
	}
	else
	{
		exit_msg("Illegal string interpretation mode in config file");
	}
	return s2;
}

char *
makeLogInSec(char *s2)
{

	if( current_spec->scripts[PM_LOG_IN] != NULL )
		exit_msg("Log in script already seen for this specification");

	current_spec->scripts[PM_LOG_IN] = current_script;
	current_script = NULL;
	return s2;
}
char *
makeScriptEl(Script_El_T mode, char *s2, char *s3, List s4)
{
	Spec_El *specl;
	int i;
	int len;
	bool found = FALSE;
	char buf1[MAX_BUF];
	char buf2[MAX_BUF];

/*
 * Four kinds of thing could be arriving.  An EXPECT with an 
 * interpretation will have non-NULL values in all positions.
 * An EXPECT without an interpretation will have s3 non-Null
 * and s4 equal to NULL.  A SEND will have s3 and s4 NULL,
 * but it may have a "%s" in it.  If it does the "found" branch
 * below insures that it is still in place.   A DELAY will 
 * have s3 and s4 both NULL.  The values are passed on to
 * make_Spec_El to be constructed.
 */
	len = strlen(s2);
	for (i = 0; i < len; i++)
		if (s2[i] == '%') found = TRUE;
	if (found)
		sprintf(buf1, s2, "%s");
	else
		sprintf(buf1, s2);
	sprintf(buf2, s3);
	specl = make_Spec_El(mode, buf1, buf2, s4);
	if (current_script == NULL)
		current_script = list_create(free_Spec_El);
	list_append(current_script, specl);
	return s2;
}

List
makeMapSec(List s1, Interpretation *s2)
{
	list_append(s1, (void *)s2);
	return s1;
}

List
makeMapSecHead(Interpretation *s1)
{
	List map;

	map = list_create(free_Interp);
	list_append(map, (void *)s1);
	return map;
}

Interpretation *
makeMapLine(char *s2, char *s3)
{
	Interpretation *interp;
	int n;

	interp = make_Interp(s3);
	n = sscanf(s2, "%d", &(interp->match_pos));
	if( n != 1) exit_msg("Could not interpret %s as an integer", s2);
	return interp;
}

char *
makeCheckLoginSec(char *s2)
{

	if( current_spec->scripts[PM_CHECK_LOGIN] != NULL )
		exit_msg("Check log in script already seen for this specification");

	current_spec->scripts[PM_CHECK_LOGIN] = current_script;
	current_script = NULL;
	return s2;
}
char *
makeLogOutSec(char *s2)
{

	if( current_spec->scripts[PM_LOG_OUT] != NULL )
		exit_msg("Log out script already seen for this specification");

	current_spec->scripts[PM_LOG_OUT] = current_script;
	current_script = NULL;
	return s2;
}
char *
makeUpdatePlugsSec(char *s2)
{

	if( current_spec->scripts[PM_UPDATE_PLUGS] != NULL )
		exit_msg("Update plugs script already seen for this specification");

	current_spec->scripts[PM_UPDATE_PLUGS] = current_script;
	current_script = NULL;
	return s2;
}
char *
makeUpdateNodesSec(char *s2)
{

	if( current_spec->scripts[PM_UPDATE_NODES] != NULL )
		exit_msg("Update nodes script already seen for this specification");

	current_spec->scripts[PM_UPDATE_NODES] = current_script;
	current_script = NULL;
	return s2;
}
char *
makePowerOnSec(char *s2)
{

	if( current_spec->scripts[PM_POWER_ON] != NULL )
		exit_msg("Power on script already seen for this specification");

	current_spec->scripts[PM_POWER_ON] = current_script;
	current_script = NULL;
	return s2;
}
char *
makePowerOffSec(char *s2)
{

	if( current_spec->scripts[PM_POWER_OFF] != NULL )
		exit_msg("Power off script already seen for this specification");

	current_spec->scripts[PM_POWER_OFF] = current_script;
	current_script = NULL;
	return s2;
}
char *
makePowerCycleSec(char *s2)
{

	if( current_spec->scripts[PM_POWER_CYCLE] != NULL )
		exit_msg("Power cycle script already seen for this specification");

	current_spec->scripts[PM_POWER_CYCLE] = current_script;
	current_script = NULL;
	return s2;
}
char *
makeResetSec(char *s2)
{

	if( current_spec->scripts[PM_RESET] != NULL )
		exit_msg("Reset script already seen for this specification");

	current_spec->scripts[PM_RESET] = current_script;
	current_script = NULL;
	return s2;
}
char *
makePlugNameLine(char *s2)
{
	int i = 0;

	if( current_spec == NULL ) 
		exit_msg("Plug Name Line outside of Spec");
	if( current_spec->plugname == NULL ) 
		exit_msg("Plug Name Line without plugname array");
	if( current_spec->size <= 0 ) 
		exit_msg("Plug Name Line with spec size = %d", current_spec->size);
	while( (current_spec->plugname[i] != NULL) && (i < current_spec->size) )
	{
		i++;
	}
	if( i == current_spec->size )
		exit_msg("More than %d Plug Name Lines", current_spec->size);
	current_spec->plugname[i] = make_String(s2);
	return s2;
}


/*
 * This and makeNode() are the only interesting pieces of the
 * parser.  We've hit a Device line.  The four parameters are the
 * proper name for the Device (s2), the name of the Spec to use (s3),
 * the (internet) host name of the Device (s4), and the TCP port
 * on which it is listening (s5).  From these facts we can fully 
 * build a Device structure.
 */   
char *
makeDevice(char *s2, char *s3, char *s4, char *s5)
{
	Device *dev;
	Spec *spec;
	reg_syntax_t cflags = REG_EXTENDED;
	int i;
	List *scripts;
	Plug *plug;

/* find that spec */
	spec = list_find_first(cheat->specs, match_Spec, s3);
	if ( spec == NULL ) exit_msg("Device specification %s not found", s3);

/* make the Device */
	dev = make_Device(s2);
	dev->spec = spec;
	dev->type = spec->type;
	dev->timeout = spec->timeout;
/* set up the host name and port */
	switch(dev->type)
	{
	case TCP_DEV :
		dev->devu.tcpd.host = make_String(s4);
		dev->devu.tcpd.service = make_String(s5);
		dev->num_plugs = spec->size;
		for (i = 0; i < spec->size; i++)
		{
			plug = make_Plug(get_String(spec->plugname[i]));
			list_append(dev->plugs, (void *)plug);
		}
		break;
	case PMD_DEV :
		dev->devu.pmd.host = make_String(s4);
		dev->devu.pmd.service = make_String(s5);
		break;
	default :
		exit_msg("That device type is not implemented yet");
	}
/* begin transfering info from the Spec to the Device */
	dev->all = copy_String(spec->all);
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	Regcomp( &(dev->on_re), get_String(spec->on), cflags);
	Regcomp( &(dev->off_re), get_String(spec->off), cflags);

	dev->prot = (Protocol *)Malloc(sizeof(Protocol));
	dev->prot->num_scripts = NUM_SCRIPTS;
	scripts = (List *)Malloc(dev->prot->num_scripts*sizeof(List));
	dev->prot->scripts = scripts;
	dev->prot->mode = spec->mode;
	
/* 
 *   Each script needs to be transferred and any Interpretations need
 * to be set up.  The make_Script_El() call transforms the EXPECT strings
 * into compiled RegEx recognizers. 
 */
	for (i = 0; i < dev->prot->num_scripts; i++)
	{
		ListIterator script;
		Spec_El *specl;
		Script_El *script_el;
		Interpretation *interp;
		Interpretation *new;
		List map;
		ListIterator map_i;

		scripts[i] = list_create(free_Script_El);
		script = list_iterator_create(spec->scripts[i]);
		while( (specl = (Spec_El *)list_next(script)) )
		{
			if( specl->map == NULL )
				map = NULL;
			else
			{
				map = list_create(free_Interp);
				map_i = list_iterator_create(specl->map);
				while( (interp = (Interpretation *)list_next(map_i)) )
				{
					new = make_Interp(get_String(interp->plug_name));
					new->match_pos = interp->match_pos;
					list_append(map, new);
				}
				list_iterator_destroy(map_i);
			}
			script_el = make_Script_El(specl->type, specl->string1, specl->string2, map, specl->tv);
			list_append(scripts[i], script_el);
		}
	}

	list_append(cheat->devs, dev);
	return s2;
}

/*
 * s1 : literal "node"
 * s2 : node name
 * s3 : hard power state device
 * s4 : hard power state plug name
 * s5 : (optional) soft power state device
 * s6 : (optional) soft power state plug name
 */
static char *
makeNode(char *s2, char *s3, char *s4, char *s5, char *s6)
{
	int i;
	Node *node;
	Interpretation *interp;
	List script;
	ListIterator script_itr;
	Script_El *script_el;
	Plug *plug;

	cheat->cluster->num++;
	node = make_Node(s2);
	list_append(cheat->cluster->nodes, (void *)node);
/* find the device controlling this nodes plug */
	node->p_dev = list_find_first(cheat->devs, match_Device, s3);
	if( node->p_dev == NULL ) exit_msg("Failed to find device %s", s3);
/* PMD_DEV Plugs get created here, other Device types have Plugs      */
/* defined in their Spec, and they must be searched to match the node */
	switch( node->p_dev->type )
	{
	case PMD_DEV :
		node->p_dev->num_plugs++;
		plug = make_Plug(s4);
		list_append(node->p_dev->plugs, (void *)plug);
		plug->node = node;
		break;
	case TCP_DEV :
		plug = (Plug *)list_find_first(node->p_dev->plugs, match_Plug, (void *)s4);
		if( plug == NULL ) exit_msg("Can not locate plug %s on device %s", s4, s3);
		plug->node = node;
		break;
	default :
		exit_msg("That device type is not implemented yet");
	}

/* Some device support hard- and soft-power status.  If not a second */
/* pair of entries will be need for the soft power status device.    */ 
	if ( s5 == NULL)
	{
		assert(s6 == NULL);
		node->n_dev = node->p_dev;
	}
	else
	{
		assert(s6 != NULL);
		node->n_dev = list_find_first(cheat->devs, match_Device, s3);
		if( node->n_dev == NULL ) exit_msg("Failed to find device %s", s3);
		plug = list_find_first(node->n_dev->plugs, match_Plug, (void *)s6);
		plug->node = node;
	}

/* Finally an exhaustive search of the Interpretations in a device */
/* is required because this node will be the target of some        */
	for (i = 0; i < node->p_dev->prot->num_scripts; i++)
	{
		script = node->p_dev->prot->scripts[i];
		script_itr = list_iterator_create(script);
		while( (script_el = (Script_El *)list_next(script_itr)) )
		{
			switch( script_el->type )
			{
			case SEND :
			case DELAY :
				break;
			case EXPECT :
				if( script_el->s_or_e.expect.map == NULL ) continue;
				interp = (Interpretation *)list_find_first(script_el->s_or_e.expect.map, match_Interp, (void *)s4);
				if( interp != NULL )
					interp->node = node;
				break;
			case SND_EXP_UNKNOWN :
			default :
			}
		}
	}
	return s2;
}

/* Other support functions not directly invoked from parse rules */

void
yyerror()
{
	errno = 0;
	exit_msg("Parse error.  %s line %d", cheat->config_file, yyline);
}
