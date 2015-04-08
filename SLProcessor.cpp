#include "SLProcessor.h"
#include "SLArithUnit.h"

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
  decodeEx.writeExt_=decode_.memEx_;

  decodeEx.wbEn_=decode_.enREG_;
  decodeEx.wbReg_=decode_.wbREG_;

  return decodeEx;
}

SLProcessor::_Ctrl SLProcessor::ctrl(const _Decode &decodeComb,const _DecodeEx &decodeExComb,const _Exec &execComb,uint32_t extMemStall)
{
  _Ctrl ctrl;

  uint32_t stallExtMemRead=0;
  if(decodeComb.memEx_ && decodeComb.enMEM_)
  {
    if(ctrl_.dataWrites[decodeComb.muxAD1_]&1)//pending addr write
      stallExtMemRead=1;

    if(decEx_.writeExt_ || extMemStall)//external mem not ready
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
  ctrl.stallExec_=decEx_.writeExt_ && extMemStall;

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

SLProcessor::_StallCtrl SLProcessor::stallCtrl(uint32_t stallFetch,uint32_t stallDec,uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline)
{
  stallDecEx|=stallExec;
  stallDec|=stallDecEx;
  stallFetch|=stallDec;

  uint32_t s3En=(((state_.stageEnable_>>0)&1) && !stallExec);
  uint32_t s2En=(((state_.stageEnable_>>1)&1) && !stallDecEx) && condExec;
  uint32_t s1En=(((state_.stageEnable_>>2)&1) && !stallDec);
  uint32_t s0En=(((state_.stageEnable_>>3)&1) && !stallFetch);
  uint32_t pcUpdate=!stallFetch && !stallDec && !stallDecEx && !stallExec;

  uint32_t stageEnNext_=state_.stageEnable_;

  if(pcUpdate)
    stageEnNext_=(state_.stageEnable_>>1)+0x10;
  else
  {
    //insert nops into pipeline if dec stage stall but decEx stage not for example
    if(!stallExec)
      stageEnNext_&=0x1E;

    if(!stallDecEx)//disable instr if loop detect but loop count end
      stageEnNext_&=0x1D;
  }

  if(!condExec)//disable execute for additional next 2 cycles
    stageEnNext_&=0x1C;

  if(flushPipeline > 0 && condExec)//dont allow conditionally executed load const
    stageEnNext_&=~((1<<flushPipeline)-1);

  return {pcUpdate,s1En,s2En,s3En,stageEnNext_};
}

SLProcessor::_State SLProcessor::state(const _Ctrl &ctrlComb,const _DecodeEx &decExComb,uint32_t loopActive)
{
  _State stateNext;

  //rising edge

  if(decode_.incAD0_ && ((state_.stageEnable_>>2)&1))//dec en
    stateNext.incAd0_=1;
  if(decode_.incAD1_ && ((state_.stageEnable_>>2)&1))//dec en
    stateNext.incAd1_=1;

  if(decode_.loopEndDetect_ && loopActive)
  {
    if((state_.stageEnable_>>3)&1)//only if enabled
      stateNext.decLoop_=1;
  }

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
      stateNext.loopCount_=decEx_.a_;
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

  exec.munit_=arithUnint_.comb(decEx_);

  if(!extMemStall)
  {
    if(decEx_.writeExt_)
      portExt_.write(decEx_.writeAddr_,decEx_.b_);
  }

  if(decEx_.writeEn_ && !decEx_.writeExt_)
    portR2_.write(decEx_.writeAddr_,decEx_.b_);

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

void SLProcessor::update(uint32_t extMemStall,uint32_t setPcEnable,uint32_t pcValue)
{
  uint32_t loopActive=state_.loopCount_ > 0;

  //before falling edge
  _MemFetch2 mem2=memFetch2();

  portF0_.update();
  portF1_.update();

  //update state falling edge
  uint32_t addrNext[2]={state_.addr_[0]+state_.incAd0_,state_.addr_[0]+state_.incAd0_};
  state_.addr_[0]=addrNext[0];
  state_.addr_[1]=addrNext[0];
  state_.loopCount_-=state_.decLoop_;

  //after falling edge
  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr();
  _Exec execNext=execute(extMemStall);
  _DecodeEx decExNext=decodeEx(execNext);
  _MemFetch1 mem1=memFetch1(decodeNext,addrNext);

  _Ctrl ctrlNext=ctrl(decodeNext,decExNext,execNext);
  _State stateNext=state(ctrlNext,decExNext,loopActive);

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
