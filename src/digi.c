/*******************************************************************
 * $Id$
 * by Andrew C. Uselton <uselton2@llnl.gov> 
 * Copyright (C) 2000 Regents of the University of California
 * See ../DISCLAIMER
 * v. 0-1-0:  2001-08-28
 *            Implements ioctl call on digi's port for each node
 * v. 0-1-1:  2001-08-31
 *           renovation in support of rpm builds
 * v. 0-1-2:  2001-09-05
 ********************************************************************/

/*
 *   Accept a list of nodes look in the digi.conf configuration file 
 * for the port associated with each node, open that tty line and 
 * check for status, if the status matches the requested status then
 * print the name of the node followed by a newline.  
 *
 *   This program needs to be:
 * # chown root digi
 * # chmod 4755 digi
 * in order to be accessable by non-root users.  The above should be 
 * incorporated in the installation script.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TIOCM_DSR 0x100

#define MAXNAME 20
#define MAXLINE 1000
#define MAXSTRING 1000

char *Version = "digi:Powerman 0.1.2";
char *powermandir  = "/usr/lib/powerman/";
char *config_file = "etc/digi.conf";
char *POWERMANDIR = "POWERMANDIR";

int verbose = 0;

void
usage(int argc, char **argv, char *msg)
{
  printf("usage: %s [-a] [-b] [-c conf] [-f fan] [-l dir] [-r] [-w node,...]\n", argv[0]);
  printf ("-a       = print state for all nodes\n");
  printf ("-b       = print a bitmap instead of node names (implies -a)\n");
  printf ("-c conf  = configuration file (default: <ldir>/etc/etherwake.conf)\n");
  printf ("-f fan   = fanout (default: 256 where implemented)\n");
  printf ("-l ldir  = powerman lirary directory (default: /usr/lib/powerman)\n");
  printf ("-r       = reverse sense, i.e. report on off nodes\n");
  printf ("-w nodes = comma separated list of nodes\n");
  printf ("-w -     = read nodes from stdin, one per line\n");
  printf ("-v       = be verbose about any errors that may have occurred\n");
  printf ("-V       = print version and exit\n");
  printf ("on       = turn on nodes (the default)\n");
  printf ("off      = turn off nodes\n");
  printf ("reset    = reset nodes\n");
  printf ("%s\n", msg);
  exit(0);
}

typedef struct nodestructtype
{
  struct nodestructtype *next;
  char *name;
  char *tty;
} nodetp;

typedef struct namestructtype
{
  struct namestructtype *next;
  char *name;
} nametp;



nodetp *
init (FILE *fp)
     /*
      *   Read through the file *fp getting two whitespace delimitted 
      * tokens per line.  The first token is the node name and the second
      * is the tty line.  Build a list of structures with the info from
      * the file.  Return that list in the order the nodes appeared in the
      * file. 
      */
{
  int p, q;
  int r = 0;
  int s;
  int tokens = 0;
  nodetp *nodes = NULL;
  nodetp *temp;
  nodetp *next;
  char buf[MAXLINE];
  char *name;
  char *tty;
  int len;
  int t1len = 0;
  int t2len = 0;

  while(fgets(buf, MAXLINE, fp))
  {
    len = strlen(buf) - 1;
    buf[len] = 0;
    for (p = 0; isspace(buf[p]) && p < len; p++)
      ;
    if((p < len) && (buf[p] != '#'))
      {
	for (q = p; !isspace(buf[q]) && q < len; q++)
	  ;
	t1len = q - p;
	tokens = 1;
	if (q < len)
	  {
	    for (r = q; isspace(buf[r]) && r < len; r++)
	      ;
	    if(r < len)
	      {
		for (s = r; !isspace(buf[s]) && s < len; s++)
		  ;
		t2len = s - r;
		tokens = 2;
	      }
	  }
      }
    if (tokens == 2)
      {
	if((name = malloc(t1len + 1)) == NULL)
	  {
	    if(verbose)
	      fprintf(stderr, "digi: memory allocation error for name string of node number %s\n", buf);
	    exit(1);
	  }
	strncpy(name, buf + p, t1len);
	name[t1len] = 0;
	if((tty = malloc(t2len + 1)) == NULL)
	  {
	    if(verbose)
	      fprintf(stderr, "digi: memory allocation error for tty string of node number %s\n", buf);
	    exit(1);
	  }
	strncpy(tty, buf + r, t2len);
	tty[t2len] = 0;
	if((next = (nodetp *)malloc(sizeof(nodetp))) == NULL)
	  {
	    if(verbose)
	      fprintf(stderr, "digi: memory allocation error for node number %s\n", buf);
	    exit(1);
	  }
	next->name = name;
	next->tty  = tty;
	next->next = nodes;
	nodes = next;
      }
  }
  /* the list is in reverse order.  turn it around. */
  temp = nodes;
  nodes = NULL;
  while ((next = temp) != NULL)
    {
      temp = next->next;
      next->next = nodes;
      nodes = next;
    }
  return nodes;
}

