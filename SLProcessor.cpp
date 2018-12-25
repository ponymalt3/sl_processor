#include "SLProcessor.h"
#include "SLArithUnit.h"
#include <cstring>
#include "SLCodeDef.h"

#include <iostream>

#include <iostream>

uint32_t simClockEnable(uint32_t oldValue,uint32_t newValue,uint32_t clkEnMask,uint32_t bitsUsed)
{
  uint32_t result=oldValue;
  for(uint32_t i=0;i<bitsUsed;++i)
  {
    uint32_t mask=1<<i;

    if(!(clkEnMask&mask))
      continue;

    result&=~mask;
    result|=newValue&mask;
  }

  return result;
}

Memory::Memory(uint32_t size)
{
  size_=size;
  data_=new uint32_t[size];
}

Memory::Port Memory::createPort()
{
  return Port(*this);
}

uint32_t Memory::getSize() const { return size_; }


Memory::Port::Port(Memory &mem)
  : memory_(mem)
{
  pendingWrite_=0;
  wData_=0;
  wAddr_=0;
}

Memory::Port::Port(const Memory::Port &cpy)
  : memory_(cpy.memory_)
{
  pendingWrite_=cpy.pendingWrite_;
  wData_=cpy.wData_;
  wAddr_=cpy.wData_;
}

uint32_t Memory::Port::read(uint32_t addr) const
{
  assert(addr < memory_.size_);
  return memory_.data_[addr];
}

void Memory::Port::write(uint32_t addr,uint32_t data)
{
  assert(addr < memory_.size_);
  wAddr_=addr;
  wData_=data;
  pendingWrite_=1;
}

void Memory::Port::update()
{
  if(pendingWrite_)
  {
    memory_.data_[wAddr_]=wData_;
    pendingWrite_=0;
  }
}


SLProcessor::SLProcessor(Memory &localMem,const Memory::Port &portExt,const Memory::Port &portCode)
  : portExt_(portExt)
  , portCode_(portCode)
  , enable_(0)
  , portF0_(localMem.createPort())
  , portF1_(localMem.createPort())
  , portR2_(localMem.createPort())
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
  arithUnint_.reset();

  std::memset(&state_,0,sizeof(state_));
  std::memset(&code_,0,sizeof(code_));
  std::memset(&decode_,0,sizeof(decode_));
  std::memset(&mem1_,0,sizeof(mem1_));
  std::memset(&mem2_,0,sizeof(mem2_));
  std::memset(&decEx_,0,sizeof(decEx_));

  state_.en_=BitData(0x38);
  state_.flushMask_=BitData(0x3F);
}

_CodeFetch SLProcessor::codeFetch()
{
  if(state_.pc_ == 16)
  {
    int a=0;
  }
  _CodeFetch fetch;
  fetch.data_=portCode_.read(state_.pc_);
  fetch.pc_=state_.pc_;

  return fetch;
}

