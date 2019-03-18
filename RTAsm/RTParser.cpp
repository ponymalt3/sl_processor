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
  startAddr_=0;
}

void RTParser::parse(Stream &stream)
{
  startAddr_=0;
  
  try
  {
    static const char irsSym[]="__IRS__";
    codeGen_.addReference(Stream::String(irsSym,0,7),0);
    static const char retSym[]="__RET__";
    codeGen_.addReference(Stream::String(retSym,0,7),1);
    static const char resSym[]="__RES__";
    codeGen_.addReference(Stream::String(resSym,0,7),2);
    static const char irsRestSym[]="__IRS_REST__";
    codeGen_.addReference(Stream::String(irsRestSym,0,12),3);
    
    parseStatements(stream);
    Error::expect(stream.readToken().getType() == Token::TOK_EOS) << stream << "missing end of file token";
  }
  catch(const std::runtime_error &e)
  {
    Error::expect(std::string(e.what()) == "fatal error");
  }
  
  codeGen_.generateEntryVector(startAddr_);
}

_Operand RTParser::parserSymbolOrConstOrMem(Stream &stream,CodeGen::TmpStorage &tmpStorage)
{
  Token token=stream.readToken();

  if(token.getType() == Token::TOK_VALUE)
    return _Operand(token.getValue());

  if(token.getType() == Token::TOK_MEM)
    return _Operand(token.getIndex(),token.getAddrInc());

  if(token.getType() == Token::TOK_INDEX)
    return _Operand::createLoopIndex(token.getIndex());

  Error::expect(token.getType() == Token::TOK_NAME) << stream << "unexpected token " << token.getName(stream);
  
  //check for intrinsic functions
  if(token.getName(stream) == "log2" || token.getName(stream) == "int" || token.getName(stream) == "shft")
  {
    Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for function call";
    
    _Operand result=_Operand::createResult();
    _Operand param=parseExpr(stream);
  
    if(token.getName(stream) == "shft")
    {
      Error::expect(stream.skipWhiteSpaces().read() == ',') << stream << "expect two parameter for shft function";
      _Operand param2=parseExpr(stream);
      
      if(param.type_ == _Operand::TY_VALUE && param2.type_ == _Operand::TY_VALUE)
      {
        result=_Operand(param.value_.logicShift(param2.value_));
      }
      else
      {
        TmpStorage tmp(codeGen_);
        codeGen_.instrOperation(param,param2,SLCode::CMD_SHFT,tmp);
      }
    }
    else if(token.getName(stream) == "log2")
    {
      if(param.type_ == _Operand::TY_VALUE)
      {
        result=_Operand(param.value_.log2());
      }
      else
      {
        codeGen_.instrUnaryOp(param,SLCode::UNARY_LOG2);
      }
    }
    else if(token.getName(stream) == "int")
    {
      if(param.type_ == _Operand::TY_VALUE)
      {
        result=_Operand(param.value_.trunc());
      }
      else
      {
        codeGen_.instrUnaryOp(param,SLCode::UNARY_TRUNC);
      }
    }
    
    Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "expect ')'";
    
    return result;
  }
  
  if(codeGen_.findFunction(token.getName(stream)).flagsIsFunction_)
  {
    uint32_t codeBlockStart=codeGen_.getCurCodeAddr();
    _Operand result=parseFunctionCall(stream,token.getName(stream));
    //move function call in front of exprs
    tmpStorage.preloadCode(codeBlockStart,codeGen_.getCurCodeAddr()-codeBlockStart);
 
    return result;
  }
  
  uint32_t index=0xFFFF;
  if(stream.skipWhiteSpaces().peek() == '(')
  {
    stream.read();
    index=stream.readInt(false).value_;
    Error::expect(stream.read() == ')') << (stream) << "missing ')'";
  }

  return _Operand::createSymbol(token.getName(stream),index);
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

    _Operand expr=parserSymbolOrConstOrMem(stream,tmpStorage);

    stream.skipWhiteSpaces();
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
        if(expr.type_ == _Operand::TY_VALUE)
        {
          expr=_Operand(expr.value_*qfp32::fromRealQfp32(-1.0));
        }
        else
        {
          codeGen_.instrUnaryOp(expr,SLCode::UNARY_NEG);
          expr=_Operand::createResult();
        }
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
    break;  
  }
  
  Error::expect(false) << stream << "invalid compare operator";
  
  return 0;
}

