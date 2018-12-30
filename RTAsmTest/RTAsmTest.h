#pragma once

#include <stdint.h>
#include "RTAsm/RTParser.h"
#include "RTAsm/Error.h"
#include "RTAsm/RTProg.h"
#include "../ProcessorTest/SLProcessorTest.h"

#include "DisAsm.h"

class RTProgTester
{
public:
  RTProgTester(RTProg prog):prog_(prog),s_(prog_),codeGen_(s_,16),parser_(codeGen_)
  {
    //zero memory (at least first 256 words)
    for(uint32_t i=0;i<512;++i)
    {
      proc_.writeMemory(i,0,true);
    }  
  }
  
  Error parse(uint32_t reserveParamter=0,bool generateFullEntryVector=false)
  {
    parser_.parse(s_);
    
    if(generateFullEntryVector)
    {
      codeGen_.generateEntryVector(4,4);
    }
    
    if(Error(prog_.getErrorHandler()).getNumErrors() == 0)
    {
      codeGen_.storageAllocationPass(512,reserveParamter);
    }
    return prog_.getErrorHandler();
  }
  
  uint16_t getCodeAt(uint32_t addr)
  {
    return codeGen_.getCodeAt(addr);
  }
  
  uint32_t getCodeSize() const
  {
    return codeGen_.getCurCodeAddr();
  }
  
  std::string getDisAsmString()
  {
    uint16_t code[1024];
    
    for(uint32_t i=0;i<getCodeSize();++i)
    {
      code[i]=getCodeAt(i);
    }
    
    return DisAsm::getStringFromCode(code,getCodeSize());
  }
  
  uint32_t getIRSAddrOfSymbol(const char *symbol)
  {
    uint32_t len=0;
    while(symbol[len] != '\0') ++len;
    
    SymbolMap::_Symbol s=codeGen_.findSymbol(Stream::String(symbol,0,len));
    
    if(s.strLength_ != 0)
    {
      return s.allocatedAddr_;
    }
    
    return 0xFFFFFFFF;
  }
  
  LoadAndSimulateProcessor& getProcessor()
  {
    return proc_;
  }
  
  void loadCode()
  {
    std::cout<<"code:\n";
    for(uint32_t i=0;i<codeGen_.getCurCodeAddr();++i)
    {
      std::cout<<std::hex<<"  "<<(getCodeAt(i))<<"\n";
      proc_.writeCode(i,getCodeAt(i));
    }
    std::cout<<std::dec<<"\n";
    
    for(uint32_t i=codeGen_.getCurCodeAddr();i<codeGen_.getCurCodeAddr()+3;++i)
    {
      proc_.writeCode(i,0xFFFF);
    }
    
    proc_.writeCodeComplete(codeGen_.getCurCodeAddr()+3);
  }
  
  void execute()
  {    
    proc_.reset();
    proc_.executeUntilAddr(codeGen_.getCurCodeAddr()-1);
  }
  
  void expectMemoryAt(qfp32_t addr,qfp32_t data)
  {
    proc_.expectThatMemIs(addr,data);
  }
  
  void expectSymbol(const std::string &symbol,qfp32_t data)
  {
    proc_.expectThatMemIs(getIRSAddrOfSymbol(symbol.c_str()),data,symbol);
  }
  
  void expectSymbol(const std::string &symbol,uint32_t offset,qfp32_t data)
  {
    proc_.expectThatMemIs(getIRSAddrOfSymbol(symbol.c_str())+offset,data,symbol);
  }
  
  void expectSymbolWithOffset(const std::string &symbol,int32_t offset,qfp32_t data)
  {
    std::stringstream ss;
    ss<<symbol<<"["<<(offset)<<"]";
    proc_.expectThatMemIs(getIRSAddrOfSymbol(symbol.c_str())+offset,data,ss.str());
  }

protected:
  RTProg prog_;
  Stream s_;
  CodeGen codeGen_;
  RTParser parser_;
  LoadAndSimulateProcessor proc_;
};