nodetp*
find(nodetp *nodes, char *name, nodetp *hint)
     /* 
      *   Find "name" in the list of nodes and return its node struct.
      * Usually the nodes will be presented to the find routine in the 
      * same order that they appear in the list, thus it is a good hint
      * to start each next search immediately following the place the
      * last search left off.
      *   I could be persuaded that this should be reimplemented as a 
      * search in a circular list.
      */
{
  nodetp *next = hint;
  int len;

  len = strlen(name);
  while(next && strncmp(name, next->name, len))
    {
      next = next->next;
    }
  if(next == NULL)
    {
      next = nodes;
      while(next && strncmp(name, next->name, len))
	{
	  if((next = next->next) == hint)
	    next = NULL;
	}
    }
  return next;
}

int
check (char *tty)
     /*
      *   This performs the actual ioctl call.  It is not clear to me 
      * but there may be a concurrency problem here.  The ioctl call
      * may block and require an extended period to complete.  The GUI
      * inteface to the original ClusterView implemented a similar call 
      * and would occasionally stop processing its event loop for
      * extended periods.
      *   Check returns 1 if the node is on, 0 if its off, and -1 if 
      * there is  some failure.  TIOCMGET is the ioctl function that
      * returns the tty state, and TIOCM_DSR is the mask for the 
      * DSR == 1 ("high" or "on") or DSR == 0 (or "low" or "off") bit.
      */
{
  int modem;
  int fd;
  int code = -1;

  if(strlen(tty) < MAXNAME)
    {
      if((fd = open(tty, O_WRONLY | O_NDELAY)) >= 0)
	{
	  ioctl(fd, TIOCMGET, &modem);
	  if(modem & TIOCM_DSR)
	    code = 1;
	  else
	    code = 0;
	}
      else
	{
	  fprintf(stderr, "digi: couldn't open %s\n", tty);
	  exit(1);
	}
    }
  return code;
}

int
fake_check(char *tty)
     /*
      *   This may be used in place of check in the call in "main" 
      * if you want to test the operation of the code without 
      * disrupting equipment.  It just return the parity of the 
      * tty string so that the return value will sometimes be 0 
      * and sometimes 1.
      */
{
  char c = 0;
  int  i, len;
  int res = 0;

  len = strlen(tty);
  for (i = 0; i < len; i++)
    c += tty[i];
  for (i = 0; i < 8; i++)
    res += (c >> i) & 0x1;
  return (res & 0x1);
}

nametp *
get_names(char *string)
     /* 
      * pull apart the comma separated list of of names and build
      *	a linked list of structs pointing to them.  If the optarg
      * is '-' get them from stdin until eof.  
      */
{
  int len;
  char buf[MAXLINE];
  char *bufptr = buf;
  char *name;
  nametp *next;
  nametp *temp;
  nametp *names = NULL;
  int p, q;
  int namelen = 0;
  int tokelen = 0;

  len = strlen(string);
  if((len == 1) && (string[0] == '-'))
    {
      /* Reading names for stdin */
      while((bufptr = fgets(buf, MAXLINE, stdin)) != NULL)
	{
	  namelen = strlen(buf);
	  if (buf[namelen-1] == '\n') namelen--;
	  buf[namelen] = 0;
	  if((name = malloc(namelen + 1)) == NULL)
	    {
	      if(verbose)
		printf ("malloc for name error while reading names\n");
	      exit(1);
	    }
	  if((next = malloc(sizeof(nametp))) == NULL)
	    {
	      if(verbose)
		printf ("malloc error for nametp while reading names\n");
	      exit(1);
	    }
	  strncpy(name, buf, namelen);
	  name[namelen] = 0;
	  next->name = name;
	  next->next = names;
	  names = next;
	}
    }
  else
    {
      /* pulling apart the comma separated list */
      p = 0;
      while(p < len)
	{
	  q = p;
	  while((q < len) && (string[q] != ','))
	    q++;
	  tokelen = q - p;
	  if((name = malloc(tokelen + 1)) == NULL)
	    {
	      if(verbose)
		printf ("malloc for name error while reading names\n");
	      exit(1);
	    }
	  if((next = malloc(sizeof(nametp))) == NULL)
	    {
	      if(verbose)
		printf ("malloc error for nametp while reading names\n");
	      exit(1);
	    }
	  strncpy(name, string+p, tokelen);	  
	  name[tokelen] = 0;
	  next->name = name;
	  next->next = names;
	  names = next;
	  p = q+1;
	}
    }
  /* the list is in reverse order.  turn it around. */
  temp = names;
  names = NULL;
  while ((next = temp) != NULL)
    {
      temp = next->next;
      next->next = names;
      names = next;
    }
  return names;
}

