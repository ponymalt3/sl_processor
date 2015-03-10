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

  Mem(uint32_t size)
  {
    size_=size;
    data_=new uint32_t[size];
  }

  uint32_t getSize() const { return size_; }

protected:
  uint32_t size_;
  uint32_t *data_;
};

class MemPort
{
public:
  MemPort(Mem &mem)
  : mem_(mem)
  {
    pendingWrite_=0;
    wData_=0;
    wAddr_=0;
  }

  uint32_t read(uint32_t addr) const { assert(addr < mem_.size_); return mem_.data_[addr]; }
  void write(uint32_t addr,uint32_t data) { assert(addr < mem_.size_); wAddr_=addr; wData_=data; pendingWrite_=1; }

  void update()
  {
    if(pendingWrite_)
    {
      mem_.data_[wAddr_]=wData_;
      pendingWrite_=0;
    }
  }

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

  enum {CMP_GT,CMP_LT,CMP_EQ,CMP_NEQ};
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
    uint32_t incAD_ : 1;
    uint32_t incAD2_ : 1;
    uint32_t auxData_ : 2;

    uint32_t goto_ : 1;
    uint32_t goto_const_ : 1;
    uint32_t loop_ : 1;
    uint32_t load_ : 1;
    uint32_t cmp_ : 1;
    uint32_t neg_ : 1;
    uint32_t wait_ : 1;
    uint32_t signal_ : 1;

    uint32_t loopEndDetect_ : 1;

    uint32_t curPc_ : 16;
  };

  struct _MemFetch1
  {
    uint32_t externalData_;
  };

  struct _MemFetch2
  {
    uint32_t readData_[2];
  };

  struct _DecodeEx
  {
    uint32_t exec_ : 1;
    uint32_t cmd_ : 3;
    uint32_t a_;
    uint32_t b_;

    uint32_t writeAddr_ : 16;
    uint32_t writeData_ : 32;
    uint32_t writeEn_ : 1;

    uint32_t wbEn_ : 1;
    uint32_t wbReg_ : 2;

    uint32_t cmpMode_ : 1;

    uint32_t en_ : 1;

    //uint32_t goto_ : 1;
    //uint32_t curPc_ : 16;
  };

  struct _State
  {
    uint32_t pc_ : 16;
    uint32_t addr_[2];//falling edge
    uint32_t irs_ : 16;
    uint32_t loopCount_ : 16;//falling edge
    uint32_t loopBeg_ : 16;//falling edge
    uint32_t loopEnd_ : 16;
    uint32_t loopEndValid_ : 1;
    uint32_t constData_ : 32;
    uint32_t constDataValid_ : 1;
  };

  struct _Exec
  {
    //add/sub/mul 2 cycles
    uint32_t result_;
    uint32_t intResult_ : 16;
    uint32_t cmp_ : 1;
    //uint32_t intResultSign_ : 1;
    uint32_t cmd_;
    uint32_t complete_ : 1;
    uint32_t sameUnitReady_ : 1;
    uint32_t empty_ : 1;

    uint32_t writeAddr_ : 16;
    uint32_t writeData_ : 32;
    uint32_t writeEn_ : 1;
  };

  struct _MUnit
  {
    uint32_t result_;
    uint32_t complete_ : 1;
    uint32_t sameUnitReady_ : 1;
    uint32_t empty_ : 1;
    uint32_t cmpLt_ : 1;
    uint32_t cmpEq_ : 1;
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

    //disable exec
    uint32_t execDisabled_ : 1;
    uint32_t execDisableCounter_ : 2;

    uint32_t stall_ : 1;
    uint32_t forceNOP_ : 1;//in dec1

    uint32_t loadConst_ : 1;

    //states
    uint32_t loadState_ : 2;
    uint32_t loopWrites_ : 3;
    uint32_t dataWrites[2];
    uint32_t pendingNOPs_ : 3;
    uint32_t stall_1d_ : 1;
  };

  struct _ExternMemAccess
  {
    uint32_t en_ : 1;
    uint32_t rw_ : 1;
    uint32_t addr_ : 16;
    uint32_t data_ : 32;
  };

  _CodeFetch codeFetch();
  _Decode decodeInstr();
  _MemFetch1 memFetch1(_Decode decode) const;
  _MemFetch2 memFetch2() const;
  _DecodeEx decodeEx();
  _Exec execute(_ExternMemAccess &externMemAccess);

  _Ctrl ctrl(_Decode decodeComb,_Exec execComb);
  _State state(_Ctrl ctrlComb);

  void update(_ExternMemAccess &externMemAccess);

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
  BitData(uint32_t data) { data_=data; }
  uint32_t operator()(uint32_t j,uint32_t i=32)
  {
    if(i == 32)
      i=j;

    return (data_>>i)&((1<<(i-j+1))-1);
  }

