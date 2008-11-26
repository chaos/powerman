/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2004-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

/* dli.c - mimic httpposer talking to a Digital Loggers Inc LPC */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "xread.h"

#define DLI_POST "\
<HTML><HEAD>\n\
\n\
<META HTTP-EQUIV=\"refresh\" content=\"0; URL=/index.htm\">\n\
\n\
</HEAD><BODY></BODY>\n"

#define DLI_GET "\
<html>\n\
<head>\n\
\n\
<META HTTP-EQUIV=\"Refresh\" CONTENT=\"300\">\n\
<title>Outlet Control  - Lite Power Controller</title></head>\n\
\n\
<body alink=\"#0000FF\" vlink=\"#0000FF\">\n\
<FONT FACE=\"Arial, Helvetica, Sans-Serif\">\n\
<table width=\"100%%\" cellspacing=0 cellpadding=0>\n\
<tr>\n\
<td valign=top width=\"17%%\" height=\"100%%\">\n\
\n\
    <!-- menu -->\n\
    <table width=\"100%%\" height=\"100%%\" align=center border=0 cellspacing=1 cellpadding=0>\n\
    <tr><td valign=top bgcolor=\"#F4F4F4\">\n\
    <table width=\"100%%\" cellpadding=1 cellspacing=5>\n\
\n\
    <tr><td align=center>\n\
    <table><tr><td><a href=\"http://www.webpowerswitch.com/\"><img src=logo.gif width=195 height=65 border=0></a></td>\n\
    <td><b><font size=-1>Ethernet Power Controller</font></b></td></tr></table>\n\
    <hr>\n\
    </td></tr>\n\
\n\
\n\
\n\
<tr><td nowrap><b><a href=\"/index.htm\">Outlet Control</a></b></td></tr>\n\
<tr><td nowrap><b><a href=\"/admin.htm\">Setup</a></b></td></tr>\n\
\n\
\n\
\n\
<tr><td nowrap><b><a href=\"/ap.htm\">AutoPing</a></b></td></tr>\n\
<tr><td nowrap><b><a href=\"/logout\">Logout</a></b></td></tr>\n\
<tr><td nowrap><b><a href=\"http://www.digital-loggers.com/lpchelp.html\">Help</a></b></td></tr>\n\
\n\
<tr><td><hr></td></tr>\n\
\n\
\n\
<tr><td><b><a href=\"http://www.digital-loggers.com\">Link 1</a></b></td></tr>\n\
\n\
<tr><td><b><a href=\"http://www.digital-loggers.com\">Link 2</a></b></td></tr>\n\
\n\
<tr><td><b><a href=\"http://www.digital-loggers.com\">Link 3</a></b></td></tr>\n\
\n\
<tr><td><b><a href=\"http://www.digital-loggers.com\">Link 4</a></b></td></tr>\n\
\n\
\n\
    </table>\n\
    </td></tr>\n\
\n\
\n\
    <tr><td valign=bottom height=\"100%%\" bgcolor=\"#F4F4F4\">\n\
    <small>Version 1.2.1 (Aug 23 2007 / 20:14:33) 1555A5A1-D43E3FC2</small>\n\
    </td></tr>\n\
    <tr><td valign=bottom height=\"100%%\" bgcolor=\"#F4F4F4\">\n\
    <small>S/N:0000130175</small>\n\
    </td></tr>\n\
\n\
    </table>\n\
    <!-- /menu -->\n\
\n\
</td>\n\
<td valign=top width=\"83%%\">\n\
\n\
    <!-- heading table -->\n\
    <table width=\"100%%\" align=center border=0 cellspacing=1 cellpadding=3>\n\
\n\
        <tr>\n\
        <th bgcolor=\"#DDDDFF\" align=left>\n\
        Controller: Lite Power Controller\n\
        </th>\n\
        </tr>\n\
\n\
        <tr bgcolor=\"#FFFFFF\" align=left>\n\
        <td>\n\
        Uptime:     0:35:01 <!-- 2101s up -->\n\
        </td>\n\
        </tr>\n\
\n\
    </table>\n\
    <!-- /heading table -->\n\
\n\
    <br>\n\
\n\
    <!-- individual control table -->\n\
    <table width=\"100%%\" align=center border=0 cellspacing=1 cellpadding=3>\n\
\n\
        <tr>\n\
        <th bgcolor=\"#DDDDFF\" colspan=5 align=left>\n\
        Individual Control\n\
        </th>\n\
        </tr>\n\
\n\
        <!-- heading rows -->\n\
        <tr bgcolor=\"#DDDDDD\">\n\
        <th>#</th>\n\
        <th align=left>Name</th>\n\
        <th align=left>State</th>\n\
        <th align=left colspan=2>Action</th>\n\
        </tr>\n\
        <!-- /heading rows -->\n\
\n\
\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>1</td>\n\
<td>Outlet 1</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?1=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?1=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>2</td>\n\
<td>Outlet 2</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?2=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?2=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>3</td>\n\
<td>Outlet 3</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?3=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?3=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>4</td>\n\
<td>Outlet 4</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?4=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?4=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>5</td>\n\
<td>Outlet 5</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?5=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?5=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>6</td>\n\
<td>Outlet 6</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?6=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?6=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>7</td>\n\
<td>Outlet 7</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?7=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?7=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
<tr bgcolor=\"#F4F4F4\"><td align=center>8</td>\n\
<td>Outlet 8</td><td>\n\
<b><font color=red>%s</font></b></td><td>\n\
<a href=outlet?8=ON>Switch ON</a>\n\
</td><td>\n\
<!-- <a href=outlet?8=CCL>Cycle</a> -->\n\
</td></tr>\n\
\n\
\n\
    </table>\n\
    <!-- /individual control table -->\n\
\n\
    <br>\n\
\n\
<table width=\"100%%\" align=center border=0 cellspacing=1 cellpadding=3>\n\
<tr><th bgcolor=\"#DDDDFF\" align=left>Master Control</th></tr>\n\
<tr><td bgcolor=\"#F4F4F4\" align=left><a href=outlet?a=OFF>All outlets OFF</a></td></tr>\n\
<tr><td bgcolor=\"#F4F4F4\" align=left><a href=outlet?a=ON>All outlets ON</a></td></tr>\n\
<tr><td bgcolor=\"#F4F4F4\" align=left><a href=outlet?a=CCL>Cycle all outlets</a></td></tr>\n\
\n\
<tr><td align=center>Sequence delay: 1 sec.</td></tr>\n\
\n\
</table>\n\
\n\
\n\
</td>\n\
</tr>\n\
</table>\n\
\n\
</body>\n\
</html>\n"

