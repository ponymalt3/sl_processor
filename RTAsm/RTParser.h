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
 * stment := symbol ('=' | '+=') exp |
 *           if |
 *           loop |
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
 * const := number
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

  void parse(Stream &stream);

  _Operand parserSymbolOrConstOrMem(Stream &stream,CodeGen::TmpStorage &tmpStorage);
  uint32_t operatorPrecedence(char op) const;
  bool isValuePrefix(char ch) const;
  _Operand parseExpr(Stream &stream);
  uint32_t parseCmpMode(Stream &stream);
  void parseIfStatement(Stream &stream);
  void parseIfExp(Stream &stream,CodeGen::Label &labelThen,CodeGen::Label &labelElse);
  void parseLoopStatement(Stream &stream);
  bool parseStatement(Stream &stream);
  void parseStatements(Stream &stream);
  _Operand parseFunctionCall(Stream &stream,const Stream::String &name);
  void parseFunctionDecl(Stream &stream);

protected:
  CodeGen &codeGen_;
  uint32_t startAddr_;
};

#endif /* RTPARSER_H_ */