void RTParser::parseIfExp(Stream &stream,CodeGen::Label &labelThen,CodeGen::Label &labelElse)
{
  Token mode=Token();
  bool lastCond=false;
  
  while(!lastCond)
  {
    //check for bracket: '(' exp <=/>=/==/!=/</> ...
    if(stream.skipWhiteSpaces().peek() == '(')
    {
      stream.markPos();
      int32_t bracketOpenCount=0;
      for(;;)
      {
        if(!stream.empty())
        {
          switch(stream.read())
          {
            case '(': ++bracketOpenCount; continue;
            case ')': --bracketOpenCount; continue;
            case '=':
            case '<':
            case '>':
            case '!':
              break;
            default:
              continue;
          }
        }
        break;
      }  
      stream.restorePos();
        
      if(bracketOpenCount > 0)
      {
        stream.read();
        CodeGen::Label localThen(codeGen_);
        CodeGen::Label localElse(codeGen_);
        parseIfExp(stream,localThen,localElse);
        Error::expect(stream.read() == ')') << stream << "missing ')'";
        
        lastCond=stream.peek() == ')';
    
        if(!lastCond)
        {
          Token tok=stream.readToken();
          Error::expect(tok.getType() == Token::TOK_IF_AND || tok.getType() == Token::TOK_IF_OR) << stream << "only or/and operator is allowed in condition";    
          Error::expect(mode.getType() == Token::TOK_EOS || mode.getType() == tok.getType()) << stream << "need to use brackets when mxing or/and";
          mode=tok;
        }
        
        if(!lastCond && mode.getType() == Token::TOK_IF_OR)
        {
          localThen.replaceWith(labelThen);//is not called by "and"-operators eg ... or (a == b and c == d) or ...
          codeGen_.instrGoto(labelThen);
          localElse.setLabel();
        }
        else
        {
          localThen.setLabel();
          localElse.replaceWith(labelElse);
        }
        
        continue;
      }
    }

    CodeGen::TmpStorage tmpStorage(codeGen_);
    _Operand op[2];

    op[0]=parseExpr(stream);

    uint32_t cmpMode=parseCmpMode(stream);

    op[1]=parseExpr(stream);
    
    lastCond=stream.peek() == ')';
    
    if(!lastCond)
    {
      Token tok=stream.readToken();
      Error::expect(tok.getType() == Token::TOK_IF_AND || tok.getType() == Token::TOK_IF_OR) << stream << "only or/and operator is allowed in condition";    
      Error::expect(mode.getType() == Token::TOK_EOS || mode.getType() == tok.getType()) << stream << "need to use brackets when mxing or/and";
      mode=tok;
    }
    
    if(mode.getType() == Token::TOK_IF_OR && !lastCond)
    {
      codeGen_.instrCompare(op[0],op[1],cmpMode,1,false,tmpStorage);
      codeGen_.instrGoto(labelThen);
    }
    else
    {
      codeGen_.instrCompare(op[0],op[1],cmpMode,1,true,tmpStorage);
      codeGen_.instrGoto(labelElse);
    }
  }
}

