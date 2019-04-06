#pragma once

#include <string>
#include <fstream>
#include <sstream>

#include "ErrorHandler.h"

class RTProg
{
public:
  RTProg(const char *code)
  {
    code_=code;
  }
  
  RTProg(const std::string &code)
  {
    code_=code;
  }
  
  void append(const char *code)
  {
    code_+=code;
  }
  
  const char* getCode() { return code_.c_str(); }
  ErrorHandler& getErrorHandler() { return errors_; }
  
  static RTProg createFromFile(const std::string &filename)
  {
    std::fstream f(filename,std::ios::in|std::ios::binary);
    
    if(f.is_open())
    {
      std::stringstream ss;
      ss << f.rdbuf();
      return RTProg(ss.str());
    }
  }
  
protected:
  std::string code_;
  ErrorHandler errors_;
};

#define RTASM(...) #__VA_ARGS__