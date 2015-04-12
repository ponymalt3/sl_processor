#include "SLProcessor.h"
#include "SLArithUnit.h"
#include <cstring>
#include "SLCodeDef.h"

Mem::Mem(uint32_t size)
{
  size_=size;
  data_=new uint32_t[size];
}

uint32_t Mem::getSize() const { return size_; }


MemPort::MemPort(Mem &mem)
  : mem_(mem)
{
  pendingWrite_=0;
  wData_=0;
  wAddr_=0;
}

uint32_t MemPort::read(uint32_t addr) const
{
  assert(addr < mem_.size_);
  return mem_.data_[addr];
}

void MemPort::write(uint32_t addr,uint32_t data)
{
  assert(addr < mem_.size_);
  wAddr_=addr;
  wData_=data;
  pendingWrite_=1;
}

void MemPort::update()
{
  if(pendingWrite_)
  {
    mem_.data_[wAddr_]=wData_;
    pendingWrite_=0;
  }
}


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
  arithUnint_.reset();

  std::memset(&state_,0,sizeof(state_));
  std::memset(&code_,0,sizeof(code_));
  std::memset(&decode_,0,sizeof(decode_));
  std::memset(&mem1_,0,sizeof(mem1_));
  std::memset(&mem2_,0,sizeof(mem2_));
  std::memset(&decEx_,0,sizeof(decEx_));

  state_.en_=BitData(0x18);
}

SLProcessor::_CodeFetch SLProcessor::codeFetch()
{
  _CodeFetch fetch;
  fetch.data_=portCode_.read(state_.pc_);
  fetch.pc_=state_.pc_;

  return fetch;
}

SLProcessor::_Decode SLProcessor::decodeInstr(uint32_t extMemStall,uint32_t loopActive,uint32_t (&addrNext)[2]) const
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

      if(decode.wbREG_ == 2)//load loop register
        decode.loop_=1;

      break;
    case 1: //MOVIRS2
      decode.enIRS_=1;
      decode.enMEM_=1;
      break;

    case 3: //GOTO
      decode.goto_=1;
      decode.goto_const_=1;

      decode.loopEndDetect_=decode(0);
      break;

    case 4: //LOAD
      decode.load_=1;
      break;

    case 4: //OP
      decode.CMD_=bdata(7 downto 5);
      incAD=bdata(4);
      incAD2=bdata(8);
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
      case 1://MOVDATA
        decode.enREG_=1;
        incAD=bdata(4);
        break;
      case 2://MOVDATA
        decode.enMEM_=1;
        incAD=bdata(4);
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

  decode.memEx_=!decode.enIRS_ && addrNext[decode.muxAD1_] >= SharedAddrBase_;

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
  if(decode.memEx_ && decode.enMEM_)
  {
    if(state_.regBlocking_.dataX_[decode.muxAD1_]&1)//pending addr write
      stallExtMemRead=1;

    //external mem not ready
    if(extMemStall)//decEx_.writeExt_ should be included in extMemStall
      stallExtMemRead=1;

    //pending mem writes
    if((state_.regBlocking_.extAddr_[decode.muxAD1_]&1) != 0 || ((state_.regBlocking_.extAddr_[decode.muxAD1_]>>1)&1) != 0)
      stallExtMemRead=1;
  }

  //check loop not avail
  uint32_t stallLoopNotAvail_=0;
  if(decode.loopEndDetect_ && (state_.regBlocking_.loop_&1))
    stallLoopNotAvail_=1;

  decode.stall_=stallExtMemRead | stallLoopNotAvail_;

  return decode;
}

SLProcessor::_MemFetch1 SLProcessor::memFetch1(const _Decode &decComb,uint32_t (&addrNext)[2]) const
{
  _MemFetch1 memFetch;

  if(decComb.memEx_)
    memFetch.externalData_=portExt_.read(addrNext[decComb.muxAD1_]);

  return memFetch;
}

