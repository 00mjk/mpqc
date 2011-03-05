//
// tbint_runtime.h
//
// Copyright (C) 2009 Edward Valeev
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

#ifdef __GNUG__
#pragma interface
#endif

#ifndef _mpqc_src_lib_chemistry_qc_mbptr12_tbint_runtime_h
#define _mpqc_src_lib_chemistry_qc_mbptr12_tbint_runtime_h

#include <string>
#include <util/state/state.h>
#include <chemistry/qc/basis/intdescr.h>
#include <math/distarray4/distarray4.h>
#include <chemistry/qc/wfn/spin.h>
#include <chemistry/qc/lcao/transform_factory.h>

namespace sc {


  /// Parsed representation of a string key that represents a set of 4-center 2-body integrals
  class ParsedTwoBodyFourCenterIntKey {
    public:
      ParsedTwoBodyFourCenterIntKey(const std::string& key);

      const std::string& key() const { return key_; }
      const std::string& bra1() const { return bra1_; }
      const std::string& bra2() const { return bra2_; }
      const std::string& ket1() const { return ket1_; }
      const std::string& ket2() const { return ket2_; }
      const std::string& oper() const { return oper_; }
      const std::string& params() const { return params_; }
      const std::string& layout() const { return layout_; }

      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& ket1,
                             const std::string& ket2,
                             const std::string& oper,
                             const std::string& params,
                             const std::string& layout);
      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& ket1,
                             const std::string& ket2,
                             const std::string& descr,
                             const std::string& layout);
      /// computes operator part of the key given an TwoBodyIntDescr object
      static std::string key(const Ref<TwoBodyIntDescr>& descr);
      /// this factory method constructs a descriptor given operator key + IntParams object + Integrals object
      static Ref<TwoBodyIntDescr> create_descr(const std::string& oper_key,
                                               const Ref<IntParams>& p,
                                               const Ref<Integral>& integral);

    private:
      std::string key_;
      std::string bra1_, bra2_, ket1_, ket2_;
      std::string oper_;
      std::string params_;
      std::string layout_;
  };

  /// Parsed representation of a string key that represents a set of 3-center 2-body integrals
  class ParsedTwoBodyThreeCenterIntKey {
    public:
      ParsedTwoBodyThreeCenterIntKey(const std::string& key);

      const std::string& key() const { return pkey_.key(); }
      const std::string& bra1() const { return pkey_.bra1(); }
      const std::string& bra2() const { return pkey_.bra2(); }
      const std::string& ket1() const { return pkey_.ket1(); }
      const std::string& oper() const { return pkey_.oper(); }
      const std::string& params() const { return pkey_.params(); }

      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& ket1,
                             const std::string& oper,
                             const std::string& params);
      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& ket1,
                             const std::string& descr);
      /// computes operator part of the key given an TwoBodyThreeCenterIntDescr object
      static std::string key(const Ref<TwoBodyThreeCenterIntDescr>& descr);
      /// this factory method constructs a descriptor given operator key + IntParams object + Integrals object
      static Ref<TwoBodyThreeCenterIntDescr> create_descr(const std::string& oper_key,
                                                          const Ref<IntParams>& p,
                                                          const Ref<Integral>& integral);

    private:
      // implemented in terms of the 4-center parser
      ParsedTwoBodyFourCenterIntKey pkey_;
  };

  /// Parsed representation of a string key that represents a set of 2-center 2-body integrals
  class ParsedTwoBodyTwoCenterIntKey {
    public:
      ParsedTwoBodyTwoCenterIntKey(const std::string& key);

      const std::string& key() const { return pkey_.key(); }
      const std::string& bra1() const { return pkey_.bra1(); }
      const std::string& bra2() const { return pkey_.bra2(); }
      const std::string& oper() const { return pkey_.oper(); }
      const std::string& params() const { return pkey_.params(); }

      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& oper,
                             const std::string& params);
      /// computes key from its components
      static std::string key(const std::string& bra1,
                             const std::string& bra2,
                             const std::string& descr);
      /// computes operator part of the key given an TwoBodyThreeCenterIntDescr object
      static std::string key(const Ref<TwoBodyTwoCenterIntDescr>& descr);
      /// this factory method constructs a descriptor given operator key + IntParams object + Integrals object
      static Ref<TwoBodyTwoCenterIntDescr> create_descr(const std::string& oper_key,
                                                        const Ref<IntParams>& p,
                                                        const Ref<Integral>& integral);

    private:
      // implemented in terms of the 4-center parser
      ParsedTwoBodyFourCenterIntKey pkey_;
  };

  /// describes the physical layout of the integrals in TwoBodyIntsAcc
  class TwoBodyIntLayout {
    public:
      static TwoBodyIntLayout b1b2_k1k2;  // physicists layout
      static TwoBodyIntLayout b1k1_b2k2;  // chemists layout

      TwoBodyIntLayout(const std::string& str);
      TwoBodyIntLayout(const TwoBodyIntLayout& other);
      TwoBodyIntLayout& operator=(const TwoBodyIntLayout& other);
      bool operator==(const TwoBodyIntLayout& other) const;
      operator std::string();

    private:
      typedef enum {
        _b1b2_k1k2, // physicists layout
        _b1k1_b2k2  // chemists layout
      } Type;
      Type type_;
  };

  /// this is a singleton registry that holds IntParams objects.
  class ParamsRegistry : public RefCount {

    public:
      /// this is a singleton
      static const Ref<ParamsRegistry>& instance();

      /** Returns key that describes params. If not registered, will do so using register_params(params)
       */
      std::string key(const Ref<IntParams>& params) const;
      /** Returns params that correspond to key. Returns null ptr if key is not known.
       */
      Ref<IntParams> value(const std::string& key) const;
      /// register key->params mapping
      void add(const std::string& key, const Ref<IntParams>& params) const;
      /// register key->params mapping. A unique random key is generated automatically and returned
      std::string add(const Ref<IntParams>& params) const;

    private:
      ParamsRegistry();

      static Ref<ParamsRegistry> instance_;

      // Maps key to IntParams. IntParams objects are not unique, hence should compare them instead of pointers
      typedef Registry<std::string, Ref<IntParams>, detail::NonsingletonCreationPolicy, std::equal_to<std::string>,
      RefObjectEqual<IntParams> > RegistryType;

      Ref<RegistryType> params_;
  };

  class DensityFittingInfo;
  namespace detail {
    // computes the type of the object that holds the two-body MO integrals
    // with the given number of centers
    template <int NumCenters> struct TwoBodyIntEval;
    template <> struct TwoBodyIntEval<4> {
      typedef TwoBodyMOIntsTransform value;
      typedef Ref<value> refvalue;
    };
    template <> struct TwoBodyIntEval<3> {
      typedef TwoBodyThreeCenterMOIntsTransform value;
      typedef Ref<value> refvalue;
    };
    template <> struct TwoBodyIntEval<2> {
      typedef RefSCMatrix value;
      typedef value refvalue;
    };
    // computes the type of the object that parses the labels for two-body MO integrals
    // with the given number of centers
    template <int NumCenters> struct ParsedTwoBodyIntKey;
    template <> struct ParsedTwoBodyIntKey<4> {
      typedef ParsedTwoBodyFourCenterIntKey value;
    };
    template <> struct ParsedTwoBodyIntKey<3> {
      typedef ParsedTwoBodyThreeCenterIntKey value;
    };
    template <> struct ParsedTwoBodyIntKey<2> {
      typedef ParsedTwoBodyTwoCenterIntKey value;
    };

    // defines the type of the object that holds parameters of the two-body MO integrals runtime
    // with the given number of centers. By default there are no parameters
    template <int NumCenters> struct TwoBodyMOIntsRuntimeParams;
    template <> struct TwoBodyMOIntsRuntimeParams<2> {
      typedef DummySavableState value;
    };
    template <> struct TwoBodyMOIntsRuntimeParams<3> {
      typedef DummySavableState value;
    };
    /// 4-center 2-body integrals can use density fitting
    template <> struct TwoBodyMOIntsRuntimeParams<4> {
      typedef DensityFittingInfo value;
    };

  };

  /**
   *    Smart runtime support for computing MO-basis integrals.
   */
  template <int NumCenters>
  class TwoBodyMOIntsRuntime : virtual public SavableState {
    public:
      typedef TwoBodyMOIntsRuntime this_type;
      typedef typename detail::TwoBodyIntEval<NumCenters>::value TwoBodyIntEval;
      typedef typename detail::TwoBodyIntEval<NumCenters>::refvalue TwoBodyIntEvalRef;
      typedef typename NCentersToDescr<NumCenters,2>::value TwoBodyIntDescr;
      typedef typename detail::ParsedTwoBodyIntKey<NumCenters>::value ParsedTwoBodyIntKey;
      typedef typename detail::TwoBodyMOIntsRuntimeParams<NumCenters>::value Params;

      TwoBodyMOIntsRuntime(const Ref<MOIntsTransformFactory>& factory);
      TwoBodyMOIntsRuntime(StateIn& si);
      void save_data_state(StateOut& so);
      ~TwoBodyMOIntsRuntime();

      /// obsoletes this object
      void obsolete();

      /// return the params object that determines optional aspects of behavior of the runtime
      const Params* params() const { return const_cast<const Params*>(params_); } // cast because of serialization this can't be const
      /// set the params object
      void params(const Params* p) { params_ = const_cast<Params*>(p); }

      /** Returns true if the given TwoBodyIntEval is available
        */
      bool exists(const std::string& key) const;

      /** Returns the TwoBodyIntEval that contains the integrals described by key.

          key must be in format recognized by ParsedTwoBodyIntKey.
          If this key is not known, an appropriate object for computing the integrals
          will be created and (possibly) evaluated.
        */
      TwoBodyIntEvalRef get(const std::string& key);   // non-const: can compute integrals

      /** Returns key that corresponds to descr.
          \sa params_key
        */
      std::string descr_key(const Ref<TwoBodyIntDescr>& descr);

      /// returns the factory
      const Ref<MOIntsTransformFactory>& factory() const { return factory_; }

    private:
      Params* params_;   //< optional parameters
      Ref<MOIntsTransformFactory> factory_;  //< that creates transforms

      // Map to TwoBodyIntEval objects that have been computed previously
      typedef Registry<std::string, TwoBodyIntEvalRef, detail::NonsingletonCreationPolicy > EvalRegistry;
      Ref<EvalRegistry> evals_;

      // creates a TwoBodyIntEval object for a given key
      const TwoBodyIntEvalRef& create_eval(const std::string& key);

      /** creates TwoBodyIntDescr given the operator and params keys.
          params_key must be in params_map_.
       */
      Ref<TwoBodyIntDescr> create_descr(const std::string& oper_key,
                                        const std::string& params_key);

      static ClassDesc class_desc_;

  };

  template <int NumCenters>
    ClassDesc
    TwoBodyMOIntsRuntime<NumCenters>::class_desc_(typeid(this_type),
                                                  (std::string("TwoBodyMOIntsRuntime<") +
                                                   static_cast<char>('0' + NumCenters) +
                                                   std::string(">")).c_str(),
                                                  1,
                                                  "virtual public SavableState", 0, 0,
                                                  create<this_type> );

  template <int NumCenters>
  TwoBodyMOIntsRuntime<NumCenters>::TwoBodyMOIntsRuntime(const Ref<MOIntsTransformFactory>& f) : factory_(f),
    evals_(EvalRegistry::instance()), params_(0)
  {
  }

  template <int NumCenters>
  void
  TwoBodyMOIntsRuntime<NumCenters>::save_data_state(StateOut& so)
  {
    SavableState::save_state(factory_.pointer(),so);
    EvalRegistry::save_instance(evals_,so);
    SavableState::save_state(params_,so);
  }

  template <int NumCenters>
  TwoBodyMOIntsRuntime<NumCenters>::~TwoBodyMOIntsRuntime()
  {
    // some compilers must be tricked ...
    const bool make_sure_class_desc_is_initialized = (&class_desc_ == 0);
  }

  template <int NumCenters>
  void
  TwoBodyMOIntsRuntime<NumCenters>::obsolete() {
    evals_->clear();
  }

  template <int NumCenters>
  bool
  TwoBodyMOIntsRuntime<NumCenters>::exists(const std::string& key) const
  {
    return evals_->key_exists(key);
  }


  template <int NumCenters>
  typename TwoBodyMOIntsRuntime<NumCenters>::TwoBodyIntEvalRef
  TwoBodyMOIntsRuntime<NumCenters>::get(const std::string& key)
  {
    if (evals_->key_exists(key)) {
      return evals_->value(key);
    }
    else {  // if not found
      try { ParsedTwoBodyIntKey parsedkey(key); }
      catch (...) {
        std::ostringstream oss;
        oss << "TwoBodyMOIntsRuntime<NumCenters>::get() -- key " << key << " does not match the format";
        throw ProgrammingError(oss.str().c_str(),__FILE__,__LINE__);
      }
      // then create evaluated tform
      const TwoBodyIntEvalRef& eval = create_eval(key);
      return eval;
    }
    assert(false); // unreachable
  }

  template <int NumCenters>
  std::string
  TwoBodyMOIntsRuntime<NumCenters>::descr_key(const Ref<TwoBodyIntDescr>& descr)
  {
    std::string result = ParsedTwoBodyIntKey::key(descr);
    Ref<IntParams> p = descr->params();
    result += ParamsRegistry::instance()->key(p);
    return result;
  }

  template <int NumCenters>
  Ref<typename TwoBodyMOIntsRuntime<NumCenters>::TwoBodyIntDescr>
  TwoBodyMOIntsRuntime<NumCenters>::create_descr(const std::string& oper_key,
                                                 const std::string& params_key)
  {
    Ref<IntParams> p = ParamsRegistry::instance()->value(params_key);
    const Ref<Integral>& integral = factory()->integral();
    return ParsedTwoBodyIntKey::create_descr(oper_key,p,integral);
  }

  //////////////////////

  typedef TwoBodyMOIntsRuntime<4> TwoBodyFourCenterMOIntsRuntime;
  typedef TwoBodyMOIntsRuntime<3> TwoBodyThreeCenterMOIntsRuntime;
  typedef TwoBodyMOIntsRuntime<2> TwoBodyTwoCenterMOIntsRuntime;

  //////////////////////

  /** TwoBodyMOIntsRuntimeUnion23 packages 2-center and 3-center runtimes.
    */
  class TwoBodyMOIntsRuntimeUnion23 : virtual public SavableState {
    public:
      typedef TwoBodyMOIntsRuntimeUnion23 this_type;

      TwoBodyMOIntsRuntimeUnion23(const Ref<MOIntsTransformFactory>& factory,
                                  const Ref<TwoBodyTwoCenterMOIntsRuntime>& runtime_2c = 0,
                                  const Ref<TwoBodyThreeCenterMOIntsRuntime>& runtime_3c = 0);
      ~TwoBodyMOIntsRuntimeUnion23();
      TwoBodyMOIntsRuntimeUnion23(StateIn&);
      void save_data_state(StateOut&);

      /// factory for creating AO->MO transforms
      const Ref<MOIntsTransformFactory>& factory() const { return factory_; }
      /// runtime for 2-center integrals
      const Ref<TwoBodyTwoCenterMOIntsRuntime>& runtime_2c() const { return runtime_2c_; }
      /// runtime for 3-center integrals
      const Ref<TwoBodyThreeCenterMOIntsRuntime>& runtime_3c() const { return runtime_3c_; }

    private:
      static ClassDesc class_desc_;

      Ref<MOIntsTransformFactory> factory_;
      Ref<TwoBodyTwoCenterMOIntsRuntime> runtime_2c_;
      Ref<TwoBodyThreeCenterMOIntsRuntime> runtime_3c_;
  };


} // end of namespace sc

#endif // end of header guard


// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