protected:
  uint32_t data_;
};

class MathUnit
{
public:
  MathUnit()
  {

  }

  SLProcessor::_MUnit comb(SLProcessor::_DecodeEx decEx)
  {
    SLProcessor::_MUnit munit;

    _qfp32_t a,b;
    a.asUint=decEx.a_;
    b.asUint=decEx.b_;

    munit.cmpEq_=0;
    munit.cmpLt_=0;
    munit.complete_=0;
    munit.sameUnitReady_=0;
    munit.empty_;

    if(pipeline_[curCycle_].cmd_ != 0xFF)
    {
      munit.result_=pipeline_[curCycle_].result_;
      munit.complete_=1;
      munit.sameUnitReady_=1;
    }

    if(decEx.en_)
    {
      if(decEx.cmd_ == SLProcessor::CMD_MOV)
      {
        munit.result_=a.asUint;
        munit.complete_=1;
        munit.sameUnitReady_=1;
      }
      else if(decEx.cmd_ == SLProcessor::CMD_NEG)
      {
        munit.result_=(a*-1).asUint;
        munit.complete_=1;
        munit.sameUnitReady_=1;
      }
      else if(decEx.cmd_ == SLProcessor::CMD_CMP)
      {
        munit.result_=0;
        munit.complete_=1;
        munit.sameUnitReady_=1;
        munit.cmpEq_=a==b;
        munit.cmpLt_=a<b;
      }
    }

    return munit;
  }

  void update(SLProcessor::_DecodeEx decEx)
  {
    _qfp32_t a,b;
    a.initFromRaw(decEx.a_);
    b.initFromRaw(decEx.b_);

    uint32_t mulRes=0;
    uint32_t addRes=0;
    uint32_t intermediateEn=0;

    if(pipeline_[curCycle_].cmd_ != 0xFF)
    {
      if(op.cmd_ == SLProcessor::CMD_MAC)
      {
        mulRes=op.result_;
        intermediateEn=1;
      }

      if(op.cmd_ == SLProcessor::CMD_ADD || op.cmd_ == SLProcessor::CMD_SUB)
        addRes=op.result_;

      pipeline_[curCycle_].cmd_=0xFF;
    }

    _qfp32_t extA=a,extB=b;

 /*   if(ma == 0)
      extA=data_[0];

    if(mb == 0)
      extA=data_[1];

    if(cB)
      data_[1]=0;
    else if(eB)
    {
      if(mc == 0)
        data_[1]=addRes;//from add
      else
        data_[1]=mulRes;//from mul
    }

    if(cA)
      data_[0]=0;
    else if(eA)
      data_[0]=addRes;/*/


    _SumMode sumModeNext={0,1,1,0,0,0,0,0,0,0};
    sumModeNext.muxA_=!macEn_;
    switch(sumModeState_)
    {
    case 1:
      sumModeNext.muxA_=1;
      sumModeNext.clearRegA_=1;
      break;
    case 2:
      sumModeNext.enRegA_=1;
      break;
    case 4:
      sumModeNext.muxC_=1;
      sumModeNext.enRegB_=1;
      break;
    case 8:
      sumModeNext.complete_=1;
      sumModeNext.muxB_=0;
      sumModeNext.clearRegB_=1;
      break;
    }

    sumModeNext.ready_=sumModeNext.muxB_ & sumModeNext.muxC_;

    uint32_t sumModeStateNext_=(sumModeStateNext_<<1)+(sumModeStateNext_>>3);

    if(!((sumModeState_ == 1 && sumEn == 0) || (sumModeState_ == 2 && sumEn == 1)))
      sumModeState_=sumModeStateNext_;

    switch(decEx.cmd_)
    {
    case SLProcessor::CMD_ADD:
      pipeline_[(curCycle_+2)%32]={(extA+extB).asUint,SLProcessor::CMD_ADD}; break;
    case SLProcessor::CMD_SUB:
      pipeline_[(curCycle_+2)%32]={(extA-extB).asUint,SLProcessor::CMD_SUB}; break;
    case SLProcessor::CMD_MUL:
      pipeline_[(curCycle_+2)%32]={(a*b).asUint,SLProcessor::CMD_MUL}; break;
    case SLProcessor::CMD_DIV:
      pipeline_[(curCycle_+32)%32]={(a/b).asUint,SLProcessor::CMD_DIV}; break;
    case SLProcessor::CMD_MAC:
      pipeline_[(curCycle_+1)%32]={(a*b).asUint,SLProcessor::CMD_MUL}; break;
    default:
    }

    curCycle_=(curCycle_+1)%32;
  }


protected:
  struct _MathOp
  {
    uint32_t result_;
    uint32_t cmd_ : 8;
  };