int
main(int argc, char **argv)
{
  FILE *conf;
  struct stat st;
  char *dir;
  char config[MAXSTRING];
  char *confl = config_file;
  nametp *names = NULL;
  nametp *name = NULL;
  nodetp *node;
  nodetp *hint;
  nodetp *nodes;
  int com = 1;
  int end;
  int opt;
  int numfound = 0;
  int all = 0;
  int bitmap = 0;
  int fanout = 256;
  int resp;

  if((dir = getenv(POWERMANDIR)) == NULL)
    {
      dir = powermandir;
    }
  /* command line processing first */

  while((opt = getopt (argc, argv, "abc:f:l:rw:vV")) != -1)
    {
      switch (opt)
	{
	case 'a' :
	  all = 1;
	  break;
	case 'b' :
	  all = 1;
	  bitmap = 1;
	  break;
	case 'c' :
	  confl = optarg;
	  break;
	case 'f' :
	  fanout = atoi(optarg);
	  break;
	case 'l' :
	  dir = optarg;
	  break;
	case 'r' :
	  com = 0;
	  break;
	case 'w' :
	  names = get_names(optarg);
	  break;
	case 'v' :
	  verbose = 1;
	  break;
	case 'V' :
	  printf("%s\n", Version);
	  exit(0);
	default :
	  usage(argc, argv, "Unrecognized argument");
	}
    }

  if( !all && names == NULL)
    {
      usage(argc, argv, "provide a list of nodes");
    }

  /* fail if the powerlib directory name doesn't name a real directory */
  if(stat(dir, &st) == 0)
    {
      if(S_ISDIR(st.st_mode))
	{
	  end = strlen(dir);
	  strncpy(config, dir, end);
	  if(config[end] != '/')
	    {
	      config[end] = '/';
	      end++;
	    }
	  config[end] = 0;
	}
      else
	{
	  if(verbose)
	    fprintf(stderr, "digi: doesn't look like a directory: %s\n", dir);
	  exit(1);
	}
    }
  else
    {
      if(verbose)
	fprintf(stderr, "digi: could not stat dir: %s\n", dir);
      exit(1);
    }

  strncpy(config + end, confl, MAXSTRING - end);
  if((conf = fopen(config, "r")) == NULL)
    {
      if(verbose)
	fprintf(stderr, "digi: couldn't open config file at: %s\n", config);
      exit(1);
    }
  nodes = init(conf);

  /*
   * at this point I can do the processing.  For "bitmap" processing print
   * a digit for each node reflecting that nodes state.  For "all" nodes
   * sequence throgh each element in the nodes list, and  if it is in the 
   * requested state print its name.
   */
  if(all || bitmap) 
    {
      node = nodes;
      while (node != NULL)
	{
	  resp = check(node->tty);
	  if(bitmap)
	    {
	      printf("%1d", resp);
	    }
	  else
	    {
	      if(resp == com)
		{
		  printf("%s\n", node->name);
		}
	    }
	  node = node->next;
	}
      if (bitmap) printf("\n");
      return 0;
    }
  /* 
   *   If this is not "bitmap" or "all" processing then sequence through 
   * the requested "names" list and, if it is in the requested state, 
   * print the node's name.
   */
  name = names;
  while(name != NULL)
    {
      hint = NULL;
      if((node = find(nodes, name->name, hint)) != NULL)
	{
	  hint = node;
	  numfound++;
	  if(check(node->tty) == com)
	    {
	      printf("%s\n", name->name);
	    }
	}
      name = name->next;
    }
  if(numfound == 0) 
    {
      if(verbose)
	fprintf(stderr, "digi: Couldn't find any of the named nodes\n");
      exit(1);
    }
  return 0;
}

