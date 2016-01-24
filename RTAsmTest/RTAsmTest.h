#pragma once

#include "RTAsm/RTParser.h"
#include "RTAsm/Error.h"
#include "RTAsm/RTProg.h"

class RTProgTester
{
public:
  RTProgTester(RTProg prog):prog_(prog),s_(prog_),codeGen_(s_),parser_(codeGen_)
  {
  }
  
  Error parse()
  {
    parser_.parse(s_);
    return prog_.getErrorHandler();
  }
  
  
protected:
  RTProg prog_;
  Stream s_;
  CodeGen codeGen_;
  RTParser parser_;  
};