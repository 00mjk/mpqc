/*      PICL to nCUBE interface library      
  All questions and criticisms should be directed to 
 Michael Colvin  Center for Computational Engineering
	   Sandia National Laboratory
    		Livermore CA 94551                         */



/* Here's the RCS junk */
/* $Log$
 * Revision 1.1  1993/12/29 12:53:47  etseidl
 * Initial revision
 *
 * Revision 1.4  1993/04/27  21:12:56  jannsen
 * Replace mynode0 with who0.  Include picl.h.
 *
 * Revision 1.3  1992/06/17  22:36:28  jannsen
 * force an exception if the message type is truncated
 *
 * Revision 1.2  1992/04/06  12:16:31  seidl
 * sandia changes
 *
 * Revision 1.1.1.1  1992/03/31  02:15:48  jannsen
 * The CCE picl routines
 *
 * Revision 1.1  1992/03/31  02:15:46  jannsen
 * Initial revision
 *
 * Revision 1.2  1991/03/02  01:37:30  colvin
 * Removed unused TEST_CASE cpp directives.
 *
 * Revision 1.1  91/02/27  22:08:59  colvin
 * Initial revision
 *  */
static char rcsid[]="$Id$";

/* See standard PICL documentation for how to use these routines */
/* Warning: These routines are strongly interdependent so this library
   may not function properly if broken up */
/* Warning #2: The clock routine will return accurate timings ONLY on 
   systems where one clock tick = 128 microseconds such as the nCUBE 2 */

/* Of course we need the standard include files */
#include <stdio.h>
#include <comm/picl/picl.h>

/* First let's define some external static variables */
static int last_count;   /* holds the size of the last msg sent or recvd */
static int last_node;    /* holds the id of where last msg came from */
static int last_type;    /* holds the type of where last msg recvd */
static int checking_flag=1;    /* specifies if parameter checking is on */

/* These are set by the setarc0 and getarc0 call.
 * Since this version of PICL only supports node programs they aren't
 * really used. */
static int arc_nproc;
static int arc_top=1;
static int arc_ord=0;
static int arc_dir=1;

/* Here are the PICL low-level primitives, in the order they appear in 
tabel 1 of the PICL documentation */
/* The Fortran versions are at the end of their C counterparts */

/**********************************************************/
/*        START NODE ROUTINES                             */
/**********************************************************/

#define NODE
#define cubesize 1024

/*******************************************************/

/* This code is common to both host and node machines. */

#if NCUBE
#  ifdef NCUBE_V2
/* The message length arguement,n, is generated by the F77 compiler automatically. */
void message0_(n,message) 
int n;
char *message; 
#  else
void message0_(message,n) 
char *message; 
int n;
#  endif
{
  char *mes = (char *) malloc(n+1);
  strncpy(mes,message,n);
  mes[n] = '\0';
  printf ("%s\n", mes);
  free(mes);
  }
#else
void message0_(message) 
char *message; 
{printf ("%s\n", message);}
#endif

/*******************************************************/

#ifdef NODE

int host0() {return 0;}
int host0_() {return 0;}

void check(checking)
int checking;
{
  checking_flag=checking;
}

double clock0()
{
  static double first;
  static initp = 0;
  double t;
  if (!initp) {
    first = (double) (ntime() - 1);
    initp = 1;
  }
  t = (double) ntime();
  return ((t - first) * .000128);
}


void close0(release)
int release;
{}

void message0(message)
char *message;
{printf ("%s\n", message);}

void open0(numproc, me, host)
int *numproc, *me, *host;
{
  int dim, pid;
  whoami (me, &pid, host, &dim);
  *numproc = 1<<dim;
}

void setarc0(nprocs,top,ord,dir)
int *nprocs;
int *top;
int *ord;
int *dir;
{
  int me,pid,host,dim;
  whoami (&me, &pid, &host, &dim);
  arc_nproc = 1<<dim;
  *nprocs = arc_nproc;
  *top = arc_top;
  *ord = arc_ord;
  *dir = arc_dir;
  }

void getarc0(nprocs,top,ord,dir)
int *nprocs;
int *top;
int *ord;
int *dir;
{
  *nprocs = arc_nproc;
  *top = arc_top;
  *ord = arc_ord;
  *dir = arc_dir;
  }

int probe0(msgtype)
int msgtype;
{
  int source= -1;     /* Any source is okay */
  int return_val;
  int ntest();        /* ncube test function */
  int length;
  return_val=((length = ntest(&source,&msgtype)) < 0)?0:1;
  if (return_val)     /* store this info for recvinfo() */
    {
      last_node=source;
      last_count=length;
      last_type=msgtype;
    }
  return(return_val);
}

