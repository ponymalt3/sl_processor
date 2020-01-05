/*
 * CodeGen.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include <stdint.h>

#include <map>
#include <vector>
#include <list>
#include <set>
#include <functional>

#include "Error.h"
#include "ErrorHandler.h"
#include "Alloc.h"
#include "Operand.h"
#include "SymbolMap.h"
#include "Stream.h"

#include "../SLCodeDef.h"

template<uint32_t _Size>
class SymStack
{
public:
  SymStack() { sp_=stack_; }

  void push(SymbolMap &map) { *(sp_++)=&map; }
  void pop()
  {
    if(sp_ > stack_)
    {
      --sp_;
    }
  }
  SymbolMap& top() { return *(sp_[-1]); }
  SymbolMap& get(uint32_t index) { return *(stack_[index]); }
  
  bool full() const { return size() == _Size; }
  bool empty() const { return size() == 0; }
  uint32_t size() const { return sp_-stack_; }

protected:
  SymbolMap *stack_[_Size];
  SymbolMap **sp_;
};


class CodeGen;

struct _Instr
{
  bool isGoto() const
  {
    return (code_^SLCode::Goto::Code)>>(16-SLCode::Goto::Bits) == 0 && (code_&1) == 1;
  }
  
  bool isGoto2() const
  {
    return (code_^SLCode::Goto::Code)>>(16-SLCode::Goto::Bits) == 0 && (code_&1) == 0;
  }
  
  bool isMovToIrsInstr() const
  {
    return (code_^SLCode::Mov::Code1)>>(16-SLCode::Mov::Bits1) == 0 && (code_&0x1000) != 0;
  }
  
  uint32_t getIrsOffset() const
  {
    return SLCode::IRS::getOffset(code_);
  }
  
  bool isIrsInstr() const
  {
    bool isIrs=false;
    
    isIrs=isIrs || (code_^SLCode::Mov::Code1)>>(16-SLCode::Mov::Bits1) == 0;//mov
    isIrs=isIrs || (code_^SLCode::Op::Code1)>>(16-SLCode::Op::Bits1) == 0;//ops
    isIrs=isIrs || (code_^SLCode::Cmp::Code)>>(16-SLCode::Cmp::Bits) == 0;//cmp
    
    return isIrs;
  }
  
  bool isLoadAddr() const
  {
    return (code_^SLCode::Load::Code)>>(16-SLCode::Load::Bits) == 0;
  }
  
  static qfp32_t restoreValueFromLoad(_Instr a,_Instr b={0,0},_Instr c={0,0})
  {
    if((a.code_&0xF000) == 0xB000)
    {
      uint32_t raw=((a.code_&0x7FF)<<19) + ((a.code_&0x800)<<20);
      
      if((b.code_&0xF000) == 0xB000)
      {
        raw+=((b.code_&0x7FF)<<8);
        raw+=((b.code_&0x800)<<19);
        
        if((c.code_&0xF000) == 0xB000)
        {
          raw+=(c.code_&0x0FF);
        }
      }
      
      return qfp32_t::initFromRaw(raw);
    }
    
    return qfp32_t(0);
  }

  void patchIrsOffset(uint32_t irsOffset);
  void patchConstant(uint32_t value,bool patch2ndWord);
  void patchGotoTarget(int32_t target);
  
  uint32_t getGotoTarget();
  
  uint16_t code_;
  uint16_t symRef_;
};

class Label : public Error
{
public:
  friend class CodeGen;
  friend class CodeGenDelegate;

  Label(CodeGen &codeGen);
  ~Label();

  void setLabel();
  void deleteLabel();
  
  void replaceWith(Label &otherLabel);

  uint32_t getLabelReference() const { return labelRef_; }
  uint32_t getAddr() const { return codeAddr_; }

protected:
  CodeGen &codeGen_;
  uint16_t labelRef_;
  uint16_t codeAddr_;
  uint16_t labelAddr_;
};

class TmpStorage
{
public:
  friend class CodeGen;

  TmpStorage(CodeGen &codeGen);

  _Operand allocate();
  _Operand preloadConstValue(qfp32 value);
  void preloadCode(uint32_t codeAddr,uint32_t size);
  _Operand getArrayBaseOffset();
  
protected:
  uint32_t getSymbolReference() const { return symbolRef_; }

  CodeGen &codeGen_;
  uint16_t symbolRef_;
  uint16_t size_;
  uint16_t blockBegin_;
};

class CodeGenInterface
{
public:  
  virtual void instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage) =0;
  virtual void instrMov(const _Operand &opa,const _Operand &opb) =0;
  virtual void instrUnaryOp(const _Operand &opa,uint32_t unaryOp) =0;
  virtual void instrLoop(const _Operand &opa) =0;
  virtual void instrBreak() =0;
  virtual void instrContinue() =0;
  virtual void instrGoto(const Label &label) =0;
  virtual void instrGoto2() =0;
  virtual void instrCompare(const _Operand &opa,const _Operand &opb,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage) =0;
  virtual void instrSignal(uint32_t target) =0;
  virtual void instrWait() =0;
  virtual void instrNop() =0;
};

class CodeGen : public Error, public CodeGenInterface
{
public:
  friend class Label;
  friend class TmpStorage;
  
  typedef ::Label Label;
  typedef ::TmpStorage TmpStorage;
  
  enum 
  {
    CMP_MODE_EQ=SLCode::CmpMode::CMP_EQ,
    CMP_MODE_LE=SLCode::CmpMode::CMP_LE,
    CMP_MODE_LT=SLCode::CmpMode::CMP_LT,
    CMP_MODE_NEQ=SLCode::CmpMode::CMP_NEQ,
    CMP_MODE_SWAP_FLAG=0x8000
  };
  
  enum {EXEC_MODE_1CYC,EXEC_MODE_3CYC};
  enum {MaxLoopDepth=6,LoopStorageIndex=0,NoLoopFrame=-1};
  enum {NoRef=0xFFFF,NoLabel=NoRef,RefLoad=0xFFFE,RefLabelOffset=0xFFC0};

  struct _LoopFrame
  {
    _LoopFrame();
    _LoopFrame(const Label *labCont,const Label *labBreak,const _Operand *counter);
    
    bool isComplex();
    void markComplex();

    const Label *labCont_;
    const Label *labBreak_;
    const _Operand *counter_;
    bool isComplex_;
    std::set<uint32_t> symbolRefsToRelease_;
  };
  
  struct _FunctionInfo
  {
    SymbolMap *symbols_;
    int32_t address_;
    uint32_t parameters;
    uint32_t size_;
    bool isInlineFunction_;
    struct _Inline
    {
      uint32_t parameterWrittenMap_;
      std::vector<_Instr> instrs_;
      std::vector<bool> alreadyPatched_;
    } inline_;
  };
  
  class CodeGenDelegate
  {
  public:
    friend class CodeGen;
    ~CodeGenDelegate();
    
    operator CodeGenInterface&() { return codeGen_; }
    CodeGenInterface* operator->() {  return &codeGen_; }
  protected:
    CodeGenDelegate(CodeGen &codeGen,Label &target);
    CodeGenDelegate(const CodeGenDelegate&);
    
    CodeGen &codeGen_;
    uint32_t startAddr_;
    Label &target_;
  };
  
  CodeGen(Stream &stream,uint32_t entryVectorSize=0,bool safeAllocationInsideLoop=true);
  ~CodeGen();

  void instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage);
  void instrMov(const _Operand &opa,const _Operand &opb);
  void instrUnaryOp(const _Operand &opa,uint32_t unaryOp);
  void instrLoop(const _Operand &opa);
  void instrBreak();
  void instrContinue();
  void instrGoto(const Label &label);
  void instrGoto2();
  void instrCompare(const _Operand &opa,const _Operand &opb,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage);
  void instrSignal(uint32_t target);
  void instrWait();
  void instrNop();

  void addArrayDeclaration(const Stream::String &str,uint32_t size);
  bool isArrayDeclInCurrentSymbolMap();
  void resizeArray(const Stream::String &str,uint32_t newSize);
  void addDefinition(const Stream::String &str,qfp32 value);
  void addReference(const Stream::String &str,uint32_t irsOffset);
  
  uint32_t getCurCodeAddr() const
  {
    return codeAddr_;
  }
  
  void removeInstr()
  {
    --codeAddr_;
  }

  void createLoopFrame(const Label &contLabel,const Label &breakLabel,const _Operand *counter=0);
  void removeLoopFrame();
  bool isLoopFrameComplex();

  void storageAllocationPass(uint32_t size,uint32_t numParams);
  
  uint16_t getCodeAt(uint32_t addr)
  {
    return instrs_[addr].code_;
  }
  
  SymbolMap::_Symbol findSymbol(const Stream::String &symbol)
  {
    uint32_t sym=symbolMaps_.top().findSymbol(symbol);
    
    if(sym == SymbolMap::InvalidLink)
    {
      return SymbolMap::_Symbol();
    }
    
    return symbolMaps_.top()[sym];
  }
  
  uint32_t findSymbolAsLink(const Stream::String &symbol)
  {
    return symbolMaps_.top().findSymbol(symbol);
  }
  
  CodeGen::_FunctionInfo& findFunction(const Stream::String &symbol);
  CodeGen::_FunctionInfo& addFunctionAtCurrentAddr(const Stream::String &symbol);
  
  std::vector<_Instr> extractRecentInstrs(uint32_t numInstrs);
  void appendInstrs(const std::vector<_Instr> &instrs);  
  
  void pushSymbolMap(SymbolMap &currentSymbolMap);
  void popSymbolMap();
  
  CodeGenDelegate insertCodeBefore(Label &label);
  
  void generateEntryVector(uint32_t startAddr);
  void generateEntryVector(uint32_t numberOfEntries,uint32_t entrySizeInInstrs);

protected:
  
  void setCodeMovedCallback(const std::function<void(uint32_t,uint32_t,uint32_t)> &callback);
  
  const SymbolMap& getDefaultSymbols() const { return defaultSymbols_; }
  const std::map<std::string,_FunctionInfo>& getFunctions() const { return functions_; }
  
  _Operand resolveOperand(const _Operand &op,bool createSymIfNotExists=false);
  
  SLCode::Operand translateOperand(_Operand op);
  SLCode::Command translateOperation(char op);

  uint32_t allocateTmpStorage();
  void changeStorageSize(const TmpStorage &storage,uint32_t size);

  uint32_t getLabelId(Label* label);
  void releaseLabel(const Label &label);
  void patchLabelInCode(const Label &label,uint32_t patchAddrStart);
  void replaceLabel(const Label &labelToBeReplced,const Label &newLabel);

  void writeCode(uint32_t code,uint32_t ref=SymbolMap::InvalidLink);
  void moveCodeBlock(uint32_t startAddr,uint32_t size,uint32_t targetAddr);
  void rebaseCode(uint32_t startAddr,uint32_t endAddr,int32_t offset);

  uint32_t loopDepth_;
  _LoopFrame loopFrames_[MaxLoopDepth];

  uint32_t labelIdBitMap_;
  Label* activeLabels_[32];

  uint32_t codeAddr_;
  _Instr instrs_[4096];
  
  uint32_t entryVectorSize_;
  
  std::multimap<uint32_t,uint32_t> arrayAllocInfo_;
  
  std::function<void(uint32_t,uint32_t,uint32_t)> callback_;
  
  SymStack<4> symbolMaps_;
  Stream &stream_;
  std::map<std::string,_FunctionInfo> functions_;
  SymbolMap defaultSymbols_;
  
  //settings
  bool safeAllocationInsideLoop_;
};

#endif /* CODEGEN_H_ */
