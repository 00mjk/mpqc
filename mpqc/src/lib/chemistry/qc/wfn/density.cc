//
// density.cc
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@limitpt.com>
// Maintainer: LPS
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

#include <util/misc/formio.h>
#include <util/render/polygons.h>
#include <math/scmat/local.h>
#include <math/scmat/vector3.h>
#include <chemistry/molecule/molecule.h>
#include <chemistry/qc/wfn/density.h>

#define CLASSNAME ElectronDensity
#define PARENTS public Volume
#define HAVE_KEYVAL_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
ElectronDensity::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = Volume::_castdown(cd);
  return do_castdowns(casts,cd);
}

ElectronDensity::ElectronDensity(const RefKeyVal &keyval):
  Volume(keyval)
{
  wfn_ = keyval->describedclassvalue("wfn");
}

ElectronDensity::ElectronDensity(const RefWavefunction& wfn):
  Volume(),
  wfn_(wfn)
{
}

ElectronDensity::~ElectronDensity()
{
}

void
ElectronDensity::compute()
{
  SCVector3 r;
  get_x(r);
  // do_gradient will automatically cause the value to be computed
  if (do_gradient()) {
      double v[3];
      set_value(wfn_->density_gradient(r,v));
      SCVector3 d(v);
      set_gradient(d);
    }
  else if (do_value()) {
      set_value(wfn_->density(r));
    }
  if (do_hessian()) {
      cerr << node0 << indent
           << "ElectronDensity::compute(): hessian isn't yet implemented\n";
      abort();
    }
}

// make a wild guess about the bounding box
void
ElectronDensity::boundingbox(double valuemin,
                             double valuemax,
                             SCVector3& p1, SCVector3& p2)
{
  Molecule& mol = *wfn_->molecule();

  if (mol.natom() == 0) {
      for (int i=0; i<3; i++) p1[i] = p2[i] = 0.0;
    }

  int i;
  for (i=0; i<3; i++) p1[i] = p2[i] = mol[0][i];
  for (i=1; i<mol.natom(); i++) {
      for (int j=0; j<3; j++) {
          if (mol[i][j] < p1[i]) p1[i] = mol[i][j];
          if (mol[i][j] > p2[i]) p2[i] = mol[i][j];
        }
    }
  for (i=0; i<3; i++) {
      p1[i] = p1[i] - 3.0;
      p2[i] = p2[i] + 3.0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// DensityColorizer

#define CLASSNAME DensityColorizer
#define PARENTS public MoleculeColorizer
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
DensityColorizer::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = MoleculeColorizer::_castdown(cd);
  return do_castdowns(casts,cd);
}

DensityColorizer::DensityColorizer(const RefKeyVal&keyval):
  MoleculeColorizer(keyval)
{
  wfn_ = keyval->describedclassvalue("wfn");
  reference_ = keyval->doublevalue("reference");
  if (keyval->error() == KeyVal::OK) have_reference_ = 1;
  else have_reference_ = 0;
  scale_ = keyval->doublevalue("scale");
  if (keyval->error() == KeyVal::OK) have_scale_ = 1;
  else have_scale_ = 0;
}

DensityColorizer::~DensityColorizer()
{
}

void
DensityColorizer::colorize(const RefRenderedPolygons &poly)
{
  const double base = 0.3;
  int i;
  int nvertex = poly->nvertex();

  if (nvertex) {
      double *data = new double[nvertex];

      for (i=0; i<nvertex; i++) {
          SCVector3 v(poly->vertex(i));
          data[i] = wfn_->density(v);
        }

      double min = data[0], max = data[0];
      for (i=1; i<nvertex; i++) {
          if (min > data[i]) min = data[i];
          if (max < data[i]) max = data[i];
        }

      double center, scale;

      if (have_reference_) center = reference_;
      else center = (max+min)/2.0; 

      double maxdiff = fabs(max - center);
      double mindiff = fabs(min - center);

      if (have_scale_) {
          scale = scale_;
        }
      else {
          if (maxdiff>mindiff && maxdiff>1.0e-6) scale = (1.0-base)/maxdiff;
          else if (mindiff>1.0e-6) scale = (1.0-base)/mindiff;
          else scale = (1.0-base);
        }

      cout << node0 << indent << "DensityColorizer:"
           << scprintf(" reference=%6.5f", center)
           << scprintf(" scale=%8.4f",scale)
           << scprintf(" (%6.5f<=rho<=%6.5f)", max, min)
           << endl;
      for (i=0; i<nvertex; i++) {
          data[i] = (data[i]-center)*scale;
        }

      for (i=0; i<nvertex; i++) {
          Color c;
          if (data[i] < 0.0) c.set_rgb(-data[i]+base,0.3,0.3);
          else c.set_rgb(0.3,0.3,data[i]+base);
          poly->set_vertex_color(i,c);
        }

      delete[] data;
    }
}

/////////////////////////////////////////////////////////////////////////////
// GradDensityColorizer

#define CLASSNAME GradDensityColorizer
#define PARENTS public MoleculeColorizer
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
GradDensityColorizer::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = MoleculeColorizer::_castdown(cd);
  return do_castdowns(casts,cd);
}

GradDensityColorizer::GradDensityColorizer(const RefKeyVal&keyval):
  MoleculeColorizer(keyval)
{
  wfn_ = keyval->describedclassvalue("wfn");
  reference_ = keyval->doublevalue("reference");
  if (keyval->error() == KeyVal::OK) have_reference_ = 1;
  else have_reference_ = 0;
  scale_ = keyval->doublevalue("scale");
  if (keyval->error() == KeyVal::OK) have_scale_ = 1;
  else have_scale_ = 0;
}

GradDensityColorizer::~GradDensityColorizer()
{
}

void
GradDensityColorizer::colorize(const RefRenderedPolygons &poly)
{
  const double base = 0.3;
  int i;
  int nvertex = poly->nvertex();

  if (nvertex) {
      double *data = new double[nvertex];

      for (i=0; i<nvertex; i++) {
          SCVector3 v(poly->vertex(i));
          SCVector3 g;
          wfn_->density_gradient(v,g.data());
          data[i] = g.norm();
        }

      double min = data[0], max = data[0];
      for (i=1; i<nvertex; i++) {
          if (min > data[i]) min = data[i];
          if (max < data[i]) max = data[i];
        }

      double center, scale;

      if (have_reference_) center = reference_;
      else center = (max+min)/2.0; 

      double maxdiff = fabs(max - center);
      double mindiff = fabs(min - center);

      if (have_scale_) {
          scale = scale_;
        }
      else {
          if (maxdiff>mindiff && maxdiff>1.0e-6) scale = (1.0-base)/maxdiff;
          else if (mindiff>1.0e-6) scale = (1.0-base)/mindiff;
          else scale = (1.0-base);
        }

      cout << node0 << indent << "GradDensityColorizer:"
           << scprintf(" reference=%6.5f", center)
           << scprintf(" scale=%6.2f",scale)
           << scprintf(" (%6.5f<=rho<=%6.5f)", max, min)
           << endl;
      for (i=0; i<nvertex; i++) {
          data[i] = (data[i]-center)*scale;
        }

      for (i=0; i<nvertex; i++) {
          Color c;
          if (data[i] < 0.0) c.set_rgb(-data[i]+base,0.3,0.3);
          else c.set_rgb(0.3,0.3,data[i]+base);
          poly->set_vertex_color(i,c);
        }

      delete[] data;
    }
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
