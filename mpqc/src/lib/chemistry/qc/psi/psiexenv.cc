
#ifdef __GNUG__
#pragma implementation
#endif

#include <string>
#include <string.h>
#include <scconfig.h>
#include <util/ref/ref.h>
#include <util/keyval/keyval.h>
#include <util/misc/formio.h>
#include <chemistry/qc/psi/psiexenv.h>

using namespace std;

static ClassDesc PsiExEnv_cd(
  typeid(PsiExEnv),"PsiExEnv",1,"public DescribedClass",
  0, create<PsiExEnv>, 0);

string PsiExEnv::inputname_("input.dat");
string PsiExEnv::file11name_("file11.dat");
int PsiExEnv::ckptfile_(30);
string PsiExEnv::defaultcwd_("/tmp");
string PsiExEnv::defaultfileprefix_("psi");
string PsiExEnv::defaultpsiprefix_("/usr/local/psi/bin");

PsiExEnv::PsiExEnv(const Ref<KeyVal>& keyval)
{
  // Find Psi
  char *psibin = getenv("PSIBIN");
  if (psibin)
    psiprefix_ = string(psibin);
  else
    psiprefix_ = string(defaultpsiprefix_);
  add_to_path(psiprefix_);

  char *cwdchar = keyval->pcharvalue("cwd");
  if (cwdchar)
    cwd_ = string(cwdchar);
  else
    cwd_ = string(defaultcwd_);
  char *fileprefixchar = keyval->pcharvalue("fileprefix");
  if (fileprefixchar)
    fileprefix_ = string(fileprefixchar);
  else
    fileprefix_ = string(defaultfileprefix_);

  nscratch_ = keyval->intvalue("nscratch");
  if (nscratch_ != keyval->count("scratch")) {
    ExEnv::err0() << indent
		  << "PsiExEnv: number of scratch directories != nscratch\n";
    abort();
  }
  scratch_ = new string[nscratch_];
  for (int i=0; i<nscratch_; i++)
    scratch_[i] = string(keyval->pcharvalue("scratch",i));

  char *s = new char[cwd_.size() + inputname_.size() + 2];
  sprintf(s,"%s/%s",cwd_.c_str(),inputname_.c_str());
  psiinput_ = new PsiInput(s);
  delete[] s;

  s = new char[cwd_.size() + file11name_.size() + 2];
  sprintf(s,"%s/%s",cwd_.c_str(),file11name_.c_str());
  psifile11_ = new PsiFile11(s);
  delete[] s;
}

PsiExEnv::PsiExEnv(char *cwd, char *fileprefix, int nscratch, char **scratch):
    cwd_(cwd), fileprefix_(fileprefix), nscratch_(nscratch)
{
  // Find Psi
  char *psibin = 0;
  psibin = getenv("PSIBIN");
  if (psibin)
    psiprefix_ = string(psibin);
  else
    psiprefix_ = string(defaultpsiprefix_);
  add_to_path(psiprefix_);

  scratch_ = new string[nscratch_];
  for(int i=0; i<nscratch_; i++)
    scratch_[i] = string(scratch[i]);
  
  char *s = new char[cwd_.size() + inputname_.size() + 2];
  sprintf(s,"%s/%s",cwd_.c_str(),inputname_.c_str());
  psiinput_ = new PsiInput(s);
  delete[] s;

  s = new char[cwd_.size() + file11name_.size() + 2];
  sprintf(s,"%s/%s",cwd_.c_str(),file11name_.c_str());
  psifile11_ = new PsiFile11(s);
  delete[] s;
}

PsiExEnv::~PsiExEnv()
{
  delete[] scratch_;
}

void PsiExEnv::add_to_path(const string& dir)
{
  if (dir.size()) {
    char *path = getenv("PATH");
    int newpath_len = strlen(path) + dir.size() + 1;
    char *newpath = new char[newpath_len];
    sprintf(newpath,"%s:%s",dir.c_str(),path);
    setenv("PATH",newpath,1);
    delete[] newpath;
  }
}

int PsiExEnv::run_psi()
{
  int errcod;
  if (errcod = run_psi_module("input"))
    return errcod;
  if (errcod = run_psi_module("psi"))
    return errcod;
}

int PsiExEnv::run_psi_module(char *module)
{
  int errcod;
  char *module_cmd = new char[cwd_.size()+strlen(module)+psiprefix_.size()+16];
  sprintf(module_cmd,"cd %s; %s/%s",cwd_.c_str(),psiprefix_.c_str(),module);
  if (errcod = system(module_cmd)) {
      ExEnv::outn() << "PsiWavefunction: module " << module << " failed" << endl;
      abort();
  }
  return errcod;
}

void PsiExEnv::print(std::ostream&o=ExEnv::out0()) const
{
  o << endl;
  o << indent << "PsiExEnv:" << endl << incindent;
  o << indent << "Location of Psi: " << psiprefix_ << endl;
  o << indent << "Current Psi Working Directory: " << cwd_ << endl;
  o << indent << "Current Psi File Prefix: " << fileprefix_ << endl;
  o << indent << "Number of Scratch Groups: " << nscratch_ << endl;
  for(int i=0; i<nscratch_; i++)
    o << indent << "Scratch Group " << i << ": " << scratch_[i] << endl;
  o << endl << decindent;
}
