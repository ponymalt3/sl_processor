/*
 * Token.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef TOKEN_H_
#define TOKEN_H_

#include <stdint.h>
#include "Stream.h"

class Token
{
public:
  enum
  {
    TOK_VALUE,
    TOK_NAME,
    TOK_IF,
    TOK_ELSE,
    TOK_END,
    TOK_LOOP,
    TOK_CONT,
    TOK_BREAK,
    TOK_INDEX,
    TOK_REGA,
    TOK_MEM,
    TOK_DECL,
    TOK_DEF,
    TOK_REF,
    TOK_FCN_DECL,
    TOK_FCN_RET,
    TOK_EOS
  };

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

  Token(const Stream::String &symbol,uint32_t type=TOK_NAME,uint32_t index=0,bool addrInc=false)
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

#endif /* TOKEN_H_ */