_Decode SLProcessor::decodeInstr(uint32_t extMemStall,uint32_t loopActive,uint32_t (&addrNext)[2]) const
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
  
  uint32_t waitMemPort0_=0;

  decode.cData_=bdata(11 downto 2);
  decode.cDataExt_=(bdata(15 downto 12)<<2) + bdata(1 downto 0);

  decode.CMD_=SLCode::CMD_MOV;//;bdata(7 downto 5)

  decode.loadWords_=bdata(1 downto 0);

  decode.goto_=0;
  decode.goto_const_=0;
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
    decode.CMD_=bdata(14 downto 12);
    incAD=bdata(11);
    decode.enIRS_=1;
  }
  else
  {
    switch(bdata(14 downto 12))
    {
    case 0: //MOVIRS1
      decode.enIRS_=1;
      decode.wbREG_=bdata(1 downto 0);
      decode.enREG_=1;
      decode.muxA_=SLCode::MUX1_MEM;//no wait for result

      if(decode.wbREG_ == 2)//load loop register
        decode.loop_=1;

      break;
    case 1: //MOVIRS2
      decode.enIRS_=1;
      decode.enMEM_=1;
      break;

    case 2: //GOTO
      decode.goto_=1;
      decode.goto_const_=1;

      decode.loopEndDetect_=bdata(0);
      break;

    case 3: //LOAD
      decode.load_=1;
      decode.muxA_=SLCode::MUX1_MEM;
      break;

    case 4: //OP
      decode.CMD_=bdata(7 downto 5);
      incAD=bdata(4);
      incAD2=bdata(8);
      waitMemPort0_=decode.muxA_;
      break;

    case 5: //CMP
      decode.cmp_=1;
      decode.CMD_=SLCode::CMD_SUB;//sub
      decode.enIRS_=1;
      decode.muxA_=SLCode::MUX1_RESULT;
      decode.muxB_=SLCode::MUX2_MEM;
      break;

    case 7:
      switch(bdata(11 downto 6))
      {
      case 0:
      case 1://MOVDATA2(1)
        decode.enREG_=1;
        incAD=bdata(4);
        waitMemPort0_=decode.muxA_;
        break;
      case 2://MOVDATA2(2)
        decode.enMEM_=1;
        incAD=bdata(4);
        incAD2=bdata(4) & decode.muxA_;
        waitMemPort0_=decode.muxA_;
        break;

      case 3://SIG
        decode.wait_=bdata(10);
        decode.signal_=(~bdata(10))&1;
        break;

      case 4://NEG
        decode.neg_=1;
        break;
      }
    }
  }

  //maybe not necessary
  decode.muxB_=(decode.enIRS_)?SLCode::MUX2_MEM:decode.muxB_;

  decode.memEx_=!decode.enIRS_ && state_.addr_[decode.muxAD1_] >= SharedAddrBase_;

  decode.curPc_=code_.pc_;

  decode.irsAddr_=state_.irs_+bdata(10 downto 2);//irsOffset

  //pre calculate jmp target
  decode.jmpTargetPc_=code_.pc_+(decode.cData_&0x1FF)*((decode.cData_&0x200)?1:-1);

  //addr inc
  decode.incAD0_=incAD && (decode.muxAD1_ == 0 || incAD2);
  decode.incAD1_=incAD && (decode.muxAD1_ == 1 || incAD2);

  //disable jump if loop end is found and loop count is zero
  if(!loopActive && decode.loopEndDetect_)
    decode.goto_=0;

  //check stall conditions
  uint32_t stallExtMemRead=0;
  
  if(state_.regBlocking_.dataX_[decode.muxAD0_]&2)
    stallExtMemRead=waitMemPort0_;
      
  if(decode.enMEM_)
  {
    if(state_.regBlocking_.dataX_[decode.muxAD1_]&2)//pending addr write
      stallExtMemRead=1;

    if(decode.memEx_)
    {
        //external mem not ready
      if(extMemStall)//decEx_.writeExt_ should be included in extMemStall
        stallExtMemRead=1;

      //pending mem writes
      if((state_.regBlocking_.extAddr_[decode.muxAD1_]&1) != 0 || ((state_.regBlocking_.extAddr_[decode.muxAD1_]>>1)&1) != 0)
        stallExtMemRead=1;
        
      //addr is written right now
      if(state_.regBlocking_.dataX_[decode.muxAD1_]&1)
        stallExtMemRead=1;
    }
  }

  //check loop not avail
  uint32_t stallLoopNotAvail_=0;
  if(decode.loopEndDetect_ && (state_.regBlocking_.loop_&1))
    stallLoopNotAvail_=1;

  decode.stall_=stallExtMemRead | stallLoopNotAvail_;

  decode.flushPipeline_=(decode.load_)?decode.loadWords_:0;

  return decode;
}

_MemFetch1 SLProcessor::memFetch1(const _Decode &decComb,uint32_t (&addrNext)[2]) const
{
  _MemFetch1 memFetch;

  if(decComb.memEx_)
    memFetch.externalData_=portExt_.read(addrNext[decComb.muxAD1_]);

  return memFetch;
}

