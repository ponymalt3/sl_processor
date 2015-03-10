/*
 * RTParser.h
 *
 *  Created on: Jan 29, 2015
 *      Author: malte
 */

#ifndef RTPARSER_H_
#define RTPARSER_H_

//#include "../TestVHDL/qfp32.h"
#include "RTAsm.h"

#include <iostream>

template<typename _T,uint32_t _Size>
class Stack
{
public:
  Stack() { sp_=stack_; }

  void push(_T data) { *(sp_++)=data; }
  _T pop() { return *(--sp_); }
  _T& top() { return sp_[-1]; }
  _T& top() const { return sp_[-1]; }

  bool empty() const { return sp_ == stack_; }
  bool full() const { return stack_+_Size == sp_; }
  uint32_t size() const { return sp_-stack_; }

protected:
  _T stack_[_Size];
  _T *sp_;
};

struct qfp32
{
public:
  uint32_t mant_ : 29;
  uint32_t exp_ : 2;
  uint32_t sign_ : 1;
};

class Stream
{
public:
  enum {InvalidMark=0xFFFFFFFF};

  Stream(const char *code)
  {
    asmText_=code;
    pos_=0;
    line_=0;

    length_=0;
    while(code[length_] != '\0') ++length_;

    markPos_=InvalidMark;
    markLine_=line_;
  }

  char peek()
  {
    while(asmText_[pos_] == '%')
    {
      ++pos_;
      while(pos_ < length_ && asmText_[pos_++] != '%');
      while(pos_ < length_ && asmText_[pos_] == ' ') ++pos_;
    }

    return asmText_[pos_];
  }
  char read() { char ch=peek(); ++pos_; return ch; }

  bool empty() const { return pos_ >= length_; }
  uint32_t getCurrentLine() const { return line_; }

  Stream& skipWhiteSpaces()
  {
    while(!empty())
    {
      char ch=asmText_[pos_];

      if(ch ==' ' || ch == '\n')
      {
        ++pos_;
        line_+=ch=='\n';
      }
      else
        break;
    }

    return *this;
  }

  uint32_t readMultipleChar(char ch,uint32_t maxCount=0)
  {
    skipWhiteSpaces();

    uint32_t beg=pos_;
    while(peek() == ch && --maxCount >= 0)
      read();

    return pos_-beg;
  }

  uint32_t readInt(bool allowSign=true)
  {
    uint32_t value=0;
    uint32_t digits=0;
    bool sign=0;

    skipWhiteSpaces();

    if(peek() == '-')
    {
      //Error::expect(allowSign) <<*this<<"sign for integer not allowed";
      read();
      sign=true;
    }

    char ch=peek();
    while(ch >= '0' && ch <= '9')
    {
      value=value*10+(ch-'0');
      ++digits;
      read();
      ch=peek();
    }

    //Error::expect(digits < 9) << (*this) << "possible const value overflow";

    return value*(sign?-1:1);
  }

  qfp32 readQfp32()
  {
    qfp32 value;
    value.sign_=0;
    value.mant_=0;
    value.exp_=0;

    uint32_t intPart=readInt();

    if(intPart < 0)
    {
      value.sign_=1;
      intPart=-intPart;
    }

    uint32_t bitsInt=log2(intPart);

    value.mant_=intPart;
    value.exp_=(bitsInt+3)/8;

    uint32_t fracPart=0;
    if(peek() == '.')
    {
      read();
      fracPart=readInt(false);
    }

    if(fracPart > 0)
    {
      //convert to binary frac
      uint32_t bitsFrac=log2(fracPart);
      uint32_t fracValue=(1<<bitsFrac)/fracPart;
      uint32_t allowedFracBits=29-bitsInt;

      uint32_t fracMant=0;
      if(allowedFracBits > bitsFrac)
        fracMant=fracValue<<(allowedFracBits-bitsFrac);
      else
        fracMant=fracValue>>(bitsFrac-allowedFracBits);

      value.mant_+=fracMant;
    }

    return value;
  }

  class String
  {
  public:
    friend class Error;

    String(const char *base,uint32_t offset,uint32_t length)
    {
      base_=base;
      offset_=offset;
      length_=length;
    }

    bool operator==(const char *str) const
    {
      for(uint32_t i=0;i<length_;++i)
      {
        if(str[i] == '\0' || str[i] != base_[offset_+i])
          return false;
      }

      return true;
    }

    bool operator==(const String &str) const
    {
      for(uint32_t i=0;i<min(length_,str.getLength());++i)
      {
        if(str[i] != base_[offset_+i])
          return false;
      }

      return true;
    }

    uint32_t hash() const
    {
      uint32_t hash=0x76AB;

      for(uint32_t i=0;i<3;++i)
      {
        hash=(hash<<4)^(hash>>1);
        if(i < length_)
          hash+=base_[offset_+i];
      }

      return (hash>>3) + (((length_^(length_>>3)))&0x7);
    }

    char operator[](uint32_t index) const { return base_[offset_+index]; }

