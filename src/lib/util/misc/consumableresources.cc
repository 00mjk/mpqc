//
// consumableresources.cc
//
// Copyright (C) 2010 Edward Valeev
//
// Author: Edward Valeev <evaleev@vt.edu>
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

#include <limits>
#include <stdexcept>
#include <sstream>
#include <util/misc/consumableresources.h>
#include <util/class/scexception.h>

using namespace sc;

size_t ConsumableResources::defaults::memory = 0; // assume infinite memory
std::pair<std::string,size_t> ConsumableResources::defaults::disk = make_pair(std::string("./"), size_t(0));

ClassDesc
ConsumableResources::class_desc_(typeid(ConsumableResources),
                     "ConsumableResources",
                     1,               // version
                     "virtual public SavableState", // must match parent
                     create<ConsumableResources>, // change to create<ConsumableResources> if this class is DefaultConstructible
                     create<ConsumableResources>, // change to 0 if this class is not KeyValConstructible
                     create<ConsumableResources>  // change to 0 if this class is not StateInConstructible
                     );

ConsumableResources::ConsumableResources() :
    memory_(defaults::memory), disk_(defaults::disk) {
  if (memory_ == 0)
    memory_ = rsize(std::numeric_limits<size_t>::max());
  if (disk_.second == 0)
    disk_.second = rsize(std::numeric_limits<size_t>::max());
}

ConsumableResources::ConsumableResources(const Ref<KeyVal>& kv) {
  memory_ = kv->sizevalue("memory", KeyValValuesize(defaults::memory));
  if (memory_ == 0)
    memory_ = rsize(std::numeric_limits<size_t>::max());

  if (kv->exists("disk"))
    if (kv->count("disk") != 2)
      throw InputError("invalid specification, must be array of length 2", __FILE__, __LINE__,
                       "disk", "", class_desc());

  disk_.first = kv->stringvalue("disk", 0, KeyValValuestring(defaults::disk.first));
  size_t disk_size = kv->sizevalue("disk", 1, KeyValValuesize(defaults::disk.second));
  if (disk_size == 0)
    disk_.second = rsize(std::numeric_limits<size_t>::max());

}

ConsumableResources::ConsumableResources(StateIn& si) : SavableState(si) {
  memory_ & si;
  si.get(disk_.first);
  disk_.second & si;
}

ConsumableResources::~ConsumableResources() {
  const bool make_sure_class_desc_initialized = (&class_desc_ != 0);

  if (!managed_arrays_.empty()) {
    std::size_t total_unfreed_memory = 0;
    for(std::map<void*, std::size_t>::const_iterator v=managed_arrays_.begin();
        v != managed_arrays_.end();
        ++v)
      total_unfreed_memory += v->second;
    if (total_unfreed_memory > 0)
      ExEnv::err0() << indent << scprintf("WARNING: %ld bytes managed by ConsumableResources was not explicitly deallocated!", total_unfreed_memory) << std::endl;
  }
}

void
ConsumableResources::save_data_state(StateOut& so) {
  memory_ & so;
  so.put(disk_.first);
  disk_.second & so;
}

// Liberally borrowed from ThreadGrp
Ref<ConsumableResources>
ConsumableResources::initial_instance(int& argc, char ** argv)
{
  Ref<ConsumableResources> inst;
  char * keyval_string = 0;

  // see if given on the command line
  if (argc && argv) {
    for (int i=0; i < argc; i++) {
      if (argv[i] && !strcmp(argv[i], "-resources")) {
        char *cr_string = argv[i];
        i++;
        if (i >= argc) {
          throw std::runtime_error("-resources must be followed by an argument");
        }
        keyval_string = argv[i];
        // move the ConsumableResources arguments to the end of argv
        int j;
        for (j=i+1; j<argc; j++) {
          argv[j-2] = argv[j];
        }
        argv[j++] = cr_string;
        argv[j++] = keyval_string;
        // decrement argc to hide the last two arguments
        argc -= 2;
        break;
      }
    }
  }

  if (!keyval_string) {
    // find out if the environment gives the resources
    keyval_string = getenv("SC_RESOURCES");
    if (keyval_string) {
      if (!strncmp("SC_RESOURCES=", keyval_string, 13)) {
        keyval_string = strchr(keyval_string, '=');
      }
      if (*keyval_string == '=') keyval_string++;
    }
  }

  // if keyval input for a integral was found, then
  // create it.
  if (keyval_string) {
    if (keyval_string[0] == '\0') return 0;
    Ref<ParsedKeyVal> strkv = new ParsedKeyVal();
    strkv->parse_string(keyval_string);
    Ref<DescribedClass> dc = strkv->describedclassvalue();
    inst = dynamic_cast<ConsumableResources*>(dc.pointer());
    if (dc.null()) {
      std::ostringstream errmsg;
      errmsg << "ConsumableResources::initial_instance: couldn't find a ConsumableResources in " << keyval_string << std::ends;
      throw std::runtime_error(errmsg.str());
    } else if (!inst) {
      std::ostringstream errmsg;
      errmsg << "ConsumableResources::initial_instance: wanted ConsumableResources but got " << dc->class_name() << std::ends;
      throw std::runtime_error(errmsg.str());
    }
    // prevent an accidental delete
    inst->reference();
    strkv = 0;
    dc = 0;
    // accidental delete not a problem anymore since all smart pointers
    // to inst are dead
    inst->dereference();
    return inst;
  }

  return 0;
}

