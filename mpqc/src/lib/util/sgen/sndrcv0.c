
/* $Log$
 * Revision 1.1  1993/12/29 12:53:40  etseidl
 * Initial revision
 *
 * Revision 1.2  1992/06/17  22:17:13  jannsen
 * cleaned up for saber-c
 *
 * Revision 1.1.1.1  1992/03/17  17:10:18  seidl
 * DOE-NIH Quantum Chemistry Library 0.0
 *
 * Revision 1.1  1992/03/17  17:10:17  seidl
 * Initial revision
 *
 * Revision 1.1  1992/01/09  12:23:11  seidl
 * Initial revision
 *
 * Revision 1.1  1991/12/20  16:10:32  seidl
 * Initial revision
 *
 * Revision 1.1  91/11/18  18:16:23  cljanss
 * Initial revision
 *  */
static char *rcsid = "$Id$";

#include <stdio.h>
#include <comm/picl/picl.h>
#include "sgen.h"

#define ALLOC_GLOBALS
#include "sndrcv0.h"
#undef ALLOC_GLOBALS

/* This is used to set up the print options for the sgen interface
 * to the sndrcv0 routines.
 *  send = print level for sending
 *  receive = print level for receiving
 *  host = host for which printing is done (-1 = all)
 */
void
sgen_sndrcv0_set_print(send,receive,host)
int send;
int receive;
int host;
{
  sgen_sndrcv0_print_send = send;
  sgen_sndrcv0_print_receive = receive;
  sgen_sndrcv0_print_host = host;
  }

/* This prints out information when debugging is on. */
void
sgen_sndrcv_print(sorr,type,root,size)
int sorr;
int type;
int root;
int size;
{
  int numproc,me,host;
  char *endline;
  static char *val = ", value = ";
  static char *new = "\n";

  who0(&numproc,&me,&host);

  if ((me == sgen_sndrcv0_print_host) || (sgen_sndrcv0_print_host == -1)) {
    if ((sgen_sndrcv0_print_send & 2) && (sorr == 's')) endline = val;
    else if ((sgen_sndrcv0_print_receive & 2) && (sorr == 'r')) endline = val;
    else endline = new;
    printf("%csndrcv0: me = %d, type = %d, root = %d, size = %d%s",
           sorr,me,type,root,size,endline);
    }
  }


void
sndrcv0_sync()
{
  int i;
  int numproc,me,host;
  int junk=0;

  who0(&numproc,&me,&host);

  /* printf("synchronizing ...\n"); */

  if (me == 0) {
    for (i=1; i<numproc; i++) {
      /* printf("sending sync_start to %d\n",i); */
      send0(&junk,sizeof(junk),SYNC_START,i);
      }
    }
  else {
    /* printf("receiving sync_start at %d\n",me); */
    recv0(&junk,sizeof(junk),SYNC_START);
    }

  if (me == 0) {
    for (i=1; i<numproc; i++) {
      /* printf("receiving sync_end\n"); */
      recv0(&junk,sizeof(junk),SYNC_END);
      }
    }
  else {
    /* printf("sending sync_end to %d\n",0); */
    send0(&junk,sizeof(junk),SYNC_END,0);
    }
  }

/* this routine will reset the sgen_sndrcv0_type counter
 * this is used to ensure that the host and node programs 
 * are in sync */
void
sgen_reset_sndrcv0()
{
  sgen_sndrcv0_type = MIN_TYPE;
  }