  _MathOp pipeline_[32];
  _MathOp macPipeline_[2];
  uint32_t curCycle_;

  uint32_t state_ : 4;
  _qfp32_t dataRegister_[2];

  struct _SumMode
  {
    uint32_t muxA_ : 1;//0 regular input; 1 internal reg
    uint32_t muxB_ : 1;
    uint32_t muxC_ : 1;//mux for mul into reg B
    uint32_t enRegA_ : 1;//enable int reg
    uint32_t enRegB_ : 1;
    uint32_t clearRegA_ : 1;
    uint32_t clearRegB_ : 1;
    uint32_t enAdd_ : 1;//enable addition
    uint32_t complete_ : 1;
    uint32_t ready_ : 1;
  };

  uint32_t sumModeState_;
  uint32_t macEn_;

  uint32_t complete_1d_ : 1;
};

SLProcessor::SLProcessor(Mem &localMem,MemPort &portExt,MemPort &portCode)
  : portExt_(portExt)
  , portCode_(portCode)
  , portF0_(localMem)
  , portF1_(localMem)
  , portR2_(localMem)
{
  SharedAddrBase_=localMem.getSize();
}


void SLProcessor::signal()
{

}

bool SLProcessor::isRunning()
{
  return true;
}

void SLProcessor::reset()
{

}



SLProcessor::_CodeFetch SLProcessor::codeFetch()
{
  _CodeFetch fetch;
  fetch.data_=portCode_.read(state_.pc_);
  fetch.pc_=state_.pc_;

  return fetch;
}