void RTParser::parseIfStatement(Stream &stream)
{
  Error::expect(stream.read() == '(') << stream << "expect '(' after 'if'";

  CodeGen::Label labelThen(codeGen_);
  CodeGen::Label labelElse(codeGen_);
  CodeGen::Label labelEnd(codeGen_);
  
  parseIfExp(stream,labelThen,labelElse);

  Error::expect(stream.read() == ')') << stream << "missing ')'";
  
  labelThen.setLabel();

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
  
  //create loop frame with storage for loopCounter 
  _Operand loopCounter=storage.allocate();
  codeGen_.createLoopFrame(cont,end,loopCounter);

  beg.setLabel();

  parseStatements(stream);
  
  if(codeGen_.isLoopFrameComplex() == false)
  {
    //use more efficient loop    
    codeGen_.insertCodeBefore(beg)->instrLoop(op);    
    cont.setLabel();
    codeGen_.instrGoto(beg);
  }
  else
  {
    //standard loop; create loop counter and generate initializing
    
    _Operand destLoopCount=op;
    if(destLoopCount.type_ != _Operand::TY_SYMBOL)
    {
      destLoopCount=storage.allocate();
      codeGen_.insertCodeBefore(beg)->instrMov(destLoopCount,op);
    }
  
    codeGen_.insertCodeBefore(beg)->instrMov(loopCounter,_Operand(qfp32::fromRealQfp32(_qfp32_t(0))));
    
    cont.setLabel();
  
    //inc counter
    codeGen_.instrOperation(_Operand(qfp32::fromRealQfp32(qfp32_t(1))),loopCounter,'+',storage);
    codeGen_.instrMov(loopCounter,_Operand::createResult());
  
    //compare
    codeGen_.instrCompare(_Operand::createResult(),destLoopCount,CodeGen::CMP_MODE_LT,1,false,storage);
    codeGen_.instrGoto(beg);
  }

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
    int32_t value=stream.readInt(false).value_;
    Error::expect(value >= 4) << stream << "irs addresses 0-3 are reserved for callstack management";
    codeGen_.addReference(stream.createStringFromToken(name.getOffset(),name.getLength()),value);
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
    break;
  }
  case Token::TOK_REGA:
  case Token::TOK_MEM:
  case Token::TOK_NAME:
  {    
    _Operand op;
    
    if(token.getType() == Token::TOK_NAME)
    {
      if(codeGen_.findFunction(token.getName(stream)).flagsIsFunction_)
      {
        parseFunctionCall(stream,token.getName(stream));
        Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
        break;
      }
      
      uint32_t index=0xFFFF;
      if(stream.skipWhiteSpaces().peek() == '(')
      {
        stream.read();
        index=stream.readInt(false).value_;
        Error::expect(stream.read() == ')') << (stream) << "missing ')'";
      }
      
      op=_Operand::createSymbol(token.getName(stream),index);
    }
      
    if(token.getType() == Token::TOK_REGA)
      op=_Operand::createInternalReg(token.getIndex()?_Operand::TY_IR_ADDR1:_Operand::TY_IR_ADDR0);
    if(token.getType() == Token::TOK_MEM)
      op=_Operand::createMemAccess(token.getIndex(),token.getAddrInc());

    Error::expect(op.type_ != _Operand::TY_INVALID) << stream << "invalid left hand side for assignment " << (token.getName(stream));
    Error::expect(stream.skipWhiteSpaces().read() == '=') << stream << "expect '=' operator";
  
    _Operand opExp=parseExpr(stream);

    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";

    codeGen_.instrMov(op,opExp);
    break;
  }
  case Token::TOK_FCN_DECL:
    parseFunctionDecl(stream);
    startAddr_=codeGen_.getCurCodeAddr();    
    break;
  case Token::TOK_FCN_RET:
    if(stream.skipWhiteSpaces().peek() != ';')
    {
      _Operand result=parseExpr(stream);
      codeGen_.instrMov(_Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__RES__",0,7))),result);      
    }
    
    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      
    codeGen_.instrMov(_Operand::createResult(),_Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__RET__",0,7))));
    codeGen_.instrGoto2();
    break;
  default:
    Error::expect(token.getType() != Token::TOK_INDEX) << stream << "cant write/use " << token.getName(stream) << " loop index";
    stream.restorePos();//unexpected token dont remove from stream
    return false;
  }
  
  return true;
}

void RTParser::parseStatements(Stream &stream)
{
  while(parseStatement(stream));
}

