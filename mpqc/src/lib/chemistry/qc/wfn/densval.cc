#include <stdio.h>
#include <iostream.h>
#include <math/newmat7/newmat.h>
#include <util/keyval/keyval.h>
#include <math/topology/point.h>
#include <chemistry/qc/basis/basis.h>

#include "wfn.h"

// Function for returning electron charge density at a point
double Wavefunction::density(cart_point&r)
{
  int nbasis = basis().nbasis();
  if (!bs_values) bs_values=new double[nbasis];

  // compute the basis set values
  basis().values(r,bs_values);

  //for (int i=0; i<nbasis; i++)
  //     printf("bs_values[%d] = % 12.8f\n",i,bs_values[i]);

  // Assuming this will be called many times for the same wavefunction,
  // it is more efficient to force the computation of the natural
  // orbitals now and use them.
  // get the natural orbitals and density
  const Matrix& nos = natural_orbitals();
  const DiagonalMatrix& nd = natural_density();
  
  // loop over natural orbitals adding contributions to elec_density
  double elec_density=0.0;
  for (int i=0; i<nbasis; i++) {
      double tmp = 0.0;
      for (int j=0; j<nbasis; j++) {
          tmp += nos.element(j,i)*bs_values[j];
        }
      elec_density += nd.element(i,i)*tmp*tmp;
    }

  return elec_density;
}     

// Function for returning electron charge density at a point.
// The grad at that point is also computed and put into double grad[3].
double Wavefunction::density_gradient(cart_point&r,double*grad)
{
  int nbasis = basis().nbasis();
  if (!bs_values) bs_values=new double[nbasis];
  if (!bsg_values) bsg_values=new double[nbasis*3];

  // compute the grad values and get the basis set values at the
  // same time
  basis().grad_values(r,bsg_values,bs_values);

  //for (int i=0; i<nbasis; i++)
  //     printf("bs_values[%d] = % 12.8f\n",i,bs_values[i]);

  // get the natural orbitals and density
  const Matrix& nos = natural_orbitals();
  const DiagonalMatrix& nd = natural_density();
    
  // loop over natural orbitals adding contributions to elec_density
  double elec_density=0.0;
  grad[0] = grad[1] = grad[2] = 0.0;
  for (int i=0; i<nbasis; i++) {
      double tmp = 0.0;
      for (int j=0; j<nbasis; j++) {
          tmp += nos.element(j,i)*bs_values[j];
        }
      elec_density += nd.element(i,i)*tmp*tmp;
      double tmpg[3];
      tmpg[0] = tmpg[1] = tmpg[2] = 0.0;
      for (j=0; j<nbasis; j++) {
          tmpg[0] += nos.element(j,i)*bsg_values[j*3+0];
          tmpg[1] += nos.element(j,i)*bsg_values[j*3+1];
          tmpg[2] += nos.element(j,i)*bsg_values[j*3+2];
        }
      grad[0] += nd.element(i,i)*tmpg[0]*tmp;
      grad[1] += nd.element(i,i)*tmpg[1]*tmp;
      grad[2] += nd.element(i,i)*tmpg[2]*tmp;
    }

  grad[0] *= 2.0;
  grad[1] *= 2.0;
  grad[2] *= 2.0;

  return elec_density;
}     
