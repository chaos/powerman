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

#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PERMS 0
#define BOX_SIZE 10
#define NUM_BOXES 3
#define BUF_SIZE 1024
#define MAX_RETRIES 1000

char tty[]      = "/dev/ttyS0";
int fd;
char log_file[] = "/tmp/vicebox.log";
FILE *lfd;

typedef struct port_struct 
{
  int present;
  int state;
}port_tp;

port_tp icebox[NUM_BOXES][BOX_SIZE];

void
init_box(port_tp *box)
{
  int i;

  for(i = 0; i < BOX_SIZE; i++)
    {
      box[i].present = 1;
      box[i].state   = 0;
    }
}

int strnlen(char *buf, int max);

void
log(char *string)
{
  fprintf(lfd, "vicebox: %s.\n", string);
  fflush(lfd);
}

void
write_line(char *buf)
{
  int len;
  int count;
  int retry = MAX_RETRIES;
  char msg[BUF_SIZE];

  len = strnlen(buf, BUF_SIZE);
  sprintf(msg, "say: %s", buf);
  log(msg);
  buf[len++] = '\r';
  buf[len++] = '\n';
  while (retry && *buf && len)
    {
      if((count = write(fd, buf, 1)) == 1)
	{
	  buf++;
	  len--;
	  if (retry != MAX_RETRIES)
	    {
	      sprintf(msg, "%d retries on write", MAX_RETRIES - retry);
	      log(msg);
	      retry = MAX_RETRIES;
	    }
	}
      else
	{
	  retry--;
	}
    }
}

void
show_file_status_flags()
{
  int f;

  if((f = fcntl(fd, F_GETFL)) < 0)
    {
      ;
      printf("Couldn't get file status (result = %d) (error = %s)\n", f, strerror(errno));
      exit(1);
    }
  printf("File Status Flags:\n");
  printf("O_RDONLY = %d\n", f & O_RDONLY);
  printf("O_WRONLY = %d\n", f & O_WRONLY);
  printf("O_RDWR = %d\n", f & O_RDWR);
  printf("O_APPEND = %d\n", f & O_APPEND);
  printf("O_NONBLOCK = %d\n", f & O_NONBLOCK);
  printf("O_SYNC = %d\n", f & O_SYNC);
  printf("O_ASYNC = %d\n", f & O_ASYNC);

}

