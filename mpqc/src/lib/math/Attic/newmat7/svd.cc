//$$svd.cxx                           singular value decomposition

// Copyright (C) 1991,2,3: R B Davies

#define WANT_MATH

#include "include.h"
#include "newmat.h"
#include "newmatrm.h"
#include "precisio.h"


void SVD(const Matrix& A, DiagonalMatrix& Q, Matrix& U, Matrix& V,
   Boolean withU, Boolean withV)
// from Wilkinson and Reinsch: "Handbook of Automatic Computation"
{
   Tracer trace("SVD");
   Real eps = FloatingPointPrecision::Epsilon();
   Real tol = FloatingPointPrecision::Minimum()/eps;

   int m = A.Nrows(); int n = A.Ncols();
   if (m<n) 
      Throw(ProgramException("Want no. Rows >= no. Cols", A));

   U = A; Real g = 0.0; Real f,h; Real x = 0.0;
   RowVector E(n); RectMatrixRow EI(E,0); Q.ReDimension(n);
   RectMatrixCol UCI(U,0); RectMatrixRow URI(U,0,1,n-1);

   for (int i=0; i<n; i++)
   {
      EI.First() = g; Real ei = g; EI.Right(); Real s = UCI.SumSquare();
      if (s<tol) Q.element(i) = 0.0;
      else
      {
	 f = UCI.First(); g = -sign(sqrt(s), f); h = f*g-s; UCI.First() = f-g;
	 Q.element(i) = g; RectMatrixCol UCJ = UCI; int j=n-i;
	 while (--j) { UCJ.Right(); UCJ.AddScaled(UCI, (UCI*UCJ)/h); }
      }

      s = URI.SumSquare();
      if (s<tol) g = 0.0;
      else
      {
	 f = URI.First(); g = -sign(sqrt(s), f); URI.First() = f-g;
	 EI.Divide(URI,f*g-s); RectMatrixRow URJ = URI; int j=m-i;
	 while (--j) { URJ.Down(); URJ.AddScaled(EI, URI*URJ); }
      }

      Real y = fabs(Q.element(i)) + fabs(ei); if (x<y) x = y;
      UCI.DownDiag(); URI.DownDiag();
   }

   if (withV)
   {
      V.ReDimension(n,n); V = 0.0; RectMatrixCol VCI(V,n,n,0);
      for (i=n-1; i>=0; i--)
      {
	 URI.UpDiag(); VCI.Left();
	 if (g!=0.0)
	 {
	    VCI.Divide(URI, URI.First()*g); int j = n-i;
	    RectMatrixCol VCJ = VCI;
	    while (--j) { VCJ.Right(); VCJ.AddScaled( VCI, (URI*VCJ) ); }
	 }
	 VCI.Zero(); VCI.Up(); VCI.First() = 1.0; g=E.element(i);
      }
   }

   if (withU)
   {
      for (i=n-1; i>=0; i--)
      {
	 UCI.UpDiag(); g = Q.element(i); URI.Reset(U,i,i+1,n-i-1); URI.Zero();
         if (g!=0.0)
         {
	    h=UCI.First()*g; int j=n-i; RectMatrixCol UCJ = UCI;
	    while (--j)
            {
	       UCJ.Right(); UCI.Down(); UCJ.Down(); Real s = UCI*UCJ;
               UCI.Up(); UCJ.Up(); UCJ.AddScaled(UCI,s/h);
            }
	    UCI.Divide(g);
         }
         else UCI.Zero();
         UCI.First() += 1.0;
      }
   }

   eps *= x;
   for (int k=n-1; k>=0; k--)
   {
      Real z; Real y; int limit = 50; int l;
      while (limit--)
      {
	 Real c=0.0; Real s=1.0; int i; int l1=k; Boolean tfc=FALSE;
	 for (l=k; l>=0; l--)
	 {
//          if (fabs(E.element(l))<=eps) goto test_f_convergence;
	    if (fabs(E.element(l))<=eps) { tfc=TRUE; break; }
	    if (fabs(Q.element(l-1))<=eps) { l1=l; break; }
	 }
         if (!tfc)
         {
	    l=l1; l1=l-1;
	    for (i=l; i<=k; i++)
	    {
	       f = s * E.element(i); E.element(i) *= c;
//             if (fabs(f)<=eps) goto test_f_convergence;
	       if (fabs(f)<=eps) break;
	       g = Q.element(i); h = sqrt(f*f + g*g); Q.element(i) = h;
	       if (withU)
	       {
	          RectMatrixCol UCI(U,i); RectMatrixCol UCJ(U,l1);
	          ComplexScale(UCI, UCJ, g/h, -f/h);
               }
            }
	 }
//    test_f_convergence: z = Q.element(k); if (l==k) goto convergence;
	 z = Q.element(k);  if (l==k) break;

	 x = Q.element(l); y = Q.element(k-1);
	 g = E.element(k-1); h = E.element(k);
	 f = ((y-z)*(y+z) + (g-h)*(g+h)) / (2*h*y); g = sqrt(f*f + 1);
	 f = ((x-z)*(x+z) + h*(y / ((f<0.0) ? f-g : f+g)-h)) / x;

	 c = 1.0; s = 1.0;
	 for (i=l+1; i<=k; i++)
	 {
	    g = E.element(i); y = Q.element(i); h = s*g; g *= c;
	    z = sqrt(f*f + h*h); E.element(i-1) = z; c = f/z; s = h/z;
	    f = x*c + g*s; g = -x*s + g*c; h = y*s; y *= c;
	    if (withV)
	    {
	       RectMatrixCol VCI(V,i); RectMatrixCol VCJ(V,i-1);
	       ComplexScale(VCI, VCJ, c, s);
	    }
	    z = sqrt(f*f + h*h); Q.element(i-1) = z; c = f/z; s = h/z;
	    f = c*g + s*y; x = -s*g + c*y;
	    if (withU)
	    {
	       RectMatrixCol UCI(U,i); RectMatrixCol UCJ(U,i-1);
	       ComplexScale(UCI, UCJ, c, s);
	    }
	 }
	 E.element(l) = 0.0; E.element(k) = f; Q.element(k) = x;
      }
      if (l!=k) { Throw(ConvergenceException(A)); }
// convergence:
      if (z < 0.0)
      {
	 Q.element(k) = -z;
	 if (withV) { RectMatrixCol VCI(V,k); VCI.Negate(); }
      }
   }
}

void SVD(const Matrix& A, DiagonalMatrix& D)
{ Matrix U; SVD(A, D, U, U, FALSE, FALSE); }


