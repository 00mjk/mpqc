/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  PICL source code                                               *
 *                                                                 *
 *  We welcome questions, comments, and bug reports, and request   *
 *  that you share any modifications with us.                      *
 *                                                                 *
 *  Patrick Worley                                                 *
 *  Oak Ridge National Laboratory                                  *
 *  worley@msr.epm.ornl.gov                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "globals.h"
#include "envglobals.h"
#include "envclock.h"
#include "envrecvend.h"
#include "envupdatetime.h"
#include "envrecvbyte.h"
#include "envrecvinfo.h"

/* node version of "finish receiving a message". */

recvend0(type)
int type;
{
  int found, blocked;
  long oldctim1, oldctim2, start1, start2, stop1, stop2;

  /* check whether open statement has been exeuted */
  if (envOPN == 1){

    /* collect trace info */
    if (envTOPN == 1){

      /* save previous idle time values */
      oldctim1 = envCTIM1;
      oldctim2 = envCTIM2;

      /* record the start time */
      ENVCLOCK(&start1, &start2);

      /* do the receive */
      blocked = ENVRECVEND(type, envCHECKING);

      /* record the stop time */
      ENVCLOCK(&stop1, &stop2);

      /* update the running total */
      ENVUPDATETIME(&envCTIM1, &envCTIM2, start1, start2, stop1, stop2);

      /* write out trace info */
      if (envTRA > 1){

        /* save information for tracing */
        ENVRECVINFO(&envLTH, &envTYP, &envSRC);

        if (blocked == 0){

          envTDATA[envNXT+KEY] = RECV;
          envTDATA[envNXT+CLOCK1] = start1;
          envTDATA[envNXT+CLOCK2] = start2;
          envTDATA[envNXT+TYPE] = envTYP;
          envTDATA[envNXT+FROM] = envSRC;
          envTDATA[envNXT+LENGTH] = envLTH;
          envNXT = envNXT + RECVSIZE;

          if (envNXT > envMLTH) envFLUSH(start1, start2);

          }
          else{

          envTDATA[envNXT+KEY] = BLOCK;
          envTDATA[envNXT+CLOCK1] = start1;
          envTDATA[envNXT+CLOCK2] = start2;
          envTDATA[envNXT+TYPE] = type;
          envNXT = envNXT + BLOCKSIZE;
  
          envTDATA[envNXT+KEY] = WAKE;
          envTDATA[envNXT+CLOCK1] = stop1;
          envTDATA[envNXT+CLOCK2] = stop2;
          envTDATA[envNXT+TYPE] = envTYP;
          envTDATA[envNXT+FROM] = envSRC;
          envTDATA[envNXT+LENGTH] = envLTH;
          envNXT = envNXT + WAKESIZE;

          if (envNXT > envMLTH) envFLUSH(stop1, stop2);

          };

        }
        else{
        ENVRECVBYTE(&envLTH);
        };

      if (envTCP > 1){

        /* write out info preceding recvend */
        envTDATA[envNXT+KEY] = COMPSTATS;
        envTDATA[envNXT+CLOCK1] = start1;
        envTDATA[envNXT+CLOCK2] = start2;
        envTDATA[envNXT+IDLE1] = oldctim1;
        envTDATA[envNXT+IDLE2] = oldctim2;
        envNXT = envNXT + COMPSTATSIZE;
   
        /* write out info following recvend */
        envTDATA[envNXT+KEY] = COMPSTATS;
        envTDATA[envNXT+CLOCK1] = stop1;
        envTDATA[envNXT+CLOCK2] = stop2;
        envTDATA[envNXT+IDLE1] = envCTIM1;
        envTDATA[envNXT+IDLE2] = envCTIM2;
        envNXT = envNXT + COMPSTATSIZE;
   
        if (envNXT > envMLTH) envFLUSH(stop1, stop2);

        };

      if (envTCM > 1){
  
        /* write out info preceding recvend */
        envTDATA[envNXT+KEY] = COMMSTATS;
        envTDATA[envNXT+CLOCK1] = start1;
        envTDATA[envNXT+CLOCK2] = start2;
        envTDATA[envNXT+MRECV] = envRCV;
        envTDATA[envNXT+MSENT] = envSNT;
        envTDATA[envNXT+MRVOL] = envRVOL;
        envTDATA[envNXT+MSVOL] = envSVOL;
        envTDATA[envNXT+MPROB] = envPRB;
        envNXT = envNXT + COMMSTATSIZE;

        /* update receive info */
        envRCV++;
        envRVOL = envRVOL + envLTH;

        /* write out info following recvend */
        envTDATA[envNXT+KEY] = COMMSTATS;
        envTDATA[envNXT+CLOCK1] = stop1;
        envTDATA[envNXT+CLOCK2] = stop2;
        envTDATA[envNXT+MRECV] = envRCV;
        envTDATA[envNXT+MSENT] = envSNT;
        envTDATA[envNXT+MRVOL] = envRVOL;
        envTDATA[envNXT+MSVOL] = envSVOL;
        envTDATA[envNXT+MPROB] = envPRB;
        envNXT = envNXT + COMMSTATSIZE;

        if (envNXT > envMLTH) envFLUSH(stop1, stop2);

        }
        else{
        /* update receive info */
        envRCV++;
        envRVOL = envRVOL + envLTH;
        };

      }
      else{

      /* do the receive */
      ENVRECVEND(type,envCHECKING);

      };

    }
    else envERRMSG("recvend0: communication channel not open - exiting");

}

