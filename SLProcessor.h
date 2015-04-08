/*
 * SLProcessor.h
 *
 *  Created on: Jan 19, 2015
 *      Author: malte
 */

#ifndef SLPROCESSOR_H_
#define SLPROCESSOR_H_

#include "qfp32.h"

#include <list>
#include <iostream>


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

class Mem
{
public:
  friend class MemPort;

  Mem(uint32_t size);

  uint32_t getSize() const;

protected:
  uint32_t size_;
  uint32_t *data_;
};

class MemPort
{
public:
  MemPort(Mem &mem);

  uint32_t read(uint32_t addr) const;
  void write(uint32_t addr,uint32_t data);

  void update();

protected:
  Mem &mem_;
  uint32_t pendingWrite_ : 1;
  uint32_t wData_;
  uint32_t wAddr_ : 16;
};


class SLProcessor
{
public:
  SLProcessor(Mem &localMem,MemPort &portExt,MemPort &portCode);

  void signal();
  bool isRunning();
  void reset();

  enum {CMP_GT,CMP_LE,CMP_EQ,CMP_NEQ};
  enum {CMD_MOV=0,CMD_ADD,CMD_SUB,CMD_MUL,CMD_DIV,CMD_MAC,CMD_NEG,CMD_CMP};
  enum {WBREG_NONE=0,WBREG_DATA0,WBREG_DATA1,WBREG_LOOP};

  struct _CodeFetch
  {
    uint32_t data_;
    uint32_t pc_ : 16;
  };

  struct _Decode
  {
    uint32_t muxAD0_ : 1;
    uint32_t muxAD1_ : 1;
    uint32_t muxA_ : 1;//shortcut RESULT
    uint32_t muxB_ : 1;//select LOOP for MEM only
    uint32_t enMEM_ : 1;
    uint32_t enIRS_ : 1;
    uint32_t enREG_ : 1;
    uint32_t CMD_ : 3;
    uint32_t wbREG_ : 2;
    uint32_t offset_ : 9;
    uint32_t cData_ : 10;
    uint32_t cDataExt_ : 6;
    uint32_t exCOND_ : 2;
    //uint32_t incAD_ : 1;
    //uint32_t incAD2_ : 1;
    uint32_t auxData_ : 2;

    uint32_t goto_ : 1;
    uint32_t goto_const_ : 1;
    uint32_t loop_ : 1;
    uint32_t load_ : 1;
    uint32_t loadNumInstrs_ : 1;
    uint32_t cmp_ : 1;
    uint32_t neg_ : 1;
    uint32_t wait_ : 1;
    uint32_t signal_ : 1;

    uint32_t cmpMode_ : 2;
    uint32_t cmpNoXCy_ : 1;

    uint32_t loopEndDetect_ : 1;

    uint32_t memEx_ : 1;

    uint32_t curPc_ : 16;
    uint32_t jmpTargetPc_ : 16;
    uint32_t incAD0_ : 1;
    uint32_t incAD1_ : 1;
  };

  struct _MemAddr
  {
    uint32_t memAD0_ : 16;
    uint32_t memAD1_ : 16;
    uint32_t addrNext_[2];
    uint32_t memExt_ : 1;
  };

  struct _MemFetch1
  {
    uint32_t externalData_;
  };

  struct _MemFetch2
  {
    uint32_t readData_[2];
    uint32_t writeAD_ : 16;
  };

  struct _DecodeEx
  {
    uint32_t cmd_ : 3;
    uint32_t a_;
    uint32_t b_;

    uint32_t writeAddr_ : 16;
    uint32_t writeEn_ : 1;
    uint32_t writeExt_ : 1;

    uint32_t wbEn_ : 1;
    uint32_t wbReg_ : 2;

    uint32_t cmp_ : 1;
    uint32_t cmpMode_ : 2;
    uint32_t cmpNoXCy_ : 1;

    uint32_t en_ : 1;//unnecessary????

    uint32_t goto_ : 1;
  };

  struct _State
  {
    uint32_t pc_ : 16;
    uint32_t addr_[2];//falling edge
    uint32_t irs_ : 16;

