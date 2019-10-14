#include "SLProcessor.h"
#include "SLArithUnit.h"
#include <cstring>
#include "SLCodeDef.h"
#include "qfp32.h"

#include <iostream>

Memory::Memory(uint32_t size)
{
  size_=size;
  data_=new uint32_t[size];
  faultHandler_=[](uint32_t addr,uint32_t,bool write){
    throw FaultException(addr,write?FaultException::Write:FaultException::Read);
    return 0U;
  };
}

Memory::Port Memory::createPort()
{
  return Port(*this);
}

uint32_t Memory::getSize() const { return size_; }

void Memory::setInvalidRegion(uint32_t beg,uint32_t size)
{
  invalidRegions_.insert(std::make_pair(beg,size));
}

void Memory::setFaultHandler(const std::function<uint32_t(uint32_t,uint32_t,bool)> &faultHandler)
{
  faultHandler_=faultHandler;
}


Memory::Port::Port(Memory &mem)
  : memory_(mem)
{
  pendingWrite_=false;
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
  if(addr >= memory_.size_)
  {
    return memory_.faultHandler_(addr,0,false);    
  }
  
  return memory_.data_[addr];
}

void Memory::Port::write(uint32_t addr,uint32_t data)
{
  if(addr >= memory_.size_)
  {
    memory_.faultHandler_(addr,0,true);
    return;
  }
  
  wAddr_=addr;
  wData_=data;
  pendingWrite_=true;
}

void Memory::Port::update()
{
  if(pendingWrite_)
  {
    memory_.data_[wAddr_]=wData_;
    pendingWrite_=false;
  }
}

bool Memory::Port::checkAccess(uint32_t addr)
{
  auto it=memory_.invalidRegions_.upper_bound(addr);
  --it;
  
  if(it != memory_.invalidRegions_.end() && it->first >= addr && addr < (it->first+it->second))
  {
    return false;
  }
  
  return true;
}

SLProcessor::SLProcessor(Memory &localMem,const Memory::Port &portExt,const Memory::Port &portCode)
  : portExt_(portExt)
  , portCode_(portCode)
  , enable_(0)
  , portR0_(localMem.createPort())
  , portR1_(localMem.createPort())
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

uint32_t SLProcessor::getExecutedAddr()
{
  return executedAddr_;
}

