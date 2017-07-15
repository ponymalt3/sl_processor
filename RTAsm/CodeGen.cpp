/*
 * CodeGen.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "CodeGen.h"
#include "BuddyAlloc.h"
#include "Stream.h"
#include "Error.h"
#include "Operand.h"

template<typename _T>
void swap(_T &a,_T &b)
{
  _T t=a;
  a=b;
  b=t;
} 

CodeGen::Label::Label(CodeGen &codeGen):Error(codeGen.getErrorHandler()), codeGen_(codeGen)
{
  labelRef_=codeGen_.getLabelId();
  codeAddr_=codeGen_.getCurCodeAddr();
  labelAddr_=0;
}

CodeGen::Label::~Label()
{
  if(labelRef_ != NoRef)
    deleteLabel();
}

void CodeGen::Label::setLabel()
{
  Error::expect(labelRef_ != NoRef) << "invalid label" << ErrorHandler::FATAL;
  //codeGen_.updateLabel(*this);
  labelAddr_=codeGen_.getCurCodeAddr();
}

void CodeGen::Label::deleteLabel()
{
  codeGen_.patchAndReleaseLabelId(*this,codeAddr_);
  labelRef_=NoRef;
}

CodeGen::TmpStorage::TmpStorage(CodeGen &codeGen):codeGen_(codeGen)
{
  symbolRef_=NoRef;
  size_=0;
  blockBegin_=codeGen_.getCurCodeAddr();
}

_Operand CodeGen::TmpStorage::allocate()
{
  if(symbolRef_ == NoRef)
    symbolRef_=codeGen_.allocateTmpStorage();

  ++size_;
  codeGen_.changeStorageSize(*this,size_);

  return _Operand::createSymAccess(symbolRef_,size_-1);
}

_Operand CodeGen::TmpStorage::preloadConstValue(qfp32 value)
{
  //loads a constant into irs and move code to begin of block (block starts where TmpStorage is created)
  uint16_t codeBlockStart=codeGen_.getCurCodeAddr();
  
  _Operand op=allocate();
  codeGen_.instrMov(op,_Operand(value));

  uint32_t blockSize=codeGen_.getCurCodeAddr()-codeBlockStart;
  codeGen_.moveCodeBlock(codeBlockStart,blockSize,blockBegin_);
  blockBegin_+=blockSize;
  
  return op;
}

void CodeGen::TmpStorage::preloadCode(uint32_t codeAddr,uint32_t size)
{
  codeGen_.moveCodeBlock(codeAddr,size,blockBegin_);
  blockBegin_+=size;
}

_Operand CodeGen::TmpStorage::getArrayBaseOffset()
{
  return _Operand::createSymAccess(symbolRef_,0xFFFF);
}

CodeGen::_LoopFrame::_LoopFrame()
{
  labCont_=0;
  labBreak_=0;
  counter_=0;
}

CodeGen::_LoopFrame::_LoopFrame(const Label *labCont,const Label *labBreak,const _Operand *counter)
{
  labCont_=labCont;
  labBreak_=labBreak;
  counter_=counter;
}


void CodeGen::_Instr::patchIrsOffset(uint32_t irsOffset)
{
  //array index already addressed => add irsOffset
  code_=SLCode::IRS::patchOffset(code_,SLCode::IRS::getOffset(code_)+irsOffset);
}

void CodeGen::_Instr::patchConstant(uint32_t value,bool patch2ndWord)
{
  //maybe static error singleton again
  //Error::expect(value < 512) << "load addr out of range " << (value);

  if(patch2ndWord)
  {
    code_=SLCode::Load::create2(value);
    return;
  }

  code_=SLCode::Load::create1(value);
}

void CodeGen::_Instr::patchGotoTarget(int32_t target)
{
  code_=SLCode::Goto::create(target,false);
  //Error::expect(target < 512) << "jump target out of range " << (target);
}

CodeGen::CodeGen(Stream &stream):Error(stream.getErrorHandler()),stream_(stream),functions_(stream),defaultSymbols_(stream)
{
  usedRefs_=0;
  loopDepth_=0;
  codeAddr_=0;
  labelIdBitMap_=0x7FFFFFFF;
  
  symbolMaps_.push(defaultSymbols_);

  //allocate first symbol for temp loop storage
  symbolMaps_.top().createSymbolNoToken(MaxLoopDepth);
}

void CodeGen::instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage)
{
  _Operand a=resolveOperand(opa);
  _Operand b=resolveOperand(opb);
  
  //handle base addr
  if(a.isArrayBaseAddr())
  {
    instrMov(_Operand::createResult(),a);
    a=_Operand::createResult();
  }
  
  if(b.isArrayBaseAddr())
  {
    instrMov(_Operand::createResult(),b);
    b=_Operand::createResult();
  }  

  Error::expect(!(a.isResult() && b.isResult())) << stream_ << "invalid operands for instruction" << ErrorHandler::FATAL;
  
  //special handling when using constant data
  if(b.type_ == _Operand::TY_VALUE && (a.type_ == _Operand::TY_VALUE || a.type_ == _Operand::TY_RESULT))
  {
    b=tmpStorage.preloadConstValue(b.value_);
  }
  
  //if operands can swapped preload const
  if(a.type_ == _Operand::TY_VALUE && b.type_ == _Operand::TY_RESULT && op != '/' && op != '-')
  {
    a=tmpStorage.preloadConstValue(a.value_);
  }

  bool aIsResult=a.isResult() || a.type_ == _Operand::TY_VALUE;
  bool bIsResult=b.isResult() || b.type_ == _Operand::TY_VALUE;
  bool aIsIRS=a.type_ == _Operand::TY_RESOLVED_SYM;
  bool bIsIRS=b.type_ == _Operand::TY_RESOLVED_SYM;

  if(((!aIsResult && bIsResult) || (aIsIRS && !bIsIRS)) && op != '/' && op != '-')//div/sub
  {
    swap(a,b);
    swap(aIsResult,bIsResult);
    swap(aIsIRS,bIsIRS);
  }

  if(bIsResult)
  {
    //store into tmp
    _Operand tmp=tmpStorage.allocate();
    instrMov(tmp,b);
    b=tmp;
    bIsIRS=true;
  }

  if(!a.isResult() && a.type_ != _Operand::TY_MEM)
  {
    //loadOperandIntoResult(a);
    instrMov(_Operand::createResult(),a);
    a=_Operand::createResult();
  }
  
  bool aIncAddr=false;
  bool bIncAddr=false;
  
  if(a.type_ == _Operand::TY_MEM && b.type_ == _Operand::TY_MEM)
  {
    aIncAddr=a.addrInc_;
    bIncAddr=b.addrInc_;
  }
  else
  {
    aIncAddr=(a.type_ == _Operand::TY_MEM && a.addrInc_) || (b.type_ == _Operand::TY_MEM && b.addrInc_);
  }
    
  uint32_t symRef=SymbolMap::InvalidLink;
  uint32_t irsOffset=0;

  if(bIsIRS)
  {
    symRef=b.mapIndex_;
    irsOffset=b.arrayOffset_;
  }

  writeCode(SLCode::Op::create(translateOperand(a),translateOperand(b),translateOperation(op),irsOffset,aIncAddr,bIncAddr),symRef);
}

void CodeGen::instrMov(const _Operand &opa,const _Operand &opb)
{
  //optimize hint
  //try to implement mov AD0,AD1,... [IRS]
  
  if(opa.isResult() && opb.isResult())
  {
    return;
  }
  
  _Operand a=resolveOperand(opa,true);
  _Operand b=resolveOperand(opb);

  Error::expect(a.type_ != _Operand::TY_VALUE) << stream_ << "cant write constant data";
  Error::expect(b.type_ != _Operand::TY_IR_ADDR0 &&
                b.type_ != _Operand::TY_IR_ADDR1 &&
                b.type_ != _Operand::TY_IR_IRS) << stream_ << "cant read ad0/ad1/irs";

  bool aIsIRS=a.type_ == _Operand::TY_SYMBOL || a.type_ == _Operand::TY_RESOLVED_SYM;
  bool bIsIRS=b.type_ == _Operand::TY_SYMBOL || b.type_ == _Operand::TY_RESOLVED_SYM;
  bool aIsMem=a.type_ == _Operand::TY_MEM;
  bool bIsMem=b.type_ == _Operand::TY_MEM;

  //mov RESULT, [IRS]
  //mov DATAx, [IRS]
  //mov LOOP, [IRS]
  //mov [IRS], RESULT
  //mov [IRS], LOOP

  //mov RESULT, [DATAx]
  //mov DATAx, [DATAx]
  //mov LOOP [DATAx]
  //mov [DATAx], RESULT
  //mov [DATAx], LOOP

  if((aIsIRS && bIsIRS) || (aIsIRS && bIsMem) || (aIsMem && bIsIRS) || (aIsMem && bIsMem) || a.isInternalReg())
  {
    instrMov(_Operand::createResult(),b);
    b=_Operand::createResult();
    bIsMem=false;
    bIsIRS=false;
  }

  //load constant into result
  if(b.type_ == _Operand::TY_VALUE || b.isArrayBaseAddr())
  {
    //qfp32 value=b.value_;
    uint32_t constData=b.value_.toRealQfp32().toRaw();

    uint32_t symRef=b.ref_;

    if(b.isArrayBaseAddr())
    {
      symRef=b.mapIndex_;
      constData=qfp32_t(40).toRaw();
    }

    //write first part of
    writeCode(SLCode::Load::create1(constData),symRef);
    
    if(SLCode::Load::constDataValue2(constData) != 0 || SLCode::Load::constDataValue3(constData) != 0)
    {
      writeCode(SLCode::Load::create2(constData),symRef);
    }
    
    if(SLCode::Load::constDataValue3(constData) != 0)
    {
      writeCode(SLCode::Load::create3(constData),symRef);
    }
    
    if(b.isArrayBaseAddr())
    {
      //add [irs+0]
      TmpStorage tmp(*this);
      instrOperation(_Operand::createResult(),_Operand::createSymAccess(0),'+',tmp);
      
      Error::expect(getCurCodeAddr() > 0 && instrs_[getCurCodeAddr()-1].isIrsInstr()) << "internal error" << ErrorHandler::FATAL;
      
      instrs_[getCurCodeAddr()-1].patchIrsOffset(0);
      instrs_[getCurCodeAddr()-1].symRef_=NoRef;
    }
    
    if(a.isResult())
      return;

    b=_Operand::createResult();
  }

  bool addrInc=(aIsMem && a.addrInc_) || (bIsMem && b.addrInc_);
  
  uint32_t symRef=SymbolMap::InvalidLink;
  uint32_t irsOffset=0;

  if(aIsIRS || bIsIRS)
  {
    if(aIsIRS)
    {
      symRef=a.mapIndex_;
      irsOffset=a.arrayOffset_;
    }
    else
    {
      symRef=b.mapIndex_;
      irsOffset=b.arrayOffset_;
    }
  }
  
  //mov result to irs
  writeCode(SLCode::Mov::create(translateOperand(a),translateOperand(b),irsOffset,addrInc),symRef);
}

void CodeGen::instrNeg(const _Operand &opa)
{
  _Operand a=resolveOperand(opa);
  
  if(!a.isResult())
  {
    instrMov(_Operand::createResult(),a);
  }
  
  writeCode(SLCode::Neg::create());
}

void CodeGen::instrLoop(const _Operand &opa)
{
}

void CodeGen::instrBreak()
{
  if(loopDepth_ > 0)
    instrGoto(*(loopFrames_[loopDepth_-1].labBreak_));
  else
    Error::expect(false) <<"'break' used outside loop";
}

void CodeGen::instrContinue()
{
  if(loopDepth_ > 0)
    instrGoto(*(loopFrames_[loopDepth_-1].labCont_));
  else
    Error::expect(false) <<"'break' used outside loop";
}

void CodeGen::instrGoto(const Label &label)
{
  writeCode(SLCode::Goto::create(0,false),label.getLabelReference());
}

void CodeGen::instrGoto2()
{
  writeCode(SLCode::Goto::create());
}

void CodeGen::instrCompare(const _Operand &opa,const _Operand &opb,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage)
{
  _Operand a=resolveOperand(opa);
  _Operand b=resolveOperand(opb);
  
  if((cmpMode&CMP_MODE_SWAP_FLAG) ^ (negate?CMP_MODE_SWAP_FLAG:0))
  {
    _Operand t=a;
    a=b;
    b=t;
  }
  
  cmpMode&=~CMP_MODE_SWAP_FLAG;

  if(negate)
  {
    switch(cmpMode)
    {
      case CMP_MODE_EQ: cmpMode=CMP_MODE_NEQ; break;
      case CMP_MODE_NEQ: cmpMode=CMP_MODE_EQ; break;
      case CMP_MODE_LE: cmpMode=CMP_MODE_LT; break;
      case CMP_MODE_LT: cmpMode=CMP_MODE_LE; break; 
    }
  }

  if(b.type_ != _Operand::TY_RESOLVED_SYM || b.isArrayBaseAddr())
  {
    _Operand tmp=tmpStorage.allocate();
    instrMov(tmp,b);
    b=tmp;
  }

  if(!a.isResult())
  {
    instrMov(_Operand::createResult(),a);
  }  

  writeCode(SLCode::Cmp::create(b.arrayOffset_,(SLCode::CmpMode)cmpMode),b.mapIndex_);
}

void CodeGen::instrSignal(uint32_t target)
{
  writeCode(0);
}

void CodeGen::instrWait()
{
  writeCode(0);
}

void CodeGen::instrNop()
{
  writeCode(0xFFFF);
}

void CodeGen::addArrayDeclaration(const Stream::String &str,uint32_t size)
{
  Error::expect(size > 0) <<"array decl "<<str<<" must be greater than 0";
  symbolMaps_.top().createSymbol(str,size);
}

void CodeGen::addDefinition(const Stream::String &str,qfp32 value)
{
  symbolMaps_.top().createConst(str,value);
}

void CodeGen::addReference(const Stream::String &str,uint32_t irsOffset)
{
  symbolMaps_.top().createReference(str,irsOffset);
}

void CodeGen::createLoopFrame(const Label &contLabel,const Label &breakLabel,const _Operand &counter)
{
  Error::expect(loopDepth_ < MaxLoopDepth) << "FATAL loop frames run out of space" << ErrorHandler::FATAL;

  //_Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  //_Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);
  
  loopFrames_[loopDepth_]=_LoopFrame(&contLabel,&breakLabel,&counter);

  ++loopDepth_;
}

void CodeGen::removeLoopFrame()
{
  assert(loopDepth_ > 0);

  --loopDepth_;

  //_Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  //_Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

  //instrMov(loopReg,irsStorage);
}

void CodeGen::storageAllocationPass(uint32_t size,uint32_t numParams)
{
  BuddyAlloc allocator(0,size);
  
  uint32_t x=allocator.allocate(4+numParams);//reserve space for parameter

  Error::expect(codeAddr_ < 0xFFFF) << "too many instructions" << ErrorHandler::FATAL;

  for(uint32_t i=0;i<codeAddr_;++i)
  {
    if(instrs_[i].symRef_ != SymbolMap::InvalidLink && instrs_[i].symRef_ < 0xFFC0)
    {
      SymbolMap::_Symbol &symInf=symbolMaps_.top()[instrs_[i].symRef_];
      
      //mark as patched
      instrs_[i].symRef_=SymbolMap::InvalidLink;

      //alloc storage if not already
      if(!symInf.flagAllocated_)
      {
        symInf.flagAllocated_=1;
        symInf.allocatedAddr_=allocator.allocate(symInf.allocatedSize_);
      }
      
      if(instrs_[i].isIrsInstr())
      {
        instrs_[i].patchIrsOffset(symInf.allocatedAddr_);
      }
      
      if(instrs_[i].isLoadAddr())
      {
        Error::expect((symInf.allocatedAddr_&7) == 0 || symInf.allocatedAddr_ < 32) << "addr to be loaded must less than 32 or 8 word aligned";
        instrs_[i].patchConstant(qfp32_t(symInf.allocatedAddr_).toRaw(),false);
      }

      //release storage
      if(symInf.lastAccess_ == i && symInf.flagAllocated_ && !symInf.flagStayAllocated_)
      {
        symInf.flagAllocated_=0;
        allocator.release(symInf.allocatedAddr_,symInf.allocatedSize_);
      }
    }
  }
}

void CodeGen::pushSymbolMap(SymbolMap &currentSymbolMap)
{
  symbolMaps_.push(currentSymbolMap);
}

void CodeGen::popSymbolMap()
{
  symbolMaps_.pop();
}

_Operand CodeGen::resolveOperand(const _Operand &op,bool createSymIfNotExists)
{
  if(op.type_ == _Operand::TY_SYMBOL)
  {
    Stream::String name=stream_.createStringFromToken(op.offset_,op.length_);
    uint32_t symRef=symbolMaps_.top().findSymbol(name);

    if(createSymIfNotExists && symRef == SymbolMap::InvalidLink)
      symRef=symbolMaps_.top().createSymbol(name,0);//single element

    Error::expect(symRef != SymbolMap::InvalidLink) << "symbol " << name << " not found" << ErrorHandler::FATAL;

    SymbolMap::_Symbol &symInf=symbolMaps_.top()[symRef];

    if(symInf.flagConst_)//is const
      return _Operand(symInf.constValue_);

    if(symInf.flagAllocated_)//is a reference
      return _Operand::createSymAccess(symRef,0);
      
    if(symInf.flagIsArray_)//is a array
      return _Operand::createSymAccess(symRef,op.index_);
      
    //normal variable      
    return _Operand::createSymAccess(symRef,0);
  }

  if(op.type_ == _Operand::TY_INDEX)
  {
    Error::expect(loopDepth_ != 0 && op.loopIndex_ < loopDepth_) << "using loop index '" << char('i'+op.loopIndex_) << "' outside loop" << ErrorHandler::FATAL;

    uint32_t index=op.loopIndex_;
    
    return *(loopFrames_[index].counter_);

   // if(loop == 0)
   //   return _Operand::createInternalReg(_Operand::TY_IR_LOOP);

    //return _Operand::createSymAccess(LoopStorageIndex,loopDepth_-loop);
  }

  return op;
}

SLCode::Operand CodeGen::translateOperand(_Operand op)
{
  switch(op.type_)
  {
    case _Operand::TY_RESULT: return SLCode::Operand::REG_RES;
    case _Operand::TY_IR_ADDR0: return SLCode::Operand::REG_AD0;
    case _Operand::TY_IR_ADDR1: return SLCode::Operand::REG_AD1;
    case _Operand::TY_IR_IRS: return SLCode::Operand::REG_IRS;
    case _Operand::TY_RESOLVED_SYM: return SLCode::Operand::IRS;
    case _Operand::TY_MEM:
      if(op.regaIndex_ == _Operand::IR_ADR0)
      {
        return SLCode::Operand::DEREF_AD0;
      }
      else
      {
        return SLCode::Operand::DEREF_AD1;
      }
    case _Operand::TY_INDEX: return SLCode::Operand::IRS;
    default:
      Error::expect(false) << "internal error: no operand translation found!" << ErrorHandler::FATAL;
  }
  
  return SLCode::Operand::INVALID_OP;
}

SLCode::Command CodeGen::translateOperation(char op)
{
  switch(op)
  {
    case '+': return SLCode::Command::CMD_ADD;
    case '-': return SLCode::Command::CMD_SUB;
    case '*': return SLCode::Command::CMD_MUL;
    case '/': return SLCode::Command::CMD_DIV;
    default: 
      break;
  }
  return SLCode::Command::CMD_MOV;
}

uint32_t CodeGen::allocateTmpStorage()
{
  return symbolMaps_.top().createSymbolNoToken(1,true);
}

uint32_t CodeGen::getLabelId()
{
  Error::expect(labelIdBitMap_ != 0) << "no more label ids available" << ErrorHandler::FATAL;
  
  uint32_t freeBitPos=Stream::log2(labelIdBitMap_);
  
  if((1<<freeBitPos) > labelIdBitMap_)
  {
    --freeBitPos;
  }
  
  labelIdBitMap_&=~(1<<freeBitPos);

  return freeBitPos + 0xFFC0;
}

void CodeGen::patchAndReleaseLabelId(const Label &label,uint32_t patchAddrStart)
{
  uint32_t labelRef=label.getLabelReference();

  for(uint32_t i=patchAddrStart;i<getCurCodeAddr();++i)
  {
    //check if goto must be patched
    if(instrs_[i].symRef_ != labelRef)
    {
      continue;
    }
    
    if(instrs_[i].isGoto())
    {
      instrs_[i].patchGotoTarget(label.labelAddr_-i);
      instrs_[i].symRef_=NoRef;
    }
    
    if(instrs_[i].isLoadAddr())
    {
      //is absolue addr
      instrs_[i].patchConstant(qfp32_t(label.labelAddr_).toRaw(),i>0 && instrs_[i-1].isLoadAddr());
      instrs_[i].symRef_=65534;
    }
  }

  //release label id
  labelIdBitMap_|=1<<(labelRef&0x1F);
}

void CodeGen::changeStorageSize(const TmpStorage &storage,uint32_t size)
{
  symbolMaps_.top()[storage.getSymbolReference()].changeArraySize(size);
}

void CodeGen::writeCode(uint32_t code,uint32_t ref)
{
  Error::expect(codeAddr_ < sizeof(instrs_)/sizeof(instrs_[0])) << "code buffer full" << ErrorHandler::FATAL;

  instrs_[codeAddr_].code_=code;
  instrs_[codeAddr_].symRef_=ref;

  if(ref != NoRef && !instrs_[codeAddr_].isGoto() && !instrs_[codeAddr_].isLoadAddr())
    symbolMaps_.top()[ref].updateLastAccess(getCurCodeAddr());

  ++codeAddr_;
}

void CodeGen::moveCodeBlock(uint32_t startAddr,uint32_t size,uint32_t targetAddr)
{
  if(startAddr == targetAddr)
  {
    return;
  }
  
  Error::expect(size <= sizeof(instrs_)/sizeof(instrs_[0])-getCurCodeAddr()) << "not engough instr buffer space" << ErrorHandler::FATAL;  
  Error::expect(targetAddr < startAddr) << "code can only be moved backwards" << ErrorHandler::FATAL;
  
  //move code to tmp
  for(uint32_t i=0;i<size;++i)
  {
    instrs_[getCurCodeAddr()+i]=instrs_[startAddr+i];
  }
  
  uint32_t j=0;
  for(int32_t i=startAddr-1;i>=(int32_t)targetAddr;--i)
  {
    instrs_[i+size]=instrs_[i];
    Error::expect(instrs_[i].isGoto() == false) << "cant move goto instruction";
    Error::expect(instrs_[i].isLoadAddr() == false || instrs_[i].symRef_ != 65534) << "cant move load code addr instruction";
  }
  
  for(uint32_t i=0;i<size;)
  {
    instrs_[targetAddr+i]=instrs_[getCurCodeAddr()+i];
    Error::expect(instrs_[targetAddr+i].isGoto() == false) << "cant move goto instruction";
    
    if(instrs_[i].isLoadAddr() && instrs_[i].symRef_ == 65534)
    {
      bool load2=(i+1)<size && instrs_[i+1].isLoadAddr();
      bool load3=(i+2)<size && instrs_[i+2].isLoadAddr();
      
      qfp32_t v=_Instr::restoreValueFromLoad(instrs_[i],
                                             load2?instrs_[i+1]:_Instr({0,0}),
                                             load3?instrs_[i+2]:_Instr({0,0}));
      
      v=v-int32_t(startAddr-targetAddr);
      
      instrs_[i].patchConstant(v.toRaw(),false);
      
      if(load2)
      {
        ++i;
        instrs_[i].patchConstant(v.toRaw(),true);
      }
      
      if(load3)
      {
        ++i;
      }
    }
    
    ++i;
  }
}
