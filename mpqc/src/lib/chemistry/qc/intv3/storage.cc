//
// storage.cc
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

#include <stdlib.h>
#include <util/misc/formio.h>
#include <chemistry/qc/intv3/macros.h>
#include <chemistry/qc/intv3/flags.h>
#include <chemistry/qc/intv3/types.h>

#include <chemistry/qc/intv3/storage.h>

#define PRINT_STORED 0

#ifdef __GNUC__
// instantiate the templates needed for integral storage
template class EAVLList<IntegralLink,IntegralKey>;
template class EAVLList<IntegralLink,int>;
template class EAVLNode<IntegralLink, IntegralKey>;
template class EAVLNode<IntegralLink, int>;
#endif

/////////////////////////////////////////////////////////////////////////
// IntegralLink members

IntegralLink::IntegralLink(IntegralKey& key, int cost, int size_):
  intlist(key),
  costlist(cost),
  size(size_)
{
}

void*
IntegralLink::operator new(size_t size, int intsize)
{
  return malloc(size + intsize*sizeof(double));
}

void
IntegralLink::operator delete(void* ptr)
{
  free(ptr);
}

void
IntegralLink::print()
{
  cout << scprintf("(%d %d|%d %d)[%d%d%d]",
         intlist.key.sh0(),
         intlist.key.sh1(),
         intlist.key.sh2(),
         intlist.key.sh3(),
         intlist.key.p12(),
         intlist.key.p34(),
         intlist.key.p13p24());
}

IntegralLink::~IntegralLink()
{
}

//////////////////////////////////////////////////////////////////////////
// IntegralStorer members

DescribedClass_REF_def(IntegralStorer);

#define CLASSNAME IntegralStorer
#define PARENTS public DescribedClass
#define HAVE_CTOR
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
IntegralStorer::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = DescribedClass::_castdown(cd);
  return do_castdowns(casts,cd);
}

IntegralStorer::IntegralStorer()
{
  table_size_ = 1597;
  table_ = new EAVLList<IntegralLink,IntegralKey>[table_size_];
  init(0);
}

IntegralStorer::~IntegralStorer()
{
  init(0);
  delete[] table_;
}

IntegralStorer::IntegralStorer(const RefKeyVal&keyval)
{
  table_size_ = keyval->intvalue("table_size");
  if (table_size_ <= 0) table_size_ = 1597;
  table_ = new EAVLList<IntegralLink,IntegralKey>[table_size_];

  int n_integral = keyval->intvalue("n_integral");
  init(n_integral);
}

void
IntegralStorer::store(IntegralKey &key, const double *buf,
                      int size, int cost, int actualsize)
{
  IntegralLink *link = new(size) IntegralLink(key, cost, size);

  int i;
  double *buffer = link->buffer();
  for (i=0; i<link->size; i++) {
      buffer[i] = buf[i];
    }

#if PRINT_STORED
  cout << scprintf("+++++ %d %d %d %d, %d %d %d size %5d cost %7d at 0x%x slot %5d\n",
         key.sh0(),key.sh1(),key.sh2(),key.sh3(),
         key.p12(), key.p34(), key.p13p24(),
         link->size,link->costlist.key,link,link->hash()%table_size_);
#endif

  currentsize_ += actualsize;
  n_integrals_ += size;
  n_shellquart_++;

  // if the table has grown too big, remove some of the members
  while (currentsize_ > maxsize_) {
      IntegralLink *eliminate = costlist.start();
      currentsize_ -= eliminate->actualsize();
      costlist.remove(eliminate);
      table_[eliminate->hash()%table_size_].remove(eliminate);
      n_shellquart_--;
      n_integrals_ -= eliminate->size;
#if PRINT_STORED
      cout << scprintf("----- %d %d %d %d, %d %d %d size %5d cost %7d at 0x%x slot %5d\n",
             eliminate->intlist.key.sh0(),eliminate->intlist.key.sh1(),
             eliminate->intlist.key.sh2(),eliminate->intlist.key.sh3(),
             eliminate->intlist.key.p12(), eliminate->intlist.key.p34(),
             eliminate->intlist.key.p13p24(),
             eliminate->size, eliminate->costlist.key,
             eliminate,
             eliminate->hash()%table_size_);
#endif
      delete eliminate;
    }

  // add the new shell quartet
  costlist.insert(link);
  table_[link->hash()%table_size_].insert(link);
}

int
IntegralStorer::should_store(int cost, int actualsize)
{
  int neededsize = actualsize - (maxsize_ - currentsize_);
  if (neededsize < 0) neededsize = 0;

  IntegralLink *i;
  int accumsize = 0;
  int accumcost = 0;
  for (i=costlist.start(); i; costlist.next(i)) {
      if (accumsize >= neededsize || accumcost >= cost) break;
      accumsize += i->actualsize();
      accumcost += i->cost();
    }
  //printf("should_store: asize = %5d nsize = %5d acost = %7d cost = %7d\n",
  //       accumsize, neededsize, accumcost, cost);
  if (accumsize >= neededsize) {
      if (accumcost < cost) return 1;
      return 0;
    }
  return 0;
}

IntegralLink*
IntegralStorer::find(IntegralKey& key)
{
  int hash = IntegralLink::shells_to_hash(key.sh0(),
                                          key.sh1(),
                                          key.sh2(),
                                          key.sh3());
  EAVLList<IntegralLink,IntegralKey> &list = table_[hash % table_size_];
  IntegralLink* link = list.find(key);
  return link;
}

void
IntegralStorer::init(int nbytes)
{
  costlist.initialize(&IntegralLink::costlist);
  for (int i=0; i<table_size_; i++) {
      table_[i].initialize(&IntegralLink::intlist);
    }
  done();
  maxsize_ = nbytes;
  // estimate a 50% loss due to memory fragmentation
  maxsize_ /= 2;
}

void
IntegralStorer::done()
{
  n_shellquart_ = 0;
  n_integrals_ = 0;
  currentsize_ = 0;
  maxsize_ = 0;

  int i;
  for (i=0; i<table_size_; i++) {
      table_[i].clear_without_delete();
    }
  costlist.clear();
}

void
IntegralStorer::print_stats()
{
  if (currentsize_ == 0) return;
  cout << scprintf("IntegralStorer: n_integrals  = %d\n", n_integrals_);
  cout << scprintf("IntegralStorer: n_shellquart = %d\n", n_shellquart_);
  cout << scprintf("IntegralStorer: currentsize = %d\n", currentsize_);
  cout << scprintf("IntegralStorer: maxsize     = %d\n", maxsize_);
#if 0
  cout << scprintf("IntegralStorer: hash table distribution:\n");
  int tlength = 0;
  int i;
  for (i=0; i<table_size_; i++) {
      int length = table_[i].length();
      tlength += length;
      cout << scprintf(" %5d", length);
      if ((i+1)%13 == 0) cout << scprintf("\n");
    }
  if (i%13 != 0) cout << scprintf("\n");
  cout << scprintf("IntegralStorer: hash table length sum = %d\n", tlength);
  cout << scprintf("IntegralStorer: cost list length      = %d\n", costlist.length());
#endif
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
