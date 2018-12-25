#pragma once

#include "ErrorHandler.h"

class RTProg
{
public:
  RTProg(const char *code)
  {
    code_=code;
  }
  
  const char* getCode() { return code_; }
  ErrorHandler& getErrorHandler() { return errors_; }
  
protected:
  const char *code_;
  ErrorHandler errors_;
};

#define RTASM(...) #__VA_ARGS__