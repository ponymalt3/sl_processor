/*
 * SLArithUnit.cpp
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#include "SLArithUnit.h"
#include "SLCodeDef.h"
#include "qfp32.h"  

#include <exception>

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
  
  newOpAdded_=false;
}

void SLArithUnit::addOperation(const _DecodeEx &decEx)
{
  _qfp32_t a,b;
  a.asUint=decEx.a_;
  b.asUint=decEx.b_;
  
  newOpAdded_=false;
  
  if(activeOp_ == 0)
  {
    activeOp_=decEx.cmd_;
    newOpAdded_=true;
  }
  /*else if(pendingOp_ == 0)
  {
    pendingOp_=decEx.cmd_;
    newOpAdded_=true;
  }
  else
  {
    throw new std::exception();
  }*/

  _qfp32_t extA=a,extB=b;

  switch(decEx.cmd_)
  {
  case SLCode::CMD_MOV:
    pipeline_[(curCycle_+0)%32].result_=(b).asUint;
    pipeline_[(curCycle_+0)%32].cmd_=SLCode::CMD_MOV;
    break;
  case SLCode::CMD_CMP:
    pipeline_[(curCycle_+0)%32].result_=(extA-extB).asUint;
    pipeline_[(curCycle_+0)%32].cmd_=SLCode::CMD_CMP;
    break;      
  default:
    ;
  }
}

_MUnit SLArithUnit::comb(const _DecodeEx &decEx)
{
  addOperation(decEx);
  
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
    
    activeOp_=0;//pendingOp_;
    pendingOp_=0;
    
    if(pipeline_[curCycle_].cmd_ == SLCode::CMD_CMP)
    {
      munit.cmpEq_=munit.result_ == 0;
      munit.cmpLt_=munit.result_ >= 0x80000000;//negative b > a
      munit.result_=0xAD48272F;
    }
    
    if(pipeline_[curCycle_].cmd_ == SLCode::CMD_MOV)
    {
      munit.complete_=0;
    }
  }

  return munit;
}

void SLArithUnit::update(const _DecodeEx &decEx,const _MUnit &comb,uint32_t en)
{
    _qfp32_t a,b;
  a.asUint=decEx.a_;
  b.asUint=decEx.b_;
  
  _qfp32_t extA=a,extB=b;
  
  if(en)
  {
    switch(decEx.cmd_)
    {
    case SLCode::CMD_ADD:
      pipeline_[(curCycle_+1)%32].result_=(extA+extB).asUint;
      pipeline_[(curCycle_+1)%32].cmd_=SLCode::CMD_ADD;
      break;
    case SLCode::CMD_SUB:
      pipeline_[(curCycle_+1)%32].result_=(extA-extB).asUint;
      pipeline_[(curCycle_+1)%32].cmd_=SLCode::CMD_SUB;
      break;
    case SLCode::CMD_MUL:
      pipeline_[(curCycle_+1)%32].result_=(a*b).asUint;
      pipeline_[(curCycle_+1)%32].cmd_=SLCode::CMD_MUL;
      break;
    case SLCode::CMD_DIV:
      pipeline_[(curCycle_+30)%32].result_=(a/b).asUint;
      pipeline_[(curCycle_+30)%32].cmd_=SLCode::CMD_DIV;
      break;
    //case SLCode::CMD_MAC:
      //pipeline_[(curCycle_+2)%32].result_=(a*b).asUint;
      //pipeline_[(curCycle_+2)%32].cmd_=SLCode::CMD_MUL;
      //break;  
    default:
      ;
    }
  }
  else if(newOpAdded_)
  {
    /*if(pendingOp_ == decEx.cmd_)
    {
      pendingOp_=0;
    }
    else*/
    {
      activeOp_=0;
    }
  }

  curCycle_=(curCycle_+1)%32;
}
