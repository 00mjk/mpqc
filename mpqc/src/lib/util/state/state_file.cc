
#include <stdio.h>
#include <util/class/class.h>
#include "state.h"

StateOutFile::StateOutFile() :
  fp_(stdout), opened_(0)
{
}

StateOutFile::StateOutFile(FILE* fp) :
  fp_(fp), opened_(0)
{
}

StateOutFile::StateOutFile(const char * path, const char * mode) :
  opened_(1)
{
  fp_ = fopen(path,mode);
}

StateOutFile::~StateOutFile()
{
  if (opened_) fclose(fp_);
  opened_=0; fp_=0;
}

void StateOutFile::flush() { fflush(fp_); }
void StateOutFile::close() { if(opened_) fclose(fp_); opened_=0; fp_=0; }
void StateOutFile::rewind() { if(fp_) fseek(fp_,0,0); }

////////////////////////////////////

StateInFile::StateInFile() :
  fp_(stdin), opened_(0)
{
}

StateInFile::StateInFile(FILE* fp) :
  fp_(fp), opened_(0)
{
}

StateInFile::StateInFile(const char * path, const char * mode) :
  opened_(1)
{
  fp_ = fopen(path,mode);
}

StateInFile::~StateInFile()
{
  if (opened_) fclose(fp_);
  opened_=0; fp_=0;
}

void StateInFile::flush() { fflush(fp_); }
void StateInFile::close() { if(opened_) fclose(fp_); opened_=0; fp_=0; }
void StateInFile::rewind() { if(fp_) fseek(fp_,0,0); }
