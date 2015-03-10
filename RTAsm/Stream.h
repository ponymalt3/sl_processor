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

struct qfp32
{
  uint32_t mant_ : 29;
  uint32_t exp_ : 2;
  uint32_t sign_ : 1;
};

class Token;

class Stream
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

  protected:
    const char *base_;
    uint16_t offset_;
    uint16_t length_;
  };

  Stream(const char *code);

  char peek();
  char read();

  bool empty() const { return pos_ >= length_; }
  uint32_t getCurrentLine() const { return line_; }

  Stream& skipWhiteSpaces();

  uint32_t readInt(bool allowSign=true);
  qfp32 readQfp32();
  String readSymbol();
  Token readToken();
  String createStringFromToken(uint32_t offset,uint32_t length) const;

  void markPos();
  void restorePos();

protected:
  static uint32_t log2(int32_t value);

  const char *asmText_;
  uint32_t pos_;
  uint32_t line_;
  uint32_t length_;
  uint32_t markPos_;
  uint32_t markLine_;
};

#endif /* STREAM_H_ */