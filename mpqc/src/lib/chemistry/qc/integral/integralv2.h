
// these provide integrals using the libintv2 routines

#ifndef _chemistry_qc_integral_integralv2_h
#define _chemistry_qc_integral_integralv2_h

#include <chemistry/qc/integral/integral.h>
#include <math/topology/pointbag.h>

class OneBodyIntv2 : public OneBodyInt
{
  private:
    int same_center;
  protected:
    struct struct_centers* c1;
    struct struct_centers* c2;
  public:
    OneBodyIntv2(const RefGaussianBasisSet&, OneBodyIntIter* =0);
    OneBodyIntv2(const RefGaussianBasisSet&, const RefGaussianBasisSet&,
                 OneBodyIntIter* =0);
    ~OneBodyIntv2();
};

class OneBody3Intv2 : public OneBody3Int
{
  private:
    int same_center;
  protected:
    struct struct_centers* c1;
    struct struct_centers* c2;
  public:
    OneBody3Intv2(const RefGaussianBasisSet&, OneBodyIntIter* =0);
    OneBody3Intv2(const RefGaussianBasisSet&, const RefGaussianBasisSet&,
                  OneBodyIntIter* =0);
    ~OneBody3Intv2();
};

class GaussianOverlapIntv2 : public OneBodyIntv2
{
  public:
    GaussianOverlapIntv2(const RefGaussianBasisSet&, OneBodyIntIter* =0);
    GaussianOverlapIntv2(const RefGaussianBasisSet&,
                         const RefGaussianBasisSet&,
                         OneBodyIntIter* =0);
    ~GaussianOverlapIntv2();
    void compute_shell(int,int,double*);
};

class GaussianKineticIntv2 : public OneBodyIntv2
{
  public:
    GaussianKineticIntv2(const RefGaussianBasisSet&, OneBodyIntIter* =0);
    GaussianKineticIntv2(const RefGaussianBasisSet&,
                         const RefGaussianBasisSet&,
                         OneBodyIntIter* =0);
    ~GaussianKineticIntv2();
    void compute_shell(int,int,double*);
};

class GaussianPointChargeIntv2 : public OneBodyIntv2
{
  private:
    int ncharge;
    double** position;
    double* charge;

    void init(PointBag_double*);
    
  public:
    GaussianPointChargeIntv2(PointBag_double*, const RefGaussianBasisSet&,
                             OneBodyIntIter* =0);
    GaussianPointChargeIntv2(PointBag_double*, const RefGaussianBasisSet&,
                             const RefGaussianBasisSet&,
                             OneBodyIntIter* =0);
    ~GaussianPointChargeIntv2();
    void compute_shell(int,int,double*);
};

class GaussianNuclearIntv2 : public GaussianPointChargeIntv2
{
  public:
    GaussianNuclearIntv2(const RefGaussianBasisSet&, OneBodyIntIter* =0);
    GaussianNuclearIntv2(PointBag_double *charges,
                         const RefGaussianBasisSet&,
                         const RefGaussianBasisSet&,
                         OneBodyIntIter* =0);

    ~GaussianNuclearIntv2();
};

class GaussianEfieldDotVectorIntv2: public OneBodyIntv2
{
  private:
    double *buffer3_; // a larger buffer is needed than that provided
    double position_[3];
    double vector_[3];
  public:
    GaussianEfieldDotVectorIntv2(const RefGaussianBasisSet&,
                                 double *postion = 0,
                                 double *vector = 0,
                                 OneBodyIntIter* =0);
    GaussianEfieldDotVectorIntv2(const RefGaussianBasisSet&,
                                 const RefGaussianBasisSet&,
                                 double *postion = 0,
                                 double *vector = 0,
                                 OneBodyIntIter* =0);
    ~GaussianEfieldDotVectorIntv2();
    void position(const double*);
    void vector(const double*);
    void compute_shell(int,int,double*);
};

class GaussianDipoleIntv2: public OneBody3Intv2
{
  private:
    double origin_[3];
  public:
    GaussianDipoleIntv2(const RefGaussianBasisSet&,
                        const double *origin = 0,
                        OneBodyIntIter* =0);
    GaussianDipoleIntv2(const RefGaussianBasisSet&,
                        const RefGaussianBasisSet&,
                        const double *origin = 0,
                        OneBodyIntIter* =0);
    ~GaussianDipoleIntv2();
    void origin(const double*);
    void compute_shell(int,int,double*);
};

#endif