void SLProcessor::reset()
{
  executedAddr_=0xFFFFFFFF;
  
  arithUnint_.reset();

  std::memset(&state_,0,sizeof(state_));
  std::memset(&code_,0,sizeof(code_));
  std::memset(&decode_,0,sizeof(decode_));
  //std::memset(&mem1_,0,sizeof(mem1_));
  //std::memset(&mem2_,0,sizeof(mem2_));
  std::memset(&decEx_,0,sizeof(decEx_));

  enable_=(1<<_State::S_FETCH);
  state_.loadState_=1;
  
  state_.loopCount_=0x00000000;
  
  cycleCount_=0;
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

_Decode SLProcessor::decodeInstr() const
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
  
  //need addr for memory read port X
  decode.enADr0_=0;
  decode.enADr1_=0;

  decode.wbREG_=bdata(6 downto 5);

  uint32_t incAD=0;
  uint32_t incAD2=0;
  
  uint32_t waitMemPort0_=0;

  decode.cData_=bdata(11 downto 2);
  decode.cDataExt_=bdata(1 downto 0);

  decode.CMD_=SLCode::CMD_MOV;//;bdata(7 downto 5)

  decode.goto_=0;
  decode.goto_const_=0;
  decode.load_=0;
  decode.cmp_=0;
  decode.neg_=0;
  decode.trunc_=0;
  decode.wait_=0;
  decode.signal_=0;
  decode.loop_=0;

  decode.cmpMode_=bdata(1 downto 0);
  decode.cmpNoXCy_=bdata(11);

  if(bdata(15) == 0)//OPIRS
  {
    decode.CMD_=bdata(14 downto 12);
    incAD2=bdata(11);
    decode.enIRS_=1;
    decode.enADr0_=decode.muxA_==SLCode::MUX1_MEM;
    if(decode.muxA_ == SLCode::MUX1_RESULT)
    {
      decode.CMD_+=bdata(0)<<3;
    }
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

      //if(decode.wbREG_ == SLCode::WBREG_UNUSED)//load loop register
      //  decode.loop_=1;

      break;
    case 1: //MOVIRS2
      decode.enIRS_=1;
      decode.enMEM_=1;
      break;

    case 2: //GOTO
      decode.goto_=1;
      decode.goto_const_=bdata(0);
      break;

    case 3: //LOAD
      decode.load_=1;
      decode.muxA_=SLCode::MUX1_RESULT;
      break;

    case 4: //OP
      decode.CMD_=bdata(8 downto 5);
      incAD=bdata(4);
      incAD2=bdata(9);
      decode.enADr0_=decode.muxA_==SLCode::MUX1_MEM;
      decode.enADr1_=1;
      break;

    case 5: //CMP
      decode.cmp_=1;
      decode.CMD_=SLCode::CMD_CMP;//sub
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
        decode.enADr1_=decode.wbREG_ == SLCode::WBREG_NONE;//result reg as dest
        break;
      case 2://MOVDATA2(2)
        decode.enMEM_=1;
        //incAD=bdata(4);
        decode.enADr0_=decode.muxA_==SLCode::MUX1_MEM;
        decode.enADr1_=1;
        incAD=bdata(4);// & decode.muxA_;
        incAD2=decode.muxA_==SLCode::MUX1_MEM && bdata(4);
        break;

      case 3://SIG
        decode.wait_=bdata(10);
        decode.signal_=(~bdata(10))&1;
        break;

      case 4://UNARY
        decode.neg_=bdata(0);
        decode.trunc_=bdata(2);
        decode.CMD_=8+bdata(5 downto 3);//cmd msb always set
        break;
        
      case 5://LOOP
        decode.loop_=1;
        break;
      default:
        decode.muxA_=SLCode::MUX1_RESULT;
      }
    }
  }

  //maybe not necessary
  decode.muxB_=(decode.enIRS_)?SLCode::MUX2_MEM:decode.muxB_;

  decode.memEx_=!decode.enIRS_ && state_.addr_[decode.muxAD1_] >= SharedAddrBase_;

  decode.curPc_=code_.pc_;

  decode.irsAddr_=state_.irs_+bdata(10 downto 2);//irsOffset

  //pre calculate jmp target
  decode.jmpBack_=(decode.cData_>>9)&1;
  decode.jmpTargetPc_=code_.pc_+(decode.cData_&0x1FF)+(((decode.cData_>>9)&1)*0xFE00);
  
  //addr inc
  decode.incAD0_=(incAD && (decode.muxAD1_ == 0)) || (incAD2 && (decode.muxAD0_ == 0));
  decode.incAD1_=(incAD && (decode.muxAD1_ == 1)) || (incAD2 && (decode.muxAD0_ == 1)); 

  return decode;
}

_MemFetch1 SLProcessor::memFetch1(const _Decode &decodeComb) const
{
  _MemFetch1 memFetch;

  //if(!decodeComb.enMEM_ && decodeComb.memEx_ && decodeComb.enADr1_)
  {
    //if(enable_(_State::S_EXEC) == 0 || decodeComb.enREG_ == 0 || decodeComb.muxAD1_ != decEx_.wbReg_)
    {
      if(state_.addr_[1] >= SharedAddrBase_)
        memFetch.externalData_=portExt_.read(state_.addr_[1]);//decComb.muxAD1_]);
    }
  }

  return memFetch;
}

_MemFetch2 SLProcessor::memFetch2(const _Decode &decComb) const
{
  _MemFetch2 memFetch;

  uint32_t addr0=state_.addr_[decComb.muxAD0_];
  uint32_t addr1=state_.addr_[decComb.muxAD1_];

  if(decComb.enIRS_)
    addr1=decComb.irsAddr_;

  memFetch.writeAD_=addr1;

  memFetch.readData_[0]=portR0_.read(addr0%SharedAddrBase_);
  if(decEx_.writeEn_ && enable_(_State::S_EXEC))
  {
    memFetch.readData_[0]=portR0_.read(decEx_.writeAddr_%SharedAddrBase_);
  }
  
  memFetch.readData_[1]=portR1_.read(addr1%SharedAddrBase_);
  
  return memFetch;
}