void
terminal_control_attributes(struct termios term)
{
  printf("Terminal Control Attributes:\n");
  printf("    c_iflag:\n");
  printf("IGNBRK = %d\n", term.c_iflag & IGNBRK);
  printf("BRKINT = %d\n", term.c_iflag & BRKINT);
  printf("IGNPAR = %d\n", term.c_iflag & IGNPAR);
  printf("PARMRK = %d\n", term.c_iflag & PARMRK);
  printf("INPCK = %d\n", term.c_iflag & INPCK);
  printf("ISTRIP = %d\n", term.c_iflag & ISTRIP);
  printf("INLCR = %d\n", term.c_iflag & INLCR);
  printf("IGNCR = %d\n", term.c_iflag & IGNCR);
  printf("ICRNL = %d\n", term.c_iflag & ICRNL);
  printf("IUCLC = %d\n", term.c_iflag & IUCLC);
  printf("IXON = %d\n", term.c_iflag & IXON);
  printf("IXANY = %d\n", term.c_iflag & IXANY);
  printf("IXOFF = %d\n", term.c_iflag & IXOFF);
  printf("IMAXBEL = %d\n", term.c_iflag & IMAXBEL);
  printf("    c_oflag:\n");
  printf("OPOST = %d\n", term.c_oflag & OPOST);
  printf("OLCUC = %d\n", term.c_oflag & OLCUC);
  printf("ONLCR = %d\n", term.c_oflag & ONLCR);
  printf("OCRNL = %d\n", term.c_oflag & OCRNL);
  printf("ONOCR = %d\n", term.c_oflag & ONOCR);
  printf("ONLRET = %d\n", term.c_oflag & ONLRET);
  printf("OFILL = %d\n", term.c_oflag & OFILL);
  printf("OFDEL = %d\n", term.c_oflag & OFDEL);
  printf("NLDLY = %d\n", term.c_oflag & NLDLY);
  printf("CRDLY = %d\n", term.c_oflag & CRDLY);
  printf("TABDLY = %d\n", term.c_oflag & TABDLY);
  printf("BSDLY = %d\n", term.c_oflag & BSDLY);
  printf("VTDLY = %d\n", term.c_oflag & VTDLY);
  printf("FFDLY = %d\n", term.c_oflag & FFDLY);
  printf("    c_cflag:\n");
  printf("CSIZE = %d\n", term.c_cflag & CSIZE);
  printf("CSTOPB = %d\n", term.c_cflag & CSTOPB);
  printf("CREAD = %d\n", term.c_cflag & CREAD);
  printf("PARENB = %d\n", term.c_cflag & PARENB);
  printf("PARODD = %d\n", term.c_cflag & PARODD);
  printf("HUPCL = %d\n", term.c_cflag & HUPCL);
  printf("CLOCAL = %d\n", term.c_cflag & CLOCAL);
  printf("CIBAUD = %d\n", term.c_cflag & CIBAUD);
  printf("CRTSCTS = %d\n", term.c_cflag & CRTSCTS);
  printf("    c_lflag:\n");
  printf("ISIG = %d\n", term.c_lflag & ISIG);
  printf("ICANON = %d\n", term.c_lflag & ICANON);
  printf("XCASE = %d\n", term.c_lflag & XCASE);
  printf("ECHO = %d\n", term.c_lflag & ECHO);
  printf("ECHOE = %d\n", term.c_lflag & ECHOE);
  printf("ECHOK = %d\n", term.c_lflag & ECHOK);
  printf("ECHONL = %d\n", term.c_lflag & ECHONL);
  printf("ECHOCTL = %d\n", term.c_lflag & ECHOCTL);
  printf("ECHOPRT = %d\n", term.c_lflag & ECHOPRT);
  printf("ECHOKE = %d\n", term.c_lflag & ECHOKE);
  printf("FLUSHO = %d\n", term.c_lflag & FLUSHO);
  printf("NOFLSH = %d\n", term.c_lflag & NOFLSH);
  printf("TOSTOP = %d\n", term.c_lflag & TOSTOP);
  printf("PENDIN = %d\n", term.c_lflag & PENDIN);
  printf("IEXTEN = %d\n", term.c_lflag & IEXTEN);
}

