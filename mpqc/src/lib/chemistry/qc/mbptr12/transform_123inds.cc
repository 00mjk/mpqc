//
// transform_123inds.cc
//
// Copyright (C) 2001 Edward Valeev
//
// Author: Edward Valeev <edward.valeev@chemistry.gatech.edu>
// Maintainer: EV
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#ifdef __GNUC__
#pragma implementation
#endif

#include <math.h>
#include <stdexcept>

#include <util/misc/formio.h>
#include <util/misc/timer.h>
#include <chemistry/qc/basis/gpetite.h>
#include <chemistry/qc/mbpt/bzerofast.h>
#include <chemistry/qc/mbpt/util.h>
#include <chemistry/qc/basis/distshpair.h>
#include <chemistry/qc/mbptr12/transform_123inds.h>

using namespace std;
using namespace sc;

extern BiggestContribs biggest_ints_1;

#define PRINT1Q 0
#define PRINT_NUM_TE_TYPES 1
#define PRINT_BIGGEST_INTS_NUM_TE_TYPES 1

// The FAST_BUT_WRONG flags is useful for exercising the communications
// layer.  It causes the first and second quarter transformation to be
// omitted, but all communication is still performed.  This permits
// problems in communications libraries to be more quickly identified.
#define FAST_BUT_WRONG 0

TwoBodyMOIntsTransform_123Inds::TwoBodyMOIntsTransform_123Inds(
  const Ref<TwoBodyMOIntsTransform>& tform, int mythread, int nthread,
  const Ref<ThreadLock>& lock, const Ref<TwoBodyInt> &tbint, double tol, int debug) :
  tform_(tform), mythread_(mythread), nthread_(nthread), lock_(lock), tbint_(tbint),
  tol_(tol), debug_(debug)
{
  timer_ = new RegionTimer();
  aoint_computed_ = 0;
  ni_ = tform_->batchsize();
  i_offset_ = 0;
}

TwoBodyMOIntsTransform_123Inds::~TwoBodyMOIntsTransform_123Inds()
{
}

/*
  Distribute work by SR
  
   for all PQ
    compute unique (PQ|RS)
    transform to (RS|IM) where M are all AOs for basis set 2
   end PQ

   use BLAS to transform each rsIM to rsIX
   transform RSIX to IJXS and accumulate to the tasks that holds respective ij-pairs.

  end SR
*/