_DecodeEx SLProcessor::decodeEx(const _Decode &decodeComb,const _MemFetch1 &mem1,const _MemFetch2 &mem2,uint32_t extMemStall)
{
  _DecodeEx decodeEx;
  
  decodeEx.mux0_=decodeComb.muxA_;

  //if(decode_.muxA_ == SLCode::MUX1_MEM)//must be zero by default => otherwise result prefetch will NOT work
    decodeEx.mem0_=mem2.readData_[0];

  decodeEx.mem1_=mem2.readData_[1];

  //if(decode_.memEx_)
    decodeEx.memX_=mem1.externalData_;

  //if(decode_.muxB_ == SLCode::MUX2_MEM)
    //decodeEx.b_=state_.loopCount_;

  decodeEx.writeAddr_=mem2.writeAD_;

  decodeEx.writeEn_=decodeComb.enMEM_;
  decodeEx.writeExt_=decodeComb.memEx_;
  //decodeEx.writeDataSel_=!decode_.muxA_ && !decode_.muxB_;//if select result and not select loop reg
  //decodeEx.writeDataSel_|=decode_.enMEM_ && decode_.muxA_;//if write mem and muxA selects also mem
  decodeEx.wbEn_=decodeComb.enREG_;
  decodeEx.wbReg_=decodeComb.wbREG_;
  
  decodeEx.load_=decodeComb.load_;
  decodeEx.loadData_=(decodeComb.cData_<<2) + decodeComb.cDataExt_;
  
  decodeEx.cmd_=decodeComb.CMD_;
  decodeEx.cmp_=decodeComb.cmp_;
  decodeEx.cmpMode_=decodeComb.cmpMode_;
  
  decodeEx.goto_=decodeComb.goto_;
  
  if((state_.loopCount_&0x1FFFFFFE) == 0 && (state_.loopCount_&0x80000000 || (state_.loopCount_&1) == 1))
  {
    decodeEx.goto_=0;
  }

  decodeEx.neg_=decodeComb.neg_;
  decodeEx.trunc_=decodeComb.trunc_;
  
  decodeEx.stall_=0;

  //stall because of pending write to address reg
  if(enable_(_State::S_EXEC) && decEx_.wbEn_)
  {
    if(decodeComb.enADr0_ && decodeComb.muxAD0_ == decEx_.wbReg_)//read port 0
    {
      decodeEx.stall_=1;
    }
    
    //FIX EXTENDED STALLING: correct addr must always specified if external memory is addressed
    if(decodeComb.enADr1_ && (decodeComb.muxAD1_ == decEx_.wbReg_))
    {
      decodeEx.stall_=1;
    }
    
    if(decodeComb.enMEM_ && (decodeComb.muxAD1_ == decEx_.wbReg_))
    {
      decodeEx.stall_=1;
    }
    
    //stall if irs reg will be written
    if(decodeComb.enIRS_ && decEx_.wbReg_ == SLCode::WBREG_IRS)
    {
      decodeEx.stall_=1;
    }
  }
  
  //stall if shared read/write port is used
  if(decodeComb.muxA_ == SLCode::MUX1_MEM && decEx_.writeEn_ && enable_(_State::S_EXEC))
  {
    decodeEx.stall_=1;
  }
  
  //ext write in progress
  if(decodeComb.enADr1_ && decodeComb.memEx_ && ((decEx_.writeEn_ && decEx_.writeExt_ && enable_(_State::S_EXEC)) || extMemStall))
  {
    decodeEx.stall_=1;
  }
  
  return decodeEx;
}