int
main(int argc, char **argv)
{
  struct flock lock;
  struct termios term, new_term;
  int i, j;
  char com[BUF_SIZE];
  int len;
  char buf[BUF_SIZE];
  char msg[BUF_SIZE];
  int offset;
  int c;
  int box_num;
  int port;
  int all;
  port_tp *box;
  int result;
  int new_state;

  if((lfd = fopen(log_file, "a")) == NULL)
    {
      printf("%s: Couldn't open %s (error = %s)\n", argv[0], log_file, strerror(errno));
      exit(1);
    }
  log("Vicebox v. 0.1:  Starting up.");
  for(i = 0; i < NUM_BOXES; i++)
    {
      init_box(icebox[i]);
      /* I just do this so I have at least one unfull box */
      icebox[0][8].present = 0;
      icebox[0][9].present = 0;
    }
  if((fd = open(tty, O_RDWR | O_SYNC, PERMS)) < 0)
    {
      printf("%s: Couldn't open %s (error = %s)\n", argv[0], tty, strerror(errno));
      exit(1);
    }
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_whence = SEEK_SET;
  if((result = fcntl(fd, F_SETLK, &lock)) <0)
    {
      printf("%s: Couldn't get lock (error = %s)\n", argv[0], strerror(errno));
      exit(1);
    }
  if((result = tcgetattr(fd, &term)) < 0)
    {
      printf("%s: Couldn't get terminal control attributes (error = %s)\n", argv[0], strerror(errno));
      exit(1);
    }
  terminal_control_attributes(term);
  new_term.c_iflag = IGNBRK | IGNPAR | INPCK;
  new_term.c_oflag = 0;
  new_term.c_cflag = CSIZE | CREAD | CLOCAL;
  new_term.c_lflag = 0;
  if((result = tcsetattr(fd, TCSANOW, &new_term)) < 0)
    {
      printf("%s: Couldn't set terminal control attributes (error = %s)\n", argv[0], strerror(errno));
      exit(1);
    }
  while (1)
    {
      for(i = 0; i < BUF_SIZE; i++)
	{
	  com[i] = 0;
	  buf[i] = 0;
	  msg[i] = 0;
	}
      if((len = read(fd, com, BUF_SIZE)) <= 0) 
	{
	  log("error on read");
	  continue;
	}
      while ((len > 0) && ((com[len - 1] == '\r') || (com[len - 1] == '\n'))) 
	{
	  len--;
	  com[len] = 0;
	}
      sprintf(msg, "hear: %s", com);
      log(msg);
      if (len == 0) continue;
      if(com[0] != 'c')
	{
	  log ("command does not start with 'c'");
	  write_line("ERROR 5");
	  continue;
	}
      i = 2;
      while ((i < len) && isdigit(com[i]))
        i++;
      if (i >= len - 1)
	{
	  log ("missing command");
	  write_line("ERROR 5");
	  continue;
	}
      c = com[i];
      com[i] = 0;
      box_num = atoi(com + 1);
      com[i] = c;
      box = icebox[box_num];
      j = i + 1;
      while ((j < len) && ! isdigit(com[j])) j++;
      all = 0;
      port = 0;
      if (j == len) all = 1;
      else if (com[j] == '*') 
	{
	  all = 1;
	  com[j] = 0;
	}
      else if (isdigit(com[j])) 
	{
	  port = com[j] - '0';
	  com[j] = 0;
	}
      else
	{
	  log("character after command must be digit or '*'");
	  write_line("ERROR 5");
	  continue;
	}
      if (strncmp(com + i, "a", 1) == 0)
	{
	  sprintf(buf, "%s", com + i);
	}
      else if (strncmp(com + i, "v", 1) == 0)
	{
	  sprintf(buf, "Ice Emu v0.1 (c)University of California Regents");
	}
      if ((strncmp(com + i, "ph", 2) == 0) ||
	  (strncmp(com + i, "pl", 2) == 0))
	{
	  new_state = 0;
	  if (*(com + i + 1) == 'h') new_state = 1; 
	  if (all)
	    {
	      for (port = 0; port < BOX_SIZE; port++)
		{
		  box[port].state = new_state;
		}
	    }
	  else
	    {
	      box[port].state = new_state;
	    }
	  sprintf(buf, "OK");
	}
      else if ((strncmp(com + i, "rp", 2) == 0) ||
	       (strncmp(com + i, "rb", 2) == 0) ||
	       (strncmp(com + i, "rd", 2) == 0))
	{
	  sprintf(buf, "OK");
	}
      else if ((strncmp(com + i, "ps", 2) == 0) ||
	       (strncmp(com + i, "ns", 2) == 0))
	{
	  offset = 0;
	  if (all)
	    {
	      for (port = 0; port < BOX_SIZE - 1; port++)
		/* note that port 9 is done below */
		{
		  sprintf(buf + offset, "N%d:%d ", port, box[port].state);
		  offset = strnlen(buf, BUF_SIZE);
		}
	    }
	  sprintf(buf + offset, "N%d:%d", port, box[port].state);
	}
      else if ((strncmp(com + i, "ts", 2) == 0) ||
	       (strncmp(com + i, "tsf", 3) == 0))
	{
	  offset = 0;
	  if (all)
	    {
	      for (port = 0; port < BOX_SIZE; port++)
		{
		  sprintf(buf + offset, "N%d:18,22 ", port);
		  offset = strnlen(buf, BUF_SIZE);
		}
	    }
	  else
	    {
	      sprintf(buf + offset, "N%d:18,22 ", port);
	      offset = strnlen(buf, BUF_SIZE);
	    }
	}
      write_line(buf);
    } /* end of while (1) loop */
  if((result = tcsetattr(fd, TCSANOW, &term)) < 0)
    {
      printf("%s: Couldn't restore terminal control attributes (error = %s)\n", argv[0], strerror(errno));
      exit(1);
    }
  lock.l_type = F_UNLCK;
  if((result = fcntl(fd, F_SETLK, &lock)) <0)
    {
      printf("%s: Couldn't release lock (result = %d)\n", argv[0], result);
      exit(1);
    }
  close(fd);
  exit(0);
}
