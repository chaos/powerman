/*******************************************************************
 * $Id$
 * by Andrew C. Uselton <uselton2@llnl.gov> 
 * Copyright (C) 2000 Regents of the University of California
 * See ../DISCLAIMER
 * v. 0-0-1:  2001-07-11
 * v. 0-0-2:  2001-07-31
 ********************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#define TIOCM_DSR 0x100

#define MAXNAME 20

int
main ( int argc, char **argv)
{
  int modem;
  int fd;
  int exit = 1;

  if(strlen(argv[1]) < MAXNAME)
    if((fd = open(argv[1], O_WRONLY | O_NDELAY)) >= 0)
      {
	ioctl(fd, TIOCMGET, &modem);
	if(modem & TIOCM_DSR)
	  printf("On\n");
	else
	  printf("Off\n");
	exit = 0;
      }
  return exit;
}