_MemFetch2 SLProcessor::memFetch2() const
{
  _MemFetch2 memFetch;

  uint32_t addr0=state_.addr_[decode_.muxAD0_];
  uint32_t addr1=state_.addr_[decode_.muxAD1_];

  if(decode_.enIRS_)
    addr1=decode_.irsAddr_;

  memFetch.writeAD_=addr1;

  memFetch.readData_[0]=portF0_.read(addr0);
  memFetch.readData_[1]=portF1_.read(addr1);

  return memFetch;
}

_DecodeEx SLProcessor::decodeEx(const _Exec &execComb,const _MemFetch1 &mem1)
{
  _DecodeEx decodeEx;

  decodeEx.a_=execComb.munit_.result_;

  if(decode_.muxA_ == SLCode::MUX1_MEM)//must be zero by default => otherwise result prefetch will NOT work
    decodeEx.a_=mem2_.readData_[0];

  decodeEx.b_=mem2_.readData_[1];

  if(decode_.memEx_)
    decodeEx.b_=mem1.externalData_;

  if(decode_.muxB_ == SLCode::MUX2_MEM)
    decodeEx.b_=state_.loopCount_;

  decodeEx.writeAddr_=mem2_.writeAD_;

  decodeEx.writeEn_=decode_.enMEM_;
  decodeEx.writeExt_=decode_.memEx_;
  decodeEx.writeDataSel_=!decode_.muxA_ && !decode_.muxB_;//if select result and not select loop reg
  decodeEx.writeDataSel_|=decode_.enMEM_ && decode_.muxA_;//if write mem and muxA selects also mem
  decodeEx.wbEn_=decode_.enREG_;
  decodeEx.wbReg_=decode_.wbREG_;


  //check stall conditions
  uint32_t stallResultNotAvail_=0;
  if(decode_.muxA_ == SLCode::MUX1_RESULT)//need result
  {
    if(execComb.munit_.complete_ == 0 && !state_.resultPrefetch_)
      stallResultNotAvail_=1;
  }

  uint32_t stallUnitNotAvail_=0;
  if(decode_.CMD_ != 0)
  {
    if(execComb.munit_.idle_ == 0 && ((execComb.munit_.cmd_&0x06) != (decode_.CMD_&0x06) || !(execComb.munit_.sameUnitReady_)))
      stallUnitNotAvail_=1;
  }

  uint32_t stallMemAddrInWB_=0;
  if(decEx_.writeEn_ == 1)
  {
    //check stalling only for port 1
    //port 2 will be forwarded automatically
    if(decode_.muxA_ == 0 && decEx_.writeEn_ && !decEx_.writeExt_)
      stallMemAddrInWB_=1;
  }

  //flush control
  uint32_t flushPipeline_=0;
  if(decode_.goto_)
    flushPipeline_=5;

  decodeEx.stall_=stallResultNotAvail_ || stallUnitNotAvail_ || stallMemAddrInWB_;
  decodeEx.flushPipeline_=flushPipeline_;

  return decodeEx;
}

