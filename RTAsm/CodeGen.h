/*
 * CodeGen.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include <stdint.h>

#include "Error.h"
#include "ErrorHandler.h"
#include "BuddyAlloc.h"
#include "Operand.h"
#include "SymbolMap.h"

#include "../SLCodeDef.h"

class CodeGen : public Error
{
public:
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
  enum {NoRef=0xFFFF,NoLabel=NoRef};

  class Label : public Error
  {
  public:
    friend class CodeGen;

    Label(CodeGen &codeGen);
    ~Label();

    void setLabel();
    void deleteLabel();

  protected:
    uint32_t getLabelReference() const { return labelRef_; }

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

  protected:
    uint32_t getSymbolReference() const { return symbolRef_; }

    CodeGen &codeGen_;
    uint16_t symbolRef_;
    uint16_t size_;
  };

  struct _LoopFrame
  {
    _LoopFrame();
    _LoopFrame(const Label *labCont,const Label *labBreak);

    const Label *labCont_;
    const Label *labBreak_;
  };

  CodeGen(Stream &stream);

  void instrOperation(const _Operand &opa,const _Operand &opb,uint32_t op,TmpStorage &tmpStorage);
  void instrMov(const _Operand &opa,const _Operand &opb);
  void instrNeg(const _Operand &opa);
  void instrLoop(const _Operand &opa);
  void instrBreak();
  void instrContinue();
  void instrGoto(const Label &label);
  void instrCompare(const _Operand &opa,const _Operand &opb,uint32_t cmpMode,uint32_t execMode,bool negate,TmpStorage &tmpStorage);
  void instrSignal(uint32_t target);
  void instrWait();

  void addArrayDeclaration(const Stream::String &str,uint32_t size);
  void addDefinition(const Stream::String &str,qfp32 value);
  void addReference(const Stream::String &str,uint32_t irsOffset);

  uint32_t getCurCodeAddr() const
  {
    return codeAddr_;
  }

  void createLoopFrame(const Label &contLabel,const Label &breakLabel);
  void removeLoopFrame();

  template<const uint32_t size>
  void storageAllocationPass(uint32_t numParams)
  {
    BuddyAlloc<9> allocator(0,size);
    
    if(numParams > 0)
    {
      allocator.allocate(numParams);//reserve space for parameter
    }

    Error::expect(codeAddr_ < 0xFFFF) << "too many instructions" << ErrorHandler::FATAL;

    for(uint32_t i=0;i<codeAddr_;++i)
    {
      if(instrs_[i].isIrsInstr())
      {
        SymbolMap::_Symbol &symInf=symbols_[instrs_[i].symRef_];

        //alloc storage if not already
        if(!symInf.flagAllocated_)
        {
          symInf.flagAllocated_=1;
          symInf.allocatedAddr_=allocator.allocate(symInf.allocatedSize_);
        }

        instrs_[i].patchIrsOffset(symInf.allocatedAddr_);

        //release storage
        if(symInf.lastAccess_ == i && symInf.flagAllocated_ && !symInf.flagStayAllocated_)
        {
          symInf.flagAllocated_=0;
          allocator.release(symInf.allocatedAddr_,symInf.allocatedSize_);
        }
      }
    }
  }
  
  uint16_t getCodeAt(uint32_t addr)
  {
    return instrs_[addr].code_;
  }
  
  SymbolMap::_Symbol findSymbol(const Stream::String &symbol)
  {
    uint32_t sym=symbols_.findSymbol(symbol);
    
    if(sym == SymbolMap::InvalidLink)
    {
      return SymbolMap::_Symbol();
    }
    
    return symbols_[sym];
  }

protected:

  _Operand resolveOperand(const _Operand &op,bool createSymIfNotExists=false);
  
  SLCode::Operand translateOperand(_Operand op);
  SLCode::Command translateOperation(char op);

  uint32_t allocateTmpStorage();
  void changeStorageSize(const TmpStorage &storage,uint32_t size);

  uint32_t getLabelId();
  void patchAndReleaseLabelId(const Label &label,uint32_t patchAddrStart);

  void writeCode(uint32_t code,uint32_t ref=SymbolMap::InvalidLink);

  struct _Instr
  {
    bool isGoto() const
    {
      return (code_^SLCode::Goto::Code)>>(16-SLCode::Goto::Bits) == 0;
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

    void patchIrsOffset(uint32_t irsOffset);
    void patchConstant(uint32_t value,bool patch2ndWord);
    void patchGotoTarget(int32_t target);

    uint16_t code_;
    uint16_t symRef_;
  };

  uint32_t loopDepth_;
  _LoopFrame loopFrames_[MaxLoopDepth];

  //uint16_t labels_[64];
  uint32_t labelIdBitMap_;
  uint16_t usedRefs_;

  uint32_t codeAddr_;
  _Instr instrs_[512];
  SymbolMap symbols_;
  Stream &stream_;
};

#endif /* CODEGEN_H_ */