void recv0(buf, bytes, type)
int type, bytes;
void *buf;
{
  int nctype = type, flag;

  last_node = -1;
  last_count = nread (buf, bytes, &last_node, &nctype, &flag);
  last_type=nctype;

#if 0
  /* Are there any more messages lying around? */
  {
    int source, type, i, j, length, totallength;
    static int biggest=0;
    totallength = 0;
    for (i=0; i<32500; i+=500) {
      nrange(i,i+500 -1);
      source = -1;
      type = -1;
      if (ntest(&source,&type) >= 0) {
        for (j=0; j<500; j++) {
          source = -1;
          type = i + j;
          if ((length = ntest(&source,&type)) > 0) totallength += length;
          }
        }
      }
    nrange(0,32767);
    if (biggest<totallength) {
      printf("On %d after recv0: at least %d bytes remain in commbuf\n",
             mynode0(),totallength);
      tim_traceback();
      biggest = totallength;

#if 1
      if (biggest > 5000) {
        for (i=0; i<32500; i+=500) {
          nrange(i,i+500 -1);
          source = -1;
          type = -1;
          if (ntest(&source,&type) >= 0) {
            for (j=0; j<500; j++) {
              source = -1;
              type = i + j;
              if ((length = ntest(&source,&type)) > 0) {
                printf(" on %d type %d of length %d from %d found\n",
                       mynode0(),type,length,source);
                }
              }
            }
          }
        }
#endif
      }
  }
#endif
}

picl_check()
{
  int source, type, i, j, length, totallength;
  int numproc,me,host;
  who0(&numproc,&me,&host);
  totallength = 0;
  for (i=0; i<32500; i+=500) {
    nrange(i,i+500 -1);
    source = -1;
    type = -1;
    if (ntest(&source,&type) >= 0) {
      for (j=0; j<500; j++) {
        source = -1;
        type = i + j;
        if ((length = ntest(&source,&type)) > 0) {
          if (!totallength) printf("picl_check:\n");
          totallength += length;
          printf(" on %d found type %d from %d of length %d\n",
                 me,type,source,length);
          }
        }
      }
    }
  nrange(0,32767);
  if (totallength > 0) {
    printf("totallength was %d\n",totallength);
    tim_traceback();
    }
  }

static void recv1(buf, bytes, type, source)
int type, bytes, source;
char *buf;
{
  int nctype = type, flag;

  last_node = source;
  last_count = nread (buf, bytes, &last_node, &nctype, &flag);
  last_type=nctype;
}

void recvinfo0(bytes, msgtype, source)
int *bytes, *msgtype, *source;
{
  *bytes=last_count;
  *source=last_node;
  *msgtype=last_type;
}
 

void send0(buf, bytes, msgtype, dest)
int bytes, msgtype, dest;
void *buf;
{
  int flag;
  static int warn=0;
  int me,pid,host,dim,nproc;
  whoami (&me, &pid, &host, &dim);
  nproc = 1<<dim;
  if ((!warn) && (msgtype != (msgtype&0xffff))) {
    printf("WARNING: message type truncated to 16 bits on the NBUBE\n");
    printf(" %d -> %d (on %d)\n",msgtype,msgtype&0xffff,mynode0());
    warn = 1/warn;
    warn = 1;
    }
  if (dest < 0 || dest > nproc) {
    printf("nwrite(0x%x,%d,%d,%d,&flag) from %d\n",buf,bytes,dest,msgtype,me);
    printf("dest = %d && nproc = %d\n",dest,nproc);
    exit(1);
    }
  nwrite (buf, bytes, dest, msgtype, &flag);

#if 0
  {
  static int biggest=0;
  if (biggest<bytes) {
    printf("Just sent msg of size %d and type %d to %d from %d\n",
           bytes,msgtype,dest,mynode0());
    biggest = bytes;
    }
  }
#endif
}

void sync0()
{
  char   dummybuf='x';
  int    mtype=456,len,me,pnum,host,d,i,iostat,ndest,np,np1; 

  /* get the cube dimension */
  whoami(&me,&pnum,&host,&d);

  /* dummy message has length 1 */
  len = 1;

  /* loop through all cube dimension */
  for (i=0; i<d; i++) {
     /* currently at dimension i */
     np = 1 << i;
     /* each processor figures out the communication partner */
     np1 = np & me;
     if (np1 == 0) ndest = me + np;
     else ndest = me - np;
     /* exchange token with partner */
     send0(&dummybuf,len,mtype,ndest);
     recv1(&dummybuf,len,mtype,ndest);
  }
}