_Operand RTParser::parseFunctionCall(Stream &stream,const Stream::String &name)
{  
  SymbolMap::_Symbol s=codeGen_.findFunction(name);
  Error::expect(s.flagsIsFunction_ == 1) << stream << "function '" << name << "' not found";  
  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for function call";
  
  CodeGen::TmpStorage callFrame(codeGen_);
  CodeGen::Label ret(codeGen_);
  
  //calculate new irs addr for function and store it
  _Operand irsAddr=callFrame.allocate();
  //return addr storage
  _Operand retAddr=callFrame.allocate();
  //write parameters/return value
  _Operand returnData=callFrame.allocate();
  //current irs addr to be restored after function returns
  _Operand irsOrig=callFrame.allocate();
  
  uint32_t numParameter=0;
  while(stream.skipWhiteSpaces().peek() != ')')
  {
    Error::expect(numParameter < 16) << stream << "too many function call parameter" << ErrorHandler::FATAL;
    
    codeGen_.instrMov(callFrame.allocate(),parseExpr(stream));
    
    ++numParameter;
    
    if(stream.skipWhiteSpaces().peek() == ',')
    {
      stream.skipWhiteSpaces().read();
      Error::expect(stream.skipWhiteSpaces().peek() != ')') << stream << "expecting parameter";
    }    
  }
  
  stream.skipWhiteSpaces().read();//discards ')'
  
  _Operand currentIRS=_Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS__",0,7)));
  
  //codeGen_.instrMov(_Operand::createResult(),callFrame.getArrayBaseOffset());
  codeGen_.instrMov(_Operand::createResult(),_Operand::createConstWithRef(callFrame.getArrayBaseOffset().mapIndex_,1));
  
  CodeGen::TmpStorage tmp(codeGen_);
  codeGen_.instrOperation(_Operand::createResult(),currentIRS,'+',tmp);
  codeGen_.instrMov(irsAddr,_Operand::createResult());
  
  //store return addr
  codeGen_.instrMov(_Operand::createResult(),_Operand::createConstWithRef(ret.getLabelReference()));
  codeGen_.instrMov(retAddr,_Operand::createResult());
  
  //store original irs address
  codeGen_.instrMov(irsOrig,currentIRS);
  
  //change irs
  codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_IRS),irsAddr);

  //call
  codeGen_.instrMov(_Operand::createResult(),_Operand(qfp32::fromRealQfp32(s.allocatedAddr_)));
  codeGen_.instrGoto2();
  
  ret.setLabel();
  
  //store current irs
  _Operand irsRestore=_Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_REST__",0,12)));
  codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_IRS),irsRestore);
  
  return returnData;  
}

void RTParser::parseFunctionDecl(Stream &stream)
{
  SymbolMap curSymbols(stream,codeGen_.getCurCodeAddr());
  codeGen_.pushSymbolMap(curSymbols);
  
  //add default symbols
  static const char irsSym[]="__IRS__";
  codeGen_.addReference(Stream::String(irsSym,0,7),0);
  static const char retSym[]="__RET__";
  codeGen_.addReference(Stream::String(retSym,0,7),1);
  static const char resSym[]="__RES__";
  codeGen_.addReference(Stream::String(resSym,0,7),2);
  static const char irsRestSym[]="__IRS_REST__";
  codeGen_.addReference(Stream::String(irsRestSym,0,12),3);
  
  Token name=stream.readToken();
  Error::expect(name.getType() == Token::TOK_NAME) << stream << "expect function name";
  
  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' after function name";
  
  uint32_t numParameter=0;
  while(stream.skipWhiteSpaces().peek() != ')')
  {
    Error::expect(numParameter < 16) << stream << "too many function parameter";
    
    Token token=stream.readToken();
    
    Error::expect(token.getType() == Token::TOK_NAME) << stream << "only parameter names are allowed";
    
    codeGen_.addReference(token.getName(stream),4+numParameter);
    
    ++numParameter;
    
    if(stream.skipWhiteSpaces().peek() == ',')
    {
      stream.skipWhiteSpaces().read();
      Error::expect(stream.skipWhiteSpaces().peek() != '}') << stream << "expecting parameter";
    }
  }
  
  stream.skipWhiteSpaces().read();//discards ')'
  
  uint32_t codeAddrBeg=codeGen_.getCurCodeAddr();
  
  //pad to 8-word aligned code mem addr
  while(codeGen_.getCurCodeAddr()&7) codeGen_.instrNop();
  
  if(name.getType() == Token::TOK_NAME)
  {
    codeGen_.addFunctionAtCurrentAddr(name.getName(stream),numParameter);
  }
  
  RTParser(codeGen_).parseStatements(stream);
  
  if(codeGen_.getCurCodeAddr() == 0 || codeGen_.getCodeAt(codeGen_.getCurCodeAddr()-1) != SLCode::Goto::Code)
  {
    //generate return if not explicit written
    codeGen_.instrMov(_Operand::createResult(),_Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__RET__",0,7))));
    codeGen_.instrGoto2();
  }
  
  codeGen_.storageAllocationPass(512,4+numParameter);
  
  codeGen_.popSymbolMap();
  
  Error::info() << "function " << name.getName(stream) << " " << (codeGen_.getCurCodeAddr()-codeAddrBeg)<<" instrs";
  
  Token token=stream.readToken();
  Error::expect(token.getType() == Token::TOK_END) << stream << "expect 'END' at the end of a function";
}