static void
prompt_loop(void)
{
    char buf[128], tmp[32];
    char plug[8][4];
    int num_plugs = 8;
    int plug_origin = 1;
    int i;
    int authenticated = 0;

    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "OFF");

    for (;;) {
        if (xreadline("httppower> ", buf, sizeof(buf)) == NULL)
            break;
        if (!strcmp(buf, "help")) {
            printf("Commands are:\n");
            printf(" auth admin:admin\n");
            printf(" get\n");
            printf(" post outlet [1-8]=ON|OFF\n");
        } else if (!strcmp(buf, "auth admin:admin")) {
            authenticated = 1;
        } else if (!strcmp(buf, "get")) {
            if (!authenticated)
                goto err;
            printf(DLI_GET, plug[0], plug[1], plug[2], plug[3],
                            plug[4], plug[5], plug[6], plug[7]);
        } else if (sscanf(buf, "post outlet %d=%s", &i, tmp) == 2) {
            if (!authenticated)
                goto err;
            if (i < plug_origin || i >= num_plugs + plug_origin)
                goto err;
            if (!strcmp(tmp, "ON"))
                strcpy(plug[i - plug_origin], "ON");
            else if (!strcmp(tmp, "OFF"))
                strcpy(plug[i - plug_origin], "OFF");
            else
                goto err;
            printf(DLI_POST);
        } else
            goto err;

        continue;    
err: 
        printf("Error\n");
    }
}

int
main(int argc, char *argv[])
{
    prompt_loop();
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
