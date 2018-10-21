/*
 * SLProcessorStructs.h
 *
 *  Created on: Apr 13, 2015
 *      Author: malte
 */

#ifndef SLPROCESSORSTRUCTS_H_
#define SLPROCESSORSTRUCTS_H_

#include <stdint.h>


#define downto ,

class BitData
{
public:
  BitData() { data_=0; }
  BitData(uint32_t data) { data_=data; }
  BitData& operator=(uint32_t data) { data_=data; return *this; }
  operator uint32_t() const { return data_; }
  uint32_t operator()(uint32_t j,uint32_t i=32) const
  {
    if(i == 32)
      i=j;

    return (data_>>i)&((1<<(j-i+1))-1);
  }

protected:
  uint32_t data_;
};

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
  uint32_t CMD_ : 4;
  uint32_t wbREG_ : 2;
  uint32_t cData_ : 10;
  uint32_t cDataExt_ : 2;
  
  uint32_t enADr0_ : 1;
  uint32_t enADr1_ : 1;

  uint32_t goto_ : 1;
  uint32_t goto_const_ : 1;
  uint32_t load_ : 1;
  uint32_t cmp_ : 1;
  uint32_t neg_ : 1;
  uint32_t trunc_ : 1;
  uint32_t wait_ : 1;
  uint32_t signal_ : 1;
  uint32_t loop_ : 1;

  uint32_t cmpMode_ : 2;
  uint32_t cmpNoXCy_ : 1;

  uint32_t irsAddr_ : 16;

  uint32_t memEx_ : 1;

  uint32_t curPc_ : 16;
  uint32_t jmpBack_ : 1;
  uint32_t jmpTargetPc_ : 16;
  uint32_t incAD0_ : 1;
  uint32_t incAD1_ : 1;
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
  uint32_t cmd_ : 4;
  uint32_t a_;
  uint32_t b_;
  
  uint32_t mem0_;
  uint32_t mem1_;
  uint32_t memX_;
  uint32_t mux0_ : 1;

  uint32_t writeAddr_ : 16;
  uint32_t writeEn_ : 1;
  uint32_t writeExt_ : 1;
  uint32_t writeDataSel_ : 1;

  uint32_t wbEn_ : 1;
  uint32_t wbReg_ : 2;

  uint32_t cmp_ : 1;
  uint32_t cmpMode_ : 2;
  uint32_t cmpNoXCy_ : 1;

  uint32_t goto_ : 1;
  
  uint32_t neg_ : 1;
  uint32_t trunc_ : 1;
  
  uint32_t load_ : 1;
  uint32_t loadData_ : 12;

  uint32_t stall_ : 1;
};

struct _State
{
  enum {S_FETCH=2,S_DEC=1,S_DECEX=1,S_EXEC=0};

  uint32_t pc_ : 16;
  uint32_t addr_[2];//rising edge
  uint32_t irs_ : 16;

  uint32_t loadState_ : 3;

  BitData enable_;
  
  uint32_t stallExec1d_ : 1;
  
  uint32_t loopCount_;
  
  uint32_t result_;
  uint32_t resultPrefetch_ : 1;//fetch result data when ready and set this flag (also used for const loading)

  //update info for some states
  uint32_t incAd0_ : 1;
  uint32_t incAd1_ : 1;
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
  int32_t intResult_;
  uint32_t execNext_ : 1;
  uint32_t stall_ : 1;
  uint32_t flush_ : 1;
};

struct _StallCtrl
{
  uint32_t stallDecEx_ : 1;
  uint32_t stallExec_ : 1;
  uint32_t flush_ : 1;
  BitData enNext_;
};

#endif /* SLPROCESSORSTRUCTS_H_ */
