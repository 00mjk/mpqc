
#ifndef _chemistry_qc_scf_scf_h
#define _chemistry_qc_scf_scf_h

#ifdef __GNUC__
#pragma interface
#endif

#include <math/scmat/elemop.h>
#include <math/scmat/block.h>
#include <math/scmat/blkiter.h>
#include <math/optimize/scextrap.h>

#include <chemistry/qc/basis/tbint.h>
#include <chemistry/qc/wfn/obwfn.h>

////////////////////////////////////////////////////////////////////////////

class SCF: public OneBodyWavefunction {
#   define CLASSNAME SCF
#   include <util/state/stated.h>
#   include <util/class/classda.h>
 protected:
    int maxiter_;

    // calculate new density matrices, returns the rms density difference
    virtual double new_density() =0;

    // reset density diff matrix and zero out delta G matrix
    virtual void reset_density() =0;

    // do setup for SCF calculation
    virtual void init_vector() =0;
    virtual void done_vector() =0;

    // do setup for gradient calculation
    virtual void init_gradient() =0;
    virtual void done_gradient() =0;

    // do setup for hessian calculation
    virtual void init_hessian() =0;
    virtual void done_hessian() =0;

    // return the scf electronic energy
    virtual double scf_energy() =0;
    
    // return the DIIS error matrices
    virtual RefSCExtrapError extrap_error() =0;

    // return the DIIS data matrices
    virtual RefSCExtrapData extrap_data() =0;
    
    // return the effective MO fock matrix
    virtual RefSymmSCMatrix effective_fock() =0;
    
    virtual void ao_fock() =0;
    virtual void make_contribution(int, int, int, int, double, int) =0;
    
    // calculate the AO fock matrices
    virtual void ao_gmat();

    // calculate the scf vector
    virtual void do_vector(double&);

    // calculate the scf gradient
    virtual void do_gradient(const RefSCVector&);
    
    // calculate the scf hessian
    virtual void do_hessian(const RefSymmSCMatrix&);
    
    virtual void compute();

    // local storage, don't save these
    RefTwoBodyInt tbi;
    RefSCMatrix scf_vector_;
    
  public:
    SCF(StateIn&);
    SCF(const RefKeyVal&);
    ~SCF();

    void save_data_state(StateOut&);

    RefSCMatrix eigenvectors();

    void print(ostream&o=cout);
};
SavableState_REF_dec(SCF);

#endif
