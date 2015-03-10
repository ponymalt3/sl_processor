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

CodeGen::Label::Label(CodeGen &codeGen):codeGen_(codeGen)
{
  labelRef_=codeGen_.createLabel();
  codeAddr_=codeGen_.getCurCodeAddr();
}

CodeGen::Label::~Label()
{
  if(labelRef_ != NoRef)
    deleteLabel();
}

void CodeGen::Label::setLabel()
{
  Error::expect(labelRef_ != NoRef) << "invalid label" << Error::FATAL;
  codeGen_.updateLabel(*this);
}

void CodeGen::Label::deleteLabel()
{
  codeGen_.patchAndRemoveLabel(*this,codeAddr_);
  labelRef_=NoRef;
}

CodeGen::TmpStorage::TmpStorage(CodeGen &codeGen):codeGen_(codeGen)
{
  symbolRef_=NoRef;
  size_=0;
}

_Operand CodeGen::TmpStorage::allocate()
{
  if(symbolRef_ == NoRef)
    symbolRef_=codeGen_.allocateTmpStorage();

  ++size_;
  codeGen_.changeStorageSize(*this,size_);

  return _Operand::createSymAccess(symbolRef_,size_-1);
}

CodeGen::_LoopFrame::_LoopFrame()
{
  labCont_=0;
  labBreak_=0;
}

CodeGen::_LoopFrame::_LoopFrame(const Label *labCont,const Label *labBreak)
{
  labCont_=labCont;
  labBreak_=labBreak;
}


void CodeGen::_Instr::patchIrsOffset(uint32_t irsOffset)
{
  uint32_t irsValue=code_&0x01FF;
  code_=(code_&0xFE00)+((irsValue+irsOffset)&0x01FF);
}

void CodeGen::_Instr::patchConstant(uint32_t value,bool patch2ndWord)
{
  Error::expect(value < 512) << "load addr out of range " << (value);

  if(patch2ndWord)
    return;

  code_=(code_&0xFC00)+value;
}

void CodeGen::_Instr::patchGotoTarget(int32_t target)
{
  bool backward=target<0;

  target=backward?-target:target;
  Error::expect(target < 512) << "jump target out of range " << (target);

  code_=(code_&0xFC00)+(target&0x01FF);
  code_+=0x0200*backward;
}

CodeGen::CodeGen(Stream &stream): symbols_(stream),stream_(stream)
{
  usedRefs_=0;
  loopDepth_=0;
  codeAddr_=0;
  lastFreeLabelPos_=0;

  //allocate first symbol for temp loop storage
  symbols_.createSymbolNoToken(MaxLoopDepth);
}

void CodeGen::instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage)
{
  _Operand a=resolveOperand(opa);
  _Operand b=resolveOperand(opb);

  assert(!(a.isResult() && b.isResult()));
  Error::expect(!(a.isResult() && b.isResult())) << stream_ << "invalid operands for instruction" << Error::FATAL;

  bool aIsResult=a.isResult() || a.type_ == _Operand::TY_VALUE || a.isInternalReg();
  bool bIsResult=b.isResult() || b.type_ == _Operand::TY_VALUE || b.isInternalReg();
  bool aIsResMem=a.type_ == _Operand::TY_RESOLVED_SYM;
  bool bIsResMem=b.type_ == _Operand::TY_RESOLVED_SYM;

  if(((!aIsResult && bIsResult) || (aIsResMem && !bIsResMem)) && op != '/' && op != '-')//div/sub
  {
    _Operand t=a;
    bool t2=aIsResult;
    a=b;
    b=t;
    aIsResult=bIsResult;
    bIsResult=t2;
  }

  if(bIsResult)
  {
    //store into tmp
    _Operand tmp=tmpStorage.allocate();
    instrMov(tmp,b);
    b=tmp;
  }

  if(!a.isResult() && a.type_ != _Operand::TY_MEM)
  {
    loadOperandIntoResult(a);
  }

  writeCode(0,getOperandSymbolRef(b));
}

