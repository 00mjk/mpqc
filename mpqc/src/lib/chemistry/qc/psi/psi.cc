
#include "psi.h"
#include "file11.h"

extern "C" {
#include <stdlib.h>
#include <math.h>
}

#include <util/misc/formio.h>
#include <util/keyval/keyval.h>
#include <util/misc/libmisc.h>
#include <math/scmat/matrix.h>
#include <math/symmetry/pointgrp.h>
#include <chemistry/molecule/molecule.h>

#define CLASSNAME PSISCF
#define PARENTS public OneBodyWavefunction
#define HAVE_KEYVAL_CTOR
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
PSISCF::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = OneBodyWavefunction::_castdown(cd);
  return do_castdowns(casts,cd);
}

PSISCF::PSISCF(const RefKeyVal&keyval):
  OneBodyWavefunction(keyval), psi_in(keyval)
{
  printf("in PSISCF constructor\n");
}

PSISCF::PSISCF(StateIn&s):
  OneBodyWavefunction(s)
  maybe_SavableState(s)
{
  abort();
}

PSISCF::~PSISCF()
{
  printf("in PSISCF destructor\n");
}

void
PSISCF::save_data_state(StateOut&s)
{
  OneBodyWavefunction::save_data_state(s);
//  s.putstring(opentype);
//  s.put(docc,_mol->point_group().char_table().nirrep());
//  s.put(socc,_mol->point_group().char_table().nirrep());
//  s.putstring(label);
}

void
PSISCF::print(ostream& o)
{
  OneBodyWavefunction::print(o);
  psi_in.print(o);

  o << indent << "grad_convergence = " << 
           desired_gradient_accuracy() << endl;
  o << indent << "energy_convergence = " << 
           desired_value_accuracy() << endl;
 
}

double
PSISCF::occupation(int)
{
  fprintf(stderr, "cannot do occupations");
  abort();
}

void
PSISCF::compute()
{
  int i;
  int ni = molecule()->point_group().char_table().nirrep();

  //molecule()->transform_to_principal_axes();

  if (!psi_in.test()){
    psi_in.write_input_file(do_gradient() ? "FIRST" : "NONE", "SCF",
      (int)-log10(desired_value_accuracy()), "input.dat");

    system("inputth");
    system("psi");
    }
  
  // read output
  if (do_gradient()) {
    int i, j, ii;
    int *reorder;
    double tol = 1e-6;
    reorder = new int[molecule()->natom()];
    FILE11 file11(0);
    for(i=0; i<molecule()->natom(); i++)
      for(j=0; j<molecule()->natom(); j++)
        if( fabs(file11.coordinate(0,i) - molecule()->atom(j)[0]) < tol &&
	    fabs(file11.coordinate(1,i) - molecule()->atom(j)[1]) < tol && 
	    fabs(file11.coordinate(2,i) - molecule()->atom(j)[2]) < tol){
          reorder[i] = j; 
	  break;
	  }
    RefSCVector g(moldim(),matrixkit());

    for (ii=0, i=0; i<molecule()->natom(); i++) {
      for (j=0; j<3; j++, ii++) {
        g(ii) = file11.gradient(j,reorder[i]);
        }
      }
    set_gradient(g);
    set_actual_gradient_accuracy(desired_gradient_accuracy());
    set_energy(file11.energy());
    }
  else {
      // read the energy from the output file
      FILE *in;
      system("rm -f psitmp.energy");
      if(!psi_in.test()){
        system("grep \"total energy *=\" output.dat > psitmp.energy");
	}
      in = fopen("psitmp.energy","r");
      if (!in) {
          fprintf(stderr,"PSISCF::compute(): cannot open psitmp.energy\n");
          abort();
        }
      double r;
      fscanf(in,"%*s %*s %*s %lf", &r);
      set_energy(r);
      fclose(in);
    }

  // clean up
}

double
PSISCF::occupation(int,int)
{
  fprintf(stderr, "cannot do occupation");
  abort();
  return 0;
}

RefDiagSCMatrix
PSISCF::eigenvalues()
{
  fprintf(stderr, "cannot do eigenvalues");
  abort();
  return 0;
}

RefSCMatrix
PSISCF::eigenvectors()
{
  fprintf(stderr, "cannot do eigenvectors");
  abort();
  return 0;
}

int
PSISCF::value_implemented()
{
  return 1;
}

int
PSISCF::gradient_implemented()
{
  return 1;
}