void who0(numproc, me, host)
int *numproc, *me, *host;
{
  int dim, pid;
  whoami (me, &pid, host, &dim);
  *me = *me % cubesize;
  *numproc = 1<<dim;
}
/**** who1 is now only in the piclext library
   void who1(dim, me, host)
   int *dim, *me, *host;
   {
     int pid;
     whoami (me, &pid, host, dim);
     *me = *me % cubesize;
   }
 ****/

/* FORTRAN stub routines */

void check_(checking)
int *checking;
{
  check(*checking);
}

double clock0_()
{
  return(clock0());
}

void close0_(release)
int *release;
{
  close0(*release);
}

void open0_(numproc,me,host)
int *numproc,*me,*host;
{
  open0(numproc,me,host);
}

int probe0_(msgtype)
int *msgtype;
{
  return(probe0(*msgtype));
}

void recv0_(buf,bytes,type)
int *type, *bytes;
char *buf;
{
  recv0(buf,*bytes,*type);
}

void recvinfo0_(bytes,msgtype,source)
int *bytes,*msgtype,*source;
{
  recvinfo0(bytes,msgtype,source);
}

void send0_(buf,bytes,msgtype,dest)
int *bytes,*msgtype,*dest;
char *buf;
{
  send0(buf,*bytes,*msgtype,*dest);
}

void sync0_()
{
  sync0();
}

void who0_(numproc,me,host)
int *numproc,*me,*host;
{
  who0(numproc,me,host);
}

#endif 

/**********************************************************/
/*        START HOST ROUTINES                             */
/**********************************************************/

#ifdef HOST
/* We need to define a host-specific global variable */
static int file_descriptor;  

void check(checking)
int checking;
{
  checking_flag=checking;
}

double clock0()
{
  static double first;
  static initp = 0;
  double t;
  if (!initp) {
    first = (double) (ntime() - 1);
    initp = 1;
  }
  t = (double) ntime();
  return ((t - first) * .000128);
}


void close0(release)
int release;
{
  nclose(file_descriptor);
}

void load0(file,node)
char *file;
int node;
{
  nload(file_descriptor,file,node);
}

void message0(message)
char *message;
{printf ("%s\n", message);}

void open0(numproc, me, host)
int *numproc, *me, *host;
{
  int dim, pid;
  whoami (me, &pid, host, &dim);
  *numproc = 1<<dim;
  file_descriptor=nopen(dim);
}

int probe0(msgtype)
int msgtype;
{
  int return_val;
  int source= -1;     /* Any source is okay */
  int ntest();        /* ncube test function */
  int length;
  return_val=((length = ntest(file_descriptor, &msgtype,&source)) < 0)?0:1;
  if (return_val)     /* store this info for recvinfo() */
    {
      last_node=source;
      last_count=length;
      last_type=msgtype;
    }
  return(return_val);
}

void recv0(buf, bytes, type)
int bytes, type;
char *buf;
{
  int nctype = type;

  last_node = -1;
  last_count = nread (file_descriptor, buf, bytes, &last_node, &nctype); 
  last_type= nctype;
}

void recvinfo0(bytes, msgtype, source)
int *bytes, *msgtype, *source;
{
  *bytes=last_count;
  *source=last_node;
  *msgtype=last_type;
}

void send0(buf, bytes, type, dest)
int bytes,type,dest;
char *buf;
{
  nwrite (file_descriptor, buf, bytes, dest, type);
}

void sync0()
{
  /* On the host sync is a dummy routine in order to maintain
     consistency with the nCUBE's capability to run without a host 
     program */
}

void who0(numproc, me, host)
int *numproc, *me, *host;
{
  int dim, pid;
  whoami (me, &pid, host, &dim);
  *numproc = 1<<dim;
}

/* FORTRAN stub routines */

void check_(checking)
int *checking;
{
  check(*checking);
}

double clock0_()
{
  return(clock0());
}

void close0_(release)
int *release;
{
  close0(*release);
}

void load0_(file,node)
int *node;
char *file;
{
  load0(file,*node);
}

void open0_(numproc,me,host)
int *numproc,*me,*host;
{
  open0(numproc,me,host);
}

int probe0_(msgtype)
int *msgtype;
{
  return(probe0(*msgtype));
}

void recv0_(buf,bytes,type)
int *type, *bytes;
char *buf;
{
  recv0(buf,*bytes,*type);
}

void recvinfo0_(bytes,msgtype,source)
int *bytes,*msgtype,*source;
{
  recvinfo0(bytes,msgtype,source);
}

void send0_(buf,bytes,msgtype,dest)
int *bytes,*msgtype,*dest;
char *buf;
{
  send0(buf,*bytes,*msgtype,*dest);
}

void sync0_()
{
  sync0();
}

void who0_(numproc,me,host)
int *numproc,*me,*host;
{
  who0(numproc,me,host);
}
#endif
