/*
 * SLArithUnit.cpp
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#include "SLArithUnit.h"
#include "SLCodeDef.h"
#include "qfp32.h"  

SLArithUnit::SLArithUnit()
{
  reset();
}

void SLArithUnit::reset()
{
  for(uint32_t i=0;i<32;++i)
    pipeline_[i].cmd_=0xFF;

  curCycle_=0;

  activeOp_=0;
  pendingOp_=0;
}

_MUnit SLArithUnit::comb(const _DecodeEx &decEx)
{
  _MUnit munit;

  munit.cmpEq_=0;
  munit.cmpLt_=0;
  munit.complete_=0;
  munit.sameUnitReady_=0;
  munit.idle_=activeOp_ == 0;
  munit.cmd_=activeOp_;
  munit.result_=0xABCDEF89;

  if(activeOp_ != 0 && pipeline_[curCycle_].cmd_ != 0xFF)
  {
    munit.result_=pipeline_[curCycle_].result_;
    munit.complete_=1;
    munit.sameUnitReady_=1;
  }
  else
  {
    if(decEx.cmd_ == SLCode::CMD_CMP)
    {
      _qfp32_t a,b;
      a.asUint=decEx.a_;
      b.asUint=decEx.b_;
      munit.cmpEq_=(a-b).asUint == 0;
      munit.cmpLt_=(a-b).asUint >= 0x80000000;//negative b > a
    }
    else
    {
      munit.result_=decEx.b_;
    }
  }

  return munit;
}

void SLArithUnit::update(const _DecodeEx &decEx,const _MUnit &comb,uint32_t en)
{
  _qfp32_t a,b;
  a.asUint=decEx.a_;
  b.asUint=decEx.b_;

  if(comb.complete_ && pendingOp_ == 0)
  {
    if(en)
      activeOp_=decEx.cmd_;
    else
      activeOp_=0;//NOP mov to result
  }
  else if(activeOp_ != 0 && en)
    pendingOp_=decEx.cmd_;

  _qfp32_t extA=a,extB=b;

  if(en)
  {
    switch(decEx.cmd_)
    {
    case SLCode::CMD_MOV:
      if(pendingOp_ == SLCode::CMD_CMP)
      {
        pendingOp_=0;
      }
      else
      {
        activeOp_=0;
      }
    //  pipeline_[(curCycle_+1)%32].result_=(b).asUint;
    //  pipeline_[(curCycle_+1)%32].cmd_=SLCode::CMD_MOV;
      break;
    case SLCode::CMD_ADD:
      pipeline_[(curCycle_+2)%32].result_=(extA+extB).asUint;
      pipeline_[(curCycle_+2)%32].cmd_=SLCode::CMD_ADD;
      break;
    case SLCode::CMD_SUB:
      pipeline_[(curCycle_+2)%32].result_=(extA-extB).asUint;
      pipeline_[(curCycle_+2)%32].cmd_=SLCode::CMD_SUB;
      break;
    case SLCode::CMD_MUL:
      pipeline_[(curCycle_+2)%32].result_=(a*b).asUint;
      pipeline_[(curCycle_+2)%32].cmd_=SLCode::CMD_MUL;
      break;
    case SLCode::CMD_DIV:
      pipeline_[(curCycle_+32)%32].result_=(a/b).asUint;
      pipeline_[(curCycle_+32)%32].cmd_=SLCode::CMD_DIV;
      break;
    //case SLCode::CMD_MAC:
      //pipeline_[(curCycle_+2)%32].result_=(a*b).asUint;
      //pipeline_[(curCycle_+2)%32].cmd_=SLCode::CMD_MUL;
      //break;
    case SLCode::CMD_CMP:
      if(pendingOp_ == SLCode::CMD_CMP)
      {
        pendingOp_=0;
      }
      else
      {
        activeOp_=0;
      }
      break;      
    default:
      ;
    }
  }

  curCycle_=(curCycle_+1)%32;
}