SLProcessor::_Decode SLProcessor::decodeInstr()
{
  _Decode decode;
  BitData bdata(code_.data_);

  decode.muxAD0_=bdata(0);
  decode.muxA_=bdata(1);
  decode.muxAD1_=bdata(2);
  decode.muxB_=bdata(3);

  decode.enIRS_=0;
  decode.enMEM_=0;
  decode.enREG_=0;
  //enCMD

  decode.wbREG_=bdata(6 downto 5);

  decode.incAD_=0;//bdata(4);
  decode.incAD2_=0;//bdata(4);

  decode.cData_=bdata(11 downto 2);
  decode.offset_=bdata(10 downto 2);

  decode.CMD_=bdata(7 downto 5);

  decode.exCOND_=bdata(5 downto 4);

  decode.goto_=0;
  decode.loop_=0;
  decode.load_=0;
  decode.cmp_=0;
  decode.neg_=0;
  decode.wait_=0;
  decode.signal_=0;

  decode.loopEndDetect_=0;

  if(bdata(15) == 0)//OPIRS
  {
    decode.CMD_=bdata(13 downto 11);
    decode.incAD_=bdata(14);
    //decode.incAD1_=bdata(14);
    decode.enIRS_=1;
  }
  else
  {
    switch(bdata(14 downto 12))
    {
    case 0: //MOVIRS1
      decode.CMD_=0;//mov
      decode.enIRS_=1;
      decode.wbREG_=bdata(1 downto 0);
      decode.enREG_=1;
      //decode.incAD0_=bdata(11);
      //decode.incAD1_=bdata(11);

      if(decode.wbREG_ == 2)//restore loop
        decode.loopEndDetect_=1;

      break;
    case 1: //MOVIRS2
      decode.CMD_=0;//mov
      decode.enIRS_=1;
      decode.enMEM_=1;
      break;

    case 2: //GOTO
      decode.goto_=1;
      decode.goto_const_=bdata(0);
      break;

    case 3: //LOOP
      decode.loop_=1;
      break;

    case 4: //LOAD
      decode.load_=1;
      break;

    case 5: //OP
      decode.incAD_=bdata(4);
      decode.incAD2_=bdata(8);
      break;

    case 6: //CMP
      decode.cmp_=1;
      decode.CMD_=1;//sub
      decode.enIRS_=bdata(5);
      decode.incAD_=bdata(4);
      break;

    case 7:

      switch(bdata(11 downto 6))
      {
      case 0:
      case 1://MOV
        decode.enREG_=1;
        decode.incAD_=bdata(4);
        break;
      case 2://MOVDATA
        decode.enMEM_=1;
        decode.incAD_=bdata(4);
        break;

      case 3://NEG
        decode.neg_=1;
        break;

      case 4://SIG
        decode.wait_=bdata(5);
        decode.signal_=(~bdata(5))&1;
        break;

      default:
        break;
      }
    }
  }

  decode.curPc_=state_.pc_;

  return decode;
}

SLProcessor::_MemFetch1 SLProcessor::memFetch1(_Decode decode) const
{
  _MemFetch1 memFetch;

  uint32_t addr=state_.addr_[decode.muxAD1_];

  if(addr >= SharedAddrBase_)
    memFetch.externalData_=portExt_.read(addr);

  return memFetch;
}

SLProcessor::_MemFetch2 SLProcessor::memFetch2() const
{
  _MemFetch2 memFetch;

  uint32_t addr0=state_.addr_[decode_.muxAD0_];
  uint32_t addr1=state_.addr_[decode_.muxAD1_];

  memFetch.readData_[0]=portF0_.read(addr0);

  if(addr1 < SharedAddrBase_)
    memFetch.readData_[1]=portF0_.read(addr1);

  return memFetch;
}

SLProcessor::_DecodeEx SLProcessor::decodeEx()
{
  _DecodeEx decodeEx;

  decodeEx.a_=exec_.result_;

  if(decode_.muxA_)
    decodeEx.a_=mem2_.readData_[0];

  decodeEx.b_=mem2_.readData_[1];

  decodeEx.writeAddr_=state_.addr_[decode_.muxAD1_];

  decodeEx.writeData_=decodeEx.a_;

  if(decode_.muxB_)
    decodeEx.writeData_=(state_.loopBeg_<<16)+state_.loopCount_;

  decodeEx.writeEn_=decode_.enMEM_;

  decodeEx.wbEn_=decode_.enREG_;
  decodeEx.wbReg_=decode_.wbREG_;

  return decodeEx;
}

