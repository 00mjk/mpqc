//
// linearr12.h
//
// Copyright (C) 2003 Edward Valeev
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

#ifdef __GNUG__
#pragma interface
#endif

#include <string>
#include <chemistry/qc/basis/integral.h>
#include <chemistry/qc/basis/intdescr.h>
#include <chemistry/qc/basis/tbint.h>

#ifndef _chemistry_qc_mbptr12_linearr12_h
#define _chemistry_qc_mbptr12_linearr12_h

namespace sc {
  namespace LinearR12 {
    
    /** CorrelationFactor is a set of one or more two-particle functions
        of the interparticle distance. Each function may be a primitive function
        or a contraction of several functions.
    */
    class CorrelationFactor : public RefCount {
      public:
        /// Definitions of primitive and contracted Geminals
        //typedef IntParamsG12::PrimitiveGeminal PrimitiveGeminal;
        //typedef IntParamsG12::ContractedGeminal ContractedGeminal;
        /// Vector of contracted 2 particle functions
        //typedef std::vector<ContractedGeminal> CorrelationParameters;

        CorrelationFactor(const std::string& label);
        virtual ~CorrelationFactor();

        /// Returns label
        const std::string& label() const;
        /// Returns the number of contracted two-particle functions in the set
        virtual unsigned int nfunctions() const;
        /// Returns the number of primitive functions in contraction c
        virtual unsigned int nprimitives(unsigned int c) const;
        
        /// Computes value of function c when electrons are at distance r12
        virtual double value(unsigned int c, double r12) const =0;
        /// Computes value of function c when electrons are at (r1, r2, r12). By default, will call value(c,r12).
        virtual double value(unsigned int c, double r12, double r1, double r2) const;
        
        /** Returns TwoBodyIntDescr needed to compute matrix elements where correlation
            function f appears in either bra or ket only.
        */
        virtual Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int f) const;
        /** Returns TwoBodyIntDescr needed to compute matrix elements where correlation
            functions fbra and fket appear in bra or ket, respectively.
        */
        virtual Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int fbra, unsigned int fket) const;
        /// Returns the maximum number of two-body integral types produced by the appropriate integral evaluator
        virtual unsigned int max_num_tbint_types() const =0;
        
        //
        // These functions are used to map the logical type of integrals ([T1,F12], etc.) to concrete types as produced by TwoBodyInt
        //
        
        /// Returns TwoBodyInt::tbint_type corresponding to electron repulsion integrals
        virtual TwoBodyInt::tbint_type tbint_type_eri() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over correlation operator
        virtual TwoBodyInt::tbint_type tbint_type_f12() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over [T1,f12]
        virtual TwoBodyInt::tbint_type tbint_type_t1f12() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over [T2,f12]
        virtual TwoBodyInt::tbint_type tbint_type_t2f12() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over f12/r12
        virtual TwoBodyInt::tbint_type tbint_type_f12eri() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over f12^2
        virtual TwoBodyInt::tbint_type tbint_type_f12f12() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over [f12,[T1,f12]]
        virtual TwoBodyInt::tbint_type tbint_type_f12t1f12() const;
        /// Returns TwoBodyInt::tbint_type corresponding to integrals over f12*f12' antisymmetrized
	/// wrt exponents, i.e. f12*f12' (exp(f12')-exp(f12))/(exp(f12')+exp(f12))
        virtual TwoBodyInt::tbint_type tbint_type_f12f12_anti() const;
        
        /// print the correlation factor
        void print(std::ostream& os = ExEnv::out0()) const;
        
      private:
        std::string label_;