void CodeGen::instrMov(const _Operand &opa,const _Operand &opb)
{
  _Operand a=resolveOperand(opa,true);
  _Operand b=resolveOperand(opb);

  Error::expect(a.type_ != _Operand::TY_VALUE) << stream_ << "cant write constant data";
  Error::expect(b.type_ != _Operand::TY_IR_ADDR0 && b.type_ != _Operand::TY_IR_ADDR1) << stream_ << "cant read ad0/ad1";

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

  if((aIsIRS && bIsIRS) || (aIsIRS && bIsMem) || (aIsMem && bIsIRS) || (aIsMem && bIsMem))
  {
    instrMov(_Operand::createResult(),b);
    b=_Operand::createResult();
    bIsMem=false;
    bIsIRS=false;
  }

  //load constant into result
  if(b.type_ == _Operand::TY_VALUE || b.isArrayBaseAddr())
  {
    qfp32 value=b.value_;
    uint32_t constData=(value.sign_<<9)+(value.mant_>>20);

    uint32_t symRef=NoRef;

    if(b.isArrayBaseAddr())
      symRef=b.mapIndex_;

    if(value.exp_ == 0 && (value.mant_&0xFFFFF) == 0)
    {
      writeCode(0+constData,symRef);
    }
    else if((value.mant_&0x0003F) == 0)
    {
      writeCode(0+(1<<10)+constData,symRef);
      writeCode((value.exp_<<12)+((value.mant_>>8)&0xFFF));
    }
    else
    {
      writeCode(0+(2<<10)+constData,symRef);
      writeCode((value.exp_<<12)+((value.mant_>>8)&0xFFF));
      writeCode(value.mant_&0xFF);
    }

    if(a.isResult())
      return;

    b=_Operand::createResult();
  }

  bool addrInc=(aIsMem && a.addrInc_) || (bIsMem && b.addrInc_);

  if(aIsIRS || bIsIRS)
  {
    uint32_t symRef=SymbolMap::InvalidLink;

    if(aIsIRS)
      symRef=a.mapIndex_;
    else
      symRef=b.mapIndex_;

    writeCode(0,symRef);
  }
  else
  {
    writeCode(0);
  }
}

void CodeGen::instrNeg(const _Operand &opa)
{
  _Operand a=resolveOperand(opa);

  instrMov(_Operand::createResult(),a);
  writeCode(0);
}

void CodeGen::instrLoop(const _Operand &opa)
{
  _Operand a=resolveOperand(opa);

  if(!a.isResult())
    loadOperandIntoResult(a);

  writeCode(0);
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
  writeCode(0,label.getLabelReference());
}

void CodeGen::instrCompare(_Operand a,_Operand b,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage)
{
  if(cmpMode&CMP_MODE_SWAP_FLAG)
  {
    _Operand t=a;
    a=b;
    b=t;
    cmpMode&=~CMP_MODE_SWAP_FLAG;
  }

  if(negate)
  {
    cmpMode=(~cmpMode)&3;//supports 4 modes
  }

  if(b.type_ != _Operand::TY_RESOLVED_SYM)
  {
    _Operand tmp=tmpStorage.allocate();
    instrMov(tmp,b);
    b=tmp;
  }

  if(!a.isResult())
    instrMov(_Operand::createResult(),a);

  uint32_t code=0+((cmpMode&3)<<10)+(negate<<9);

  writeCode(code,getOperandSymbolRef(b));
}

void CodeGen::instrSignal(uint32_t target)
{
  writeCode(0);
}

void CodeGen::instrWait()
{
  writeCode(0);
}

void CodeGen::addArrayDeclaration(const Stream::String &str,uint32_t size)
{
  Error::expect(size > 0) <<"array decl "<<str<<" must be greater than 0";
  symbols_.createSymbol(str,size);
}

void CodeGen::addDefinition(const Stream::String &str,qfp32 value)
{
  symbols_.createConst(str,value);
}

void CodeGen::addReference(const Stream::String &str,uint32_t irsOffset)
{
  symbols_.createReference(str,irsOffset);
}

void CodeGen::createLoopFrame(const Label &contLabel,const Label &breakLabel)
{
  Error::expect(loopDepth_ < MaxLoopDepth) << "FATAL loop frames run out of space" << Error::FATAL;

  _Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  _Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

  instrMov(irsStorage,loopReg);

  loopFrames_[loopDepth_]=_LoopFrame(&contLabel,&breakLabel);

  ++loopDepth_;
}

