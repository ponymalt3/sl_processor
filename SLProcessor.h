/*
 * SLProcessor.h
 *
 *  Created on: Jan 19, 2015
 *      Author: malte
 */

#ifndef SLPROCESSOR_H_
#define SLPROCESSOR_H_

#include <list>
#include <iostream>

#include "SLArithUnit.h"
#include "SLProcessorStructs.h"

/*
 * IRS=Intermediate Result Storage (Stack)
 *
 * MOV RESULT, DATAx, LOOP  [IRS, DATAx], CONST16, CONST32
 * MOV [DATAx, IRS]   RESULT, LOOP
 *
 * OP RESULT, [DATAx]  [DATAx, IRS]
 *
 * ADD,SUB,MUL,DIV,MAC,NEG
 *
 * CMPLE => SUB + neg
 * CMPEQ => SUB + neg
 *
 * GOTO RESULT, CONST10
 * LOOP RESULT  CONST10
 *
 * LEND
 * WAIT
 * SIGNAL
 *
 * Register:
 *
 * DATAx
 * RESULT
 * LOOP
 *
 * IRS
 * IP
 * FLAGS
 *
 */

#include <stdint.h>
#include <assert.h>
#include <functional>
#include <map>

class Memory
{
public:
  class FaultException : public std::exception
  {
  public:
    enum Type {Read,Write};
    
    FaultException(uint32_t addr,Type type)
    {
      addr_=addr;
      type_=type;
    }
    
    uint32_t getAddr() const { return addr_; }
    Type getType() const { return type_; }
    
    virtual const char* what() const noexcept
    {
      static std::string s;
      s=(getType()==Read?"read fault at ":"write fault at ") + std::to_string(getAddr());
      return s.c_str();
    }
  
  protected:
  uint32_t addr_;
  Type type_;
  };
  
  class Port
  {
  public:
    friend class Memory;

    Port(const Port &cpy);

    uint32_t read(uint32_t addr) const;
    void write(uint32_t addr,uint32_t data);

    void update();

  protected:
    Port(Memory &mem);
    bool checkAccess(uint32_t addr);

    Memory &memory_;
    bool pendingWrite_;
    uint32_t wData_;
    uint32_t wAddr_;
  };

  Memory(uint32_t size);

  Port createPort();
  uint32_t getSize() const;
  
  void setInvalidRegion(uint32_t beg,uint32_t size);  
  void setFaultHandler(const std::function<uint32_t(uint32_t,uint32_t,bool)> &faultHandler);

protected:
  uint32_t size_;
  uint32_t *data_;
  std::function<uint32_t(uint32_t,uint32_t,bool)> faultHandler_;
  std::map<uint32_t,uint32_t> invalidRegions_;
};


class SLProcessor
{
public:
  SLProcessor(Memory &localMem,const Memory::Port &portExt,const Memory::Port &portCode);

  void signal();
  bool isRunning();
  void reset();

  void update(uint32_t extMemStall,uint32_t setPcEnable,uint32_t pcValue);
  
  uint32_t getExecutedAddr();
  
  uint32_t getCurrentPC() const
  {
    return decode_.curPc_;
  }
  
  bool isDecodeActive() const { return enable_(_State::S_DECEX); }
  bool isBusLocked() const { return state_.busLock_; }

protected:
  _CodeFetch codeFetch();
  _Decode decodeInstr() const;
  _MemFetch1 memFetch1(const _Decode &decodeComb) const;
  _MemFetch2 memFetch2(const _Decode &decComb) const;
  _DecodeEx decodeEx(const _Decode &decodeComb,const _MemFetch1 &mem1,const _MemFetch2 &mem2,uint32_t extMemStall);
  _Exec execute(uint32_t extMemStall,const _Decode &decComb);

  _StallCtrl control(uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline) const;
  _State updateState(const _Decode &decComb,const _Exec &execNext,uint32_t setPcEnable,uint32_t pcValue) const;

  SLArithUnit arithUnint_;

  _State state_;
  _CodeFetch code_;
  _Decode decode_;
  //_MemFetch1 mem1_;
  //_MemFetch2 mem2_;
  _DecodeEx decEx_;
  
  BitData enable_;

  Memory::Port portR0_;
  Memory::Port portR1_;
  Memory::Port portExt_;
  Memory::Port portCode_;

  uint32_t SharedAddrBase_;
  uint32_t executedAddr_;
  
  uint32_t cycleCount_;
};

#endif /* SLPROCESSOR_H_ */