_State::_RegBlocking SLProcessor::blockCtrl(const _Decode &decodeComb) const
{
  _State::_RegBlocking ctrl;
  ctrl=state_.regBlocking_;

  ctrl.loop_=ctrl.loop_>>1;
  ctrl.dataX_[0]=ctrl.dataX_[0]>>1;
  ctrl.dataX_[1]=ctrl.dataX_[1]>>1;

  if(decodeComb.enREG_ && enable_(_State::S_DEC))
  {
    if(decodeComb.wbREG_ == SLCode::REG_LOOP)
      ctrl.loop_|=7;
    if(decodeComb.wbREG_ == SLCode::REG_AD0)
      ctrl.dataX_[0]|=3;
    if(decodeComb.wbREG_ == SLCode::REG_AD1)
      ctrl.dataX_[1]|=3;
  }

  ctrl.extAddr_[0]=ctrl.extAddr_[0]>>1;
  ctrl.extAddr_[1]=ctrl.extAddr_[1]>>1;

  if(decodeComb.memEx_ && enable_(_State::S_DEC))
  {
    //external data write pending
    if((decodeComb.incAD0_ == 0 && decodeComb.muxAD1_ == 0) || (decodeComb.incAD1_ == 0 && decodeComb.muxAD1_ == 1))
    {
      //store information that ADx is not incremented and future reads (with ADx) should blocked because maybe the same addr will be written
      ctrl.extAddr_[decodeComb.muxAD1_]|=2;//update only if not stalling
    }

    ctrl.extAddr_[decodeComb.muxAD1_^1]|=2;
  }

  return ctrl;
}

_StallCtrl SLProcessor::stallCtrl(uint32_t stallFetch,uint32_t stallDec,uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline1,uint32_t flushPipeline2)
{
  //mask with active stages
  stallExec&=enable_(_State::S_EXEC);
  stallDecEx&=enable_(_State::S_DECEX);
  stallDec&=enable_(_State::S_DEC);
  stallFetch&=enable_(_State::S_FETCH);

  //propagate stalls
  stallDecEx|=stallExec;
  stallDec|=stallDecEx;
  stallFetch|=stallDec;

  uint32_t enNext=enable_(5 downto 0);

  enNext=(enNext>>1)+0x20;//shift in '1'
  //insert nops into pipeline if fetch stage stalls but others not

  if(stallDecEx)//disable instr if loop detect but loop count end
    enNext&=0x3E;//111110

  if(stallDec)
    enNext&=0x3D;//111101
    
  if(stallFetch)
    enNext&=0x3B;//111011
    
  uint32_t flushMask=0x3F;//(state_.flushMask_(5 downto 0)>>1) + 0x20;

  if(enable_(_State::S_DEC) && flushPipeline1)
  {
    uint32_t mask=~(((1<<flushPipeline1)-1)<<2);//start from dec stage
    flushMask&=mask;
  }

  uint32_t flushMaskExec=flushMask;
  if(enable_(_State::S_DECEX) && flushPipeline2)
    flushMaskExec&=~(((1<<flushPipeline2)-1)<<1);//disable if pipeline is flushed; signal is evaluated in decex stage

  uint32_t flushMaskNotExec=flushMask & 0x3E;//disable if it is not executed

  if(!condExec)//disable execute for additional next 2 cycles
    return {stallFetch,stallDec,stallDecEx,stallExec,enNext,flushMaskNotExec};
  else
    return {stallFetch,stallDec,stallDecEx,stallExec,enNext,flushMaskExec};
}