Ref<ConsumableResources> ConsumableResources::default_instance_(new ConsumableResources);

void
ConsumableResources::set_default_instance(const Ref<ConsumableResources>& inst)
{
  default_instance_ = inst;
}

const Ref<ConsumableResources>&
ConsumableResources::get_default_instance()
{
  return default_instance_;
}

std::string
ConsumableResources::sprint() const {
  std::ostringstream o;
  o << indent << "ConsumableResources: ( memory = " << rsize::value_to_string(memory_.max_value());
  o << indent << " disk = [" << disk_.first << " " << rsize::value_to_string(disk_.second.max_value()) <<"] )";
  return o.str();
}

void
ConsumableResources::print(std::ostream& o, bool print_state, bool print_stats) const {
  o << indent << "ConsumableResources: (" << std::endl << incindent;

  o << indent << "memory = " << rsize::value_to_string(memory_.max_value());
  if (print_state || print_stats) {
    o << " (";
    if (print_state) o << " avail: " << rsize::value_to_string(memory_.value()) << " ";
    if (print_stats) o << " max used: " << rsize::difference_to_string(memory_.max_value() - memory_.lowest_value()) << " ";
    o << ")";
  }
  o << std::endl;

  o << indent << "disk = [" << disk_.first << " " << rsize::value_to_string(disk_.second.max_value()) <<"]";
  if (print_state || print_stats) {
    o << " (";
    if (print_state) o << " avail: " << rsize::value_to_string(disk_.second.value()) << " ";
    if (print_stats) o << " max used: " << rsize::difference_to_string(disk_.second.max_value() - disk_.second.lowest_value()) << " ";
    o << ")";
  }
  o << std::endl << decindent;

  o << indent << ")" << std::endl;
}

size_t ConsumableResources::max_memory() const { return memory_.max_value(); }
size_t ConsumableResources::max_disk() const { return disk_.second.max_value(); }

size_t ConsumableResources::memory() const { return memory_; }
size_t ConsumableResources::disk() const { return disk_.second; }

void ConsumableResources::consume_memory(size_t value) {
  rsize& resource = memory_;
  if (value <= resource)
    resource -= value;
  else
    throw LimitExceeded<size_t>("not enough memory",
                                __FILE__, __LINE__,
                                resource.max_value(),
                                resource.max_value() - resource.value() + value,
                                class_desc());
}
void ConsumableResources::consume_disk(size_t value) {
  rsize& resource = disk_.second;
  if (value <= resource)
    resource -= value;
  else
    throw LimitExceeded<size_t>("not enough disk",
                                __FILE__, __LINE__,
                                resource.max_value(),
                                resource.max_value() - resource.value() + value,
                                class_desc());
}

void ConsumableResources::release_memory(size_t value) {
  rsize& resource = memory_;
  if (value + resource <= resource.max_value())
    resource += value;
  else
#if !IGNORE_RESOURCE_OVERUSE
    throw ProgrammingError("too much memory released -- some memory consumption must be untracked",
                           __FILE__, __LINE__,
                           class_desc());
#else
    resource += value;
#endif
}
void ConsumableResources::release_disk(size_t value) {
  rsize& resource = disk_.second;
  if (value + resource <= resource.max_value())
    resource += value;
  else
    throw ProgrammingError("too much disk released -- some disk consumption must be untracked",
                           __FILE__, __LINE__,
                           class_desc());
}

const std::string& ConsumableResources::disk_location() const { return disk_.first; }

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