_StallCtrl SLProcessor::control(uint32_t stallDecEx,uint32_t stallExec,uint32_t condExec,uint32_t flushPipeline) const
{
  //mask with active stages
  stallExec&=enable_(_State::S_EXEC);
  stallDecEx&=enable_(_State::S_DECEX);

  //propagate stalls
  stallDecEx|=stallExec;

  uint32_t enNext=enable_;
  
  if(!stallDecEx)
  {
    enNext=(enNext>>1)+(1<<_State::S_FETCH);//shift in 1
    
    if(!condExec && enable_(_State::S_EXEC))
    {
      enNext&=~(1<<_State::S_EXEC);//disable exec
    }
  }
  else if(!stallExec)
  {
    enNext&=~(1<<_State::S_EXEC);//disable exec
  }
  
  flushPipeline&=enable_(_State::S_EXEC);
  
  if(flushPipeline)
  {
    enNext=1<<_State::S_FETCH;
  }
  
  return {stallDecEx,stallExec,flushPipeline,enNext};
}

_State SLProcessor::updateState(const _Decode &decComb,const _Exec &execNext,uint32_t setPcEnable,uint32_t pcValue) const
{
  _State stateNext;
  
  stateNext.stallExec1d_=0;

  stateNext.irs_=state_.irs_;
  
  stateNext.addr_[0]=state_.addr_[0];
  stateNext.addr_[1]=state_.addr_[1];
  
  stateNext.incAd0_=0;
  stateNext.incAd1_=0;
  
  if(decComb.incAD0_ && enable_(_State::S_DEC))
    stateNext.incAd0_=1;
  if(decComb.incAD1_ && enable_(_State::S_DEC))
    stateNext.incAd1_=1;

  uint32_t pcNext=state_.pc_+1;

  if(decEx_.goto_ && enable_(_State::S_EXEC))//goto cannot stall!!
  {
    if(decode_.goto_const_)
    {
      pcNext=decode_.jmpTargetPc_;
    }
    else
    {
      pcNext=execNext.intResult_;// (int32_t)(_qfp32_t::initFromRaw(state_.result_).abs());
    }
  }
  
  stateNext.loopCount_=state_.loopCount_;
    
  if(decode_.goto_const_ == 1 && enable_(_State::S_EXEC) && decode_.jmpBack_ == 1)
  {
    if(state_.loopCount_ != 0)
    {
      stateNext.loopCount_=(state_.loopCount_&0x1FFFFFFF)-1;
    }
  }
    
  stateNext.result_=state_.result_;
  stateNext.resultPrefetch_=state_.resultPrefetch_;

  stateNext.pc_=pcNext;
  stateNext.loadState_=state_.loadState_;

  //update load state
  if(enable_(_State::S_EXEC))
  {
    //const loading
    if(decEx_.load_)
    {
      stateNext.loadState_<<=1;
    }
    else
    {
      stateNext.loadState_=1;
    }      
  }

  return stateNext;
}

