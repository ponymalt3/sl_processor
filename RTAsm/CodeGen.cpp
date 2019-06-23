/*
 * CodeGen.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "CodeGen.h"
#include "Alloc.h"
#include "Stream.h"
#include "Error.h"
#include "Operand.h"
#include "string.h"

template<typename _T>
void swap(_T &a,_T &b)
{
  _T t=a;
  a=b;
  b=t;
} 

Label::Label(CodeGen &codeGen):Error(codeGen.getErrorHandler()), codeGen_(codeGen)
{
  labelRef_=codeGen_.getLabelId(this);
  codeAddr_=codeGen_.getCurCodeAddr();
  labelAddr_=0;
}

Label::~Label()
{
  if(labelRef_ != CodeGen::NoRef)
    deleteLabel();
}

void Label::setLabel()
{
  Error::expect(labelRef_ != CodeGen::NoRef) << "invalid label" << ErrorHandler::FATAL;
  //codeGen_.updateLabel(*this);
  labelAddr_=codeGen_.getCurCodeAddr();
}

void Label::deleteLabel()
{
  codeGen_.patchLabelInCode(*this,codeAddr_);
  codeGen_.releaseLabel(*this);
  labelRef_=CodeGen::NoRef;
}

void Label::replaceWith(Label &otherLabel)
{
  if(codeAddr_ < otherLabel.codeAddr_)
  {
    otherLabel.codeAddr_=codeAddr_;
  }
  
  codeGen_.replaceLabel(*this,otherLabel);
  codeGen_.releaseLabel(*this);
  labelRef_=CodeGen::NoRef;
}

TmpStorage::TmpStorage(CodeGen &codeGen):codeGen_(codeGen)
{
  symbolRef_=CodeGen::NoRef;
  size_=0;
  blockBegin_=codeGen_.getCurCodeAddr();
}

_Operand TmpStorage::allocate()
{
  if(symbolRef_ == CodeGen::NoRef)
    symbolRef_=codeGen_.allocateTmpStorage();

  ++size_;
  codeGen_.changeStorageSize(*this,size_);

  return _Operand::createSymAccess(symbolRef_,size_-1);
}

_Operand TmpStorage::preloadConstValue(qfp32 value)
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

void TmpStorage::preloadCode(uint32_t codeAddr,uint32_t size)
{
  codeGen_.moveCodeBlock(codeAddr,size,blockBegin_);
  blockBegin_+=size;
}

_Operand TmpStorage::getArrayBaseOffset()
{
  return _Operand::createSymAccess(symbolRef_,0xFFFF);
}

CodeGen::_LoopFrame::_LoopFrame()
{
  labCont_=0;
  labBreak_=0;
  counter_=0;
  isComplex_=false;
}

CodeGen::_LoopFrame::_LoopFrame(const Label *labCont,const Label *labBreak,const _Operand *counter)
{
  labCont_=labCont;
  labBreak_=labBreak;
  counter_=counter;
  isComplex_=false;
}

bool CodeGen::_LoopFrame::isComplex()
{
  return isComplex_;
}

void CodeGen::_LoopFrame::markComplex()
{
  isComplex_=true;
}

CodeGen::CodeGenDelegate::~CodeGenDelegate()
{
  uint32_t blockSize=codeGen_.getCurCodeAddr()-startAddr_;
  codeGen_.moveCodeBlock(startAddr_,blockSize,target_.labelAddr_);
  target_.labelAddr_+=blockSize;
}

CodeGen::CodeGenDelegate::CodeGenDelegate(CodeGen &codeGen,Label &target):codeGen_(codeGen),target_(target)
{
  startAddr_=codeGen_.getCurCodeAddr();
}

void _Instr::patchIrsOffset(uint32_t irsOffset)
{
  code_=SLCode::IRS::patchOffset(code_,irsOffset);
}

void _Instr::patchConstant(uint32_t value,bool patch2ndWord)
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

void _Instr::patchGotoTarget(int32_t target)
{
  code_=SLCode::Goto::create(target,false);
  //Error::expect(target < 512) << "jump target out of range " << (target);
}

uint32_t _Instr::getGotoTarget()
{
  int32_t offset=((code_>>2)&0x1FF)+(0xFFFFFE00*((code_>>9)&1));
  return offset;
}

CodeGen::CodeGen(Stream &stream,uint32_t entryVectorSize):Error(stream.getErrorHandler()),stream_(stream),defaultSymbols_(stream,0)
{
  loopDepth_=0;
  codeAddr_=entryVectorSize;
  labelIdBitMap_=0x7FFFFFFF;
  
  entryVectorSize_=entryVectorSize;
  for(uint32_t i=0;i<entryVectorSize;++i)
  {
    instrs_[i]={SLCode::Nop::create(),NoRef};
  }
  
  symbolMaps_.push(defaultSymbols_);

  //allocate first symbol for temp loop storage
  symbolMaps_.top().createSymbolNoToken(MaxLoopDepth);
  
  for(uint32_t i=0;i<sizeof(activeLabels_)/sizeof(activeLabels_[0]);++i)
  {
    activeLabels_[i]=0;
  }
}

CodeGen::~CodeGen()
{
  for(auto &i : functions_)
  {
    delete i.second.symbols_;
  }
}

void CodeGen::instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage)
{
  _Operand a=resolveOperand(opa);
  _Operand b=resolveOperand(opb);
  
  //handle base addr
  if(a.isArrayBaseAddr())
  {
    if(b.isResult())
    {
      _Operand t=tmpStorage.allocate();
      instrMov(t,_Operand::createResult());
      b=t;
    }
    
    instrMov(_Operand::createResult(),a);
    a=_Operand::createResult();
  }
  
  if(b.isArrayBaseAddr())
  {
    if(a.isResult())
    {
      _Operand t=tmpStorage.allocate();
      instrMov(t,_Operand::createResult());
      a=t;
    }
    
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
  if(a.type_ == _Operand::TY_VALUE && b.type_ == _Operand::TY_RESULT && op != '/' && op != '-' && op != SLCode::CMD_SHFT)
  {
    a=tmpStorage.preloadConstValue(a.value_);
  }

  bool aIsResult=a.isResult() || a.type_ == _Operand::TY_VALUE;
  bool bIsResult=b.isResult() || b.type_ == _Operand::TY_VALUE;
  bool aIsIRS=a.type_ == _Operand::TY_RESOLVED_SYM;
  bool bIsIRS=b.type_ == _Operand::TY_RESOLVED_SYM;

  if(((!aIsResult && bIsResult) || (aIsIRS && !bIsIRS)) && op != '/' && op != '-' && op != SLCode::CMD_SHFT)//div/sub/shft
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
    //special case: for operations >= 8 no mem as first parameter a allowed when second is irs
    if(a.type_ == _Operand::TY_MEM && op >= 8 && op < 15)
    {
      instrMov(_Operand::createResult(),a);
      a=_Operand::createResult();
    }
    
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

void CodeGen::instrUnaryOp(const _Operand &opa,uint32_t unaryOp)
{
  _Operand a=resolveOperand(opa);
  
  if(!a.isResult())
  {
    instrMov(_Operand::createResult(),a);
  }
  
  Error::expect(unaryOp == SLCode::UNARY_LOG2 ||
                unaryOp == SLCode::UNARY_NEG ||
                unaryOp == SLCode::UNARY_TRUNC) << "unary op internal error";
  
  writeCode(SLCode::UnaryOp::create(static_cast<SLCode::UnaryCommand>(unaryOp)));
}

void CodeGen::instrLoop(const _Operand &opa)
{
  instrMov(_Operand::createResult(),opa);
  writeCode(SLCode::Loop::create());
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
    Error::expect(false) <<"'continue' used outside loop";
}

void CodeGen::instrGoto(const Label &label)
{
  if(loopDepth_ > 0 && label.codeAddr_ != 0)
  {
    //backward jumping gotos are only generated by RTAsm loops therefore it is not necessary to mark goto instr as loop complex 
    loopFrames_[loopDepth_-1].markComplex();
  }
  
  writeCode(SLCode::Goto::create(0,false),label.getLabelReference());
}

void CodeGen::instrGoto2()
{
  if(loopDepth_ > 0)
  {
    loopFrames_[loopDepth_-1].markComplex();
  }
  
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
  
  //optimize when operands can be swapped!!!!!!!!
  
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
  
  arrayAllocInfo_.insert({getCurCodeAddr(),symbolMaps_.top().findSymbol(str)});
}

void CodeGen::resizeArray(const Stream::String &str,uint32_t newSize)
{
  Error::expect(newSize > 0) <<"array size "<<str<<" must be greater than 0";
  uint32_t arrRef=symbolMaps_.top().findSymbol(str);
  Error::expect(arrRef != NoRef || symbolMaps_.top()[arrRef].flagIsArray_ == 0) 
          << "internal error: array '" << str << "' not declared";
          
  symbolMaps_.top()[arrRef].allocatedSize_=newSize;
}

void CodeGen::addDefinition(const Stream::String &str,qfp32 value)
{
  symbolMaps_.top().createConst(str,value);
}

void CodeGen::addReference(const Stream::String &str,uint32_t irsOffset)
{
  symbolMaps_.top().createReference(str,irsOffset);
}

void CodeGen::createLoopFrame(const Label &contLabel,const Label &breakLabel,const _Operand *counter)
{
  Error::expect(loopDepth_ < MaxLoopDepth) << "internal error: no more loop frames available" << ErrorHandler::FATAL;

  //_Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  //_Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);
  
  if(loopDepth_ > 0)
  {
    loopFrames_[loopDepth_-1].markComplex();
  }
  
  loopFrames_[loopDepth_]=_LoopFrame(&contLabel,&breakLabel,counter);

  ++loopDepth_;
}

void CodeGen::removeLoopFrame()
{
  Error::expect(loopDepth_ > 0) << "loop frame expected" << ErrorHandler::FATAL;

  --loopDepth_;

  //_Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
  //_Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

  //instrMov(loopReg,irsStorage);
}

bool CodeGen::isLoopFrameComplex()
{
  Error::expect(loopDepth_ > 0) << "loop frame expected" << ErrorHandler::FATAL;
  return loopFrames_[loopDepth_-1].isComplex();
}

void CodeGen::storageAllocationPass(uint32_t size,uint32_t numParams)
{
  Allocator allocator(size);
  
  uint32_t x=allocator.allocate(4+numParams);//reserve space for parameter

  Error::expect(codeAddr_ < 0xFFFF) << "too many instructions" << ErrorHandler::FATAL;

  for(uint32_t i=symbolMaps_.top().getStartAddr();i<codeAddr_;++i)
  {
    //handle case where array must be allocated before a certain operation
    while(arrayAllocInfo_.size() > 0 && arrayAllocInfo_.begin()->first == i)
    {
      SymbolMap::_Symbol &symInf=symbolMaps_.top()[arrayAllocInfo_.begin()->second];
      symInf.flagAllocated_=1;
      symInf.allocatedAddr_=allocator.allocate(symInf.allocatedSize_,symInf.flagsAllocateHighest_,symInf.flagIsArray_);
      arrayAllocInfo_.erase(arrayAllocInfo_.begin());
    }
    
    if(instrs_[i].symRef_ != SymbolMap::InvalidLink && instrs_[i].symRef_ < RefLabelOffset)
    {
      SymbolMap::_Symbol &symInf=symbolMaps_.top()[instrs_[i].symRef_];
      
      //mark as patched
      instrs_[i].symRef_=SymbolMap::InvalidLink;

      //alloc storage if not already
      if(!symInf.flagAllocated_)
      {
        symInf.flagAllocated_=1;
        symInf.allocatedAddr_=allocator.allocate(symInf.allocatedSize_,symInf.flagsAllocateHighest_,symInf.flagIsArray_);
      }
      
      if(instrs_[i].isIrsInstr())
      {
        //array index already addressed => add irsOffset
        instrs_[i].patchIrsOffset(instrs_[i].getIrsOffset()+symInf.allocatedAddr_);
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

CodeGen::_FunctionInfo& CodeGen::findFunction(const Stream::String &symbol)
{  
  std::string s=std::string(symbol.getBase()+symbol.getOffset(),symbol.getLength());
  
  auto it=functions_.find(s);
  
  if(it == functions_.end())
  {
    static _FunctionInfo fi{nullptr,NoRef,0,0};
    return fi;
  }
  
  return it->second;
}

CodeGen::_FunctionInfo& CodeGen::addFunctionAtCurrentAddr(const Stream::String &symbol)
{
  std::string s=std::string(symbol.getBase()+symbol.getOffset(),symbol.getLength());
  
  _FunctionInfo function{new SymbolMap(stream_,getCurCodeAddr()),
                         getCurCodeAddr(),
                         0,
                         0,
                         false,
                         {
                           0,
                           std::vector<_Instr>(),
                           std::vector<std::list<uint32_t>>()                          
                         }
                         };
  
  return (functions_.insert(std::make_pair(s,function)).first)->second;
}

void CodeGen::pushSymbolMap(SymbolMap &currentSymbolMap)
{
  Error::expect(symbolMaps_.full() == false) << "SymbolMaps stack overflow" << ErrorHandler::FATAL;
  symbolMaps_.push(currentSymbolMap);
}

void CodeGen::popSymbolMap()
{
  Error::expect(symbolMaps_.empty() == false) << "SymbolMaps stack underflow" << ErrorHandler::FATAL;
  symbolMaps_.pop();
}

CodeGen::CodeGenDelegate CodeGen::insertCodeBefore(Label &label)
{
  return CodeGenDelegate(*this,label);
}

void CodeGen::generateEntryVector(uint32_t startAddr)
{
  if(startAddr < entryVectorSize_)
  {
    startAddr=entryVectorSize_;
  }
  
  Error::expect(entryVectorSize_ >= 3) << "Entry vector size too small" << ErrorHandler::FATAL;
  
  uint32_t addr=0;
  uint32_t startAddrAsRaw=qfp32::fromRealQfp32((double)startAddr).toRealQfp32().getAsRawUint();    
  instrs_[addr++]={SLCode::Load::create1(startAddrAsRaw),NoRef};
  instrs_[addr++]={SLCode::Load::create2(startAddrAsRaw),NoRef};
  instrs_[addr++]={SLCode::Goto::create(),NoRef};
}

void CodeGen::generateEntryVector(uint32_t numberOfEntries,uint32_t entrySizeInInstrs)
{
  Error::expect(entryVectorSize_ >= numberOfEntries*entrySizeInInstrs) << "Entry vector size too small" << ErrorHandler::FATAL;
  
  int32_t defFunctionAddr=findFunction(Stream::String("main",0,4)).address_;
  
  uint32_t addr=0;
  for(uint32_t i=0;i<numberOfEntries;++i)
  {
    char buf[8]="main   ";
    uint32_t j=4;
    if(i > 9)
    {
      buf[j++]='0'+((i+1)/10)%10;
    }    
    buf[j++]='0'+(i+1)%10;
        
    int32_t specFunctionAddr=findFunction(Stream::String(buf,0,j)).address_;
    
    if(specFunctionAddr == NoRef)
    {
      specFunctionAddr=defFunctionAddr;
    }
    
    Error::expect(specFunctionAddr != NoRef) << "No entry function for core '" << (i) << "'";
    
    uint32_t fctAddrAsRaw=qfp32::fromRealQfp32(qfp32_t(specFunctionAddr)).toRealQfp32().getAsRawUint();    
    instrs_[addr++]={SLCode::Load::create1(fctAddrAsRaw),NoRef};
    instrs_[addr++]={SLCode::Load::create2(fctAddrAsRaw),NoRef};
    instrs_[addr++]={SLCode::Goto::create(),NoRef};
    
    Error::expect(entrySizeInInstrs >= 3) << "Entry function size too small for jump" << ErrorHandler::FATAL;
    for(uint32_t j=3;j<entrySizeInInstrs;++j)
    {
      instrs_[addr++]={SLCode::Nop::create(),NoRef};
    }
  }
}

void CodeGen::setCodeMovedCallback(const std::function<void(uint32_t,uint32_t,uint32_t)> &callback)
{
  callback_=callback;
}

_Operand CodeGen::resolveOperand(const _Operand &op,bool createSymIfNotExists)
{
  if(op.type_ == _Operand::TY_SYMBOL)
  {
    Stream::String name=stream_.createStringFromToken(op.offset_,op.length_);
    uint32_t symRef=symbolMaps_.top().findSymbol(name);

    if(createSymIfNotExists && symRef == SymbolMap::InvalidLink)
      symRef=symbolMaps_.top().createSymbol(name,0);//single element
     
    //check outer symbol map (if exist) and use if it is a constant
    if(symRef == SymbolMap::InvalidLink && symbolMaps_.size() > 1)
    {
      SymbolMap &symMapOuter=symbolMaps_.get(symbolMaps_.size()-2);
      uint32_t symRef=symMapOuter.findSymbol(name);
      if(symRef != SymbolMap::InvalidLink && symMapOuter[symRef].flagConst_)
      {
        return _Operand(symMapOuter[symRef].constValue_);
      }        
    }

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
    Error::expect(loopFrames_[op.loopIndex_].counter_ != 0) << "no index available in while loop";
    
    uint32_t index=op.loopIndex_;
    
    loopFrames_[index].markComplex();//need counter
    
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
      if(op < 16)//probably already a command
      {
        return static_cast<SLCode::Command>(op);
      }
      break;
  }
  return SLCode::Command::CMD_MOV;
}

uint32_t CodeGen::allocateTmpStorage()
{
  return symbolMaps_.top().createSymbolNoToken(1,true);
}

uint32_t CodeGen::getLabelId(Label* label)
{
  Error::expect(labelIdBitMap_ != 0) << "no more label ids available" << ErrorHandler::FATAL;
  
  uint32_t freeBitPos=Stream::log2(labelIdBitMap_);
  
  if((1<<freeBitPos) > labelIdBitMap_)
  {
    --freeBitPos;
  }
  
  labelIdBitMap_&=~(1<<freeBitPos);
  
  activeLabels_[freeBitPos]=label;

  return freeBitPos + RefLabelOffset;
}

void CodeGen::releaseLabel(const Label &label)
{
  uint32_t labelRef=label.getLabelReference();
  
    //release label id
  labelIdBitMap_|=1<<(labelRef&0x1F);
  activeLabels_[(labelRef&0x1F)]=0;
}

void CodeGen::patchLabelInCode(const Label &label,uint32_t patchAddrStart)
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
      instrs_[i].symRef_=RefLoad;
    }
  }
}

void CodeGen::replaceLabel(const Label &labelToBeReplaced,const Label &newLabel)
{
  uint32_t labelRef=labelToBeReplaced.getLabelReference();

  for(uint32_t i=labelToBeReplaced.getAddr();i<getCurCodeAddr();++i)
  {
    //check if goto must be patched
    if(instrs_[i].symRef_ != labelRef)
    {
      continue;
    }
    
    instrs_[i].symRef_=newLabel.getLabelReference();
  }
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
    
  if(ref < RefLabelOffset && instrs_[codeAddr_].isLoadAddr())
  {
    //load array base addr
    symbolMaps_.top()[ref].updateLastAccess(0xFFFFFFFF);//dont release cause we dont know when data is referenced last
  }

  ++codeAddr_;
}

void CodeGen::moveCodeBlock(uint32_t startAddr,uint32_t size,uint32_t targetAddr)
{
  if(startAddr == targetAddr || size == 0)
  {
    return;
  }
  
  Error::expect(size <= sizeof(instrs_)/sizeof(instrs_[0])-getCurCodeAddr()) << "not engough instr buffer space" << ErrorHandler::FATAL;  
  
  uint32_t size2=abs(startAddr-targetAddr);
  
  uint32_t blockStart=0;
  uint32_t blockTarget=0;
  if(startAddr < targetAddr)
  {
    blockStart=startAddr+size;//memmove(instrs_+startAddr,instrs_+startAddr+size,sizeof(_Instr)*size2);
    blockTarget=startAddr;//rebaseCode(startAddr,startAddr+size2-1,-size);
  }
  else
  {
    blockStart=targetAddr;//memmove(instrs_+targetAddr+size,instrs_+targetAddr,sizeof(_Instr)*size2);
    blockTarget=targetAddr+size;//rebaseCode(targetAddr+size,targetAddr+size+size2-1,size);
  }
  
  memcpy(instrs_+getCurCodeAddr(),instrs_+startAddr,sizeof(_Instr)*size);
  
  memmove(instrs_+blockTarget,instrs_+blockStart,sizeof(_Instr)*size2);
  rebaseCode(blockTarget,blockTarget+size2-1,blockTarget-blockStart);
    
  memcpy(instrs_+targetAddr,instrs_+getCurCodeAddr(),sizeof(_Instr)*size);
  rebaseCode(targetAddr,targetAddr+size-1,targetAddr-startAddr);
  
  //rebase labels
  for(uint32_t i=0;i<sizeof(activeLabels_)/sizeof(activeLabels_[0]);++i)
  {
    if(activeLabels_[i] == 0)
    {
      continue;
    }
    
    uint32_t a=activeLabels_[i]->labelAddr_;
    
    if(a >= startAddr && a < (startAddr+size))
    {
      //activeLabels_[i]->rebase(targetAddr,targetAddr+size-1,targetAddr-startAddr);
    }
    else
    {
      //activeLabels_[i]->rebase(blockTarget,blockTarget+size2-1,blockTarget-blockStart);
    }
  }
  
  if(callback_ != nullptr)
  {
    callback_(startAddr,size,targetAddr);
  }
}

void CodeGen::rebaseCode(uint32_t startAddr,uint32_t endAddr,int32_t offset)
{ 
  std::multimap<uint32_t,uint32_t> rebasedAllocInfo;
  
  for(int32_t i=startAddr;i<=endAddr;)
  {
    if(instrs_[i].isGoto())
    {
      int32_t gotoAddr=i+instrs_[i].getGotoTarget();
      
      if(gotoAddr < startAddr || gotoAddr > (endAddr+1))
      {
        instrs_[i].patchGotoTarget(instrs_[i].getGotoTarget()-offset);
      }
    }
    
    //update last access position
    if(instrs_[i].symRef_ != NoRef && !instrs_[i].isGoto() && !instrs_[i].isLoadAddr())
    {
      symbolMaps_.top()[instrs_[i].symRef_].updateLastAccess(i);
    }
    
    if(instrs_[i].isLoadAddr() && instrs_[i].symRef_ == RefLoad)
    {
      bool load2=(i+1)<endAddr && instrs_[i+1].isLoadAddr();
      bool load3=(i+2)<endAddr && instrs_[i+2].isLoadAddr();
      
      qfp32_t v=_Instr::restoreValueFromLoad(instrs_[i],
                                             load2?instrs_[i+1]:_Instr({0,0}),
                                             load3?instrs_[i+2]:_Instr({0,0}));
      
      v=v+offset;
      
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
    
    auto search=arrayAllocInfo_.find(i);
    
    if(search != arrayAllocInfo_.end())
    {
      while(search->first == i)
      {
        rebasedAllocInfo.insert({std::min(search->first,search->first+offset),search->second});
        auto rm=search;
        ++search;
        arrayAllocInfo_.erase(rm);
      }
    }
    
    ++i;
  }
  
  arrayAllocInfo_.insert(rebasedAllocInfo.begin(),rebasedAllocInfo.end());
}