/*
 * SLArithUnit.cpp
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#include "SLArithUnit.h"

SLArithUnit::SLArithUnit()
{
  reset();
}

void SLArithUnit::reset()
{
  for(uint32_t i=0;i<32;++i)
    pipeline_[i].cmd_=0xFF;

  macPipeline_[0].cmd_=0xFF;
  macPipeline_[1].cmd_=0xFF;

  curCycle_=0;
  dataRegister_={0,0};

  sumModeState_=1;
  activeOp_=0;
  pendingOp_=0;
}

SLProcessor::_MUnit SLArithUnit::comb()
{
  SLProcessor::_MUnit munit;

  munit.cmpEq_=0;
  munit.cmpLt_=0;
  munit.complete_=0;
  munit.sameUnitReady_=0;
  munit.idle_=activeOp_ != 0;
  munit.cmd_=activeOp_;
  munit.result_=0xABCDEF89;

  if(activeOp_ == SLCode::CMD_MAC)
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

      if(pipeline_[curCycle_].cmd_ == SLCode::CMD_SUB)
      {
        munit.cmpEq_=pipeline_[curCycle_].result_ == 0;
        munit.cmpLt_=pipeline_[curCycle_].result_ >= 0x80000000;
      }
    }
  }

  return munit;
}

void SLArithUnit::update(const SLProcessor::_DecodeEx &decEx,const SLProcessor::_MUnit &comb,uint32_t en)
{
  _qfp32_t a,b;
  a.initFromRaw(decEx.a_);
  b.initFromRaw(decEx.b_);

  if(comb.complete_ && pendingOp_ == 0)
  {
    if(en)
      activeOp_=decEx.cmd_;
    else
      activeOp_=0;//NOP mov to result
  }
  else if(activeOp_ != 0 && en)
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

  if(pipeline_[curCycle_].cmd_ == SLCode::CMD_MAC)
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
    macPipeline_[(curCycle_+2)%2].cmd_=SLCode::CMD_MAC;
    macPipeline_[(curCycle_+2)%2].result_=(extA+extB).asUint;
  }
  else
  {
    macPipeline_[(curCycle_+2)%2].cmd_=0xFF;
    extA=a.asUint;
    extB=b.asUint;
  }

  uint32_t sumModeStateNext_=(sumModeState_<<1)+(sumModeState_>>3);

  if(!((sumModeState_ == 1 && activeOp_ != SLCode::CMD_MAC) || (sumModeState_ == 2 && decEx.cmd_ != SLCode::CMD_MAC_RES)))
    sumModeState_=sumModeStateNext_;

  if(en)
  {
    switch(decEx.cmd_)
    {
    case SLCode::CMD_MOV:
      pipeline_[(curCycle_+1)%32]={(extB).asUint,SLCode::CMD_MOV}; break;
    case SLCode::CMD_ADD:
      pipeline_[(curCycle_+2)%32]={(extA+extB).asUint,SLCode::CMD_ADD}; break;
    case SLCode::CMD_SUB:
      pipeline_[(curCycle_+2)%32]={(extA-extB).asUint,SLCode::CMD_SUB}; break;
    case SLCode::CMD_MUL:
      pipeline_[(curCycle_+2)%32]={(a*b).asUint,SLCode::CMD_MUL}; break;
    case SLCode::CMD_DIV:
      pipeline_[(curCycle_+32)%32]={(a/b).asUint,SLCode::CMD_DIV}; break;
    case SLCode::CMD_MAC:
      pipeline_[(curCycle_+2)%32]={(a*b).asUint,SLCode::CMD_MUL}; break;//why +1 before
    default:
    }
  }

  curCycle_=(curCycle_+1)%32;
}