	/// Print out parameters of function f. Base implementation prints nothing.
	virtual void print_params(std::ostream& os, unsigned int f) const;

    };
    
    /** NullCorrelationFactor stands for no correlation factor */
    class NullCorrelationFactor : public CorrelationFactor {
      public:
      NullCorrelationFactor();
      
      /// Implementation of CorrelationFactor::max_num_tbint_types()
      unsigned int max_num_tbint_types() const { return 1; }
      /// Implementation of CorrelationFactor::value()
      double value(unsigned int c, double r12) const;
    };
    
    /** R12CorrelationFactor stands for no correlation factor */
    class R12CorrelationFactor : public CorrelationFactor {
      public:
      R12CorrelationFactor();
      
      /// Implementation of CorrelationFactor::max_num_tbint_types()
      unsigned int max_num_tbint_types() const { return 4; }
      /// Reimplementation of CorrelationFactor::tbint_type_f12()
      TwoBodyInt::tbint_type tbint_type_f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_t1f12()
      TwoBodyInt::tbint_type tbint_type_t1f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_t2f12()
      TwoBodyInt::tbint_type tbint_type_t2f12() const;
      /// Overload of CorrelationFactor::tbintdescr(f)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int f) const;
      /// Implementation of CorrelationFactor::value()
      double value(unsigned int c, double r12) const;
    };
    
    /** G12CorrelationFactor stands for Gaussian geminals correlation factor,
	usable with methods that require commutator integrals */
    class G12CorrelationFactor : public CorrelationFactor {
      public:
      /// Definitions of primitive and contracted Geminals
      typedef IntParamsG12::PrimitiveGeminal PrimitiveGeminal;
      typedef IntParamsG12::ContractedGeminal ContractedGeminal;
      /// Vector of contracted 2 particle functions
      typedef std::vector<ContractedGeminal> CorrelationParameters;

      G12CorrelationFactor(const CorrelationParameters& params);
      
      /// Reimplementation of CorrelationFactor::nfunctions()
      unsigned int nfunctions() const;
      /// Returns contracted function c
      const ContractedGeminal& function(unsigned int c) const;
      /// Reimplementation of CorrelationFactor::nprimitives()
      unsigned int nprimitives(unsigned int c) const;
      /// Returns std::pair<primitive_parameter,coefficient> in primitive p of contraction c
      const PrimitiveGeminal& primitive(unsigned int c, unsigned int p) const;
      /// Implementation of CorrelationFactor::max_num_tbint_types()
      unsigned int max_num_tbint_types() const { return 6; }
      /// Reimplementation of CorrelationFactor::tbint_type_f12()
      TwoBodyInt::tbint_type tbint_type_f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12eri()
      TwoBodyInt::tbint_type tbint_type_f12eri() const;
      /// Reimplementation of CorrelationFactor::tbint_type_t1f12()
      TwoBodyInt::tbint_type tbint_type_t1f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_t2f12()
      TwoBodyInt::tbint_type tbint_type_t2f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12f12()
      TwoBodyInt::tbint_type tbint_type_f12f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12t1f12()
      TwoBodyInt::tbint_type tbint_type_f12t1f12() const;
      /// Overload of CorrelationFactor::tbintdescr(f)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int f) const;
      /// Overload of CorrelationFactor::tbintdescr(fbra,fket)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int fbra, unsigned int fket) const;
      /// Implementation of CorrelationFactor::value()
      double value(unsigned int c, double r12) const;

    private:
      CorrelationParameters params_;

      /// Reimplementation of CorrelationFactor::print_params()
	void print_params(std::ostream& os, unsigned int f) const;

    };

    /** G12NCCorrelationFactor stands for Gaussian geminals correlation factor,
	usable with methods that do not require commutator integrals */
    class G12NCCorrelationFactor : public CorrelationFactor {
      public:
      /// Definitions of primitive and contracted Geminals
      typedef IntParamsG12::PrimitiveGeminal PrimitiveGeminal;
      typedef IntParamsG12::ContractedGeminal ContractedGeminal;
      /// Vector of contracted 2 particle functions
      typedef std::vector<ContractedGeminal> CorrelationParameters;

      G12NCCorrelationFactor(const CorrelationParameters& params);
      
      /// Reimplementation of CorrelationFactor::nfunctions()
      unsigned int nfunctions() const;
      /// Returns contracted function c
      const ContractedGeminal& function(unsigned int c) const;
      /// Reimplementation of CorrelationFactor::nprimitives()
      unsigned int nprimitives(unsigned int c) const;
      /// Returns std::pair<primitive_parameter,coefficient> in primitive p of contraction c
      const PrimitiveGeminal& primitive(unsigned int c, unsigned int p) const;
      /// Implementation of CorrelationFactor::max_num_tbint_types()
      unsigned int max_num_tbint_types() const { return 6; }
      /// Reimplementation of CorrelationFactor::tbint_type_f12()
      TwoBodyInt::tbint_type tbint_type_f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12eri()
      TwoBodyInt::tbint_type tbint_type_f12eri() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12f12()
      TwoBodyInt::tbint_type tbint_type_f12f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12t1f12()
      TwoBodyInt::tbint_type tbint_type_f12t1f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12f12_anti()
      TwoBodyInt::tbint_type tbint_type_f12f12_anti() const;
      /// Overload of CorrelationFactor::tbintdescr(f)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int f) const;
      /// Overload of CorrelationFactor::tbintdescr(fbra,fket)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int fbra, unsigned int fket) const;
      /// Implementation of CorrelationFactor::value()
      double value(unsigned int c, double r12) const;

    private:
      CorrelationParameters params_;

      /// Reimplementation of CorrelationFactor::print_params()
	void print_params(std::ostream& os, unsigned int f) const;

    };

    /** GenG12CorrelationFactor stands for no correlation factor */
    class GenG12CorrelationFactor : public CorrelationFactor {
      public:
      /// Definitions of primitive and contracted Geminals
      typedef IntParamsGenG12::PrimitiveGeminal PrimitiveGeminal;
      typedef IntParamsGenG12::ContractedGeminal ContractedGeminal;
      /// Vector of contracted 2 particle functions
      typedef std::vector<ContractedGeminal> CorrelationParameters;

      GenG12CorrelationFactor(const CorrelationParameters& params);
      
      /// Reimplementation of CorrelationFactor::nfunctions()
      unsigned int nfunctions() const;
      /// Returns contracted function c
      const ContractedGeminal& function(unsigned int c) const;
      /// Reimplementation of CorrelationFactor::nprimitives()
      unsigned int nprimitives(unsigned int c) const;
      /// Returns std::pair<primitive_parameter,coefficient> in primitive p of contraction c
      const PrimitiveGeminal& primitive(unsigned int c, unsigned int p) const;
      /// Implementation of CorrelationFactor::max_num_tbint_types()
      unsigned int max_num_tbint_types() const { return 4; }
      /// Reimplementation of CorrelationFactor::tbint_type_f12()
      TwoBodyInt::tbint_type tbint_type_f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12eri()
      TwoBodyInt::tbint_type tbint_type_f12eri() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12f12()
      TwoBodyInt::tbint_type tbint_type_f12f12() const;
      /// Reimplementation of CorrelationFactor::tbint_type_f12t1f12()
      TwoBodyInt::tbint_type tbint_type_f12t1f12() const;
      /// Overload of CorrelationFactor::tbintdescr(f)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int f) const;
      /// Overload of CorrelationFactor::tbintdescr(fbra,fket)
      Ref<TwoBodyIntDescr> tbintdescr(const Ref<Integral>& IF, unsigned int fbra, unsigned int fket) const;
      /// Implementation of CorrelationFactor::value()
      double value(unsigned int c, double r12) const;
      /// Reimplementation of CorrelationFactor::value()
      double value(unsigned int c, double r12, double r1, double r2) const;

    private:
      CorrelationParameters params_;

      /// Reimplementation of CorrelationFactor::print_params()
      void print_params(std::ostream& os, unsigned int f) const;

    };
    
    /**
      Projector of linear R12 methods:
      0: Q_{12} = 1
      1: Q_{12} = (1 - P_1)(1 - P_2)
      2: Q_{12} = (1 - V_1 V_2)(1 - O_1)(1 - O_2)
      3: Q_{12} = 1 - P_1 P_2
    */
    enum Projector {Projector_0 = 0,
         Projector_1 = 1,
	 Projector_2 = 2,
	 Projector_3 = 3};
    enum StandardApproximation {StdApprox_A = 0,
				StdApprox_Ap = 1,
				StdApprox_App = 2,
				StdApprox_B = 3,
                                StdApprox_C = 4};
    enum ABSMethod {ABS_ABS = 0,
		    ABS_ABSPlus = 1,
		    ABS_CABS = 2,
		    ABS_CABSPlus = 3};

    enum OrbitalProduct {
	OrbProd_ij = 0,
	OrbProd_pq = 1
    };

    enum PositiveDefiniteB {
      PositiveDefiniteB_no = 0,
      PositiveDefiniteB_yes = 1,
      PositiveDefiniteB_weak = 2
    };
			       
  }

}

#endif

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:


