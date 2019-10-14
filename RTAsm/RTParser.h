/*
 * RTParser.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef RTPARSER_H_
#define RTPARSER_H_

#include <stdint.h>
#include "Stream.h"
#include "CodeGen.h"
#include "Operand.h"
#include "Token.h"

/*
 * exp := const exp' |
 *        symbol exp'|
 *        'sizeof' '(' symbol ')' |
 *        'log2' '(' exp ')' |
 *        'int' '(' exp ')' |
 *        'shft' '(' exp ',' exp ')' |
 *        '(' exp ')';
 *
 * exp' := e |
 *         ('+' | '-' | '*' | '/') exp;
 *
 * if := 'if' '(' ifexp ')' stments ifelse 'end'
 * 
 * ifexp := exp ('>' | '<' | '>=' | '<=' | '==' | '!=') exp ( { 'and' ifexp } | { 'or' ifexp } ) |
 *          '(' ifexp ')' ( { 'and' ifexp } | { 'or' ifexp } )
 *
 * ifelse := e |
 *           'else' stments;
 *
 * loop := 'loop' '(' exp ')' stments 'end';
 * 
 * while := 'while' '(' ifexp ')' stments 'end';
 *
 * stment := symbol '=' exp |
 *           if |
 *           loop |
 *           while |
 *           array |
 *           ('decl' | 'array')  name int |
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
 * const := number | hex
 * 
 * array := name '{' exp { ',' exp } '}';
 *
 */


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

class RTParser : Error
{
public:
  enum {UnaryMinus=1};

  RTParser(CodeGen &codeGen);
  
  const std::map<uint32_t,uint32_t>& getLineMapping() const;

  void parse(Stream &stream,uint32_t inlineFunctionThreshold=0);

  _Operand parserSymbolOrConstOrMem(Stream &stream,CodeGen::TmpStorage &tmpStorage);
  uint32_t operatorPrecedence(char op) const;
  bool isValuePrefix(char ch) const;
  _Operand parseExpr(Stream &stream);
  uint32_t parseCmpMode(Stream &stream);
  void parseIfStatement(Stream &stream);
  void parseIfExp(Stream &stream,CodeGen::Label &labelThen,CodeGen::Label &labelElse);
  void parseLoopStatement(Stream &stream);
  void parseWhileStatement(Stream &stream);
  bool parseStatement(Stream &stream);
  void parseStatements(Stream &stream);
  _Operand parseFunctionCall(Stream &stream,const Stream::String &name);
  void parseFunctionDecl(Stream &stream);
  void parseArrayDecl(Stream &stream,const Stream::String &name);

protected:
  enum {NonMovableLineFlag=0x80000000};
  
  void streamCallback(uint32_t line,bool invalidate);
  void codeMovedCallback(uint32_t startAddr,uint32_t size,uint32_t targetAddr);
  
  void markLineAsNoMovable();

  CodeGen &codeGen_;
  uint32_t startAddr_;
  uint32_t inlineFunctionThreshold_;
  std::map<uint32_t,uint32_t> codeTolineMapping_;
};

#endif /* RTPARSER_H_ */