_Exec SLProcessor::execute(uint32_t extMemStall,const _Decode &decComb)  //after falling edge
{
  _Exec exec;

  exec.munit_=arithUnint_.comb(decEx_);
  
  //fragmented load
  if(enable_(_State::S_EXEC) && decEx_.load_)
  {
    switch(state_.loadState_)
    {
      case 1: exec.munit_.result_=(((decEx_.loadData_>>11)&1)<<31) + ((decEx_.loadData_&0x7FF)<<19); break;
      case 2: exec.munit_.result_=state_.result_ | ((((decEx_.loadData_>>11)&0x1)<<30) + ((decEx_.loadData_&0x7FF)<<8)); break;
      case 4: exec.munit_.result_=state_.result_ | ((decEx_.loadData_)&0xFF); break;
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

  uint32_t data=state_.result_;//decEx_.b_;

  //if(decEx_.writeDataSel_ )
   // data=decEx_.a_;
  
  //write mem
  if(enable_(_State::S_EXEC) && decEx_.writeEn_)
  {
    if(decEx_.writeExt_)
    {
      portExt_.write(decEx_.writeAddr_,data);
    }
    else
    {
      portR0_.write(decEx_.writeAddr_,data);
    }
  }
  
  exec.intResult_=(int32_t)(_qfp32_t::initFromRaw(decEx_.a_).abs());

  exec.execNext_=1;

  if(enable_(_State::S_EXEC) && decEx_.cmp_)
  {
    switch(decEx_.cmpMode_)
    {
    case SLCode::CMP_EQ:
      exec.execNext_=exec.munit_.cmpEq_; break;
    case SLCode::CMP_NEQ:
      exec.execNext_=not exec.munit_.cmpEq_; break;
    case SLCode::CMP_LT:
      exec.execNext_=exec.munit_.cmpLt_; break;
    case SLCode::CMP_LE:
      exec.execNext_=exec.munit_.cmpLt_ | exec.munit_.cmpEq_; break;
    }
  }
  
  exec.stall_=0;
  
  //stall ctrl  
  if(!exec.munit_.complete_ && !exec.munit_.idle_)
  {
    exec.stall_=1;
  }
  
  if(decEx_.writeEn_ && decEx_.writeExt_)
  {
    exec.stall_=extMemStall;
  }

  exec.flush_=decEx_.goto_;

  return exec;
}

void SLProcessor::update(uint32_t extMemStall,uint32_t setPcEnable,uint32_t pcValue)
{
  ++cycleCount_;
  
  //comb inputs
  decEx_.a_=(decEx_.mux0_ == SLCode::MUX1_MEM)?decEx_.mem0_:state_.result_;
  decEx_.b_=(decEx_.writeExt_)?decEx_.memX_:decEx_.mem1_;

  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr();
  _Exec execNext=execute(extMemStall,decodeNext);
  
    //before falling edge
  _MemFetch1 mem1Next=memFetch1(decodeNext);
  
  //************************************ at falling edge **********************************************
  
  //************************************ after falling edge *******************************************
  
  _MemFetch2 mem2=memFetch2(decodeNext);
  _DecodeEx decExNext=decodeEx(decodeNext,mem1Next,mem2,extMemStall);

  //controls
  _StallCtrl stall=control(decExNext.stall_,execNext.stall_,execNext.execNext_,execNext.flush_);
  _State stateNext=updateState(decodeNext,execNext,setPcEnable,pcValue);

  portExt_.update();
  portR0_.update();
  portR1_.update();
  
  //************************************ at rising edge *******************************************
  if(enable_(_State::S_DECEX) && execNext.execNext_ && !stall.stallDecEx_ && !stall.flush_)
  {
    stateNext.addr_[0]=state_.addr_[0]+stateNext.incAd0_;
    stateNext.addr_[1]=state_.addr_[1]+stateNext.incAd1_;
  }

  //register write (in EXEC stage)
  if(enable_(_State::S_EXEC) && decEx_.wbEn_)
  {
    _qfp32_t a=_qfp32_t::initFromRaw(state_.result_);
    switch(decEx_.wbReg_)
    {
    case SLCode::REG_AD0: stateNext.addr_[0]=execNext.intResult_; break;
    case SLCode::REG_AD1: stateNext.addr_[1]=execNext.intResult_; break;
    case SLCode::REG_IRS: stateNext.irs_=execNext.intResult_; break;
    }
  }
  
  if(enable_(_State::S_EXEC) && decode_.loop_ == 1)
  {
    stateNext.loopCount_=execNext.intResult_;
    stateNext.loopCount_|=0x80000000;
  }

  //************************************ update register *******************************************
  if(false)
  {
    stateNext.resultPrefetch_=0;
  }
  
  if(execNext.munit_.complete_ || (enable_(_State::S_EXEC) && decEx_.wbEn_ && decEx_.wbReg_ == SLCode::WBREG_NONE))
  {
    stateNext.result_=execNext.munit_.result_;
    stateNext.resultPrefetch_=1;
  }

  //update math unit
  arithUnint_.update(decEx_,execNext.munit_,enable_(_State::S_EXEC) && !state_.stallExec1d_);// && !stall.stallExec_);

  if(!stall.stallDecEx_)
  {
    //retired instr
    if(enable_(_State::S_EXEC))
    {
      executedAddr_=decode_.curPc_;
    }
    
    code_=codeNext;
    decode_=decodeNext;
    decEx_=decExNext;
  }
  else
  {
    stateNext.pc_=state_.pc_;
  }
  
  stateNext.stallExec1d_=stall.stallExec_;
  
  state_=stateNext;
  enable_=stall.enNext_;
}
