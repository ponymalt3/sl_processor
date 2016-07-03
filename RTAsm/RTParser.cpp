/*
 * RTParser.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "RTParser.h"
#include "Token.h"


RTParser::RTParser(CodeGen &codeGen) : Error(codeGen.getErrorHandler()), codeGen_(codeGen)
{
}

void RTParser::parse(Stream &stream)
{
  parseStatements(stream);

  Error::expect(stream.readToken().getType() == Token::TOK_EOS) << stream << "missing end of file token";
}

_Operand RTParser::parserSymbolOrConstOrMem(Stream &stream)
{
  Token token=stream.readToken();

  if(token.getType() == Token::TOK_VALUE)
    return _Operand(token.getValue());

  if(token.getType() == Token::TOK_MEM)
    return _Operand(token.getIndex(),token.getAddrInc());

  if(token.getType() == Token::TOK_INDEX)
    return _Operand::createLoopIndex(token.getIndex());

  Error::expect(token.getType() == Token::TOK_NAME) << stream << "unexpected token '" << token.getName(stream) << "'";

  return _Operand::createSymbol(token.getName(stream),token.getIndex());
}

uint32_t RTParser::operatorPrecedence(char op) const
{
  if(op == '+' || op == '-')
    return 20;

  if(op == '/' || op == '*')
    return 30;
    
  if(op == UnaryMinus)
    return 40;
    
  if(op == '(' || op == ')')
    return 10;

  return 0;//lowest priority otherwise
}

bool RTParser::isValuePrefix(char ch) const
{
  return ch == '-' || ch == '.' || (ch >= '0' && ch <= '9');
}

_Operand RTParser::parseExpr(Stream &stream)
{
  Stack<_Operand,32> operands;
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

    stream.skipWhiteSpaces();//.markPos();
    ch=stream.peek();

    bool neg=false;
    
    //combine
    while(!ops.empty() && operatorPrecedence(ch) <= operatorPrecedence(ops.top()))
    {            
      //handle brackets
      if(ops.top() == '(')
      {
        Error::expect(ch == ')') << stream << "expect missing ')'";
        
        ops.pop();
        stream.read();
        stream.skipWhiteSpaces();
        ch=stream.peek();
        continue;
      }
      
      if(ops.top() == UnaryMinus)
      {
        ops.pop();
        neg=true;
      }
      
      //reduce a+-b => a-b if possible
      if(!ops.empty() && (neg == false || ops.top() == '+' || ops.top() == '-'))
      {
        char op=ops.pop();

        if(neg)
          op=op=='+'?'-':'+';

        neg=false;

        _Operand a=operands.pop();
        
        //const only operation => no instr needed
        if(a.type_ == _Operand::TY_VALUE && expr.type_ == _Operand::TY_VALUE)
        {
          switch(op)
          {
            case '+': expr=_Operand(a.value_+expr.value_); break;
            case '-': expr=_Operand(a.value_-expr.value_); break; 
            case '*': expr=_Operand(a.value_*expr.value_); break;
            case '/': expr=_Operand(a.value_/expr.value_); break; 
            default:
              Error::expect(false) << stream << "unkown operator '" << (op) << "'";
          }
        }
        else
        {
          codeGen_.instrOperation(a,expr,op,tmpStorage);
          expr=_Operand::createResult();//result operand
        }
      }
      
      if(neg)
      {
        codeGen_.instrNeg(expr);
        expr=_Operand::createResult();
        neg=false;
      }
    }
    
    //discard cause already in ch
    stream.skipWhiteSpaces().markPos();
    stream.read();      

    //pending (valid) operand
    //if(ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(')
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

uint32_t RTParser::parseCmpMode(Stream &stream)
{
  switch(stream.skipWhiteSpaces().read())
  {
  case '<':
    if(stream.peek() == '=')//less than or equal
    {
      stream.read();
      return CodeGen::CMP_MODE_LE;
    }
    return CodeGen::CMP_MODE_LT;
  case '>':
    if(stream.peek() == '=')//greater than or equal
    {
      stream.read();
      return CodeGen::CMP_MODE_LE+CodeGen::CMP_MODE_SWAP_FLAG;
    }
    return CodeGen::CMP_MODE_LT+CodeGen::CMP_MODE_SWAP_FLAG;
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

void RTParser::parseIfStatement(Stream &stream)
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

  Token token=stream.readToken();

  if(token.getType() == Token::TOK_ELSE)
  {
    codeGen_.instrGoto(labelEnd);
    labelElse.setLabel();//move label
    
    parseStatements(stream);
    token=stream.readToken();
  }

  Error::expect(token.getType() == Token::TOK_END) << stream << "expect 'END' token";

  labelEnd.setLabel();
}

void RTParser::parseLoopStatement(Stream &stream)
{
  Error::expect(stream.read() == '(') << stream << "expect '(' after 'loop'";

  _Operand op=parseExpr(stream);

  Error::expect(stream.read() == ')') << stream << "missing ')'";

  CodeGen::Label beg(codeGen_);
  CodeGen::Label cont(codeGen_);
  CodeGen::Label end(codeGen_);
  
  CodeGen::TmpStorage storage(codeGen_);
  
  _Operand destLoopCount=op;
  if(destLoopCount.type_ != _Operand::TY_SYMBOL)
  {
    destLoopCount=storage.allocate();
    codeGen_.instrMov(destLoopCount,op);
  }  
  
  _Operand loopCounter=storage.allocate();
  codeGen_.instrMov(loopCounter,_Operand(qfp32::fromUint32(qfp32_t(0).asUint)));
  
  codeGen_.createLoopFrame(cont,end,loopCounter);

  beg.setLabel();

  parseStatements(stream);

  cont.setLabel();
  
  //inc counter
  codeGen_.instrOperation(_Operand(qfp32::fromUint32(qfp32_t(1).asUint)),loopCounter,'+',storage);
  codeGen_.instrMov(loopCounter,_Operand::createResult());
  
  //compare
  codeGen_.instrCompare(_Operand::createResult(),destLoopCount,CodeGen::CMP_MODE_LT,1,false,storage);
  codeGen_.instrGoto(beg);

  end.setLabel();

  codeGen_.removeLoopFrame();

  Error::expect(stream.readToken().getType() == Token::TOK_END) << stream << "missing 'end' token";
}

bool RTParser::parseStatement(Stream &stream)
{
  stream.markPos();

  Token token=stream.readToken();

  switch(token.getType())
  {
  case Token::TOK_IF:
    parseIfStatement(stream); break;
  case Token::TOK_LOOP:
    parseLoopStatement(stream); break;
  case Token::TOK_CONT:
  {
    codeGen_.instrContinue();
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
    break;
  }
  case Token::TOK_BREAK:
  {
    codeGen_.instrBreak();
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
    break;
  }
  case Token::TOK_DECL:
  {
    Token name=stream.readToken();
    assert(name.getType() == Token::TOK_NAME);
    codeGen_.addArrayDeclaration(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readInt(false).value_);
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
    break;
  }
  case Token::TOK_DEF:
  {
    Token name=stream.readToken();
    assert(name.getType() == Token::TOK_NAME);
    codeGen_.addDefinition(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readQfp32());
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
    break;
  }
  case Token::TOK_REF:
  {
    Token name=stream.readToken();
    assert(name.getType() == Token::TOK_NAME);
    codeGen_.addReference(stream.createStringFromToken(name.getOffset(),name.getLength()),stream.readInt(false).value_);
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
      op=_Operand::createInternalReg(token.getIndex()?_Operand::TY_IR_ADDR1:_Operand::TY_IR_ADDR0);
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

void RTParser::parseStatements(Stream &stream)
{
  while(parseStatement(stream));
}