void CodeGen::removeLoopFrame()
{
  assert(loopDepth_ > 0);

  --loopDepth_;

  _Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  _Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

  instrMov(loopReg,irsStorage);
}

void CodeGen::loadOperandIntoResult(const _Operand &op)
{
  instrMov(_Operand::createResult(),op);
}

uint32_t CodeGen::getOperandSymbolRef(const _Operand &a)
{
  if(a.type_ == _Operand::TY_RESOLVED_SYM)
  {
    symbols_[a.mapIndex_].updateLastAccess(getCurCodeAddr());
    return a.mapIndex_;
  }

  return SymbolMap::InvalidLink;
}

_Operand CodeGen::resolveOperand(const _Operand &op,bool createSymIfNotExists)
{
  if(op.type_ == _Operand::TY_SYMBOL)
  {
    Stream::String name=stream_.createStringFromToken(op.offset_,op.length_);
    uint32_t symRef=symbols_.findSymbol(name);

    if(createSymIfNotExists && symRef == SymbolMap::InvalidLink)
      symRef=symbols_.createSymbol(name,op.index_);

    Error::expect(symRef != SymbolMap::InvalidLink) << "symbol " << name << " not found" << Error::FATAL;

    SymbolMap::_Symbol &symInf=symbols_[symRef];

    if(symInf.flagConst_)//is const
      return _Operand(symInf.constValue_);

    if(symInf.flagAllocated_)//is a reference
      return _Operand::createSymAccess(symInf.allocatedAddr_,0);

    return _Operand::createSymAccess(symRef,0);
  }

  if(op.type_ == _Operand::TY_INDEX)
  {
    Error::expect(loopDepth_ != 0 && op.loopIndex_ < loopDepth_) << "using loop index '" << ('i'+op.loopIndex_) << " outside loop" << Error::FATAL;

    uint32_t loop=loopDepth_-op.loopIndex_-1;

    if(loop == 0)
      return _Operand::createInternalReg(_Operand::TY_IR_LOOP);

    return _Operand::createSymAccess(LoopStorageIndex,loopDepth_-loop);
  }

  return op;
}

uint32_t CodeGen::allocateTmpStorage()
{
  return symbols_.createSymbolNoToken(1);
}

uint32_t CodeGen::createLabel()
{
  const uint32_t maxNumLabels=sizeof(labels_)/sizeof(labels_[0]);

  Error::expect(lastFreeLabelPos_ < maxNumLabels) << "label buffer too small" << Error::FATAL;

  uint32_t result=lastFreeLabelPos_;

  for(uint32_t i=lastFreeLabelPos_+1;i<maxNumLabels;++i)
  {
    if(labels_[i] == NoLabel)
    {
      lastFreeLabelPos_=i;
      break;
    }
  }

  labels_[result]=0;
  return result;
}

void CodeGen::updateLabel(const Label &label)
{
  labels_[label.getLabelReference()]=getCurCodeAddr();
}

void CodeGen::patchAndRemoveLabel(const Label &label,uint32_t patchAddrStart)
{
  uint32_t labelRef=label.getLabelReference();

  for(uint32_t i=patchAddrStart;i<getCurCodeAddr();++i)
  {
    //check if goto must be patched
    if(instrs_[i].isGoto() && instrs_[i].symRef_ == labelRef)
    {
      instrs_[i].patchGotoTarget(label.codeAddr_-i);
      instrs_[i].symRef_=NoRef;
    }
  }

  labels_[labelRef]=NoLabel;

  if(lastFreeLabelPos_ > labelRef)
    lastFreeLabelPos_=labelRef;
}

void CodeGen::changeStorageSize(const TmpStorage &storage,uint32_t size)
{
  symbols_[storage.getSymbolReference()].changeArraySize(size);
}

void CodeGen::writeCode(uint32_t code,uint32_t ref)
{
  Error::expect(codeAddr_ < sizeof(instrs_)/sizeof(instrs_[0])) << "code buffer full" << Error::FATAL;

  instrs_[codeAddr_].code_=code;
  instrs_[codeAddr_].symRef_=ref;

  if(ref != NoRef && !instrs_[codeAddr_].isGoto())
    symbols_[ref].updateLastAccess(getCurCodeAddr());

  ++codeAddr_;
}
