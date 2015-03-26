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
    uint32_t addrNext_[2];//no register only comb
    uint32_t irs_ : 16;

    uint32_t loopCount_;//falling edge
    uint32_t loopEndAddr_ : 16;
    uint32_t loopStartAddr_ : 16;
    uint32_t loopValid_ : 1;

    uint32_t loadState_ : 2;

    uint32_t resultPrefetch_ : 1;//fetch result data when ready and set this flag (also used for const loading)
    uint32_t stageEnable_ : 5;
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

  struct _PipelineCtrl
  {
    uint32_t s1En_ : 1;//dec
    uint32_t s2En_ : 1;//decEx
    uint32_t pcUpdate_ : 1;
    uint32_t stallDec_ : 1;
    uint32_t stallDecEx_ : 1;
    uint32_t condExec_ : 1;
    uint32_t flushPipeline_ : 3;
  };

  _CodeFetch codeFetch();
  _Decode decodeInstr();
  _MemFetch1 memFetch1(const _Decode &decComb) const;
  _MemFetch2 memFetch2() const;
  _DecodeEx decodeEx(const _Exec &execComb);
  _Exec execute(_ExternMemAccess &externMemAccess);

  _Ctrl ctrl(const _Decode &decodeComb,const _DecodeEx &decodeExComb,const _Exec &execComb);
  _State state(const _Ctrl ctrlComb,const _DecodeEx &decExComb);

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
    munit.idle_=activeOp_ != 0;
    munit.cmd_=activeOp_;
    
    if(activeOp_ == SLProcessor::CMD_MAC)
    {
      munit.sameUnitReady_=sumModeState_<=2;
      munit.complete_=sumModeState_==8;
      munit.result_=macPipeline_[curCycle_%2];
    }
    else if(activeOp_ != 0)
    {
      if(pipeline_[curCycle_].cmd_ != 0xFF)
      {
        munit.result_=pipeline_[curCycle_].result_;
        munit.complete_=1;
        munit.sameUnitReady_=1;
      }
    }
    else
    {
      munit.complete_=1;
      munit.result_=0xABCDEF89;
    }

    if(decEx.en_)
    {
      if(decEx.cmd_ == SLProcessor::CMD_MOV)
      {
        munit.result_=a.asUint;
      }
      else if(decEx.cmd_ == SLProcessor::CMD_NEG)
      {
        munit.result_=(a*-1).asUint;
      }
      else if(decEx.cmd_ == SLProcessor::CMD_CMP)
      {
        munit.result_=0xABCDEF89;
        munit.cmpEq_=a==b;
        munit.cmpLt_=a<b;
      }
    }

    return munit;
  }

  void update(const SLProcessor::_DecodeEx &decEx,const SLProcessor::_MUnit &comb)
  {
    _qfp32_t a,b;
    a.initFromRaw(decEx.a_);
    b.initFromRaw(decEx.b_);
	
    if(comb.complete_ && pendingOp_ == 0)
    {
      if(decEx.en_)
        activeOp_=decEx.cmd_;
      else
        activeOp_=0;//NOP mov to result
    }
    else if(activeOp_ != 0 && decEx.en_)
      pendingOp_=decEx.cmd_;
    
    _SumMode sumModeNext={0,1,1,0,0,0,0,0,0,0};
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
      sumModeNext.muxB_=0;
      sumModeNext.clearRegB_=1;
      break;
    }
    
    sumModeNext.muxB_=sumModeNext.muxA_;
    
    uint32_t mulRes=0;
    uint32_t intermediateEn=0;

    if(pipeline_[curCycle_].cmd_ == SLProcessor::CMD_MAC)
    {
      mulRes=pipeline_[curCycle_].result_;
      intermediateEn=1;

      pipeline_[curCycle_].cmd_=0xFF;
    }
    
    //update data regs
    if(sumModeNext.enRegA_)
      dataRegister_[0]=macPipeline_[curCycle_%2];
    
    if(sumModeNext.clearRegA_)
      dataRegister_[0]=0;
    
    if(sumModeNext.muxC_ == 0)
    {
      if(intermediateEn)
        dataRegister_[1]=mulRes;
      else
        dataRegister_[1]=0;
    }
      
    if(sumModeNext.muxC_ == 1 && sumModeNext.enRegB_)
      dataRegister_[1]=macPipeline_[curCycle_%2]; 

    if(sumModeNext.clearRegB_)
      dataRegister_[1]=0;
      

    _qfp32_t extA=a,extB=b;

    if(sumModeNext.muxA_ == 0)
    {
      extA=dataRegister_[0];
      extB=dataRegister_[1];
      macPipeline_[(curCycle_+2)%2].cmd_=SLProcessor::CMD_MAC;
      macPipeline_[(curCycle_+2)%2].result_=(extA+extB).asUint;
    }
    else
    {
      macPipeline_[(curCycle_+2)%2].cmd_=0xFF;
      extA=a.asUint;
      extB=b.asUint;
    }

    uint32_t sumModeStateNext_=(sumModeState_<<1)+(sumModeState_>>3);

    if(!((sumModeState_ == 1 && activeOp_ != SLProcessor::CMD_MAC) || (sumModeState_ == 2 && decEx.cmd_ != 99)))
      sumModeState_=sumModeStateNext_;
  
    if(decEx.en_)
    {
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
  uint32_t activeOp_;
  uint32_t pendingOp_;
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

  uint32_t incAD=0;
  uint32_t incAD2=0;
  //decode.incAD_=0;//bdata(4);
  //decode.incAD2_=0;//bdata(4);

  decode.cData_=bdata(11 downto 2);
  decode.offset_=bdata(10 downto 2);

  decode.CMD_=bdata(7 downto 5);

  decode.exCOND_=bdata(5 downto 4);

  decode.loadNumInstrs_=bdata(11 downto 10);

  decode.goto_=0;
  decode.loop_=0;
  decode.load_=0;
  decode.cmp_=0;
  decode.neg_=0;
  decode.wait_=0;
  decode.signal_=0;

  decode.cmpMode_=bdata(1 downto 0);
  decode.cmpNoXCy_=bdata(11);

  decode.loopEndDetect_=0;

  if(bdata(15) == 0)//OPIRS
  {
    decode.CMD_=bdata(13 downto 11);
    //decode.incAD_=bdata(14);
    incAD=bdata(14);
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
      //decode.incAD_=bdata(4);
      //decode.incAD2_=bdata(8);
      incAD=bdata(4);
      incAD2=bdata(8);
      break;

    case 6: //CMP
      decode.cmp_=1;
      decode.CMD_=1;//sub
      decode.enIRS_=1;
      decode.muxA_=1;
      decode.muxB_=0;
      //decode.incAD_=bdata(4);
      incAD=bdata(4);
      break;

    case 7:

      switch(bdata(11 downto 6))
      {
      case 0:
      case 1://MOV
        decode.enREG_=1;
        //decode.incAD_=bdata(4);
        incAD=bdata(4);
        break;
      case 2://MOVDATA
        decode.enMEM_=1;
        //decode.incAD_=bdata(4);
        incAD=bdata(4);
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

  decode.memEx_=!decode.enIRS_ && state_.addrNext_[decode.muxAD1_] >= SharedAddrBase_;

  decode.curPc_=code_.pc_;

  //pre calculate jmp target
  decode.jmpTargetPc_=code_.pc_+(decode.cData_&0x1FF)*((decode.cData_&0x200)?0:-1);

  //addr inc
  decode.incAD0_=incAD && (decode.muxAD1_ == 0 || incAD2);
  decode.incAD1_=incAD && (decode.muxAD1_ == 1 || incAD2);

  return decode;
}

SLProcessor::_MemFetch1 SLProcessor::memFetch1(const _Decode &decComb) const
{
  _MemFetch1 memFetch;

  if(decComb.memEx_)
    memFetch.externalData_=portExt_.read(state_.addrNext_[decComb.muxAD1_]);

  return memFetch;
}

SLProcessor::_MemFetch2 SLProcessor::memFetch2() const
{
  _MemFetch2 memFetch;

  uint32_t addr0=state_.addr_[decode_.muxAD0_];
  uint32_t addr1=state_.addr_[decode_.muxAD1_];

  if(decode_.enIRS_)
    addr1=state_.irs_+decode_.offset_;

  memFetch.writeAD_=addr1;

  memFetch.readData_[0]=portF0_.read(addr0);
  memFetch.readData_[1]=portF1_.read(addr1);

  return memFetch;
}

SLProcessor::_DecodeEx SLProcessor::decodeEx(const _Exec &execComb)
{
  _DecodeEx decodeEx;

  decodeEx.a_=execComb.munit_.result_;

  if(decode_.muxA_)//must be zero by default => otherwise result prefetch will NOT work
    decodeEx.a_=mem2_.readData_[0];

  decodeEx.b_=mem2_.readData_[1];

  if(decode_.muxB_)
      decodeEx.b_=state_.loopCount_;

  decodeEx.writeAddr_=mem2_.writeAD_;

  decodeEx.writeEn_=decode_.enMEM_;

  decodeEx.wbEn_=decode_.enREG_;
  decodeEx.wbReg_=decode_.wbREG_;

  return decodeEx;
}

SLProcessor::_Ctrl SLProcessor::ctrl(const _Decode &decodeComb,const _DecodeEx &decodeExComb,const _Exec &execComb)
{
  _Ctrl ctrl;

  uint32_t stallExtMemRead=0;
  if(decodeComb.memEx_ && !decodeComb.enMEM_)
  {
    if(ctrl_.dataWrites[decodeComb.muxAD1_]&1)//pending addr write
      stallExtMemRead=1;

    if(decEx_.writeExt_)// || !externalMemAvailable)//external mem not ready
      stallExtMemRead=1;

    //pending mem writes
    if((ctrl_.extWriteAdIncCtrl_[decodeComb.muxAD1_]&1) != 0 || ((ctrl_.extWriteAdIncCtrl_[decodeComb.muxAD1_]>>1)&1) != 0)
      stallExtMemRead=1;
  }

  ctrl.stallExternalAddrNotAvail_=ctrl_.dataWrites[decodeComb.muxAD1_]&1;

  ctrl.stallLoopNotAvail_=0;
  if(decodeComb.loopEndDetect_ && (ctrl_.loopWrites_&1))
    ctrl.stallLoopNotAvail_=1;

  //decEx stage stalls
  ctrl.stallResultNotAvail_=0;
  if(decode_.muxA_ == 1)//need result
  {
    if(execComb.munit_.complete_ == 0 && !state_.resultPrefetch_)
      ctrl.stallResultNotAvail_=1;
  }

  ctrl.stallUnitNotAvail_=0;
  if(decode_.CMD_ != 0 && execComb.munit_.idle_ == 0 && ((execComb.munit_.cmd_&0x06) != (decode_.CMD_&0x06) || !(execComb.munit_.sameUnitReady_)))
    ctrl.stallUnitNotAvail_=0;

  ctrl.stallMemAddrInWB_=0;
  if(decEx_.writeEn_ == 1)
  {
    //addrNext changes on falling edge
    if(decode_.muxA_ == 0 && decEx_.writeAddr_)
      ctrl.stallMemAddrInWB_=1;

    //port 2
    if(decode_.muxB_ ==  0 && mem2_.writeAD_ == decEx_.writeAddr_)
      ctrl.stallMemAddrInWB_=1;
  }

  //exec stage
  ctrl.stallExec_=decEx_.writeExt_;// && !externalMemAvailable;

  ctrl.stallDec_=stallExtMemRead;

  ctrl.stallDecEx_=ctrl.stallExternalAddrNotAvail_ || ctrl.stallLoopNotAvail_ || ctrl.stallResultNotAvail_ || ctrl.stallUnitNotAvail_ || ctrl.stallMemAddrInWB_;

  ctrl.flushPipeline_=0;
  if(decode_.goto_)
    ctrl.flushPipeline_=5;
  if(decode_.load_ && decode_.loadNumInstrs_ > 0)
    ctrl.flushPipeline_=decode_.loadNumInstrs_;

  ctrl.loopWrites_=ctrl_.loopWrites_>>1;
  ctrl.dataWrites[0]=ctrl_.dataWrites[0]>>1;
  ctrl.dataWrites[1]=ctrl_.dataWrites[1]>>1;

  if(decodeComb.enREG_)
  {
    if(decodeComb.wbREG_ == WBREG_LOOP)
      ctrl.loopWrites_|=7;
    if(decodeComb.wbREG_ == WBREG_DATA0)
      ctrl.dataWrites[0]|=7;
    if(decodeComb.wbREG_ == WBREG_DATA1)
      ctrl.dataWrites[1]|=7;
  }

  ctrl.extWriteAdIncCtrl_[0]=ctrl_.extWriteAdIncCtrl_[0]>>1;
  ctrl.extWriteAdIncCtrl_[1]=ctrl_.extWriteAdIncCtrl_[1]>>1;

  if(decodeComb.memEx_ && decodeComb.enMEM_)
  {
    //external data write pending
    if((decodeComb.incAD0_ == 0 && decodeComb.muxAD1_ == 0) || (decodeComb.incAD1_ == 0 && decodeComb.muxAD1_ == 1))
    {
      //store information that ADx is not incremented and future reads (with ADx) should blocked because maybe the same addr will be written
      ctrl.extWriteAdIncCtrl_[decodeComb.muxAD1_]|=2;//update only if not stalling
    }

    ctrl.extWriteAdIncCtrl_[decodeComb.muxAD1_^1]|=2;
  }

  return ctrl;
}
/*
 * exec=instr==cmp?translateCmpMode(eq,gt):1;
 * exec2=exec & cmpExecMode;
 * pendingNOPs=cmpExecMode?2:0;
 * decExExecEn = pendingNOPs > 0 || stall || exec;
 * decExecEn= pendingNOPs > 1 || stall || exec2;
 */

class PipelineCtrl
{
public:
  SLProcessor::_PipelineCtrl pipelineCtrl(uint32_t stallDec,uint32_t stallDecEx,uint32_t condExec,uint32_t flushPipeline)
  {
    stallDec|=stallDecEx;

    uint32_t s2En=(((stageEnable_>>0)&1) && !stallDecEx) && condExec;
    uint32_t s1En=(((stageEnable_>>1)&1) && !stallDec);
    uint32_t pcUpdate=!stallDec && !stallDecEx;

    if(pcUpdate)
      stageEnable_=(stageEnable_>>1)+0x10;
    else
      stageEnable_&=~stallDecEx;//disable s2 enable stallDec is enabled but not stallDecEx

    if(!condExec)//disable execute for additional next 2 cycles
      stageEnable_&=0x1C;

    if(flushPipeline > 0 && condExec)//dont allow conditionally executed load const
      stageEnable_&=~((1<<flushPipeline)-1);

    return {s1En,s2En,pcUpdate,stallDec,stallDecEx,condExec,flushPipeline};
  }

protected:
  uint32_t stageEnable_;
};


SLProcessor::_State SLProcessor::state(const _Ctrl ctrlComb,const _DecodeEx &decExComb)//,_Exec execComb)
{
  _State stateNext;

  //falling edge
  stateNext.addr_[0]=state_.addr_[0];
  stateNext.addr_[1]=state_.addr_[1];
  if(decode_.incAD0_ && ((state_.stageEnable_>>1)&1))//decEx en
    stateNext.addr_[0]=state_.addr_[0]+1;
  if(decode_.incAD1_ && ((state_.stageEnable_>>1)&1))//decEx en
    stateNext.addr_[1]=state_.addr_[1]+1;

  uint32_t loopActive=state_.loopCount_ > 0;

  if(decode_.loopEndDetect_ && loopActive)
  {
    if((state_.stageEnable_>>2)&1)//only if enabled
      stateNext.loopCount_=state_.loopCount_-1;
  }

  //rising edge

  uint32_t pcNext=state_.pc_+1;

  if(decEx_.goto_)
    pcNext=decode_.jmpTargetPc_;

  if(pcNext == state_.loopEndAddr_ && state_.loopValid_ && loopActive)
    pcNext=state_.loopStartAddr_;

  if(0)//external pc set
    pcNext=0;

  stateNext.pc_=pcNext;


  if(decode_.loopEndDetect_)
  {
    if(state_.loopValid_ == 0)
    {
      stateNext.loopStartAddr_=decode_.jmpTargetPc_;
      stateNext.loopEndAddr_=decode_.curPc_;
    }

    stateNext.loopValid_=1;
  }
  else if(decode_.enREG_ && decode_.wbREG_ == WBREG_LOOP)
    stateNext.loopValid_=0;

  //const loading
  stateNext.loadState_=state_.loadState_>>1;
  if(decode_.load_)
  {
    if(decode_.loadNumInstrs_ > 0)
      stateNext.loadState_=1<<decode_.loadNumInstrs_;
  }

  //register write (in EXEC stage)
  if(decEx_.wbEn_)
  {
    switch(decEx_.wbReg_)
    {
    case WBREG_LOOP:
      stateNext.loopCount_=decEx_.a_&0xFFFF;
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

SLProcessor::_Exec SLProcessor::execute(uint32_t extMemStall)  //after falling edge
{
  _Exec exec;

  if(!extMemStall)
  {
    if(decEx_.writeExt_)
      portExt_.write(decEx_.writeAddr_,decEx_.b_);

    if(decEx_.writeEn_ && !decEx_.writeExt_)
      portR2_.write(decEx_.writeAddr_,decEx_.b_);
  }

  _qfp32_t a;
  a.asUint=decEx_.b_;

  exec.intResult_=(int32_t)(a.abs());

  uint32_t result=0;
  uint32_t le=0;
  uint32_t eq=0;

  exec.condExec_=0;

  if(decEx_.cmp_)
  {
    switch(decEx_.cmpMode_)
    {
    case CMP_EQ:
      exec.condExec_=exec.munit_.cmpEq_; break;
    case CMP_NEQ:
      exec.condExec_=not exec.munit_.cmpEq_; break;
    case CMP_GT:
      exec.condExec_=not exec.munit_.cmpLt_; break;
    case CMP_LE:
      exec.condExec_=exec.munit_.cmpLt_ | exec.munit_.cmpEq_; break;
    }
  }

  return exec;
}

void SLProcessor::update(_ExternMemAccess &externMemAccess)
{
  //only after reset
  state_.addrNext_[0]=state_.addr_[0];
  state_.addrNext_[1]=state_.addr_[1];

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

  //special handling for operand a
  if(0)//decEx en
    state_.resultPrefetch_=0;

  if(decode_.load_)
  {
    decEx_.a_=((decode_.cData_>>9)&1)<<31 + (decode_.cData_&0x1FF)<<20;
    state_.resultPrefetch_=1;
  }
  else if(state_.loadState_ == 2)
    decEx_.a_+=((decode_.cDataExt_>>4)&0x3)<<29 + (decode_.cDataExt_&0xF)<<16 + decode_.cData_<<6;
  else if(state_.loadState_ == 1)
    decEx_.a_+=decode_.cDataExt_;

  if(!state_.resultPrefetch_)// && stall decEx && complete
  {
    state_.resultPrefetch_=1;
    decEx_.a_=0;//result
  }
}





#endif /* SLPROCESSOR_H_ */
