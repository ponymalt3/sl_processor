/*
 * SLProcessorTest.h
 *
 *  Created on: Apr 13, 2015
 *      Author: malte
 */

#ifndef SLPROCESSORTEST_H_
#define SLPROCESSORTEST_H_

#include <assert.h>
#include "../SLProcessor.h"
#include "../SLCodeDef.h"
#include "qfp32.h"
#include <mtest.h>

#include <fstream>
#include <iomanip>


class VHDLTestDataGenerator
{
public:
  VHDLTestDataGenerator()
  {
  }
  
  void enable(const std::string &filename)
  {
    file_=std::fstream(filename,std::ios::trunc|std::ios::out);
    file_<<std::hex;
    file_.fill('0');
  }
  
  void createNewTestCase(const std::string &name)
  {
    if(file_.is_open())
    {
      file_<<"0000"<<" "<<(name)<<"\n";
    }
  }
  
  void addCode(const uint32_t *code,uint32_t size)
  {
    if(!file_.is_open())
    {
      return;
    }
    
    file_<<"0001";
    for(uint32_t i=0;i<size;++i)
    {
      file_<<" "<<std::setw(4)<<((uint16_t)(code[i]));
    }
    file_<<"\n";
  }
  
  void runCycles(uint32_t cycles,bool stallExtMem=false)
  {
    if(file_.is_open())
    {
      file_<<(stallExtMem?"0005":"0002")<<" "<<std::setw(8)<<(cycles)<<"\n";
    }
  }
  
  void runUntilAddr(uint32_t addr)
  {
    if(file_.is_open())
    {
      file_<<"0003"<<" "<<std::setw(8)<<(addr)<<"\n";
    }
  }
  
  void writeMem(uint32_t addr,uint32_t data)
  {
    if(file_.is_open())
    {
      file_<<"0004"<<" "<<std::setw(8)<<(addr)<<" "<<std::setw(8)<<(data)<<"\n";
    }
  }
  
  void expect(uint32_t addr,uint32_t data)
  {
    if(file_.is_open())
    {
      file_<<"000F"<<" "<<std::setw(8)<<(addr)<<" "<<std::setw(8)<<(data)<<"\n";
    }
  }
  
protected:
  std::fstream file_;
};

class LoadAndSimulateProcessor
{
public:
  LoadAndSimulateProcessor()
    : localMem_(512)
    , codeMem_(4096)
    , extMem_(1024)
    , codePort_(codeMem_.createPort())
    , localPort_(localMem_.createPort())
    , extPort_(extMem_.createPort()) 
    , processor_(localMem_,extMem_.createPort(),codeMem_.createPort())
  {
    getVdhlTestGenerator().createNewTestCase(mtest::manager::instance().getCurrentTestName());
  }
  
  template<const uint32_t CodeSize>
  LoadAndSimulateProcessor(uint32_t (&code)[CodeSize])
    : localMem_(512)
    , codeMem_(4096)
    , extMem_(1024)
    , codePort_(codeMem_.createPort())
    , localPort_(localMem_.createPort())
    , extPort_(extMem_.createPort()) 
    , processor_(localMem_,extMem_.createPort(),codeMem_.createPort())
  {
    for(uint32_t i=0;i<CodeSize;++i)
    {
      codePort_.write(i,code[i]);
      codePort_.update();
    }
    
    getVdhlTestGenerator().createNewTestCase(mtest::manager::instance().getCurrentTestName());
    getVdhlTestGenerator().addCode(code,CodeSize);
  }
  
  Memory& getExternalMemory() { return extMem_; }
  
  void writeCode(uint32_t addr,uint16_t value)
  {
    codePort_.write(addr,value);
    codePort_.update();
  }
  
  void writeCodeComplete(uint32_t validCodeWords)
  {
    uint32_t code[65336];
    for(uint32_t i=0;i<validCodeWords;++i)
    {
      code[i]=codePort_.read(i);
    }
    
    codeMem_.setInvalidRegion(validCodeWords,codeMem_.getSize()-validCodeWords);
    
    getVdhlTestGenerator().addCode(code,validCodeWords);
  }
  
  void writeMemory(qfp32_t addr,uint32_t value)
  {
    writeMemory((int32_t)addr,value);
  }
  