    uint32_t getOffset() const { return offset_; }
    uint32_t getLength() const { return length_; }

  protected:
    const char *base_;
    uint16_t offset_;
    uint16_t length_;
  };

  String readSymbol()
  {
    skipWhiteSpaces();

    uint32_t begin=pos_;

    do
    {
      char ch=peek();

      if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
      {
        read();
        continue;
      }

      if(ch >= '0' && ch <= '9')
      {
        read();
        continue;
      }

      break;
    }while(!empty());

    uint32_t length=pos_-begin;

    return String(asmText_,begin,length);
  }

  String createStringFromToken(uint32_t offset,uint32_t length) const
  {
    return String(asmText_,offset,length);
  }

  void markPos()
  {
    markPos_=pos_;
    markLine_=line_;
  }

  void restorePos()
  {
    assert(markPos_ != InvalidMark);

    pos_=markPos_;
    line_=markLine_;
    markPos_=InvalidMark;
  }

//protected:
  const char *asmText_;
  uint32_t pos_;
  uint32_t line_;
  uint32_t length_;
  uint32_t markPos_;
  uint32_t markLine_;
};

class Error
{
public:
  enum Type {FATAL};

  static uint32_t getNumErrors()
  {
    return instance_.errors_;
  }

  static uint32_t LineNumber(Stream &stream,uint32_t pos)
  {
    //if(!instance_.isFault_)
      return 0xFFFFFFF;

    //Stream::String str=stream.createStringFromToken(pos,1);
    uint32_t lines=0;
    uint32_t i=0;
    /*while(i < pos)
    {
      if(str.base_[i++] == '\n')
        ++lines;
    }*/

    return lines;
  }

  static Error& expect(bool expr)
  {
    if(instance_.newLinePending_)
      std::cout<<"\n";

    instance_.newLinePending_=false;
    instance_.isFault_=!expr;
    return instance_;
  }

  Error& operator<<(const char *str)
  {
    if(!isFault_)
      return *this;

    printHeader();
    std::cout<<"  "<<str;
    return *this;
  }

  Error& operator<<(const Stream::String &str)
  {
    if(!isFault_)
      return *this;

    printHeader();

    std::cout<<"  ";
    for(uint32_t i=0;i<str.getLength();++i)
      std::cout<<str[i];

    return *this;
  }

  Error& operator<<(uint32_t value)
  {
    if(!isFault_)
      return *this;

    printHeader();
    std::cout<<"  "<<value;
    return *this;
  }

  Error& operator<<(const Stream &stream)
  {
    line_=stream.getCurrentLine();
    linePrinted_=false;
    return *this;
  }

  Error& operator<<(Type type)
  {
    if(type == FATAL)
      assert(isFault_ == false);

    return *this;
  }

protected:
  Error()
  {
    isFault_=false;
    linePrinted_=false;
    newLinePending_=false;
    line_=0;
    errors_=0;
  }

  void printHeader()
  {
    if(!linePrinted_)
      std::cout<<"at "<<(line_)<<":\n";

    linePrinted_=true;
    newLinePending_=true;
  }

  static Error instance_;

  bool isFault_;
  bool linePrinted_;
  bool newLinePending_;
  uint32_t line_;
  uint32_t errors_;
};

Error Error::instance_;

class SymbolMap
{
public:
  enum {InvalidLink=0xFFFF};
  struct _Symbol
  {
    _Symbol()
    {
      strOffset_=0;
      strLength_=0;
      flagAllocated_=0;
      flagConst_=0;
      flagUseAddrAsArraySize_=0;
      allocatedAddr_=0;
      lastAccess_=0;
      link_=InvalidLink;
    }

    _Symbol(const Stream::String &str)
    {
      strOffset_=str.getOffset();
      strLength_=str.getLength();
      flagAllocated_=0;
      flagConst_=0;
      flagUseAddrAsArraySize_=0;
      allocatedAddr_=0;
      lastAccess_=0;
      link_=InvalidLink;
    }

    void updateLastAccess(uint32_t codeAddr) { lastAccess_=codeAddr; }
    void changeArraySize(uint32_t size)
    {
      flagUseAddrAsArraySize_=size!=1;
      if(flagUseAddrAsArraySize_)
        allocatedAddr_=size;
    }

    uint16_t strOffset_;
    uint16_t strLength_ : 8;
    uint16_t flagAllocated_ : 1;
    uint16_t flagConst_ : 1;
    uint16_t flagUseAddrAsArraySize_ : 1;
    uint16_t link_;
    union
    {
      struct
      {
        uint16_t allocatedAddr_;
        uint16_t lastAccess_;
      };
      qfp32 constValue_;
    };
  };

  SymbolMap(Stream &stream):stream_(stream)
  {
    symCount_=0;

    for(uint32_t i=0;i<(sizeof(hashTable_)/sizeof(hashTable_[0]));++i)
      hashTable_[i]=InvalidLink;
  }

  uint32_t findSymbol(const Stream::String &str)
  {
    return findSymbol(str,str.hash());
  }

