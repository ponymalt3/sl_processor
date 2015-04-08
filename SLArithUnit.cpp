/*
 * SLArithUnit.cpp
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#include "SLArithUnit.h"

SLArithUnit::SLArithUnit()
{
}

SLProcessor::_MUnit SLArithUnit::comb(const SLProcessor::_DecodeEx &decEx)
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

void SLArithUnit::update(const SLProcessor::_DecodeEx &decEx,const SLProcessor::_MUnit &comb)
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
