
#ifndef _libQC_chemelem_h
#define _libQC_chemelem_h

// This class is a repository of element information
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <util/class/class.h>
#include <util/state/state.h>

// Macro to expand to query function
#define QUERY_FUNCTION_PROTO(type,property) \
  inline const type property() const { return atom_info[Z_].property; }

							      
class ChemicalElement :
  virtual public DescribedClass, virtual public SavableState
{
  DescribedClass_DECLARE(ChemicalElement)
  SavableState_DECLARE(ChemicalElement)
  private:
    int Z_;
    struct atom_info_type
    {
      char   *name;
      char   *symbol;
      int     number;
      double     mass;
      int     family;
      int     row;
      int     valence;
      int     melting_pt;
      int     boiling_pt;
      int     first_ion_pt;
      int     year_discovered;
      double  electronegativity;
      double  specific_heat;
      double  density;
      double  atomic_radius;
      double  vdw_radius;
    };
  
    static int max_atomic_number;
    static atom_info_type atom_info[];
    const double ang_to_au = 1.0/0.52917706;

  public:
    ChemicalElement(int Z=1);
    ChemicalElement(const ChemicalElement&);
    ChemicalElement(const char* name);

    ~ChemicalElement();
    
    void save_data_state(StateOut&);
    void restore_data_state(int,StateIn&);

    // Here are all of the query functions for the properties
    QUERY_FUNCTION_PROTO(char* ,name);
    QUERY_FUNCTION_PROTO(char* ,symbol);
    QUERY_FUNCTION_PROTO(int   ,number);
    QUERY_FUNCTION_PROTO(double,mass);
    QUERY_FUNCTION_PROTO(int   ,family);
    QUERY_FUNCTION_PROTO(int   ,row);
    QUERY_FUNCTION_PROTO(int   ,valence);
    QUERY_FUNCTION_PROTO(int   ,melting_pt);
    QUERY_FUNCTION_PROTO(int   ,boiling_pt);
    QUERY_FUNCTION_PROTO(int   ,first_ion_pt);
    QUERY_FUNCTION_PROTO(int   ,year_discovered);
    QUERY_FUNCTION_PROTO(double,electronegativity);
    QUERY_FUNCTION_PROTO(double,specific_heat);
    QUERY_FUNCTION_PROTO(double,density);
    QUERY_FUNCTION_PROTO(double,atomic_radius);
    inline double atomic_radius_au() const
      { return atom_info[Z_].atomic_radius * ang_to_au; }
    QUERY_FUNCTION_PROTO(double,vdw_radius);
    inline double charge() { return (double) number(); }
};

#endif