  uint32_t createSymbol(const Stream::String &str,uint32_t size=0)
  {
    uint32_t hash=str.hash();

    uint32_t i=findSymbol(str,hash);
    Error::expect(i == InvalidLink) << stream_ << "symbol " << str << " already defined at ";//<< Error::LineNumber(stream_,symbols_[i].strOffset_);

    return insertSymbol(_Symbol(str),hash);
  }

  uint32_t findOrCreateSymbol(const Stream::String &str,uint32_t size=0)
  {
    uint32_t hash=str.hash();

    uint32_t i=findSymbol(str,hash);

    if(i != InvalidLink)
      return i;

    return insertSymbol(_Symbol(str),hash,size);
  }

  uint32_t createSymbolNoToken(uint32_t size)
  {
    return insertSymbol(_Symbol(),0,size);
  }

  uint32_t createConst(const Stream::String &str,qfp32 value)
  {
    uint32_t hash=str.hash();

    uint32_t i=findSymbol(str,hash);
    Error::expect(i == InvalidLink) << stream_ << "const "<< str << " already defined";

    _Symbol newSym=_Symbol(str);
     newSym.constValue_=value;
     newSym.flagConst_=1;

     return insertSymbol(newSym,hash);
  }

  uint32_t createReference(const Stream::String &str,uint32_t irsOffset)
  {
    uint32_t hash=str.hash();

    uint32_t i=findSymbol(str,hash);
    Error::expect(i == InvalidLink) << stream_ << "const " << str << " already defined at ";// << Error::LineNumber(stream_,symbols_[i].strOffset_);

    _Symbol newSym=_Symbol(str);
     newSym.flagAllocated_=1;
     newSym.allocatedAddr_=irsOffset;

     return insertSymbol(newSym,hash);
  }

  _Symbol& operator[](uint32_t i)
  {
    Error::expect(i < symCount_) << stream_ << "symbol reference out of range" << (i) << Error::FATAL;
    return symbols_[i];
  }

protected:
  uint32_t insertSymbol(const _Symbol &sym,uint32_t hashIndex=0,uint32_t size=0)
  {
    Error::expect(symCount_ < (sizeof(symbols_)/sizeof(symbols_[0]))) << stream_ << "no more symbol storage available" << Error::FATAL;

    _Symbol &newSym=symbols_[symCount_];

    newSym=sym;

    hashIndex&=0xFF;
    newSym.link_=hashTable_[hashIndex];
    hashTable_[hashIndex]=symCount_;

    if(size > 1)
    {
      newSym.flagUseAddrAsArraySize_=1;
      newSym.allocatedAddr_=size;
    }

    return symCount_++;
  }

  uint32_t findSymbol(const Stream::String &str,uint32_t hashIndex)
  {
    uint32_t cur=hashTable_[hashIndex&0xFF];
    while(cur != InvalidLink)
    {
      if(stream_.createStringFromToken(symbols_[cur].strOffset_,symbols_[cur].strLength_) == str)
        return cur;

      cur=symbols_[cur].link_;
    }

    return InvalidLink;
  }

  _Symbol symbols_[200];
  uint16_t hashTable_[256];
  uint16_t symCount_;
  Stream &stream_;
};


/*
 * exp := const exp' |
 *        symbol exp'|
 *        '(' exp ')';
 *
 * exp' := e |
 *         ('+' | '-' | '*' | '/') exp;
 *
 * if := 'if' ifexp '\n' stments ifelse 'end'
 *
 * ifexp := exp ('>' | '<' | '>=' | '<=' | '==' | '!=') exp
 *
 * ifelse := e |
 *           'else' stments;
 *
 * loop := 'loop' '(' exp ')' stments 'end';
 *
 * stment := symbol ('=' | '+=') exp |
 *           if |
 *           loop |
 *           'decl' name int |
 *           'def' name const |
 *           'ref' name const |
 *           'break' |
 *           'continue';
 *
 * stments := stment stments';
 *
 * stments' := e |
 *             stment;
 *
 * symbol := name ['(' uint ')'] |
 *           '[' 'a' ('0'|'1') ['++'] ']'
 *
 * const := number
 *
 */

struct _Operand
{
  enum Type {TY_INVALID,TY_VALUE,TY_MEM,TY_SYMBOL,TY_RESULT,TY_INDEX,TY_IR_LOOP,TY_IR_ADDR0,TY_IR_ADDR1,TY_RESOLVED_SYM};
  enum InternalReg {IR_ADR0,IR_ADR1,IR_LOOP};

  _Operand(uint32_t regaIndex,bool addrInc)
  {
    type_=TY_MEM;
    regaIndex_=regaIndex;
    addrInc_=addrInc;
  }

  _Operand(qfp32 value)
  {
    type_=TY_VALUE;
    value_=value;
  }

  _Operand(uint32_t offset,uint32_t length,uint32_t index)
  {
    type_=TY_SYMBOL;
    offset_=offset;
    length_=length;
    index_=index;
  }

