/*
 * Operand.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef OPERAND_H_
#define OPERAND_H_

#include "Stream.h"
#include <assert.h>
#include <stdint.h>

struct _Operand
{
  enum Type {TY_INVALID,TY_VALUE,TY_MEM,TY_SYMBOL,TY_RESULT,TY_INDEX,TY_IR_IRS,TY_IR_ADDR0,TY_IR_ADDR1,TY_RESOLVED_SYM};
  enum InternalReg {IR_ADR0,IR_ADR1,IR_IRS};

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
    ref_=0xFFFF;
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

  static _Operand createSymAccess(uint32_t mapIndex,uint32_t index=0)
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
  
  static _Operand createConstWithRef(uint16_t ref,uint32_t loadsToGenerate=2)
  {
    float v=10.0f*(1<<(12*(loadsToGenerate-1)));
    _Operand op(qfp32::fromRealQfp32(v));//need 2 loads
    op.ref_=ref;
    return op;
  }

  bool isResult() const { return type_ == TY_RESULT; }
  bool isArrayBaseAddr() const { return type_ == TY_RESOLVED_SYM && arrayOffset_ == 0xFFFF; }
  bool isInternalReg() const { return type_ == TY_IR_IRS || type_ == TY_IR_ADDR0 || type_ == TY_IR_ADDR1; }
  bool isMemAccess() const { return type_ == TY_MEM; }
  bool isMemAccessWithInc() const { return type_ == TY_MEM && addrInc_ != 0; }

  uint16_t type_;
  union
  {
    struct
    {
      qfp32 value_;
      uint16_t ref_;
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

#endif /* OPERAND_H_ */