SLProcessor::_Ctrl SLProcessor::ctrl(_Decode decodeComb,_Exec execComb)
{
  _Ctrl ctrl;

  ctrl.stallExternalDataWrite_=state_.addr_[decodeComb.muxAD1_] >= SharedAddrBase_ && exec_.writeEn_ == 1 && exec_.writeAddr_ >= SharedAddrBase_;
  ctrl.stallExternalAddrNotAvail_=ctrl_.dataWrites[decodeComb.muxAD1_] != 0;
  ctrl.stallLoopNotAvail_=ctrl_.loopWrites_ != 0;


  ctrl.stallResultNotAvail_=decode_.muxA_ == 1 && (execComb.complete_ == 0 && execComb.empty_ == 0);
  ctrl.stallUnitNotAvail_=decode_.CMD_ != 0 && (decode_.CMD_ != exec_.cmd_ && execComb.complete_ == 0 && execComb.empty_ == 0);

  ctrl.stallMemAddrInWB_=0;
  if(exec_.writeEn_ == 1)
  {
    if(decode_.muxA_ == 0 && state_.addr_[decode_.muxAD0_] == exec_.writeAddr_)
      ctrl.stallMemAddrInWB_=1;

    if(state_.addr_[decode_.muxAD1_] == exec_.writeAddr_)
      ctrl.stallMemAddrInWB_=1;
  }

  ctrl.execDisabled_=0;

  if(ctrl_.execDisableCounter_ > 0)
  {
    ctrl.execDisabled_=1;
    --ctrl.execDisableCounter_;
  }

  if(execComb.cmp_)
  {
    ctrl.execDisabled_=1;
    ctrl.execDisableCounter_=decEx_.cmpMode_;
  }

  ctrl.stall_=ctrl.stallExternalDataWrite_ || ctrl.stallExternalAddrNotAvail_ || ctrl.stallLoopNotAvail_ || ctrl.stallResultNotAvail_ || ctrl.stallUnitNotAvail_ || ctrl.stallMemAddrInWB_;

  //handling flush
  ctrl.forceNOP_=0;

  if(ctrl.pendingNOPs_ > 0)
  {
    ctrl.pendingNOPs_--;
  }

  ctrl.loadConst_=0;

  if(decode_.load_)
  {
    ctrl.pendingNOPs_=decode_.load_;
    ctrl.forceNOP_=1;

    if(ctrl.loadState_ == 0)
      ctrl.loadConst_=1;
  }
  else if((decode_.goto_ || decode_.goto_const_) && ctrl.execDisabled_ == 0)
  {
    ctrl.pendingNOPs_=5;
    ctrl.forceNOP_=1;
  }
  else if(decodeComb.loopEndDetect_ == 1)
  {
    ctrl.pendingNOPs_=1;
    ctrl.forceNOP_=1;
  }

  //update load state
  ctrl.loadState_=ctrl_.loadState_;

  if(ctrl_.loadState_ > 0)
    ctrl.loadState_=ctrl_.loadState_-1;
  else if(ctrl.loadConst_)
    ctrl.loadState_=decode_.load_;

  ctrl.loopWrites_<<=1;
  ctrl.dataWrites[0]=(ctrl.dataWrites[0]<<1)&0x7;
  ctrl.dataWrites[1]=(ctrl.dataWrites[1]<<1)&0x7;

  if(decodeComb.enREG_)
  {
    if(decodeComb.wbREG_ == WBREG_LOOP)
      ctrl.loopWrites_+=1;
    if(decodeComb.wbREG_ == WBREG_DATA0)
      ctrl.dataWrites[0]+=1;
    if(decodeComb.wbREG_ == WBREG_DATA1)
      ctrl.dataWrites[1]+=1;
  }

  return ctrl;
}

