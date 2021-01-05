#pragma once

#include "DebuggerInterface.h"

#include "SLProcessor.h"
#include "SLCodeDef.h"
#include "Assembler/DisAsm.h"
#include "testing/Asm/Peripherals.h"

class Simulator : public DebuggerInterface
{
public:
  Simulator(uint32_t codeMemSize=16384,uint32_t localMemSize=512,uint32_t extMemSize=16384)
  : localMem_(localMemSize)
  , codeMem_(codeMemSize)
  , extMem_(localMemSize+extMemSize)
  , codePort_(codeMem_.createPort())
  , localPort_(localMem_.createPort())
  , extPort_(extMem_.createPort()) 
  , processor_(localMem_,extMem_.createPort(),codeMem_.createPort())
  {
    peripherals_.createPeripherals();
    extMem_.setFaultHandler([&](uint32_t addr,uint32_t data,bool write){
      //if(addr < extMem_.getSize() && write)
      //{
      //  throw Memory::FaultException(addr,Memory::FaultException::Write):
      //}
      auto result=peripherals_.accessPeripheral(addr,data,write);
      if(!result.first)
      {
        throw Memory::FaultException(addr,write?Memory::FaultException::Write:Memory::FaultException::Read);
      }
      return result.second;
    });
  }
  
  bool addBreakpoint(uint32_t addr)
  {
    breakpoints_.push_back(addr);
    return true;
  }
  
  bool removeBreakpoint(uint32_t addr)
  {
    for(auto it=breakpoints_.begin();it!=breakpoints_.end();)
    {
      if(*it == addr)
      {
        auto r=it;
        ++it;
        breakpoints_.erase(r);
      }
    }
    
    return true;
  }
  
  bool addMemoryBreakpoint(uint32_t addr)
  {
    extMem_.setInvalidRegion(addr,1);
    return true;
  }
  
  bool readMem(uint32_t addr,uint32_t numWords,uint32_t *buffer)
  {
    for(uint32_t i=0;i<numWords;++i)
    {
      if((addr+i) >= localMem_.getSize())
      {
        buffer[i]=extPort_.read(addr+i);
      }
      else
      {
        buffer[i]=localPort_.read(addr+i);
      }
    }
    
    return true;
  }
  
  bool writeMem(uint32_t addr,uint32_t numWords,const uint32_t *buffer)
  {
    for(uint32_t i=0;i<numWords;++i)
    {
      if((addr+i) >= (localMem_.getSize()))
      {
        extPort_.write(addr+i,buffer[i]);
        extPort_.update();
      }
      else
      {
        localPort_.write(addr+i-codeMemSize_,buffer[i]);
        localPort_.update();
      }
    }
    
    return true;
  }
  
  virtual bool writeCode(const uint16_t *code,uint32_t codeSize)
  {
    for(uint32_t i=0;i<codeSize;++i)
    {
      codePort_.write(i,code[i]);
      codePort_.update();
    }
  }
  
  DebuggerInterface::_Run runToAddr(uint32_t addr)
  {
    do
    {      
      uint32_t oldPC=processor_.getCurrentPC();
      while(!processor_.isDecodeActive() || processor_.getCurrentPC() == oldPC)
      {
        try
        {
          processor_.update(0,0,0);
        }
        catch(const Memory::FaultException &e)
        {
          if(e.getType() == Memory::FaultException::Read)
          {
            return {DebuggerInterface::_Run::ACCESS_FAULT_READ,e.getAddr()};
          }
          
          if(e.getType() == Memory::FaultException::Write)
          {
            return {DebuggerInterface::_Run::ACCESS_FAULT_WRITE,e.getAddr()};
          }
          
          //if(e.getType() == Memory::FaultException::Breakpoint)
          //{
          //  return {DebuggerInterface::_Run::MEMORY_BP_HIT,e.getAddr()};
          //}
          
          return {DebuggerInterface::_Run::ERROR,processor_.getCurrentPC()};
        }
        
        ++cycleCount_;
      }
      
      for(auto i : breakpoints_)
      {
        if(i == processor_.getCurrentPC())
        {
          return {DebuggerInterface::_Run::BREAKPOINT_HIT,processor_.getCurrentPC()};
        }
      }
      
    }while(processor_.getCurrentPC() != addr);
    
    return {DebuggerInterface::_Run::OK,processor_.getCurrentPC()};
  }
  
  void halt()
  {
    //return 0xFFFFFFFF;
  }
  
  void reset()
  {
    processor_.reset();
    cycleCount_=0;
  }
  
  uint32_t getPC()
  {
    return processor_.getCurrentPC();
  }
  
protected:
  Memory localMem_;
  Memory codeMem_;
  Memory extMem_;
  Memory::Port codePort_;
  Memory::Port localPort_;
  Memory::Port extPort_;
  SLProcessor processor_;
  std::vector<uint32_t> breakpoints_;
  uint32_t cycleCount_;
  uint32_t codeMemSize_;
  Peripherals peripherals_;
};
