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
#define MAXDEV  128

static char ttyD[] = "/dev/ttyD";

int
main ( int argc, char **argv )
{
  int modem;
  int fd;
  int dev;
  char dev_name[MAXNAME];
  
  for (dev=0; dev < MAXDEV; dev++)
    {
      sprintf(dev_name, "%s%d", ttyD, dev);
      
      if((fd = open(dev_name, O_WRONLY | O_NDELAY)) >= 0)
	{
	  ioctl(fd, TIOCMGET, &modem);
	  if(modem & TIOCM_DSR)
	    printf("1");
	  else
	    printf("0");
	}
      else
	break;
    }
  printf("\n");
  return 0;
}

