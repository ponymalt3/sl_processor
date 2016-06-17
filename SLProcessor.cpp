#include "SLProcessor.h"
#include "SLArithUnit.h"
#include <cstring>
#include "SLCodeDef.h"
#include "qfp32.h"

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
  , portR0_(localMem.createPort())
  , portR1_(localMem.createPort())
  , portF2_(localMem.createPort())
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
}

_CodeFetch SLProcessor::codeFetch()
{
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

  decode.cData_=bdata(11 downto 2);
  decode.cDataExt_=bdata(1 downto 0);

  decode.CMD_=SLCode::CMD_MOV;//;bdata(7 downto 5)

  decode.goto_=0;
  decode.goto_const_=0;
  decode.load_=0;
  decode.cmp_=0;
  decode.neg_=0;
  decode.wait_=0;
  decode.signal_=0;

  decode.cmpMode_=bdata(1 downto 0);
  decode.cmpNoXCy_=bdata(11);

  if(bdata(15) == 0)//OPIRS
  {
    decode.CMD_=bdata(14 downto 12);
    incAD=bdata(11);
    decode.enIRS_=1;
    decode.enADr0_=decode.muxA_==SLCode::MUX1_MEM;
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
      decode.goto_const_=1;
      break;

    case 3: //LOAD
      decode.load_=1;
      decode.muxA_=SLCode::MUX1_RESULT;
      break;

    case 4: //OP
      decode.CMD_=bdata(7 downto 5);
      incAD=bdata(4);
      incAD2=bdata(8);
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
        incAD=bdata(4);// & decode.muxA_;
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
  decode.jmpTargetPc_=code_.pc_+(decode.cData_&0x1FF)*((decode.cData_&0x200)?-1:1);
  
  
  incAD2=incAD2 || (!bdata(15) && incAD);

  //addr inc
  decode.incAD0_=(incAD && (decode.muxAD1_ == 0)) || (incAD2 && (decode.muxAD0_ == 0));
  decode.incAD1_=(incAD && (decode.muxAD1_ == 1)) || (incAD2 && (decode.muxAD0_ == 1)); 

  return decode;
}

_MemFetch1 SLProcessor::memFetch1() const
{
  _MemFetch1 memFetch;

  if(state_.addr_[1] >= SharedAddrBase_)
    memFetch.externalData_=portExt_.read(state_.addr_[1]);//decComb.muxAD1_]);

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
  memFetch.readData_[1]=portR1_.read(addr1%SharedAddrBase_);
  
  return memFetch;
}

_DecodeEx SLProcessor::decodeEx(const _Decode &decodeComb,const _Exec &execComb,const _MemFetch1 &mem1,const _MemFetch2 &mem2,uint32_t extMemStall)
{
  _DecodeEx decodeEx;

  decodeEx.result_=execComb.munit_.result_;
  
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
  
  decodeEx.neg_=decodeComb.neg_;
  
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

_State SLProcessor::updateState(const _Decode &decComb,uint32_t execNext,uint32_t setPcEnable,uint32_t pcValue) const
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
    pcNext=decode_.jmpTargetPc_;

  if(setPcEnable)//external pc set
    pcNext=pcValue;
    
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

_Exec SLProcessor::execute(uint32_t extMemStall)  //after falling edge
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
      portF2_.write(decEx_.writeAddr_,data);
    }
  }
  
  _qfp32_t a;
  a.asUint=decEx_.b_;//data;

  exec.intResult_=(int32_t)(a.abs());

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
  //comb inputs
  decEx_.a_=(decEx_.mux0_ == SLCode::MUX1_MEM)?decEx_.mem0_:state_.result_;
  decEx_.b_=(decEx_.writeExt_)?decEx_.memX_:decEx_.mem1_;
  
  //before falling edge
  _MemFetch1 mem1Next=memFetch1();

  _CodeFetch codeNext=codeFetch();
  _Decode decodeNext=decodeInstr();
  _Exec execNext=execute(extMemStall);
  
  //************************************ at falling edge **********************************************
  portF2_.update();
  
  //************************************ after falling edge *******************************************
  
  _MemFetch2 mem2=memFetch2(decodeNext);
  _DecodeEx decExNext=decodeEx(decodeNext,execNext,mem1Next,mem2,extMemStall);

  //controls
  _StallCtrl stall=control(decExNext.stall_,execNext.stall_,execNext.execNext_,execNext.flush_);
  _State stateNext=updateState(decodeNext,stall.stallDecEx_,setPcEnable,pcValue);

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
    _qfp32_t a;
    a.asUint=state_.result_;
    //a.asUint=decEx_.writeDataSel_?decEx_.a_:decEx_.b_;
    int32_t test=(int32_t)(a.abs());
    switch(decEx_.wbReg_)
    {
    case SLCode::REG_AD0: stateNext.addr_[0]=(int32_t)(a.abs()); break;
    case SLCode::REG_AD1: stateNext.addr_[1]=(int32_t)(a.abs()); break;
    }
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
    executedAddr_=decode_.curPc_;
    
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
