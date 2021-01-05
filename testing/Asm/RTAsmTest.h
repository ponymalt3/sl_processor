#pragma once

#include <stdint.h>
#include "Assembler/RTParser.h"
#include "Assembler/Error.h"
#include "Assembler/RTProg.h"
#include "../Instr/SLProcessorTest.h"
#include "Peripherals.h"

#include "Assembler/DisAsm.h"

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
    
    peripherals_.createPeripherals();
    proc_.getExternalMemory().setFaultHandler([&](uint32_t addr,uint32_t data,bool write){
      auto result=peripherals_.accessPeripheral(addr,data,write);
      if(!result.first)
      {
        throw Memory::FaultException(addr,write?Memory::FaultException::Write:Memory::FaultException::Read);
      }
      return result.second;
    });    
  }
  
  Error parse(uint32_t reserveParameter=0,bool generateFullEntryVector=false,uint32_t inlineFunctionThreshold=0)
  {
    parser_.parse(s_,inlineFunctionThreshold);
    
    if(generateFullEntryVector)
    {
      codeGen_.generateEntryVector(4,4);
    }
    else if(Error(prog_.getErrorHandler()).getNumErrors() == 0)
    {
      codeGen_.storageAllocationPass(512,reserveParameter);
    }
    
    std::cout<<"functions:\n";
    for(auto &i : codeGen_.functions_)
    {
      if(!i.second.isInlineFunction_)
      {
        std::cout<<"  "<<(i.first)<<"  at "<<(i.second.address_)<<" size: "<<(i.second.size_)<<"\n";
      }
    }
    std::cout<<"inlined functions:\n";
    for(auto &i : codeGen_.functions_)
    {
      if(i.second.isInlineFunction_)
      {
        std::cout<<"  "<<(i.first)<<"\n";
      }
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
    uint16_t code[65536];
    
    for(uint32_t i=0;i<getCodeSize();++i)
    {
      code[i]=getCodeAt(i);
      std::cout<<std::dec<<(i)<<". "<<std::hex<<(code[i])<<"\n";
    }
    
    std::cout<<std::dec;
    
    std::string result;
    auto lines=DisAsm::getLinesFromCode(code,getCodeSize());
    uint32_t i=0;
    for(auto &j : lines)
    {
      result+=std::to_string(i++)+". "+j+"\n";
    }
    return result;
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
    
    proc_.reset();
  }
  
  void execute()
  {
    uint32_t cycles=0;
    
    //proc_.reset();
    cycles=proc_.executeUntilAddr(codeGen_.getCurCodeAddr()-1);
    
    std::cout<<"cycles: "<<(cycles)<<"\n";
  }
  
  void execute(uint32_t cycles)
  {    
    //proc_.reset();
    proc_.execute(cycles);
    
    std::cout<<"cycles executed: "<<(cycles)<<"\n";
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
  Peripherals peripherals_;
};
