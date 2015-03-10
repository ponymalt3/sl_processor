/*
 * Stream.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "Stream.h"
#include <assert.h>
#include "Error.h"
#include "Token.h"
#include <algorithm>

Stream::String::String(const char *base,uint32_t offset,uint32_t length)
{
  base_=base;
  offset_=offset;
  length_=length;
}

bool Stream::String::operator==(const char *str) const
{
  for(uint32_t i=0;i<length_;++i)
  {
    if(str[i] == '\0' || str[i] != base_[offset_+i])
      return false;
  }

  return true;
}

bool Stream::String::operator==(const String &str) const
{
  for(uint32_t i=0;i<std::min((uint32_t)length_,str.getLength());++i)
  {
    if(str[i] != base_[offset_+i])
      return false;
  }

  return true;
}

uint32_t Stream::String::hash() const
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


Stream::Stream(const char *code)
{
  asmText_=code;
  pos_=0;
  line_=0;

  length_=0;
  while(code[length_] != '\0') ++length_;

  markPos_=InvalidMark;
  markLine_=line_;
}

char Stream::peek()
{
  while(asmText_[pos_] == '%')
  {
    ++pos_;
    while(pos_ < length_ && asmText_[pos_++] != '%');
    while(pos_ < length_ && asmText_[pos_] == ' ') ++pos_;
  }

  return asmText_[pos_];
}
char Stream::read()
{
  char ch=peek();
  ++pos_;
  return ch;
}

Stream& Stream::skipWhiteSpaces()
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

uint32_t Stream::readInt(bool allowSign)
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

qfp32 Stream::readQfp32()
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

Stream::String Stream::readSymbol()
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

Token Stream::readToken()
{
  skipWhiteSpaces();

  if(empty())
    return Token();

  char ch=peek();

  if((ch >= '0' && ch <= '9') || ch == '-' || ch == '.')
    return Token(readQfp32());

  if(ch == '[')
  {
    read();
    Token rega=readToken();
    Error::expect(rega.getType() == Token::TOK_REGA) << (*this) << "invalid token '" << rega.getName(*this) << "': only addr register (a0/a1) can be deferenced";

    bool addrInc=false;
    if(peek() == '+')
    {
      read();
      Error::expect(read() == '+') << (*this) << "expect '+' only post increment operator allowed";
      addrInc=true;
    }

    Error::expect(read() == ']') << (*this) << "missing ']'";

    return Token(createStringFromToken(rega.getOffset(),rega.getLength()),Token::TOK_MEM,rega.getOffset(),addrInc);
  }

  Stream::String sym=readSymbol();

  Error::expect(sym.getLength() > 0) << (*this) << "unexpected character " << (ch);

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
  if(peek() == '(')//array access
  {
    read();
    index=readInt(false);
    Error::expect(read() == ')') << (*this) << "missing ')'";
  }

  return Token(sym,Token::TOK_NAME,index);
}

Stream::String Stream::createStringFromToken(uint32_t offset,uint32_t length) const
{
  return String(asmText_,offset,length);
}

void Stream::markPos()
{
  markPos_=pos_;
  markLine_=line_;
}

void Stream::restorePos()
{
  assert(markPos_ != InvalidMark);

  pos_=markPos_;
  line_=markLine_;
  markPos_=InvalidMark;
}

uint32_t Stream::log2(int32_t value)
{
  if(value < 0)
    value=-value;

  uint32_t bits=0;
  while(value >= (1U<<bits)) ++bits;
  return bits;
}