  void writeMemory(qfp32_t addr,qfp32_t value)
  {
    writeMemory(addr,value.getAsRawUint());
  }
  
  void writeMemory(uint32_t addr,double value)
  {
    writeMemory((int32_t)addr,qfp32_t(value));
  }
  
  void writeMemory(uint32_t addr,qfp32_t value)
  {
    writeMemory((int32_t)addr,value.getAsRawUint());
  }
  
  void writeMemory(uint32_t addr,uint32_t value,bool dontLogForVhdlTest=false)
  {
    if(addr >= localMem_.getSize())
    {
      extPort_.write(addr,value);
      extPort_.update();
    }
    else
    {
      localPort_.write(addr,value);
      localPort_.update();
    }
    
    if(!dontLogForVhdlTest)
    {
      getVdhlTestGenerator().writeMem(addr,value);
    }
  }
  
  uint32_t readMemory(qfp32_t addr)
  {
    return readMemory((int32_t)addr);
  }
  
  uint32_t readMemory(uint32_t addr)
  {
    if(addr >= localMem_.getSize())
    {
      return extPort_.read(addr);
    }
    else
    {
      return localPort_.read(addr);
    }
  }

  void run(uint32_t cycles)
  {
    reset();
    execute(cycles);
  }
  
  void reset()
  {
    processor_.reset();
  }
  
  void execute(uint32_t cycles)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(0,0,0);
    
    getVdhlTestGenerator().runCycles(cycles);
  }
  
  void executeWithMemExtStall(uint32_t cycles)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(1,0,0);
    
    getVdhlTestGenerator().runCycles(cycles,true);
  }
  
  void executeWithSetPc(uint32_t pcValue,uint32_t cycles=1)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(0,1,pcValue);
  }
  
  uint32_t executeUntilAddr(uint32_t addr)
  {
    uint32_t cycles=0;
    
    while(processor_.getExecutedAddr() == 0xFFFFFFFF || processor_.getExecutedAddr() < addr)
    {
      processor_.update(0,0,0);
      ++cycles;    
    }
    
    getVdhlTestGenerator().runUntilAddr(addr);
    
    return cycles;
  }
  
  void expectThatMemIs(qfp32_t addr,qfp32_t expectedValue,const std::string &expectExpr="")
  {
    expect(expectedValue,(int32_t)addr,expectExpr);
    getVdhlTestGenerator().expect((int32_t)addr,expectedValue.toRaw());
  }  
  void expectThatMemIs(qfp32_t addr,uint32_t expectedValue,const std::string &expectExpr="")
  {
    expect(qfp32_t::initFromRaw(expectedValue),(int32_t)addr,expectExpr);
    getVdhlTestGenerator().expect((int32_t)addr,expectedValue);
  }
  void expectThatMemIs(uint32_t addr,qfp32_t expectedValue,const std::string &expectExpr="")
  {
    expect(expectedValue,addr,expectExpr);
    getVdhlTestGenerator().expect(addr,expectedValue.toRaw());
  }
  void expectThatMemIs(uint32_t addr,uint32_t expectedValue,const std::string &expectExpr="")
  {
    expect(qfp32_t::initFromRaw(expectedValue),addr,expectExpr);
    getVdhlTestGenerator().expect(addr,expectedValue);
  }
  
  void expect(qfp32_t expectedValue,uint32_t addr,const std::string &expectString="")
  {
    std::stringstream ss;
    if(expectString == "")
    {
      ss << "expect: mem[" << (addr) << "] == " << (expectedValue);
    }
    else
    {
      ss << "expect: " << expectString << " == " << (expectedValue);
    }
     
    qfp32_t value=qfp32_t::initFromRaw(readMemory(addr));
    
    ss << "  but is " << (value);
    
    mtest::manager::instance()=
      mtest::condition(expectedValue == value,ss.str(),mtest::condition::expect);
  }
  static VHDLTestDataGenerator& getVdhlTestGenerator()
  {
    static VHDLTestDataGenerator instance_;
    return instance_;
  }
  
protected:
  Memory localMem_;
  Memory codeMem_;
  Memory extMem_;
  Memory::Port codePort_;
  Memory::Port localPort_;
  Memory::Port extPort_;
  SLProcessor processor_;
};

#endif /* SLPROCESSORTEST_H_ */