SLProcessor::_MemFetch2 SLProcessor::memFetch2() const
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
  decodeEx.writeExt_=decode_.memEx_;
  decodeEx.writeDataSel_=decode_.enMEM_ && !decode_.muxA_ && !decode_.muxB_;//if select result and not select loop reg

  decodeEx.wbEn_=decode_.enREG_;
  decodeEx.wbReg_=decode_.wbREG_;


  //check stall conditions
  uint32_t stallResultNotAvail_=0;
  if(decode_.muxA_ == 1)//need result
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
  if(decode_.load_ && decode_.loadWords_ > 0)
    flushPipeline_=decode_.loadWords_;

  decodeEx.stall_=stallResultNotAvail_ || stallUnitNotAvail_ || stallMemAddrInWB_;
  decodeEx.flushPipeline_=flushPipeline_;

  return decodeEx;
}

SLProcessor::_State::_RegBlocking SLProcessor::blockCtrl(const _Decode &decodeComb) const
{
  _State::_RegBlocking ctrl;

  ctrl.loop_=ctrl.loop_>>1;
  ctrl.dataX_[0]=ctrl.dataX_[0]>>1;
  ctrl.dataX_[1]=ctrl.dataX_[1]>>1;

  if(decodeComb.enREG_)
  {
    if(decodeComb.wbREG_ == SLCode::REG_LOOP)
      ctrl.loop_|=7;
    if(decodeComb.wbREG_ == SLCode::REG_AD0)
      ctrl.dataX_[0]|=7;
    if(decodeComb.wbREG_ == SLCode::REG_AD1)
      ctrl.dataX_[1]|=7;
  }

  ctrl.extAddr_[0]=ctrl.extAddr_[0]>>1;
  ctrl.extAddr_[1]=ctrl.extAddr_[1]>>1;

  if(decodeComb.memEx_)
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

SLProcessor::_StallCtrl SLProcessor::stallCtrl(uint32_t stallFetch,uint32_t stallDec,uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline)
{
  //mask with active stages
  stallExec&=state_.en_(_State::S_EXEC);
  stallDecEx&=state_.en_(_State::S_DECEX);
  stallDec&=state_.en_(_State::S_DEC);
  stallFetch&=state_.en_(_State::S_FETCH);

  //propagate stalls
  stallDecEx|=stallExec;
  stallDec|=stallDecEx;
  stallFetch|=stallDec;

  uint32_t enNext=state_.en_(4 downto 0);

  if(!stallFetch)
    enNext=(enNext>>1)+0x20;//shift in '1'
  else
  {
    //insert nops into pipeline if fetch stage stalls but others not

    if(!stallExec)
      enNext&=0x1E;//11110

    if(!stallDecEx)//disable instr if loop detect but loop count end
      enNext&=0x1D;//11101

    if(!stallDecEx)
      enNext&=0x1B;//11011

    //now all are blocked => for performance gain may insert only one nop a correct position
  }

  uint32_t enNextExec=enNext & (~((1<<flushPipeline)-1));//disable if pipeline is flushed

  uint32_t enNextNotExec=enNext & 0x18;//disable if it is not executed

  if(!condExec)//disable execute for additional next 2 cycles
    return {stallFetch,stallDec,stallDecEx,stallExec,enNextNotExec};
  else
    return {stallFetch,stallDec,stallDecEx,stallExec,enNextExec};
}

SLProcessor::_State SLProcessor::updateState(const _Decode &decComb,uint32_t stallDec,uint32_t loopActive,uint32_t setPcEnable,uint32_t pcValue) const
{
  _State stateNext;

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

  if(decComb.incAD0_ && !stallDec)
    stateNext.incAd0_=1;
  if(decComb.incAD1_ && !stallDec)
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
  stateNext.loadState_=state_.loadState_>>1;
  if(decComb.load_)
  {
    if(decComb.loadWords_ > 0)
      stateNext.loadState_=1<<decode_.loadWords_;
  }

  return stateNext;
}

SLProcessor::_Exec SLProcessor::execute(uint32_t extMemStall)  //after falling edge
{
  _Exec exec;

  exec.munit_=arithUnint_.comb();

  uint32_t data=decEx_.b_;

  if(decEx_.writeDataSel_ )
    data=decEx_.a_;

  if(!extMemStall)
  {
    if(state_.en_(_State::S_EXEC) && decEx_.writeExt_)
      portExt_.write(decEx_.writeAddr_,data);
  }

  if(state_.en_(_State::S_EXEC) && decEx_.writeEn_ && !decEx_.writeExt_)
    portR2_.write(decEx_.writeAddr_,data);

  _qfp32_t a;
  a.asUint=data;

  exec.intResult_=(int32_t)(a.abs());

  uint32_t result=0;
  uint32_t le=0;
  uint32_t eq=0;

  exec.condExec_=0;

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
  uint32_t loopActive=state_.loopCount_ > 0;

  uint32_t addrNext[2];//={state_.addr_[0]+state_.incAd0_,state_.addr_[0]+state_.incAd0_};
  addrNext[0]=state_.addr_[0]+state_.incAd0_;
  addrNext[0]=state_.addr_[1]+state_.incAd1_;

  //before falling edge
  _MemFetch2 mem2=memFetch2();

  portF0_.update();
  portF1_.update();

  //************************************ at falling edge *******************************************
  state_.addr_[0]=addrNext[0];
  state_.addr_[1]=addrNext[1];
  state_.loopCount_-=state_.decLoop_;
  addrNext[0]=state_.addr_[0]+state_.incAd0_;
  addrNext[0]=state_.addr_[1]+state_.incAd1_;

  //register write (in EXEC stage)
  if(decEx_.wbEn_ && state_.en_(_State::S_EXEC))
  {
    _qfp32_t a;
    a.asUint=decEx_.writeDataSel_?decEx_.a_:decEx_.b_;
    switch(decEx_.wbReg_)
    {
    case SLCode::REG_LOOP: state_.loopCount_=(uint32_t)(a.abs()); break;
    case SLCode::REG_AD0: state_.addr_[0]=(uint32_t)(a.abs()); break;
    case SLCode::REG_AD1: state_.addr_[1]=(uint32_t)(a.abs()); break;
    default:
    }
  }

  //************************************ after falling edge *******************************************
  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr(extMemStall || decEx_.writeExt_,loopActive,addrNext);
  _Exec execNext=execute(extMemStall);
  _DecodeEx decExNext=decodeEx(execNext);
  _MemFetch1 mem1=memFetch1(decodeNext,addrNext);

  //controls
  _StallCtrl stall=stallCtrl(0,decodeNext.stall_,decExNext.stall_,execNext.stall_,execNext.condExec_,decExNext.flushPipeline_);
  _State::_RegBlocking rblockNext=blockCtrl(decodeNext);
  _State stateNext=updateState(decodeNext,stall.stallDec_,loopActive,setPcEnable,pcValue);

  portR2_.update();
  portExt_.update();

  //************************************ update register *******************************************

  //update math unit
  arithUnint_.update(decExNext,execNext.munit_,state_.en_(_State::S_DECEX) && !stall.stallDecEx_);

  //decEx A/B handling
  if(decEx_.writeEn_ && decode_.enMEM_ && decEx_.writeAddr_ == mem2.writeAD_)
  {
    decExNext.b_=decEx_.b_;//keep operand b (forward mem data)
  }

  uint32_t constData=((decode_.cDataExt_&0x1C)<<10) + (decode_.cData_<<2) + (decode_.cDataExt_&0x03);
  if(decode_.load_)
  {
    decExNext.a_=((constData>>9)&1)<<31 + (constData&0x1FF)<<20;
    state_.resultPrefetch_=1;
  }
  else if(state_.loadState_ == 2)
    decExNext.a_=decEx_.a_ | (((constData>>4)&0x3)<<29 + (constData&0xF)<<16 + constData<<6);
  else if(state_.loadState_ == 1)
    decExNext.a_=decEx_.a_ | constData;

  if(!state_.resultPrefetch_ && stall.stallDecEx_ && execNext.munit_.complete_)
  {
    state_.resultPrefetch_=1;
    decExNext.a_=execNext.munit_.result_;
  }

  //state regBlocking
  if(!stall.stallDec_)
  {
    state_.regBlocking_=rblockNext;//not optimal... some later stages may proceed which is not reflected here
  }

  if(!stall.stallFetch_)
    code_=codeNext;

  if(!stall.stallDec_)
    decode_=decodeNext;

  if(!stall.stallDecEx_)
    decEx_=decExNext;

  //state assignment
  state_=stateNext;
}