SLProcessor::_State SLProcessor::state(_Ctrl ctrlComb)//,_Exec execComb)
{
  _State stateNext;

  bool incData0=decode_.incAD_ && ctrl_.stall_1d_ == 0;
  state_.addr_[0]+=incData0;
  if(ctrlComb.execDisabled_ == 0)
    ;

  bool incData1=decode_.incAD2_ && ctrl_.stall_1d_ == 0;
  stateNext.addr_[1]+=incData1;
  if(ctrlComb.execDisabled_ == 0)
    ;//clk en


  uint32_t pcNext=state_.pc_+1;
  uint32_t pcNext2=pcNext;
  if(state_.loopEndValid_ && state_.loopEnd_ == pcNext)
  {
    pcNext2=state_.loopBeg_;
  }

  if(decode_.goto_const_)
  {
    if((decode_.cData_&0x200) == 0)
      pcNext2=decode_.curPc_+(decode_.cData_&0x1FF);
    else
      pcNext2=decode_.curPc_-(decode_.cData_&0x1FF);
  }

  /*if(decEx_.goto_)
  {
    if(execComb.intResultSign_ == 0)
      pcNext2=decEx_.curPc+execComb.intResult_;
    else
      pcNext2=decEx_.curPc-execComb.intResult_;
  }*/


  if(decode_.loopEndDetect_)
  {
    stateNext.loopEnd_=decode_.curPc_;
    stateNext.loopEndValid_=1;
  }
  else if(decode_.enREG_ && decode_.wbREG_ == WBREG_LOOP)
    stateNext.loopEndValid_=0;

  if(ctrl_.stall_1d_ == 0)
  {
    if(decode_.loopEndDetect_ || (state_.loopEndValid_ && state_.loopEnd_ == pcNext))
      --stateNext.loopCount_;
  }

  //const loading
  if(decode_.load_ && ctrl_.loadState_ == 0)
  {
    stateNext.constData_=((decode_.cData_>>7)&1)<<31 + (decode_.cData_&0x1FF)<<20;
  }
  else if(ctrl_.loadState_ == 1)
  {
    stateNext.constData_=stateNext.constData_ + ((decode_.cDataExt_>>4)&0x3)<<29 + (decode_.cDataExt_&0xF)<<16 + decode_.cData_<<6;
  }
  else if(ctrl_.loadState_ == 2)
  {
    stateNext.constData_=stateNext.constData_ + decode_.cDataExt_;
  }

  //register write (in EXEC stage)
  if(decEx_.wbEn_)
  {
    switch(decEx_.wbReg_)
    {
    case WBREG_LOOP:
      stateNext.loopCount_=decEx_.a_&0xFFFF;
      stateNext.loopBeg_=decEx_.a_>>16;
      break;
    case WBREG_DATA0:
      stateNext.addr_[0]=decEx_.a_;
      break;
    case WBREG_DATA1:
      stateNext.addr_[1]=decEx_.a_;
      break;
    default:
    }
  }

  return stateNext;
}

SLProcessor::_Exec SLProcessor::execute(_ExternMemAccess &externMemAccess)
{
  _Exec exec;

  exec.complete_=1;

  if(decEx_.writeEn_)
  {
    if(externMemAccess.en_)
    {
      exec.complete_=0;
      if(externMemAccess.rw_ == 0)
        externMemAccess.data_=portExt_.read(externMemAccess.addr_);
      else
        portExt_.write(externMemAccess.addr_,externMemAccess.data_);
    }
    else if(decEx_.writeAddr_ >= SharedAddrBase_)
    {
      portExt_.write(decEx_.writeAddr_&(SharedAddrBase_-1),decEx_.writeData_);
    }

    if(decEx_.writeAddr_ < SharedAddrBase_)
    {
      portR2_.write(decEx_.writeAddr_,decEx_.writeData_);
    }
  }

  uint32_t result=0;
  uint32_t le=0;
  uint32_t eq=0;

  exec.cmp_=0;
  if(decEx_.cmd_ == CMD_CMP)
  {
    switch(decEx_.cmpMode_)
    {
    case CMP_EQ:
      exec.cmp_=eq; break;
    case CMP_NEQ:
      exec.cmp_=not eq; break;
    case CMP_GT:
      exec.cmp_=(not le) & (not eq); break;
    case CMP_LT:
      exec.cmp_=le; break;
    }
  }

  return exec;
}

void SLProcessor::update(_ExternMemAccess &externMemAccess)
{
  _Exec execNext=execute(externMemAccess);

  //before falling edge
  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr();
  _MemFetch2 mem2=memFetch2();

  _Ctrl ctrlNext=ctrl(decodeNext,execNext);
  _State stateNext=state(ctrlNext);

  portF0_.update();
  portF1_.update();

  //after falling edge
  _MemFetch1 mem1=memFetch1(decodeNext);
  _DecodeEx decExNext=decodeEx();

  portR2_.update();
  portExt_.update();
}





#endif /* SLPROCESSOR_H_ */