  _Operand(Type type=TY_INVALID)
  {
    type_=type;
  }

  static _Operand createLoopIndex(uint32_t index)
  {
    _Operand op(TY_INDEX);
    op.loopIndex_=index;
    return op;
  }

  static _Operand createResult()
  {
    return _Operand(TY_RESULT);
  }

  static _Operand createSymAccess(uint32_t mapIndex,uint32_t index)
  {
    _Operand op(TY_RESOLVED_SYM);
    op.mapIndex_=mapIndex;
    op.arrayOffset_=index;
    return op;
  }

  static _Operand createMemAccess(uint32_t regaIndex,bool addrInc)
  {
    _Operand op(TY_MEM);
    op.regaIndex_=regaIndex;
    op.addrInc_=addrInc;
    return op;
  }

  static _Operand createInternalReg(Type reg)
  {
    _Operand op(reg);
    assert(op.isInternalReg());
    return op;
  }

  static _Operand createSymbol(const Stream::String &name,uint32_t index=0)
  {
    return _Operand(name.getOffset(),name.getLength(),index);
  }

  bool isResult() const { return type_ == TY_RESULT; }
  bool isArrayBaseAddr() const { return type_ == TY_RESOLVED_SYM && index_ == 0xFFFF; }
  bool isInternalReg() const { return type_ == TY_IR_LOOP || type_ == TY_IR_ADDR0 || type_ == TY_IR_ADDR1; }

  uint16_t type_;
  union
  {
    struct
    {
      qfp32 value_;
    };
    struct//mem access (a0,a1)
    {
      uint16_t regaIndex_;
      uint16_t addrInc_;
    };
    struct//symbolic mem access
    {
      uint16_t offset_;
      uint16_t length_;
      uint16_t index_;
    };
    struct//(resolved) memory access
    {
      uint16_t mapIndex_;
      uint16_t arrayOffset_;
    };
    struct
    {
      uint16_t loopIndex_;
    };
  };
};


class CodeGen
{
public:
  enum {CMP_MODE_GT=0,CMP_MODE_LE=3,CMP_MODE_EQ=1,CMP_MODE_NEQ=2,CMP_MODE_SWAP_FLAG=0x8000};
  enum {EXEC_MODE_1CYC,EXEC_MODE_3CYC};
  enum {MaxLoopDepth=6,LoopStorageIndex=0,NoLoopFrame=-1};
  enum {NoRef=0xFFFF,NoLabel=NoRef};

  CodeGen(Stream &stream): symbols_(stream),stream_(stream)
  {
    usedRefs_=0;
    loopDepth_=0;
    codeAddr_=0;
    lastFreeLabelPos_=0;

    //allocate first symbol for temp loop storage
    symbols_.createSymbolNoToken(MaxLoopDepth);
  }

  class Label
  {
  public:
    friend class CodeGen;

    Label(CodeGen &codeGen):codeGen_(codeGen)
    {
      labelRef_=codeGen_.createLabel();
      codeAddr_=codeGen_.getCurCodeAddr();
    }
    ~Label()
    {
      if(labelRef_ != NoRef)
        deleteLabel();
    }

    void setLabel()
    {
      Error::expect(labelRef_ != NoRef) << "invalid label" << Error::FATAL;
      codeGen_.updateLabel(*this);
    }
    void deleteLabel() {}// codeGen_.patchAndRemoveLabel(*this,codeAddr_); labelRef_=NoRef; }

  protected:
    uint32_t getLabelReference() const { return labelRef_; }

    CodeGen &codeGen_;
    uint16_t labelRef_;
    uint16_t codeAddr_;
  };

  class TmpStorage
  {
  public:
    friend class CodeGen;

    TmpStorage(CodeGen &codeGen):codeGen_(codeGen)
    {
      symbolRef_=NoRef;
      size_=0;
    }

    _Operand allocate()
    {
      if(symbolRef_ == NoRef)
        symbolRef_=codeGen_.allocateTmpStorage();

      ++size_;
      codeGen_.changeStorageSize(*this,size_);

      return _Operand::createSymAccess(symbolRef_,size_-1);
    }

  protected:
    uint32_t getSymbolReference() const { return symbolRef_; }

    CodeGen &codeGen_;
    uint16_t symbolRef_;
    uint16_t size_;
  };

  struct _LoopFrame
  {
    _LoopFrame()
    {
      labCont_=0;
      labBreak_=0;
    }
    _LoopFrame(const Label *labCont,const Label *labBreak)
    {
      labCont_=labCont;
      labBreak_=labBreak;
    }

    const Label *labCont_;
    const Label *labBreak_;
  };