void
TwoBodyMOIntsTransform_123Inds::run()
{
  Ref<MemoryGrp> mem = tform_->mem();
  Ref<MessageGrp> msg = tform_->msg();
  int me = msg->me();
  int nproc = msg->n();
  Ref<MOIndexSpace> space1 = tform_->space1();
  Ref<MOIndexSpace> space2 = tform_->space2();
  Ref<MOIndexSpace> space3 = tform_->space3();
  Ref<MOIndexSpace> space4 = tform_->space4();

  Ref<GaussianBasisSet> bs1 = space1->basis();
  Ref<GaussianBasisSet> bs2 = space2->basis();
  Ref<GaussianBasisSet> bs3 = space3->basis();
  Ref<GaussianBasisSet> bs4 = space4->basis();
  bool bs1_eq_bs2 = (bs1 == bs2);
  if (!bs1_eq_bs2)
    throw std::runtime_error("TwoBodyMOIntsTransform_ixjy_12Qtr::run() -- bs1 != bs2");
  bool bs3_eq_bs4 = (bs3 == bs4);
  if (!bs3_eq_bs4)
    throw std::runtime_error("TwoBodyMOIntsTransform_ixjy_12Qtr::run() -- bs3 != bs4");

  bool dynamic = tform_->dynamic();
  double print_percent = tform_->print_percent();

  int nfuncmax1 = bs1->max_nfunction_in_shell();
  int nfuncmax2 = bs2->max_nfunction_in_shell();
  int nfuncmax3 = bs3->max_nfunction_in_shell();
  int nfuncmax4 = bs4->max_nfunction_in_shell();
  int nsh1 = bs1->nshell();
  int nsh2 = bs2->nshell();
  int nsh3 = bs3->nshell();
  int nsh4 = bs4->nshell();
  int nbasis1 = bs1->nbasis();
  int nbasis2 = bs2->nbasis();
  int nbasis3 = bs3->nbasis();
  int nbasis4 = bs4->nbasis();
  double dtol = pow(2.0,tol_);

  /*-------------------------------------------------------------
    Find integrals buffers to 1/r12, r12, and [r12,T1] integrals
   -------------------------------------------------------------*/
  int num_te_types = tform_->num_te_types();
  enum te_types {eri=0, r12=1, r12t1=2};
  const double *intbuf[num_te_types];
  intbuf[eri] = tbint->buffer(TwoBodyInt::eri);
  intbuf[r12] = tbint->buffer(TwoBodyInt::r12);
  intbuf[r12t1] = tbint->buffer(TwoBodyInt::r12t1);

  /*-----------------------------------------------------
    Allocate buffers for partially transformed integrals
   -----------------------------------------------------*/
  double *ijxs_contrib[num_te_types];  // local contributions to integral_ijxs
  double *ijxr_contrib[num_te_types];  // local contributions to integral_ijxr
  double *rsiq_ints[num_te_types];     // quarter-transformed integrals for each RS pair
  for(int te_type=0;te_type<num_te_types;te_type++) {
    ijxs_contrib[te_type]  = mem->malloc_local_double(nbasis2*nfuncmax4);
    if (bs3_eq_bs4)
      ijxr_contrib[te_type]  = mem->malloc_local_double(nbasis2*nfuncmax4);
    else
      ijxr_contrib[te_type]  = NULL;
    rsiq_ints[te_type] = new double[ni_*nbasis2*nfuncmax3*nfuncmax4];
  }

  /*-----------------------------
    Initialize work distribution
   -----------------------------*/
  sc::exp::DistShellPair shellpairs(msg,nthread,mythread,lock,bs3,bs4,dynamic_);
  shellpairs.set_debug(debug);
  if (debug_) shellpairs.set_print_percent(print_percent/10.0);
  else shellpairs.set_print_percent(print_percent);
  int work_per_thread = bs3_eq_bs4 ? 
    ((nsh3*(nsh3+1))/2)/(nproc*nthread) :
    (nsh3*nsh4)/(nproc*nthread) ;
  int print_interval = work_per_thread/100;
  int time_interval = work_per_thread/10;
  int print_index = 0;
  if (print_interval == 0) print_interval = 1;
  if (time_interval == 0) time_interval = 1;
  if (work_per_thread == 0) work_per_thread = 1;

  if (debug) {
    lock->lock();
    ExEnv::outn() << scprintf("%d:%d: starting get_task loop",me,mythread) << endl;
    lock->unlock();
  }

  // Assuming all basis sets are the same (bs1_eq_bs2 and bs3_eq_bs4)
  // I don't know yet how to overcome the static type problem here
  canonical_aaaa c4(bs1,bs2,bs3,bs4);
  Ref<GPetite4<canonical_aaaa> > p4list
    = new GPetite4<canonical_aaaa>(bs1,bs2,bs3,bs4,c4);

#if FAST_BUT_WRONG
  for(int te_type=0;te_type<num_te_types;te_type++) {
    bzerofast(rsiq_ints[te_type], ni_*nbasis2*nfuncmax3*nfuncmax4);
    bzerofast(ijxs_contrib[te_type], nbasis2*nfuncmax4);
    if (bs3_eq_bs4)
      bzerofast(ijxr_contrib[te_type], nbasis2*nfuncmax4);
    }
#endif

  int R = 0;
  int S = 0;
  while (shellpairs.get_task(S,R)) {
    // if bs3_eq_bs4 then S >= R always (see sc::exp::DistShellPair)
    int nr = bs3->shell(R).nfunction();
    int r_offset = bs3->shell_to_function(R);
    
    int ns = bs4->shell(S).nfunction();
    int s_offset = bs4->shell_to_function(S);
    
    int nrs = nr*ns;

    if (debug > 1 && (print_index++)%print_interval == 0) {
      lock->lock();
      ExEnv::outn() << scprintf("%d:%d: (PQ|%d %d) %d%%",
			       me,mythread,R,S,(100*print_index)/work_per_thread)
		   << endl;
      lock->unlock();
    }
    if (debug > 1 && (print_index)%time_interval == 0) {
      lock->lock();
      ExEnv::outn() << scprintf("timer for %d:%d:",me,mythread) << endl;
      timer->print();
      lock->unlock();
    }

#if !FAST_BUT_WRONG
    for(int te_type=0;te_type<num_te_types;te_type++)
      bzerofast(rsiq[te_type], ni_*nbasis2*nrs);

    for (int P=0; P<nsh1; P++) {
      int np = bs1->shell(P).nfunction();
      int p_offset = bs1->shell_to_function(P);

      int Qmax = (bs1_eq_bs2 ? P : nsh2-1);
      for (int Q=0; Q<=Qmax; Q++) {
	int nq = bs2->shell(Q).nfunction();
	int q_offset = bs3->shell_to_function(Q);
        
	// check if symmetry unique and compute degeneracy
	int deg = p4list->in_p4(P,Q,R,S);
	if (deg == 0)
	  continue;
	double symfac = (double) deg;

        if (tbint->log2_shell_bound(P,Q,R,S) < tol_) {
          continue;  // skip shell quartets less than tol
	}

        aoint_computed++;

        timer->enter("AO integrals");
        tbint->compute_shell(P,Q,R,S);
        timer->exit("AO integrals");

        timer->enter("1. q.t.");

        // Begin first quarter transformation;
        // generate (iq|rs) for i active
        // if bs1_eq_bs2 then (ip|rs) are also generated
        // store the integrals as rsiq
	for(int te_type=0; te_type<num_te_types; te_type++) {
	  const double *pqrs_ptr = intbuf[te_type];

	  for (int bf1 = 0; bf1 < np; bf1++) {
	    int p = p_offset + bf1;
            int qmax = (bs1_eq_bs2 && P == Q) ? p : nq-1;

	    for (int bf2 = 0; bf2 <= qmax; bf2++) {
	      int q = q_offset + bf2;

	      for (int bf3 = 0; bf3 < nr; bf3++) {
                int smin = (bs3_eq_bs4 && R == S) ? 0 : nr;
                pqrs_ptr += smin;

		for (int bf4 = smin; bf4 <nq; bf4++) {

                  // Only transform integrals larger than the threshold
		  if (fabs(*pqrs_ptr) > dtol) {

		    const double* rsiq_ptr = &rsiq_ints[te_type][bf2 + ni_*(bs4 + nq*bf3)];
		    const double* c_pi = vector1[p] + i_offset_;

                    const double* rsip_ptr;
		    const double* c_qi;
                    if (bs1_eq_bs2) {
		      rsip_ptr = &integral_iqrs[te_type][bf1 + ni_*(bs4 + nq*bf3)];
		      c_qi = vector1[q] + i_offset_;
                    }

		    tmpval = *pqrs_ptr;
		    // multiply each integral by its symmetry degeneracy factor
		    tmpval *= symfac;
                    
                    if (bs1_eq_bs2) {

                      double rsip_perm_pfac = 1.0;
                      if (te_type == r12t1)
                      rsip_perm_pfac = -1.0;

		        if (p == q) {
                          // p == q
                          for (i=0; i<ni_; i++) {
                            *rsiq_ptr += *c_qi++*tmpval;
                            rsiq_ptr += offset;
                          }
                        }
                        else {
                          // p != q
                          for (i=0; i<ni_; i++) {
                            *rsip_ptr += rsip_perm_pfac * *c_qi++ * tmpval;
                            rsip_ptr += offset;
                            *rsiq_ptr += *c_pi++*tmpval;
                            rsiq_ptr += offset;
                          }
                        }
                        
                      }
                      else {
                        // bs1_eq_bs2
		        if (p == q) {
                          // te_type == 2, p == q
                          for (i=0; i<ni_; i++) {
                            *iprs_ptr -= *c_qi++*tmpval;
                            iprs_ptr += offset;
                          }
                        }
                        else {
                          // te_type == 2, p != q
                          for (i=0; i<ni_; i++) {
                            *iprs_ptr -= *c_qi++*tmpval;
                            iprs_ptr += offset;
                            *iqrs_ptr += *c_pi++*tmpval;
                            iqrs_ptr += offset;
                          }
                        }
                      }  // endif te_type == 2
                    } // endif bs1_eq_bs2
                  }   // endif dtol

		  pqrs_ptr++;
                } // exit bf4 loop
              }   // exit bf3 loop
            }     // exit bf2 loop
            pqrs_ptr += (nq - qmax - 1) * nrs;
          }       // exit bf1 loop
	  // end of first quarter transformation
	}
	timer->exit("1. q.t.");

        }           // exit P loop
      }             // exit Q loop
#endif // !FAST_BUT_WRONG

#if PRINT1Q
      {
      lock->lock();
      for(te_type=0; te_type<PRINT_NUM_TE_TYPES; te_type++) {
	double *tmp = integral_iqrs[te_type];
	for (int i = 0; i<ni_; i++) {
	  for (int r = 0; r<nr; r++) {
	    for (int q = 0; q<nbasis2; q++) {
	      for (int s = 0; s<ns; s++) {
		printf("1Q: (%d %d|%d %d) = %12.8f\n",
		       i+i_offset,q,r+r_offset,s+s_offset,*tmp);
		tmp++;
              }
            }
          }
        }
      }
      lock->unlock();
      }
#endif
#if PRINT_BIGGEST_INTS
      {
      lock->lock();
      for(te_type=0; te_type<PRINT_BIGGEST_INTS_NUM_TE_TYPES; te_type++) {
	double *tmp = integral_iqrs[te_type];
	for (int i = 0; i<ni_; i++) {
	  for (int r = 0; r<nr; r++) {
	    for (int q = 0; q<nbasis2; q++) {
	      for (int s = 0; s<ns; s++) {
		biggest_ints_1.insert(*tmp,i+i_offset,q,r+r_offset,s+s_offset);
		tmp++;
              }
            }
          }
        }
      }
      lock->unlock();
      }
#endif

    timer->enter("2. q.t.");
    // Begin second quarter transformation;
    // generate (iq|jr) for i active and j active or frozen
    for (i=0; i<ni_; i++) {
      for (j=0; j<nocc_act; j++) {
	int j_offset = nocc - nocc_act;
	int ij_proc =  (i*nocc_act + j)%nproc;
	int ij_index = (i*nocc_act + j)/nproc;
	int ijsq_start[num_te_types];
	ijsq_start[0] = num_te_types*nbasis2*nbasis4*ij_index;

	for(te_type=0; te_type<num_te_types; te_type++) {
	  if (te_type)
	    ijsq_start[te_type] = ijsq_start[te_type-1] + nbasis2*nbasis4;

#if !FAST_BUT_WRONG
	  bzerofast(iqjs_contrib[te_type], nbasis2*ns);
	  // bs3_eq_bs4
	  bzerofast(iqjr_contrib[te_type], nbasis2*nr);
// 	  bzerofast(iqjs_contrib[te_type], nbasis2*nfuncmax4);
// 	  // bs3_eq_bs4
// 	  bzerofast(iqjr_contrib[te_type], nbasis2*nfuncmax4);

	  for (bf1=0; bf1<ns; bf1++) {
	    s = s_offset + bf1;
	    double *c_sj = &scf_vector[s][j+j_offset];
	    // bs3_eq_bs4
	    double *iqjr_ptr = iqjr_contrib[te_type];
	    for (bf2=0; bf2<nr; bf2++) {
	      r = r_offset + bf2;
	      // bs3_eq_bs4
	      if (r > s) {
		break; // skip to next bf1 value
              }
	      // bs3_eq_bs4
	      double c_rj = scf_vector[r][j+j_offset];
	      iqjs_ptr = &iqjs_contrib[te_type][bf1*nbasis2];
	      iqrs_ptr = &integral_iqrs[te_type][bf1 + ns*nbasis2*(bf2 + nr*i)];
#if 1 // this code has conditionals in the inner loop (apparently can be faster)
	      for (q=0; q<nbasis2; q++) {
		*iqjs_ptr++ += c_rj * *iqrs_ptr;
		// bs3_eq_bs4
		if (r != s) *iqjr_ptr += *c_sj * *iqrs_ptr;
		iqjr_ptr++;
		iqrs_ptr += ns;
              } // exit q loop
#else // this code has conditionals removed from the inner loop
              // bs3_eq_bs4
              if (r == s) {
                for (q=0; q<nbasis2; q++) {
                  *iqjs_ptr++ += c_rj * *iqrs_ptr;
                  iqrs_ptr += ns;
                  }
                }
              else {
                double c_sj_val = *c_sj;
                for (q=0; q<nbasis2; q++) {
                  double iqrs_val = *iqrs_ptr;
                  *iqjs_ptr++ += c_rj * iqrs_val;
                  *iqjr_ptr += c_sj_val * iqrs_val;
                  iqjr_ptr++;
                  iqrs_ptr += ns;
                  }
                }
#endif
            }   // exit bf2 loop
          }     // exit bf1 loop
#endif // !FAST_BUT_WRONG
	  
	  // We now have contributions to iqjs and iqjr for one pair i,j,
	  // all q, r in R and s in S; send iqjs and iqjr to the node
	  // (ij_proc) which is going to have this ij pair

	  // Sum the iqjs_contrib to the appropriate place
	  int ij_offset = nbasis2*s_offset + ijsq_start[te_type];
	  mem->sum_reduction_on_node(iqjs_contrib[te_type],
				     ij_offset, ns*nbasis2, ij_proc);
	  
	  // bs3_eq_bs4
	  ij_offset = nbasis2*r_offset + ijsq_start[te_type];
	  mem->sum_reduction_on_node(iqjr_contrib[te_type],
				     ij_offset, nr*nbasis2, ij_proc);
	  
	}
      }     // exit j loop
    }       // exit i loop
    // end of second quarter transformation
    timer->exit("2. q.t.");
  }         // exit while get_task

  if (debug) {
    lock->lock();
    ExEnv::outn() << scprintf("%d:%d: done with get_task loop",me,mythread) << endl;
    lock->unlock();
  }

  //  lock->lock();
  for(te_type=0; te_type<num_te_types; te_type++) {
    delete[] integral_iqrs[te_type];
    mem->free_local_double(iqjs_contrib[te_type]);
    mem->free_local_double(iqjr_contrib[te_type]);
  }
  //  lock->unlock();
}

////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
