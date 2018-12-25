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

class LoadAndSimulateProcessor
{
public:
  template<const uint32_t CodeSize>
  LoadAndSimulateProcessor(uint32_t (&code)[CodeSize])
    : localMem_(512)
    , codeMem_(256)
    , extMem_(16*1024)
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
  }
  
  void writeMemory(uint32_t addr,uint32_t value)
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
  }
  
  uint32_t readMemory(qfp32_t addr)
  {
    uint32_t t=readMemory((int32_t)addr);
    std::cout<<"read: "<<(qfp32_t::initFromRaw(t))<<"\n";
    return t;
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
    processor_.reset();
//    processor.update(0,1,0);

    for(uint32_t i=0;i<cycles;++i)
      processor_.update(0,0,0);
  }
    
    getVdhlTestGenerator().runCycles(cycles,true);
  }
  
  void executeWithSetPc(uint32_t pcValue,uint32_t cycles=1)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(0,1,pcValue);
  }
  
  void executeUntilAddr(uint32_t addr)
  {
    while(processor_.getExecutedAddr() == 0xFFFFFFFF || processor_.getExecutedAddr() < addr)
    {
      processor_.update(0,0,0);    
    }
    
    getVdhlTestGenerator().runUntilAddr(addr);
  }
  
  void expectThatMemIs(qfp32_t addr,qfp32_t expectedValue)
  {
    EXPECT(readMemory(addr) == expectedValue.toRaw());
    getVdhlTestGenerator().expect((int32_t)addr,expectedValue.toRaw());
  }  
  void expectThatMemIs(qfp32_t addr,uint32_t expectedValue)
  {
    EXPECT(readMemory(addr) == expectedValue);
    getVdhlTestGenerator().expect((int32_t)addr,expectedValue);
  }
  void expectThatMemIs(uint32_t addr,qfp32_t expectedValue)
  {
    std::cout<<"EXPECT: "<<(qfp32_t::initFromRaw(readMemory(addr)))<<" == "<<(expectedValue)<<"\n";
    EXPECT(readMemory(addr) == expectedValue.toRaw());
    getVdhlTestGenerator().expect(addr,expectedValue.toRaw());
  }
  void expectThatMemIs(uint32_t addr,uint32_t expectedValue)
  {
    EXPECT(readMemory(addr) == expectedValue);
    getVdhlTestGenerator().expect(addr,expectedValue);
  }
  void expectThatMemIs(uint32_t addr,int32_t expectedValue)
  {
    EXPECT(readMemory(addr) == (uint32_t)expectedValue);
    getVdhlTestGenerator().expect(addr,expectedValue);
  }
  void expectThatMemIs(uint32_t addr,double expectedValue)
  {
    expectThatMemIs(addr,_qfp32_t(expectedValue));
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