    uint32_t loopCount_;//falling edge
    uint32_t loopEndAddr_ : 16;
    uint32_t loopStartAddr_ : 16;
    uint32_t loopValid_ : 1;

    uint32_t loadState_ : 2;

    uint32_t resultPrefetch_ : 1;//fetch result data when ready and set this flag (also used for const loading)
    uint32_t stageEnable_ : 5;



    //update info for some states
    uint32_t incAd0_ : 1;
    uint32_t incAd1_ : 1;
    uint32_t decLoop_ : 1;
  };

  struct _MUnit
  {
    uint32_t result_;
    uint32_t complete_ : 1;
    uint32_t sameUnitReady_ : 1;
    uint32_t cmd_ : 3;
    uint32_t idle_ : 1;//!idle
    uint32_t cmpLt_ : 1;
    uint32_t cmpEq_ : 1;
  };

  struct _Exec
  {
    //add/sub/mul 2 cycles
    _MUnit munit_;
    uint32_t intResult_ : 16;
    uint32_t condExec_ : 1;
  };

  struct _Ctrl
  {
    //stall dec1
    uint32_t stallExternalDataWrite_ : 1;//from WB unit
    uint32_t stallExternalAddrNotAvail_ : 1;
    uint32_t stallLoopNotAvail_ : 1;

    //stall dec2
    uint32_t stallResultNotAvail_ : 1;
    uint32_t stallUnitNotAvail_ : 1;
    uint32_t stallMemAddrInWB_ : 1;

    //stall
    uint32_t stallExec_ : 1;
    uint32_t stallDec_ : 1;
    uint32_t stallDecEx_ : 1;
    uint32_t flushPipeline_ : 3;

    //states
    uint32_t loopWrites_ : 3;
    uint32_t dataWrites[2];
    uint32_t extWriteAdIncCtrl_[2];
  };

  struct _ExternMemAccess
  {
    uint32_t en_ : 1;
    uint32_t rw_ : 1;
    uint32_t addr_ : 16;
    uint32_t data_ : 32;
  };

  struct _StallCtrl
  {
    uint32_t pcUpdate_ : 1;
    uint32_t s1En_ : 1;//dec
    uint32_t s2En_ : 1;//decEx
    uint32_t s3En_ : 1;//exec
    uint32_t stageEnNext_ : 5;
  };

  _CodeFetch codeFetch();
  _Decode decodeInstr();
  _MemFetch1 memFetch1(const _Decode &decComb,uint32_t (&addrNext)[2]) const;
  _MemFetch2 memFetch2() const;
  _DecodeEx decodeEx(const _Exec &execComb);
  _Exec execute(uint32_t extMemStall);

  _Ctrl ctrl(const _Decode &decodeComb,const _DecodeEx &decodeExComb,const _Exec &execComb,uint32_t extMemStall);
  _State state(const _Ctrl &ctrlComb,const _DecodeEx &decExComb,uint32_t loopActive);
  _StallCtrl stallCtrl(uint32_t stallDec,uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline);

  void update(uint32_t extMemStall,uint32_t setPcEnable,uint32_t pcValue);

  SLArithUnit arithUnint_;

  _State state_;
  _CodeFetch code_;
  _Decode decode_;
  _MemFetch1 mem1_;
  _MemFetch2 mem2_;
  _DecodeEx decEx_;
  _Exec exec_;
  _Ctrl ctrl_;
  _State stateNext_;

  MemPort portF0_;
  MemPort portF1_;
  MemPort portR2_;
  MemPort &portExt_;
  MemPort &portCode_;

  uint32_t SharedAddrBase_;
};

#define downto ,

class BitData
{
public:
  BitData() { data_=0; }
  BitData(uint32_t data) { data_=data; }
  BitData& operator=(uint32_t data) { data_=data; return *this; }
  operator uint32_t() const { return data_; }
  uint32_t operator()(uint32_t j,uint32_t i=32)
  {
    if(i == 32)
      i=j;

    return (data_>>i)&((1<<(i-j+1))-1);
  }

protected:
  uint32_t data_;
};

#endif /* SLPROCESSOR_H_ */
