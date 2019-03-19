/*
 * Stream.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef STREAM_H_
#define STREAM_H_

#include <stdint.h>
//#include "Token.h"
#include "Error.h"
#include "qfp32.h" 

struct qfp32
{  
  uint32_t mant_ : 29;
  uint32_t exp_ : 2;
  uint32_t sign_ : 1;
  
  static qfp32 fromRealQfp32(_qfp32_t v) { return {v.getMant(),v.getExp(),v.getSign()}; }
  _qfp32_t toRealQfp32() const { return _qfp32_t(sign_,exp_,mant_); }
  
  qfp32 operator+(const qfp32 &rhs) const { return fromRealQfp32(toRealQfp32()+rhs.toRealQfp32()); }
  qfp32 operator-(const qfp32 &rhs) const { return fromRealQfp32(toRealQfp32()-rhs.toRealQfp32()); }
  qfp32 operator*(const qfp32 &rhs) const { return fromRealQfp32(toRealQfp32()*rhs.toRealQfp32()); }
  qfp32 operator/(const qfp32 &rhs) const { return fromRealQfp32(toRealQfp32()/rhs.toRealQfp32()); }
  
  qfp32 log2() const { return fromRealQfp32(toRealQfp32().log2()); }
  qfp32 trunc() const { return fromRealQfp32(toRealQfp32().trunc()); }
  qfp32 logicShift(const qfp32 &rhs) const { return fromRealQfp32(toRealQfp32().logicShift(rhs.toRealQfp32())); }
};

class Token;
class RTProg;

class Stream : public Error
{
public:
  enum {InvalidMark=0xFFFFFFFF};

  class String
  {
  public:
    friend class Error;

    String(const char *base,uint32_t offset,uint32_t length);

    bool operator==(const char *str) const;
    bool operator==(const String &str) const;

    uint32_t hash() const;

    char operator[](uint32_t index) const { return base_[offset_+index]; }

    uint32_t getOffset() const { return offset_; }
    uint32_t getLength() const { return length_; }
    const char* getBase() const { return base_; }

  protected:
    const char *base_;
    uint16_t offset_;
    uint16_t length_;
  };

  Stream(RTProg &rtProg);//const char *code);

  char peek();
  char read();

  bool empty() const { return pos_ >= length_; }
  uint32_t getCurrentLine() const { return line_; }

  Stream& skipWhiteSpaces();
  
  struct value_t
  {
    uint32_t value_;
    uint32_t digits_;
    bool sign_;
  };

  value_t readInt(bool allowSign=true);
  qfp32 readQfp32();
  String readSymbol();
  Token readToken();
  String createStringFromToken(uint32_t offset,uint32_t length) const;
  String createStringFromToken(const char *base,uint32_t length) const;

  void markPos();
  void restorePos();

  static uint32_t log2(int32_t value);

protected:
  char toLowerCase(char ch);
  
  const char *asmText_;  
  uint32_t pos_;
  uint32_t line_;
  uint32_t length_;
  uint32_t markPos_;
  uint32_t markLine_;
};

#endif /* STREAM_H_ */
