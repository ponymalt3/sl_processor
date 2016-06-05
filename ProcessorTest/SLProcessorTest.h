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

class LoadAndSimulateProcessor
{
public:
  LoadAndSimulateProcessor()
    : localMem_(512)
    , codeMem_(256)
    , extMem_(16*1024)
    , codePort_(codeMem_.createPort())
    , localPort_(localMem_.createPort())
    , extPort_(extMem_.createPort()) 
    , processor_(localMem_,extMem_.createPort(),codeMem_.createPort())
  {
  }
  
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
  
  void writeCode(uint32_t addr,uint16_t value)
  {
    codePort_.write(addr,value);
    codePort_.update();
  }
  
  void writeMemory(qfp32_t addr,uint32_t value)
  {
    writeMemory((int32_t)addr,value);
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
  }
  
  void executeWithMemExtStall(uint32_t cycles)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(1,0,0);
  }
  
  void executeWithSetPc(uint32_t pcValue,uint32_t cycles=1)
  {
    for(uint32_t i=0;i<cycles;++i)
      processor_.update(0,1,pcValue);
  }
  
  void executeUntilAddr(uint32_t addr)
  {
    while(processor_.getExecutedAddr() != addr)
    {
      processor_.update(0,0,0);
    }
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