  void instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage)
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

  void instrMov(const _Operand &opa,const _Operand &opb)
  {
    _Operand a=resolveOperand(opa,true);
    _Operand b=resolveOperand(opb);

    assert(a.type_ != _Operand::TY_VALUE);
    assert(b.type_ != _Operand::TY_IR_ADDR0);
    assert(b.type_ != _Operand::TY_IR_ADDR1);

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

  void instrNeg(const _Operand &opa)
  {
    _Operand a=resolveOperand(opa);

    instrMov(_Operand::createResult(),a);
    writeCode(0);
  }

  void instrLoop(const _Operand &opa)
  {
    _Operand a=resolveOperand(opa);

    if(!a.isResult())
      loadOperandIntoResult(a);

    writeCode(0);
  }

  void instrBreak()
  {
    if(loopDepth_ > 0)
      instrGoto(*(loopFrames_[loopDepth_-1].labBreak_));
    else
      Error::expect(false) <<"'break' used outside loop";
  }

  void instrContinue()
  {
    if(loopDepth_ > 0)
      instrGoto(*(loopFrames_[loopDepth_-1].labCont_));
    else
      Error::expect(false) <<"'break' used outside loop";
  }

  void instrGoto(const Label &label)
  {
    writeCode(0,label.getLabelReference());
  }

  void instrCompare(_Operand a,_Operand b,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage)
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

  void instrSignal(uint32_t target)
  {
    writeCode(0);
  }

  void instrWait()
  {
    writeCode(0);
  }

  void addArrayDeclaration(const Stream::String &str,uint32_t size)
  {
    Error::expect(size > 0) <<"array decl "<<str<<" must be greater than 0";
    symbols_.createSymbol(str,size);
  }

  void addDefinition(const Stream::String &str,qfp32 value)
  {
    symbols_.createConst(str,value);
  }

  void addReference(const Stream::String &str,uint32_t irsOffset)
  {
    symbols_.createReference(str,irsOffset);
  }

  uint32_t getCurCodeAddr() const
  {
    return codeAddr_;
  }

  void createLoopFrame(const Label &contLabel,const Label &breakLabel)
  {
    Error::expect(loopDepth_ < MaxLoopDepth) << "FATAL loop frames run out of space" << Error::FATAL;

    _Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
    _Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

    instrMov(irsStorage,loopReg);

    loopFrames_[loopDepth_]=_LoopFrame(&contLabel,&breakLabel);

    ++loopDepth_;
  }

  void removeLoopFrame()
  {
    assert(loopDepth_ > 0);

    --loopDepth_;

    _Operand irsStorage=_Operand::createSymAccess(LoopStorageIndex,loopDepth_);
    _Operand loopReg=_Operand::createInternalReg(_Operand::TY_IR_LOOP);

    instrMov(loopReg,irsStorage);
  }

  template<const uint32_t size>
  void storageAllocationPass(uint32_t numParams)
  {
    BuddyAlloc<9> allocator(0,size);
    allocator.allocate(numParams);//reserve space for parameter

    Error::expect(codeAddr_ < 0xFFFF) << "too many instructions" << Error::FATAL;

    for(uint32_t i=0;i<codeAddr_;++i)
    {
      if(instrs_[i].isIrsInstr())
      {
        const SymbolMap::_Symbol &symInf=symbols_[instrs_[i].symRef_];

        //alloc storage if not already
        if(!symInf.flagAllocated_)
        {
          uint32_t allocSize=symInf.allocatedAddr_;

          if(!symInf.flagUseAddrAsArraySize_)
            allocSize=1;

          symInf.flagAllocated_=1;
          symInf.allocatedAddr_=allocator.allocate(allocSize);
        }

        instrs_[i].patchIrsOffset(symInf.allocatedAddr_);

        //release storage
        if(symInf.lastAccess_ == i && symInf.flagAllocated_)
        {
          symInf.flagAllocated_=0;
          allocator.release(symInf.allocatedAddr_);
        }
      }
    }
  }

protected:
  void loadOperandIntoResult(const _Operand &op)
  {
    instrMov(_Operand::createResult(),op);
  }

  uint32_t getOperandSymbolRef(const _Operand &a)
  {
    if(a.type_ == _Operand::TY_RESOLVED_SYM)
    {
      symbols_[a.mapIndex_].updateLastAccess(getCurCodeAddr());
      return a.mapIndex_;
    }

    return SymbolMap::InvalidLink;
  }

  _Operand resolveOperand(const _Operand &op,bool createSymIfNotExists=false)
  {
    if(op.type_ == _Operand::TY_SYMBOL)
    {
      Stream::String name=stream_.createStringFromToken(op.offset_,op.length_);
      uint32_t symRef=symbols_.findSymbol(name);

      if(createSymIfNotExists && symRef == SymbolMap::InvalidLink)
        symRef=symbols_.createSymbol(name,op.index_);

      Error::expect(symRef != SymbolMap::InvalidLink) << "symbol " << name << " not found";

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

  uint32_t allocateTmpStorage()
  {
    return symbols_.createSymbolNoToken(1);
  }

  uint32_t createLabel()
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

  void updateLabel(const Label &label)
  {
    labels_[label.getLabelReference()]=getCurCodeAddr();
  }

  void patchAndRemoveLabel(const Label &label,uint32_t patchAddrStart)
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

  void changeStorageSize(const TmpStorage &storage,uint32_t size) { symbols_[storage.getSymbolReference()].changeArraySize(size); }

  struct _Instr
  {
    bool isGoto() const { return true; }
    bool isIrsInstr() const { return true; }
    bool isLoadAddr() const { return true; }//is load const

    void patchIrsOffset(uint32_t irsOffset)
    {
      uint32_t irsValue=code_&0x01FF;
      code_=(code_&0xFE00)+((irsValue+irsOffset)&0x01FF);
    }

    void patchConstant(uint32_t value,bool patch2ndWord)
    {
      Error::expect(value < 512) << "load addr out of range " << (value);

      if(patch2ndWord)
        return;

      code_=(code_&0xFC00)+value;
    }

    void patchGotoTarget(int32_t target)
    {
      bool backward=target<0;

      target=backward?-target:target;
      Error::expect(target < 512) << "jump target out of range " << (target);

      code_=(code_&0xFC00)+(target&0x01FF);
      code_+=0x0200*backward;
    }

    uint16_t code_;
    uint16_t symRef_;
  };

  void writeCode(uint32_t code,uint32_t ref=SymbolMap::InvalidLink)
  {
    Error::expect(codeAddr_ < sizeof(instrs_)/sizeof(instrs_[0])) << "code buffer full" << Error::FATAL;

    instrs_[codeAddr_].code_=code;
    instrs_[codeAddr_].symRef_=ref;

    if(ref != NoRef && !instrs_[codeAddr_].isGoto())
      symbols_[ref].updateLastAccess(getCurCodeAddr());

    ++codeAddr_;
  }

  uint32_t loopDepth_;
  _LoopFrame loopFrames_[MaxLoopDepth];

  uint16_t labels_[64];
  uint16_t lastFreeLabelPos_;
  uint16_t usedRefs_;

  uint32_t codeAddr_;
  _Instr instrs_[512];
  SymbolMap symbols_;
  Stream &stream_;
};

class RTParser
{
public:
  enum {UnaryMinus=1};

  RTParser(CodeGen &codeGen) : codeGen_(codeGen)
  {
  }

  void parse(Stream &stream)
  {
    parseStatements(stream);

    Error::expect(readToken(stream).getType() == Token::TOK_EOS) << stream << "missing end of file token";
  }

  class Token
  {
  public:
    enum {TOK_VALUE,TOK_NAME,TOK_IF,TOK_ELSE,TOK_END,TOK_LOOP,TOK_CONT,TOK_BREAK,TOK_INDEX,TOK_REGA,TOK_MEM,TOK_DECL,TOK_DEF,TOK_REF,TOK_EOS};

    Token()
    {
      type_=TOK_EOS;
      index_=0;
      addrInc_=0;
    }

    Token(qfp32 value)
    {
      type_=TOK_VALUE;
      value_=value;
      index_=0;
      addrInc_=0;
    }

    Token(Stream::String symbol,uint32_t type=TOK_NAME,uint32_t index=0,bool addrInc=false)
    {
      type_=type;
      offset_=symbol.getOffset();
      length_=symbol.getLength();
      index_=index;
      addrInc_=addrInc;
    }

    uint32_t getType() const { return type_; }
    uint32_t getOffset() const { return offset_; }
    uint32_t getIndex() const { return index_; }
    uint32_t getAddrInc() const { return addrInc_; }
    uint32_t getLength() const { return length_; }
    qfp32 getValue() const { return value_; }

    Stream::String getName(Stream &stream) const { return stream.createStringFromToken(offset_,length_); }

  protected:
    uint16_t type_;
    union
    {
      struct
      {
        uint16_t offset_;
        uint16_t length_;
      };
      qfp32 value_;
    };
    uint16_t index_ : 15;
    uint16_t addrInc_ : 1;
  };

  Token readToken(Stream &stream)
  {
    stream.skipWhiteSpaces();

    std::cout<<"stream pos: "<<(stream.pos_)<<"\n";

    if(stream.empty())
      return Token();

    char ch=stream.peek();

    if((ch >= '0' && ch <= '9') || ch == '-' || ch == '.')
      return Token(stream.readQfp32());

    if(ch == '[')
    {
      stream.read();
      Token rega=readToken(stream);
      Error::expect(rega.getType() == Token::TOK_REGA) << stream << "invalid token '" << rega.getName(stream) << "': only addr register (a0/a1) can be deferenced";

      bool addrInc=false;
      if(stream.peek() == '+')
      {
        stream.read();
        Error::expect(stream.read() == '+') << stream << "expect '+' only post increment operator allowed";
        addrInc=true;
      }

      Error::expect(stream.read() == ']') << stream << "missing ']'";

      return Token(stream.createStringFromToken(rega.getOffset(),rega.getLength()),Token::TOK_MEM,rega.getOffset(),addrInc);
    }

    std::cout<<"stream pos before sym read "<<(stream.pos_)<<"\n";
    Stream::String sym=stream.readSymbol();

    Error::expect(sym.getLength() > 0) << stream << "unexpected character " << (ch);

    if(sym.getLength() == 1 && sym[0] >= 'i' && sym[0] <= 'n')
      return Token(sym,Token::TOK_INDEX,sym[0]-'i');

    if(sym.getLength() == 2)
    {
      if(sym == "if")
        return Token(sym,Token::TOK_IF);

      if(sym == "a0" || sym == "a1")
        return Token(sym,Token::TOK_REGA,sym[1]-'0');
    }

    if(sym.getLength() == 3)
    {
      if(sym == "end")
        return Token(sym,Token::TOK_END);
      if(sym == "def")
        return Token(sym,Token::TOK_DEF);
      if(sym == "ref")
        return Token(sym,Token::TOK_REF);
    }

    if(sym.getLength() == 4)
    {
      if(sym == "else")
        return Token(sym,Token::TOK_ELSE);
      if(sym == "loop")
        return Token(sym,Token::TOK_LOOP);
      if(sym == "decl")
        return Token(sym,Token::TOK_DECL);
    }

    if(sym.getLength() == 5 && sym == "break")
      return Token(sym,Token::TOK_BREAK);

    if(sym.getLength() == 8 && sym == "continue")
      return Token(sym,Token::TOK_CONT);

    uint32_t index=0xFFFF;
    if(stream.peek() == '(')//array access
    {
      stream.read();
      index=stream.readInt(false);
      Error::expect(stream.read() == ')') << stream << "missing ')'";
    }

    return Token(sym,Token::TOK_NAME,index);
  }

  _Operand parserSymbolOrConstOrMem(Stream &stream)
  {
    Token token=readToken(stream);

    if(token.getType() == Token::TOK_VALUE)
      return _Operand(token.getValue());

    if(token.getType() == Token::TOK_MEM)
      return _Operand(token.getIndex(),token.getAddrInc());

    if(token.getType() == Token::TOK_INDEX)
      return _Operand::createLoopIndex(token.getIndex());

    Error::expect(token.getType() == Token::TOK_NAME) << stream << "unexpected token '" << token.getName(stream) << "'";

    return _Operand::createSymbol(token.getName(stream),token.getIndex());
  }

  uint32_t operatorPrecedence(char op) const
  {
    if(op == '+' || op == '-')
      return 2;

    if(op == '/' || op == '*')
      return 3;

    if(op == ')')
      return 1;

    return 0;//lowest priority otherwise
  }

  bool isValuePrefix(char ch)
  {
    return ch == '-' || ch == '.' || (ch >= '0' && ch <= '9');
  }

  _Operand parseExpr(Stream &stream)
  {
    Stack<_Operand,32> operands;
    volatile char dummy=0;
    Stack<char,40> ops;

    CodeGen::TmpStorage tmpStorage(codeGen_);

    char ch;
    do
    {
      stream.skipWhiteSpaces().markPos();
      ch=stream.read();

      if(ops.top() != UnaryMinus  && ch == '-' && !isValuePrefix(stream.peek()))//is unary minus with symbol
      {
        ops.push(UnaryMinus);
        continue;
      }

      if(ch == '(')
      {
        ops.push(ch);
        continue;
      }

      stream.restorePos();

      _Operand expr=parserSymbolOrConstOrMem(stream);

      //handle combine, unary minus and bracket close
      do
      {
        stream.skipWhiteSpaces().markPos();

        ch=stream.read();

        bool neg=false;
        if(ops.top() == UnaryMinus)
        {
          ops.pop();
          neg=true;
        }

        //combine
        if(!ops.empty() && operatorPrecedence(ch) <= operatorPrecedence(ops.top()))
        {
          //reduce a+-b => a-b if possible
          if(neg == false || ops.top() == '+' || ops.top() == '-')
          {
            char op=ops.pop();

            if(neg)
              op=op=='+'?'-':'+';

            neg=false;

            _Operand a=operands.pop();
            codeGen_.instrOperation(a,expr,op,tmpStorage);
            expr=_Operand::createResult();//result operand
          }
        }

        if(neg)
        {
          codeGen_.instrNeg(expr);
          expr=_Operand::createResult();
        }

        //expr used in 'if' or 'loop' is terminated with ')' otherwise with ';'
        if(ch == ';' || (ch == ')' && (stream.peek() == '\n' || stream.peek() == ' ')))
        {
          if(!ops.empty())
          {
            stream.restorePos();
            continue;
          }
          else
            break;
        }

        if(ch == ')')
          Error::expect(ops.pop() == '(') << stream << "expect ')'";

      }while(ch == ')' || ch == ';');

      //pending operand
      ops.push(ch);

      if(!operands.empty() && operands.top().isResult())
      {
        _Operand tmp=tmpStorage.allocate();//alloc tmp storage
        codeGen_.instrMov(tmp,operands.pop());
        operands.push(tmp);
      }

      operands.push(expr);

    }while(ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(');

    stream.restorePos();

    Error::expect(operands.size() == 1) << stream << "expression not valid";

    return operands.pop();
  }

  uint32_t parseCmpMode(Stream &stream)
  {
    switch(stream.skipWhiteSpaces().read())
    {
    case '<':
      if(stream.peek() == '=')//less than or equal
      {
        stream.read();
        return CodeGen::CMP_MODE_LE;
      }
      return CodeGen::CMP_MODE_GT+CodeGen::CMP_MODE_SWAP_FLAG;
    case '>':
      if(stream.peek() == '=')//greater than or equal
      {
        stream.read();
        return CodeGen::CMP_MODE_LE+CodeGen::CMP_MODE_SWAP_FLAG;
      }
      return CodeGen::CMP_MODE_GT;
    case '!':
    {
      if(stream.read() == '=')
        return CodeGen::CMP_MODE_NEQ;
      break;
    }
    case '=':
    {
      if(stream.read() == '=')
        return CodeGen::CMP_MODE_EQ;
      break;
    }
    default:
      Error::expect(false) << stream << "invalid compare operator";
    }
    return 0;
  }

  void parseIfStatement(Stream &stream)
  {
    Error::expect(stream.read() == '(') << stream << "expect '(' after 'if'";

    CodeGen::TmpStorage tmpStorage(codeGen_);
    _Operand op[2];

    op[0]=parseExpr(stream);

    if(op[0].isResult())
    {
      _Operand tmp=tmpStorage.allocate();
      codeGen_.instrMov(tmp,op[0]);
      op[0]=tmp;
    }

    uint32_t cmpMode=parseCmpMode(stream);

    op[1]=parseExpr(stream);

    CodeGen::Label labelElse(codeGen_);
    CodeGen::Label labelEnd(codeGen_);

    codeGen_.instrCompare(op[0],op[1],cmpMode,1,true,tmpStorage);
    codeGen_.instrGoto(labelElse);

    Error::expect(stream.read() == ')') << stream << "missing ')'";

    parseStatements(stream);
    labelElse.setLabel();

    Token token=readToken(stream);

    if(token.getType() == Token::TOK_ELSE)
    {
      codeGen_.instrGoto(labelEnd);
      parseStatements(stream);
      token=readToken(stream);
    }

    Error::expect(token.getType() == Token::TOK_END) << stream << "expect 'END' token";

    labelEnd.setLabel();
  }

  void parseLoopStatement(Stream &stream)
  {
    Error::expect(stream.read() == '(') << stream << "expect '(' after 'loop'";

    _Operand op=parseExpr(stream);

    Error::expect(stream.read() == ')') << stream << "missing ')'";

    CodeGen::Label beg(codeGen_);
    CodeGen::Label cont(codeGen_);
    CodeGen::Label end(codeGen_);

    codeGen_.createLoopFrame(cont,end);

    codeGen_.instrLoop(op);

    beg.setLabel();

    parseStatements(stream);

    cont.setLabel();

    codeGen_.instrGoto(beg);

    end.setLabel();

    codeGen_.removeLoopFrame();

    Error::expect(readToken(stream).getType() == Token::TOK_END) << stream << "missing 'end' token";
  }

  bool parseStatement(Stream &stream)
  {
    stream.markPos();

    Token token=readToken(stream);

    switch(token.getType())
    {
    case Token::TOK_IF:
      parseIfStatement(stream); break;
    case Token::TOK_LOOP:
      parseLoopStatement(stream); break;
    case Token::TOK_DECL:
    {
      Token name=readToken(stream);
      assert(name.getType() == Token::TOK_NAME);
      codeGen_.addArrayDeclaration(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readInt());
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_DEF:
    {
      Token name=readToken(stream);
      assert(name.getType() == Token::TOK_NAME);
      codeGen_.addDefinition(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readQfp32());
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_REF:
    {
      Token name=readToken(stream);
      assert(name.getType() == Token::TOK_NAME);
      codeGen_.addReference(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readInt(false));
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_REGA:
    case Token::TOK_MEM:
    case Token::TOK_NAME:
    {
      Error::expect(stream.skipWhiteSpaces().read() == '=') << stream << "expect '=' operator";

      _Operand op;
      if(token.getType() == Token::TOK_REGA)
        op=_Operand::createInternalReg(token.getIndex()?_Operand::TY_IR_ADDR0:_Operand::TY_IR_ADDR1);
      if(token.getType() == Token::TOK_MEM)
        op=_Operand::createMemAccess(token.getIndex(),token.getAddrInc());
      if(token.getType() == Token::TOK_NAME)
        op=_Operand::createSymbol(token.getName(stream),token.getIndex());

      Error::expect(op.type_ != _Operand::TY_INVALID) << stream << "invalid left hand side for assignment " << (token.getName(stream));

      _Operand opExp=parseExpr(stream);

      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";

      codeGen_.instrMov(op,opExp);
      break;
    }
    default:
      stream.restorePos();//unexpected token dont remove from stream
      return false;
    }

    return true;
  }

  void parseStatements(Stream &stream)
  {
    while(parseStatement(stream));
  }

protected:
  CodeGen &codeGen_;
};


#endif /* RTPARSER_H_ */
