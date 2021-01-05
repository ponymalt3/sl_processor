#pragma once

#include <stdint.h>

class DebuggerInterface
{
public:
  virtual bool readMem(uint32_t addr,uint32_t numWords,uint32_t *buffer) =0;
  virtual bool writeMem(uint32_t addr,uint32_t numWords,const uint32_t *buffer) =0;
  
  virtual bool writeCode(const uint16_t *code,uint32_t codeSize) =0;
  
  struct _Run
  {
    enum Result {ACCESS_FAULT_WRITE,ACCESS_FAULT_READ,ACCESS_FAULT_CODE,BREAKPOINT_HIT,MEMORY_BP_HIT,ERROR,OK};
    
    Result result_;
    uint32_t addr_;
  };
  
  virtual _Run runToAddr(uint32_t addr) =0;
  virtual void halt() =0;
  virtual void reset() =0;
  virtual uint32_t getPC() =0;
  
  virtual bool addBreakpoint(uint32_t addr) =0;
  virtual bool removeBreakpoint(uint32_t addr) =0;
};
