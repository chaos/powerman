#define SSH_LOGIN_PROMPT "SUNSP00144FEE320F login: "
#define SSH_PASSWD_PROMPT "Password: "

#define ILOM_PROMPT " -> "

#define ILOM_BANNER "\n\
Sun(TM) Integrated Lights Out Manager\n\
\n\
Version 2.0.2.3\n\
\n\
Copyright 2007 Sun Microsystems, Inc. All rights reserved.\n\
Use is subject to license terms.\n\
\n\
Warning: password is set to factory default.\n\n"

#define ILOM_PROP_SYS "\
  /SYS\n\
    Properties:\n\
        type = Host System\n\
        chassis_name = SUN FIRE X4140\n\
        chassis_part_number = 540-7618-XX\n\
        chassis_serial_number = 0226LHF-0823A600HM\n\
        chassis_manufacturer = SUN MICROSYSTEMS\n\
        product_name = SUN FIRE X4140\n\
        product_part_number = 602-4125-01\n\
        product_serial_number = 0826QAD075\n\
        product_manufacturer = SUN MICROSYSTEMS\n\
        power_state = %s\n\n\n"

#define ILOM_STOP_RESP "Stopping /SYS immediately\n\n"
#define ILOM_STOP_RESP2 "stop: Target already stopped\n\n"
#define ILOM_START_RESP "Starting /SYS\n\n"
#define ILOM_START_RESP2 "start: Target already started\n\n"

#define ILOM_CMD_INVAL "\
Invalid command 'foo' - type help for a list of commands.\n\n"

#define ILOM_HELP "\
The help command is used to view information about commands and targets\n\
\n\
Usage: help [-o|-output terse|verbose] [<command>|legal|targets]\n\
\n\
Special characters used in the help command are\n\
[]   encloses optional keywords or options\n\
<>   encloses a description of the keyword\n\
     (If <> is not present, an actual keyword is indicated)\n\
|    indicates a choice of keywords or options\n\
\n\
help targets  displays a list of targets\n\
help legal    displays the product legal notice\n\
\n\
Commands are:\n\
cd\n\
create\n\
delete\n\
exit\n\
help\n\
load\n\
reset\n\
set\n\
show\n\
start\n\
stop\n\
version\n\n"

#define ILOM_VERS "\
SP firmware 2.0.2.3\n\
SP firmware build number: 29049\n\
SP firmware date: Thu Feb 21 19:42:30 PST 2008\n\
SP filesystem version: 0.1.16\n\n"


int
main(int argc, char *argv[])
{
}