_State SLProcessor::updateState(const _Decode &decComb,uint32_t stallDec,uint32_t loopActive,uint32_t setPcEnable,uint32_t pcValue) const
{
  _State stateNext;

  stateNext.irs_=state_.irs_;
  stateNext.resultPrefetch_=state_.resultPrefetch_;

  //should be updated before eg falling edge
  stateNext.addr_[0]=state_.addr_[0];
  stateNext.addr_[1]=state_.addr_[1];
  stateNext.loopCount_=state_.loopCount_;

  //rising edge
  stateNext.incAd0_=0;
  stateNext.incAd1_=0;
  stateNext.decLoop_=0;
  stateNext.loopStartAddr_=state_.loopStartAddr_;
  stateNext.loopEndAddr_=state_.loopEndAddr_;
  stateNext.loopValid_=state_.loopValid_;

  if(decComb.incAD0_ && !state_.stallDec_1d_ && enable_(_State::S_DEC))
    stateNext.incAd0_=1;
  if(decComb.incAD1_ && !state_.stallDec_1d_ && enable_(_State::S_DEC))
    stateNext.incAd1_=1;

  if(decComb.loopEndDetect_ && loopActive)
  {
    if(!stallDec)//only if enabled
      stateNext.decLoop_=1;
  }

  uint32_t pcNext=state_.pc_+1;

  if(decEx_.goto_)//goto cannot stall!!
    pcNext=decode_.jmpTargetPc_;

  if(pcNext == state_.loopEndAddr_ && state_.loopValid_ && loopActive)
  {
    pcNext=state_.loopStartAddr_;

    if(!stallDec)//only if enabled
      stateNext.decLoop_=1;
  }

  if(setPcEnable)//external pc set
    pcNext=pcValue;

  stateNext.pc_=pcNext;

  //update load state
  stateNext.loadState_=state_.loadState_>>1;

  if(enable_(_State::S_DEC))
  {
    if(decComb.loopEndDetect_)
    {
      if(state_.loopValid_ == 0)
      {
        stateNext.loopStartAddr_=decComb.jmpTargetPc_;
        stateNext.loopEndAddr_=decComb.curPc_;
      }

      stateNext.loopValid_=1;
    }
    else if(decComb.enREG_ && decComb.wbREG_ == SLCode::REG_LOOP)
      stateNext.loopValid_=0;

    //const loading
    if(decComb.load_)
    {
      if(decComb.loadWords_ > 0)
        stateNext.loadState_=1<<(decComb.loadWords_);
    }
  }

  return stateNext;
}

_Exec SLProcessor::execute(uint32_t extMemStall)  //after falling edge
{
  _Exec exec;

  exec.munit_=arithUnint_.comb(decEx_);
  
  std::cout<<"arith result: "<<(exec.munit_.result_)<<"\n";
  
  //fragmented load
  if(enable_(_State::S_EXEC) && decEx_.load_)
  {
    std::cout<<"arith result load overwrite\n";
    switch(state_.loadState_)
    {
      case 1: exec.munit_.result_=(((decEx_.loadData_>>11)&1)<<31) + ((decEx_.loadData_&0x7FF)<<19); break;
      case 2: exec.munit_.result_=state_.result_ | ((((decEx_.loadData_>>11)&0x1)<<30) + ((decEx_.loadData_&0x7FF)<<8)); break;
      case 4: exec.munit_.result_=state_.result_ | ((decEx_.loadData_)&0xFF); break;
    }
    
    if(qfp32_t::initFromRaw(exec.munit_.result_) == qfp32_t(12.9))
    {
      int a=0;
    }
    
    exec.munit_.complete_=1;
  }
  
  if(enable_(_State::S_EXEC) && decEx_.neg_)
  {
    exec.munit_.result_=state_.result_^0x80000000;
    exec.munit_.complete_=1;
  }
  
  if(enable_(_State::S_EXEC) && decEx_.trunc_)
  {
    exec.munit_.result_=state_.result_&(~(0xFFFFFF>>(((state_.result_>>29)&0x3)*8)));
    if((exec.munit_.result_&0x7FFFFFFF) == 0)
    {
      exec.munit_.result_=0;
    }
    exec.munit_.complete_=1;
  }

  uint32_t data=decEx_.b_;

  if(decEx_.writeDataSel_ )
    data=decEx_.a_;

  if(!extMemStall)
  {
    if(decEx_.writeExt_)
    {
      std::cout<<"WX: "<<(decEx_.writeAddr_)<<" "<<(qfp32_t::initFromRaw(data))<<"\n";
      portExt_.write(decEx_.writeAddr_,data);
  }

  if(enable_(_State::S_EXEC) && decEx_.writeEn_ && !decEx_.writeExt_)
    portR2_.write(decEx_.writeAddr_,data);

  _qfp32_t a;
  a.asUint=decEx_.b_;//data;

  exec.intResult_=(int32_t)(a.abs());

  exec.condExec_=1;

  if(decEx_.cmp_)
  {
    switch(decEx_.cmpMode_)
    {
    case SLCode::CMP_EQ:
      exec.condExec_=exec.munit_.cmpEq_; break;
    case SLCode::CMP_NEQ:
      exec.condExec_=not exec.munit_.cmpEq_; break;
    case SLCode::CMP_GT:
      exec.condExec_=not exec.munit_.cmpLt_; break;
    case SLCode::CMP_LE:
      exec.condExec_=exec.munit_.cmpLt_ | exec.munit_.cmpEq_; break;
    }
  }

  //stall
  uint32_t stallExec=decEx_.writeExt_ && extMemStall;

  exec.stall_=stallExec;

  return exec;
}

void SLProcessor::update(uint32_t extMemStall,uint32_t setPcEnable,uint32_t pcValue)
{
  enable_=BitData(state_.en_(5 downto 0) & state_.flushMask_(5 downto 0));
  uint32_t loopActive=state_.loopCount_ > 0;

  uint32_t addrNext[2];//={state_.addr_[0]+state_.incAd0_,state_.addr_[0]+state_.incAd0_};
  addrNext[0]=state_.addr_[0]+state_.incAd0_;
  addrNext[1]=state_.addr_[1]+state_.incAd1_;

  //before falling edge
  _MemFetch2 mem2Next=memFetch2();

  portF0_.update();
  portF1_.update();

  //************************************ at falling edge *******************************************
  state_.addr_[0]=addrNext[0];
  state_.addr_[1]=addrNext[1];
  state_.loopCount_-=state_.decLoop_;
  addrNext[0]=state_.addr_[0]+state_.incAd0_;
  addrNext[1]=state_.addr_[1]+state_.incAd1_;

  portExt_.update();
  portR0_.update();
  portR1_.update();
  
  //************************************ at rising edge *******************************************
  if(enable_(_State::S_DECEX) && execNext.execNext_ && !stall.stallDecEx_ && !stall.flush_)
  {
    if(stateNext.incAd0_)
    {
      int b=0;
    }
    stateNext.addr_[0]=state_.addr_[0]+stateNext.incAd0_;
    stateNext.addr_[1]=state_.addr_[1]+stateNext.incAd1_;
  }

  //register write (in EXEC stage)
  if(decEx_.wbEn_ && enable_(_State::S_EXEC))
  {
    _qfp32_t a=_qfp32_t::initFromRaw(state_.result_);
    std::cout<<"*****************DEC: "<<(decEx_.cmd_)<<"  mux: "<<(decEx_.mux0_)<<"  wb: "<<(decEx_.wbReg_)<<"\n";
    //a.asUint=decEx_.writeDataSel_?decEx_.a_:decEx_.b_;
    int32_t test=_qfp32_t::initFromRaw(execNext.intResult_);//(int32_t)(a.abs());
    switch(decEx_.wbReg_)
    {
    case SLCode::REG_LOOP: state_.loopCount_=(int32_t)(a.abs()); break;
    case SLCode::REG_AD0: state_.addr_[0]=(int32_t)(a.abs()); break;
    case SLCode::REG_AD1: state_.addr_[1]=(int32_t)(a.abs()); break;
    }
  }

  mem2_=mem2Next;

  //************************************ after falling edge *******************************************
  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr(extMemStall || (enable_(_State::S_EXEC) && decEx_.writeExt_),loopActive,addrNext);
  _Exec execNext=execute(extMemStall);
  _MemFetch1 mem1=memFetch1(decodeNext,addrNext);
  _DecodeEx decExNext=decodeEx(execNext,mem1);

  //controls
  _StallCtrl stall=stallCtrl(0,decodeNext.stall_,decExNext.stall_,execNext.stall_,execNext.condExec_,decodeNext.flushPipeline_,decExNext.flushPipeline_);
  _State::_RegBlocking rblockNext=blockCtrl(decodeNext);
  _State stateNext=updateState(decodeNext,stall.stallDec_,loopActive,setPcEnable,pcValue);

  portR2_.update();
  portExt_.update();

  //************************************ update register *******************************************

  //update math unit
  arithUnint_.update(decExNext,execNext.munit_,enable_(_State::S_DECEX) && !stall.stallDecEx_);

  //decEx A/B handling
  if(enable_(_State::S_DECEX) && decode_.enMEM_ && decEx_.writeEn_ && decEx_.writeAddr_ == mem2_.writeAD_)
  {
    decExNext.b_=decEx_.b_;//keep operand b (forward mem data)
  }

  uint32_t constData=((decode_.cDataExt_&0x3C)<<10) + (decode_.cData_<<2) + (decode_.cDataExt_&0x03);
  if(enable_(_State::S_DECEX) && decode_.load_)
  {
    decExNext.a_=(((decode_.cData_>>9)&1)<<31) + ((decode_.cData_&0x1FF)<<20);
    stateNext.resultPrefetch_=1;//may be long path
  }
  else if(state_.loadState_ == 1)
    decExNext.a_=decEx_.a_ | ((((constData>>14)&0x3)<<29) + ((constData&0x3FFE)<<6));
  else if(state_.loadState_ == 2)
    decExNext.a_=decEx_.a_ | ((constData>>1)&0x7F);

  if(!state_.resultPrefetch_ && !stateNext.resultPrefetch_ && stall.stallDecEx_ && execNext.munit_.complete_)
  {
    stateNext.result_=execNext.munit_.result_;
    std::cout<<"result: "<<(qfp32_t::initFromRaw(stateNext.result_))<<"[0x"<<std::hex<<(stateNext.result_)<<std::dec<<"]\n";
    stateNext.resultPrefetch_=1;
    decExNext.a_=execNext.munit_.result_;
  }

  //still open problems:
  //reset resultPrefetch X
  //possible dec stage stall for const data (does not matter but performance issue)

  if(!stall.stallFetch_)
    code_=codeNext;
    
  uint32_t decodeLoadTmp=decode_.load_;
  if(!stall.stallDec_)
    decode_=decodeNext;

  if(!stall.stallDecEx_)
  {
    //retired instr
    if(enable_(_State::S_EXEC))
    {
      executedAddr_=decode_.curPc_;
      std::cout<<"exec addr... "<<(executedAddr_)<<"\n";
    }
    
    code_=codeNext;
    decode_=decodeNext;
    decEx_=decExNext;
    if(enable_(_State::S_DECEX) && decodeLoadTmp == 0)//reset only if unit has next result
      stateNext.resultPrefetch_=0;
  }

  //update regBlocking info; incorperate clk enable signals stall*
  uint32_t clkEnMask=~0x30 | ((stall.stallFetch_<<3) | (stall.stallDec_<<2) | (stall.stallDecEx_<<1) | stall.stallExec_);

  stateNext.regBlocking_.dataX_[0]=simClockEnable(state_.regBlocking_.dataX_[0],rblockNext.dataX_[0],clkEnMask,3);
  stateNext.regBlocking_.dataX_[1]=simClockEnable(state_.regBlocking_.dataX_[1],rblockNext.dataX_[1],clkEnMask,3);
  stateNext.regBlocking_.extAddr_[0]=simClockEnable(state_.regBlocking_.extAddr_[0],rblockNext.extAddr_[0],clkEnMask,2);
  stateNext.regBlocking_.extAddr_[1]=simClockEnable(state_.regBlocking_.extAddr_[1],rblockNext.extAddr_[1],clkEnMask,2);
  stateNext.regBlocking_.loop_=simClockEnable(state_.regBlocking_.loop_,rblockNext.loop_,clkEnMask,3);
  
  //state assignment
  if(stall.stallFetch_)
    stateNext.pc_=state_.pc_;
    
  stateNext.en_=simClockEnable(state_.en_,stall.enNext_,clkEnMask,6);
  stateNext.flushMask_=simClockEnable(state_.flushMask_,stall.flushMaskNext_,clkEnMask,6);
  stateNext.stallDec_1d_=stall.stallDec_;
  
  state_=stateNext;
  enable_=stall.enNext_;
}
