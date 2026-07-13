/*
 * RTParser.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "RTParser.h"

#include <map>

#include "Token.h"

RTParser::RTParser(CodeGen& codeGen) : Error(codeGen.getErrorHandler()), codeGen_(codeGen)
{
  startAddr_ = 0;
}

const std::map<uint32_t, uint32_t>& RTParser::getLineMapping() const
{
  return codeTolineMapping_;
}

void RTParser::parse(Stream& stream, uint32_t inlineFunctionThreshold)
{
  startAddr_ = 0;
  inlineFunctionThreshold_ = inlineFunctionThreshold;

  codeTolineMapping_.clear();
  codeTolineMapping_.insert(std::make_pair(0U, 0U));
  std::cout << "codeen start: " << (codeGen_.getCurCodeAddr()) << "\n";

  stream.setCallback([&](uint32_t line, bool invalidate) { streamCallback(line, invalidate); });

  codeGen_.setCodeMovedCallback([&](uint32_t startAddr, uint32_t size, uint32_t targetAddr) {
    codeMovedCallback(startAddr, size, targetAddr);
  });

  try
  {
    static const char irsSym[] = "__IRS_AND_RES__";
    codeGen_.addReference(Stream::String(irsSym, 0, 15), 0);
    static const char retSym[] = "__RET__";
    codeGen_.addReference(Stream::String(retSym, 0, 7), 1);
    static const char irsRestSym[] = "__IRS_REST__";
    codeGen_.addReference(Stream::String(irsRestSym, 0, 12), 2);

    parseStatements(stream);
    Error::expect(stream.readToken().getType() == Token::TOK_EOS) << stream << "missing end of file token";
  }
  catch(const std::runtime_error& e)
  {
    Error::expect(std::string(e.what()) == "fatal error");
  }

  codeGen_.generateEntryVector(startAddr_);

  decltype(codeTolineMapping_) t;
  for(auto& i : codeTolineMapping_)
  {
    auto it = t.insert(std::make_pair(i.first / 10, i.second)).first;
    it->second = i.second & (~NonMovableLineFlag);
  }

  codeTolineMapping_ = std::move(t);
}

_Operand RTParser::parserSymbolOrConstOrMem(Stream& stream, CodeGen::TmpStorage& tmpStorage)
{
  Token token = stream.readToken();

  if(token.getType() == Token::TOK_VALUE)
    return _Operand(token.getValue());

  if(token.getType() == Token::TOK_MEM)
    return _Operand(token.getIndex(), token.getAddrInc());

  if(token.getType() == Token::TOK_INDEX)
    return _Operand::createLoopIndex(token.getIndex());

  if(token.getType() == Token::TOK_ARRAY_SIZE)
  {
    Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '('";

    Token name = stream.readToken();
    Error::expect(name.getType() == Token::TOK_NAME) << stream << "expect symbol";

    SymbolMap::_Symbol sym = codeGen_.findSymbol(name.getName(stream));
    Error::expect(sym.flagIsArray_ != 0) << stream << "expect array name";

    Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "missing ')'";

    stream.skipWhiteSpaces();

    return _Operand(qfp32::fromRealQfp32(qfp32_t::fromDouble(sym.allocatedSize_)));
  }

  Error::expect(token.getType() == Token::TOK_NAME) << stream << "unexpected token " << token.getName(stream);

  // check for intrinsic functions
  if(token.getName(stream) == "log2" || token.getName(stream) == "int" || token.getName(stream) == "shft")
  {
    Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for function call";

    _Operand result = _Operand::createResult();
    _Operand param = parseExpr(stream);

    if(token.getName(stream) == "shft")
    {
      Error::expect(stream.skipWhiteSpaces().read() == ',')
          << stream << "expect two parameter for shft function";
      _Operand param2 = parseExpr(stream);

      if(param.type_ == _Operand::TY_VALUE && param2.type_ == _Operand::TY_VALUE)
      {
        result = _Operand(param.value_.logicShift(param2.value_));
      }
      else
      {
        TmpStorage tmp(codeGen_);
        codeGen_.instrOperation(param, param2, SLCode::CMD_SHFT, tmp);
      }
    }
    else if(token.getName(stream) == "log2")
    {
      if(param.type_ == _Operand::TY_VALUE)
      {
        result = _Operand(param.value_.log2());
      }
      else
      {
        codeGen_.instrUnaryOp(param, SLCode::UNARY_LOG2);
      }
    }
    else if(token.getName(stream) == "int")
    {
      if(param.type_ == _Operand::TY_VALUE)
      {
        result = _Operand(param.value_.trunc());
      }
      else
      {
        codeGen_.instrUnaryOp(param, SLCode::UNARY_TRUNC);
      }
    }

    Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "expect ')'";

    stream.skipWhiteSpaces();

    return result;
  }

  if(token.getName(stream) == "macres")
  {
    Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for macres function";
    Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "macres takes no arguments";

    codeGen_.writeCode(SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES));

    stream.skipWhiteSpaces();

    return _Operand::createResult();
  }

  if(codeGen_.findFunction(token.getName(stream)).symbols_ != nullptr)
  {
    _Operand result = parseFunctionCall(stream, token.getName(stream));

    stream.skipWhiteSpaces();

    return result;
  }

  uint32_t index = 0xFFFF;
  uint32_t structRefFieldOffset = 0;

  if(parseFieldOrIndexSuffix(stream, token.getName(stream), index, structRefFieldOffset))
  {
    // struct reference field read: address is recomputed into a1 on every access (a1 is volatile)
    _Operand result =
        materializeStructRefFieldRead(_Operand::createSymbol(token.getName(stream), 0), structRefFieldOffset);

    stream.skipWhiteSpaces();

    return result;
  }

  stream.skipWhiteSpaces();

  return _Operand::createSymbol(token.getName(stream), index);
}

uint32_t RTParser::parseFieldName(Stream& stream, uint32_t structTypeId)
{
  // assumes the caller already confirmed stream.peek() == '.'
  stream.read();
  Stream::String field = stream.readSymbol();
  Error::expect(field.getLength() > 0) << stream << "expect field name after '.'";

  uint32_t fieldOffset = codeGen_.findStructField(structTypeId, field);
  Error::expect(fieldOffset != CodeGen::NoRef) << stream << "struct has no field '" << field << "'";

  return fieldOffset;
}

bool RTParser::parseFieldOrIndexSuffix(Stream& stream,
                                       const Stream::String& name,
                                       uint32_t& index,
                                       uint32_t& structRefFieldOffset)
{
  SymbolMap::_Symbol sym = codeGen_.findSymbol(name);

  index = 0xFFFF;

  if(stream.skipWhiteSpaces().peek() == '(')
  {
    Error::expect(!sym.flagIsStructRef_)
        << stream << "cannot use '(index)' on struct reference '" << name << "', use '.field' instead";

    stream.read();
    index = stream.readInt(false).value_;
    Error::expect(stream.read() == ')') << (stream) << "missing ')'";

    return false;
  }

  if(stream.peek() == '.')
  {
    Error::expect(sym.structTypeId_ != SymbolMap::InvalidLink)
        << stream << "'" << name << "' is not a struct instance";

    uint32_t fieldOffset = parseFieldName(stream, sym.structTypeId_);

    if(sym.flagIsStructRef_)
    {
      structRefFieldOffset = fieldOffset;
      return true;
    }

    index = fieldOffset;
    return false;
  }

  return false;
}

void RTParser::emitStructRefAddrIntoA1(const Stream::String& refName, uint32_t fieldOffset)
{
  emitStructRefAddrIntoA1(_Operand::createSymbol(refName, 0), fieldOffset);
}

void RTParser::emitStructRefAddrIntoA1(const _Operand& addr, uint32_t fieldOffset)
{
  // a1 is volatile: the field address is always recomputed right before use, never cached across statements
  if(fieldOffset == 0)
  {
    codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_ADDR1), addr);
    return;
  }

  CodeGen::TmpStorage tmp(codeGen_);
  codeGen_.instrOperation(
      addr, _Operand(qfp32::fromRealQfp32(qfp32_t(static_cast<int32_t>(fieldOffset)))), '+', tmp);
  codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_ADDR1), _Operand::createResult());
}

_Operand RTParser::materializeStructRefFieldRead(const _Operand& addr, uint32_t fieldOffset)
{
  emitStructRefAddrIntoA1(addr, fieldOffset);
  codeGen_.instrMov(_Operand::createResult(), _Operand::createMemAccess(1, false));
  return _Operand::createResult();
}

void RTParser::parseStructRefFieldAssignment(Stream& stream,
                                             const Stream::String& refName,
                                             uint32_t fieldOffset)
{
  Error::expect(stream.skipWhiteSpaces().read() == '=') << stream << "expect '=' operator";

  _Operand opExp = parseExpr(stream);

  Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";

  // evaluate rhs fully into a stable slot *before* recomputing a1, since a1 may be
  // clobbered again while evaluating opExp (e.g. if it itself reads another struct reference field)
  if(opExp.isResult())
  {
    CodeGen::TmpStorage tmpStorage(codeGen_);
    _Operand tmp = tmpStorage.allocate();
    codeGen_.instrMov(tmp, opExp);
    opExp = tmp;
  }

  emitStructRefAddrIntoA1(refName, fieldOffset);

  codeGen_.instrMov(_Operand::createMemAccess(1, false), opExp);
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

  return 0;  // lowest priority otherwise
}

bool RTParser::isValuePrefix(char ch) const
{
  return ch == '-' || ch == '.' || (ch >= '0' && ch <= '9');
}

_Operand RTParser::parseExpr(Stream& stream)
{
  Stack<_Operand, 1024> operands;
  Stack<char, 1024> ops;

  CodeGen::TmpStorage tmpStorage(codeGen_);

  char ch;
  do
  {
    stream.skipWhiteSpaces().markPos();
    ch = stream.read();

    if((ops.empty() || ops.top() != UnaryMinus) && ch == '-' &&
       !isValuePrefix(stream.peek()))  // is unary minus with symbol
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

    uint32_t codeBlockStart = codeGen_.getCurCodeAddr();

    _Operand expr = parserSymbolOrConstOrMem(stream, tmpStorage);

    if(codeBlockStart != codeGen_.getCurCodeAddr())
    {
      if(expr.isResult())
      {
        _Operand tmp = tmpStorage.allocate();
        codeGen_.instrMov(tmp, expr);
        expr = tmp;
      }

      tmpStorage.preloadCode(codeBlockStart, codeGen_.getCurCodeAddr() - codeBlockStart);
    }

    stream.skipWhiteSpaces();
    ch = stream.peek();

    bool neg = false;

    // combine
    while(!ops.empty() && operatorPrecedence(ch) <= operatorPrecedence(ops.top()))
    {
      // handle brackets
      if(ops.top() == '(')
      {
        Error::expect(ch == ')') << stream << "expect missing ')'";

        ops.pop();
        stream.read();
        stream.skipWhiteSpaces();
        ch = stream.peek();
        continue;
      }

      if(ops.top() == UnaryMinus)
      {
        ops.pop();
        neg = true;
      }

      // reduce a+-b => a-b if possible
      if(!ops.empty() && (neg == false || ops.top() == '+' || ops.top() == '-'))
      {
        char op = ops.pop();

        if(neg)
          op = op == '+' ? '-' : '+';

        neg = false;

        _Operand a = operands.pop();

        // const only operation => no instr needed
        if(a.type_ == _Operand::TY_VALUE && expr.type_ == _Operand::TY_VALUE)
        {
          switch(op)
          {
            case '+':
              expr = _Operand(a.value_ + expr.value_);
              break;
            case '-':
              expr = _Operand(a.value_ - expr.value_);
              break;
            case '*':
              expr = _Operand(a.value_ * expr.value_);
              break;
            case '/':
              expr = _Operand(a.value_ / expr.value_);
              break;
            default:
              Error::expect(false) << stream << "unkown operator '" << (op) << "'";
          }
        }
        else
        {
          codeGen_.instrOperation(a, expr, op, tmpStorage);
          expr = _Operand::createResult();  // result operand
        }
      }

      if(neg)
      {
        if(expr.type_ == _Operand::TY_VALUE)
        {
          expr = _Operand(expr.value_ * qfp32::fromRealQfp32(-1.0));
        }
        else
        {
          codeGen_.instrUnaryOp(expr, SLCode::UNARY_NEG);
          expr = _Operand::createResult();
        }
        neg = false;
      }
    }

    // discard cause already in ch
    stream.skipWhiteSpaces().markPos();
    stream.read();

    // pending (valid) operand
    // if(ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(')
    ops.push(ch);

    if(!operands.empty() && operands.top().isResult())
    {
      _Operand tmp = tmpStorage.allocate();  // alloc tmp storage
      codeGen_.instrMov(tmp, operands.pop());
      operands.push(tmp);
    }

    operands.push(expr);

  } while(ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(');

  stream.restorePos();

  Error::expect(operands.size() == 1) << stream << "expression not valid";

  stream.skipWhiteSpaces();

  return operands.pop();
}

uint32_t RTParser::parseCmpMode(Stream& stream)
{
  switch(stream.skipWhiteSpaces().read())
  {
    case '<':
      if(stream.peek() == '=')  // less than or equal
      {
        stream.read();
        return CodeGen::CMP_MODE_LE;
      }
      return CodeGen::CMP_MODE_LT;
    case '>':
      if(stream.peek() == '=')  // greater than or equal
      {
        stream.read();
        return CodeGen::CMP_MODE_LE + CodeGen::CMP_MODE_SWAP_FLAG;
      }
      return CodeGen::CMP_MODE_LT + CodeGen::CMP_MODE_SWAP_FLAG;
    case '!': {
      if(stream.read() == '=')
        return CodeGen::CMP_MODE_NEQ;
      break;
    }
    case '=': {
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

void RTParser::parseIfExp(Stream& stream, CodeGen::Label& labelThen, CodeGen::Label& labelElse)
{
  Token mode = Token();
  bool lastCond = false;

  while(!lastCond)
  {
    // check for bracket: '(' exp <=/>=/==/!=/</> ...
    if(stream.skipWhiteSpaces().peek() == '(')
    {
      stream.markPos();
      int32_t bracketOpenCount = 0;
      for(;;)
      {
        if(!stream.empty())
        {
          switch(stream.read())
          {
            case '(':
              ++bracketOpenCount;
              continue;
            case ')':
              --bracketOpenCount;
              continue;
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
        parseIfExp(stream, localThen, localElse);
        Error::expect(stream.read() == ')') << stream << "missing ')'";

        lastCond = stream.peek() == ')';

        if(!lastCond)
        {
          Token tok = stream.readToken();
          Error::expect(tok.getType() == Token::TOK_IF_AND || tok.getType() == Token::TOK_IF_OR)
              << stream << "only or/and operator is allowed in condition";
          Error::expect(mode.getType() == Token::TOK_EOS || mode.getType() == tok.getType())
              << stream << "need to use brackets when mixing or/and";
          mode = tok;
        }

        if(!lastCond && mode.getType() == Token::TOK_IF_OR)
        {
          localThen.replaceWith(
              labelThen);  // is not called by "and"-operators eg ... or (a == b and c == d) or ...
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

    op[0] = parseExpr(stream);

    uint32_t cmpMode = parseCmpMode(stream);

    op[1] = parseExpr(stream);

    lastCond = stream.peek() == ')';

    if(!lastCond)
    {
      Token tok = stream.readToken();
      Error::expect(tok.getType() == Token::TOK_IF_AND || tok.getType() == Token::TOK_IF_OR)
          << stream << "only or/and operator is allowed in condition";
      Error::expect(mode.getType() == Token::TOK_EOS || mode.getType() == tok.getType())
          << stream << "need to use brackets when mxing or/and";
      mode = tok;
    }

    if(mode.getType() == Token::TOK_IF_OR && !lastCond)
    {
      codeGen_.instrCompare(op[0], op[1], cmpMode, 1, false, tmpStorage);
      codeGen_.instrGoto(labelThen);
    }
    else
    {
      codeGen_.instrCompare(op[0], op[1], cmpMode, 1, true, tmpStorage);
      codeGen_.instrGoto(labelElse);
    }
  }

  stream.skipWhiteSpaces();
}

void RTParser::parseIfStatement(Stream& stream)
{
  Error::expect(stream.read() == '(') << stream << "expect '(' after 'if'";

  CodeGen::Label labelThen(codeGen_);
  CodeGen::Label labelElse(codeGen_);
  CodeGen::Label labelEnd(codeGen_);

  parseIfExp(stream, labelThen, labelElse);

  Error::expect(stream.read() == ')') << stream << "missing ')'";

  labelThen.setLabel();

  parseStatements(stream);
  labelElse.setLabel();

  Token token = stream.readToken();

  if(token.getType() == Token::TOK_ELSE)
  {
    codeGen_.instrGoto(labelEnd);
    labelElse.setLabel();  // move label

    parseStatements(stream);
    token = stream.readToken();
  }

  Error::expect(token.getType() == Token::TOK_END) << stream << "expect 'END' token";

  labelEnd.setLabel();

  stream.skipWhiteSpaces();
}

void RTParser::parseLoopStatement(Stream& stream)
{
  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' after 'loop'";

  _Operand op = parseExpr(stream);

  Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "missing ')'";

  markLineAsNoMovable();  // only for line to code mapping
  stream.skipWhiteSpaces();

  CodeGen::Label beg(codeGen_);
  CodeGen::Label cont(codeGen_);
  CodeGen::Label end(codeGen_);

  CodeGen::TmpStorage storage(codeGen_);

  // create loop frame with storage for loopCounter
  _Operand loopCounter = storage.allocate();
  codeGen_.createLoopFrame(cont, end, &loopCounter);

  beg.setLabel();

  parseStatements(stream);
  stream.skipWhiteSpaces();

  if(codeGen_.isLoopFrameComplex() == false)
  {
    // use more efficient loop
    codeGen_.insertCodeBefore(beg)->instrLoop(op);
    cont.setLabel();
    codeGen_.instrGoto(beg);
  }
  else
  {
    // standard loop; create loop counter and generate initializing

    _Operand destLoopCount = op;
    if(destLoopCount.type_ != _Operand::TY_SYMBOL)
    {
      destLoopCount = storage.allocate();
      codeGen_.insertCodeBefore(beg)->instrMov(destLoopCount, op);
    }

    codeGen_.insertCodeBefore(beg)->instrMov(loopCounter, _Operand(qfp32::fromRealQfp32(_qfp32_t(0))));

    cont.setLabel();

    // inc counter
    codeGen_.instrOperation(_Operand(qfp32::fromRealQfp32(qfp32_t(1))), loopCounter, '+', storage);
    codeGen_.instrMov(loopCounter, _Operand::createResult());

    // compare
    codeGen_.instrCompare(_Operand::createResult(), destLoopCount, CodeGen::CMP_MODE_LT, 1, false, storage);
    codeGen_.instrGoto(beg);
  }

  end.setLabel();

  codeGen_.removeLoopFrame();

  Error::expect(stream.readToken().getType() == Token::TOK_END) << stream << "missing 'end' token";

  stream.skipWhiteSpaces();
}

void RTParser::parseWhileStatement(Stream& stream)
{
  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '('";

  CodeGen::Label beg(codeGen_);
  CodeGen::Label cont(codeGen_);
  CodeGen::Label end(codeGen_);

  codeGen_.createLoopFrame(cont, end);

  cont.setLabel();
  parseIfExp(stream, beg, end);

  Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "missing ')'";

  markLineAsNoMovable();
  stream.skipWhiteSpaces();

  beg.setLabel();

  parseStatements(stream);

  codeGen_.instrGoto(cont);

  end.setLabel();

  codeGen_.removeLoopFrame();

  Error::expect(stream.skipWhiteSpaces().readToken().getType() == Token::TOK_END) << stream << "expect 'end'";

  stream.skipWhiteSpaces();
}

bool RTParser::parseStatement(Stream& stream)
{
  stream.markPos();

  Token token = stream.readToken();

  switch(token.getType())
  {
    case Token::TOK_IF:
      parseIfStatement(stream);
      break;
    case Token::TOK_LOOP:
      parseLoopStatement(stream);
      break;
    case Token::TOK_WHILE:
      parseWhileStatement(stream);
      break;
    case Token::TOK_CONT: {
      codeGen_.instrContinue();
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_BREAK: {
      codeGen_.instrBreak();
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_ARRAY:
    case Token::TOK_DECL: {
      Token name = stream.readToken();
      Error::expect(name.getType() == Token::TOK_NAME);
      codeGen_.addArrayDeclaration(stream.createStringFromToken(name.getOffset(), name.getLength()),
                                   stream.readInt(false).value_);
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_STRUCT:
      parseStructDecl(stream);
      break;
    case Token::TOK_DEF: {
      Token name = stream.readToken();
      Error::expect(name.getType() == Token::TOK_NAME);
      codeGen_.addDefinition(stream.createStringFromToken(name.getOffset(), name.getLength()),
                             stream.readQfp32());
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_REF: {
      Token name = stream.readToken();
      Error::expect(name.getType() == Token::TOK_NAME);

      uint32_t structTypeId = codeGen_.findStructType(name.getName(stream));
      if(structTypeId != CodeGen::NoStructType)
      {
        //'ref TypeName ptrName [= expr];' declares a typed struct pointer
        Token pointerName = stream.readToken();
        Error::expect(pointerName.getType() == Token::TOK_NAME)
            << stream << "expect pointer name after 'ref " << name.getName(stream) << "'";

        parseStructPointerDecl(stream, structTypeId, pointerName.getName(stream));
        Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
        break;
      }

      int32_t value = stream.readInt(false).value_;
      Error::expect(value >= 4) << stream << "irs addresses 0-3 are reserved for callstack management";
      codeGen_.addReference(stream.createStringFromToken(name.getOffset(), name.getLength()), value);
      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
      break;
    }
    case Token::TOK_REGA:
    case Token::TOK_MEM:
    case Token::TOK_NAME: {
      _Operand op;

      if(token.getType() == Token::TOK_NAME)
      {
        uint32_t structTypeId = codeGen_.findStructType(token.getName(stream));
        if(structTypeId != CodeGen::NoStructType)
        {
          Token instanceName = stream.readToken();
          Error::expect(instanceName.getType() == Token::TOK_NAME)
              << stream << "expect instance name after struct type '" << token.getName(stream) << "'";

          parseStructInstanceDecl(stream, structTypeId, instanceName.getName(stream));
          Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
          break;
        }

        if(token.getName(stream) == "mac")
        {
          Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for mac function";

          _Operand a = parseExpr(stream);
          Error::expect(stream.skipWhiteSpaces().read() == ',')
              << stream << "expect two parameter for mac function";
          _Operand b = parseExpr(stream);

          Error::expect(stream.skipWhiteSpaces().read() == ')') << stream << "expect ')'";

          CodeGen::TmpStorage tmp(codeGen_);
          codeGen_.instrOperation(a, b, SLCode::CMD_MAC, tmp);

          Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
          break;
        }

        if(codeGen_.findFunction(token.getName(stream)).symbols_ != nullptr)
        {
          parseFunctionCall(stream, token.getName(stream));
          Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
          break;
        }

        if(stream.skipWhiteSpaces().peek() == '{')
        {
          parseArrayDecl(stream, token.getName(stream));
          Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";
          break;
        }

        uint32_t index = 0xFFFF;
        uint32_t structRefFieldOffset = 0;

        if(parseFieldOrIndexSuffix(stream, token.getName(stream), index, structRefFieldOffset))
        {
          parseStructRefFieldAssignment(stream, token.getName(stream), structRefFieldOffset);
          break;
        }

        op = _Operand::createSymbol(token.getName(stream), index);
      }

      if(token.getType() == Token::TOK_REGA)
        op = _Operand::createInternalReg(token.getIndex() ? _Operand::TY_IR_ADDR1 : _Operand::TY_IR_ADDR0);
      if(token.getType() == Token::TOK_MEM)
        op = _Operand::createMemAccess(token.getIndex(), token.getAddrInc());

      Error::expect(op.type_ != _Operand::TY_INVALID)
          << stream << "invalid left hand side for assignment " << (token.getName(stream));
      Error::expect(stream.skipWhiteSpaces().read() == '=') << stream << "expect '=' operator";

      _Operand opExp = parseExpr(stream);

      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";

      codeGen_.instrMov(op, opExp);
      break;
    }
    case Token::TOK_FCN_DECL:
      parseFunctionDecl(stream);
      startAddr_ = codeGen_.getCurCodeAddr();
      break;
    case Token::TOK_FCN_RET:
      if(stream.skipWhiteSpaces().peek() != ';')
      {
        _Operand result = parseExpr(stream);
        codeGen_.instrMov(
            _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_AND_RES__", 0, 15))),
            result);
      }
      else
      {
        // return 0 by default
        codeGen_.instrMov(
            _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_AND_RES__", 0, 15))),
            _Operand(qfp32::fromRealQfp32(qfp32_t::initFromRaw(0))));
      }

      Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';'";

      codeGen_.instrMov(
          _Operand::createResult(),
          _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__RET__", 0, 7))));
      codeGen_.instrGoto2();
      break;
    case Token::TOK_BUSLOCK:
      Error::expect(stream.skipWhiteSpaces().read() == '{') << stream << "expect '{'";
      codeGen_.instrLock(false);
      parseStatements(stream);
      Error::expect(stream.skipWhiteSpaces().read() == '}') << stream << "expect '}'";
      codeGen_.instrLock(true);
      break;
    default:
      Error::expect(token.getType() != Token::TOK_INDEX)
          << stream << "cant write/use " << token.getName(stream) << " loop index";
      stream.restorePos();  // unexpected token dont remove from stream
      return false;
  }

  stream.skipWhiteSpaces();

  return true;
}

void RTParser::parseStatements(Stream& stream)
{
  while(parseStatement(stream))
    ;
}

_Operand RTParser::parseFunctionCall(Stream& stream, const Stream::String& name)
{
  CodeGen::_FunctionInfo fi = codeGen_.findFunction(name);
  Error::expect(fi.symbols_ != nullptr) << stream << "function '" << name << "' not found";
  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' for function call";

  CodeGen::TmpStorage callFrame(codeGen_);
  CodeGen::Label ret(codeGen_);

  // calculate new irs addr for function and store it also used as return value
  _Operand irsAddr = callFrame.allocate();
  // return addr storage
  _Operand retAddr = callFrame.allocate();
  // current irs addr to be restored after function returns
  _Operand irsOrig = callFrame.allocate();

  // prepare inline instr
  if(fi.isInlineFunction_)
  {
    uint32_t offset = codeGen_.symbolMaps_.top().append(*(fi.symbols_), 3 + fi.parameters);

    uint32_t j = 0;
    for(auto& i : fi.inline_.instrs_)
    {
      if(i.isIrsInstr() && i.symRef_ != CodeGen::NoRef)
      {
        if(i.symRef_ >= 3 + fi.parameters)
        {
          i.symRef_ += offset - (3 + fi.parameters);
        }
        else if(i.symRef_ == 0)
        {
          Error::expect(irsAddr.type_ == _Operand::TY_RESOLVED_SYM) << stream;
          i.symRef_ = irsAddr.mapIndex_;
          i.patchIrsOffset(irsAddr.arrayOffset_);
          fi.inline_.alreadyPatched_[j] = true;
        }
      }
      ++j;
    }
  }

  uint32_t numParameter = 0;
  while(stream.skipWhiteSpaces().peek() != ')')
  {
    Error::expect(numParameter < 16) << stream << "too many function call parameter" << ErrorHandler::FATAL;

    uint32_t expectedStructTypeId = (numParameter < fi.paramStructTypeId_.size())
                                        ? fi.paramStructTypeId_[numParameter]
                                        : CodeGen::NoStructType;

    _Operand param = parseExpr(stream);

    if(expectedStructTypeId != CodeGen::NoStructType)
    {
      Error::expect(param.type_ == _Operand::TY_SYMBOL)
          << stream << "expect struct instance for parameter " << (numParameter + 1);

      SymbolMap::_Symbol argSym =
          codeGen_.findSymbol(stream.createStringFromToken(param.offset_, param.length_));
      Error::expect(argSym.structTypeId_ == expectedStructTypeId)
          << stream << "struct type mismatch for parameter " << (numParameter + 1);
    }

    if(fi.isInlineFunction_ == false)
    {
      codeGen_.instrMov(callFrame.allocate(), param);
    }
    else
    {
      // force loop frame to be complex => normal function call uses goto2 which implicits this
      CodeGen::Label dummy(codeGen_);
      codeGen_.createLoopFrame(dummy, dummy);
      codeGen_.removeLoopFrame();

      if(param.type_ == _Operand::TY_SYMBOL)
      {
        param = codeGen_.resolveOperand(param, false);
      }

      if((param.type_ != _Operand::TY_RESOLVED_SYM) || param.isArrayBaseAddr() ||
         (fi.inline_.parameterWrittenMap_ & (1 << numParameter)) != 0)
      {
        _Operand op = callFrame.allocate();
        codeGen_.instrMov(op, param);
        param = op;
      }

      // patch parameter
      uint32_t j = 0;
      for(auto& i : fi.inline_.instrs_)
      {
        if(i.symRef_ == (3 + numParameter) && fi.inline_.alreadyPatched_[j] == false)
        {
          if(param.type_ == _Operand::TY_RESOLVED_SYM)
          {
            i.symRef_ = param.mapIndex_;
            i.patchIrsOffset(param.arrayOffset_);
          }
          else
          {
            uint32_t mapIndex = codeGen_.symbolMaps_.top().findSymbol(
                stream.createStringFromToken(param.offset_, param.length_));
            Error::expect(mapIndex != SymbolMap::InvalidLink) << stream;
            i.symRef_ = mapIndex;
            i.patchIrsOffset(param.index_);
          }

          fi.inline_.alreadyPatched_[j] = true;
        }

        ++j;
      }
    }

    ++numParameter;

    if(stream.skipWhiteSpaces().peek() == ',')
    {
      stream.skipWhiteSpaces().read();
      Error::expect(stream.skipWhiteSpaces().peek() != ')') << stream << "expecting parameter";
    }
  }

  stream.skipWhiteSpaces().read();  // discards ')'

  Error::expect(fi.parameters == numParameter) << stream << "parameter missmatch";

  if(fi.isInlineFunction_)
  {
    codeGen_.appendInstrs(fi.inline_.instrs_);
    return irsAddr;
  }

  _Operand currentIRS =
      _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_AND_RES__", 0, 15)));

  codeGen_.instrMov(_Operand::createResult(),
                    _Operand::createConstWithRef(callFrame.getArrayBaseOffset().mapIndex_, 1));

  CodeGen::TmpStorage tmp(codeGen_);
  codeGen_.instrOperation(_Operand::createResult(), currentIRS, '+', tmp);
  codeGen_.instrMov(irsAddr, _Operand::createResult());

  // store return addr
  codeGen_.instrMov(_Operand::createResult(), _Operand::createConstWithRef(ret.getLabelReference()));
  codeGen_.instrMov(retAddr, _Operand::createResult());

  // store original irs address
  codeGen_.instrMov(irsOrig, currentIRS);

  // change irs
  codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_IRS), irsAddr);

  // call
  codeGen_.instrMov(_Operand::createResult(), _Operand(qfp32::fromRealQfp32(qfp32_t(fi.address_))));
  codeGen_.instrGoto2();

  ret.setLabel();

  // store current irs
  _Operand irsRestore =
      _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_REST__", 0, 12)));
  codeGen_.instrMov(_Operand::createInternalReg(_Operand::TY_IR_IRS), irsRestore);

  stream.skipWhiteSpaces();

  return irsAddr;
}

void RTParser::parseFunctionDecl(Stream& stream)
{
  Token name = stream.readToken();
  Error::expect(name.getType() == Token::TOK_NAME) << stream << "expect function name";

  // pad to 8-word aligned code mem addr
  while(codeGen_.getCurCodeAddr() & 7)
    codeGen_.instrNop();

  // SymbolMap curSymbols(stream,codeGen_.getCurCodeAddr());
  CodeGen::_FunctionInfo& fi = codeGen_.addFunctionAtCurrentAddr(name.getName(stream));
  codeGen_.pushSymbolMap(*(fi.symbols_));

  uint32_t codeAddrBeg = codeGen_.getCurCodeAddr();

  // add default symbols
  static const char irsSym[] = "__IRS_AND_RES__";
  codeGen_.addReference(Stream::String(irsSym, 0, 15), 0);
  static const char retSym[] = "__RET__";
  codeGen_.addReference(Stream::String(retSym, 0, 7), 1);
  static const char irsRestSym[] = "__IRS_REST__";
  codeGen_.addReference(Stream::String(irsRestSym, 0, 12), 2);

  Error::expect(stream.skipWhiteSpaces().read() == '(') << stream << "expect '(' after function name";

  uint32_t numParameter = 0;
  Stream::String params[16];
  while(stream.skipWhiteSpaces().peek() != ')')
  {
    Error::expect(numParameter < 16) << stream << "too many function parameter";

    Token token = stream.readToken();

    Error::expect(token.getType() == Token::TOK_NAME) << stream << "only parameter names are allowed";

    uint32_t structTypeId = codeGen_.findStructType(token.getName(stream));

    if(structTypeId != CodeGen::NoStructType)
    {
      // struct parameters are always passed by reference (a single pointer slot, like an array)
      Token paramName = stream.readToken();
      Error::expect(paramName.getType() == Token::TOK_NAME)
          << stream << "expect parameter name after struct type '" << token.getName(stream) << "'";

      params[numParameter] = paramName.getName(stream);
    }
    else
    {
      params[numParameter] = token.getName(stream);
    }

    codeGen_.addReference(params[numParameter], 3 + numParameter);

    if(structTypeId != CodeGen::NoStructType)
    {
      codeGen_.markSymbolAsStructReference(params[numParameter], structTypeId);
    }

    fi.paramStructTypeId_.push_back(structTypeId);

    ++numParameter;

    if(stream.skipWhiteSpaces().peek() == ',')
    {
      stream.read();
      Error::expect(stream.skipWhiteSpaces().peek() != ')') << stream << "expecting parameter";
    }
  }

  stream.skipWhiteSpaces().read();  // discards ')'

  fi.parameters = numParameter;

  parseStatements(stream);

  if(codeGen_.getCurCodeAddr() == codeAddrBeg ||
     codeGen_.getCodeAt(codeGen_.getCurCodeAddr() - 1) != SLCode::Goto::Code)
  {
    // generate return if not explicit written
    codeGen_.instrMov(
        _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__IRS_AND_RES__", 0, 15))),
        _Operand(qfp32::fromRealQfp32(qfp32_t::initFromRaw(0))));

    codeGen_.instrMov(_Operand::createResult(),
                      _Operand::createSymAccess(codeGen_.findSymbolAsLink(Stream::String("__RET__", 0, 7))));
    codeGen_.instrGoto2();
  }

  fi.size_ = codeGen_.getCurCodeAddr() - codeAddrBeg;

  bool isMainFunction = false;
  std::string nameAsStr(name.getName(stream).getBase() + name.getOffset(), name.getLength());

  if(nameAsStr.substr(0, 4) == "main")
  {
    isMainFunction =
        nameAsStr.length() == 4 ||
        (std::isdigit(nameAsStr[4]) &&
         (nameAsStr.length() == 5 ||
          (std::isdigit(nameAsStr[5]) &&
           (nameAsStr.length() == 6 || (std::isdigit(nameAsStr[6]) && (nameAsStr.length() == 7))))));
  }

  // check if function uses static arrays and matches inline function requirement
  if(!isMainFunction && fi.size_ < inlineFunctionThreshold_ && !codeGen_.isArrayDeclInCurrentSymbolMap())
  {
    // remove code mapping
    codeTolineMapping_.erase(codeTolineMapping_.lower_bound(codeAddrBeg), codeTolineMapping_.end());

    fi.isInlineFunction_ = true;
    fi.inline_.parameterWrittenMap_ = 0;

    auto instrs = codeGen_.extractRecentInstrs(fi.size_);

    // remove last 2 instr; load addr and goto
    instrs.pop_back();
    instrs.pop_back();
    fi.size_ -= 2;

    uint32_t returnAddrSymRef = codeGen_.findSymbolAsLink(Stream::String("__RET__", 0, 7));
    uint32_t returnDataSymRef = codeGen_.findSymbolAsLink(Stream::String("__IRS_AND_RES__", 0, 15));

    uint32_t j = 0;
    for(auto& i : instrs)
    {
      if(i.isIrsInstr())
      {
        if(i.symRef_ == returnAddrSymRef)
        {
          i.code_ =
              SLCode::Goto::create(static_cast<int32_t>(fi.size_ - j), false);  // jump to end of function
          i.symRef_ = CodeGen::NoRef;
          ++j;
          continue;
        }

        if(i.symRef_ == returnDataSymRef)
        {
          ++j;
          continue;
        }

        if(i.symRef_ != CodeGen::NoRef)
        {
          SymbolMap::_Symbol& sym = (*fi.symbols_)[i.symRef_];
          if(sym.flagAllocated_ && sym.allocatedAddr_ >= 3)
          {
            uint32_t paramId = sym.allocatedAddr_ - 3;
            if(i.isMovToIrsInstr() && paramId < fi.parameters)
            {
              fi.inline_.parameterWrittenMap_ |= 1 << paramId;
            }
          }
        }
      }

      ++j;
    }

    fi.inline_.instrs_ = std::move(instrs);
    fi.inline_.alreadyPatched_.resize(fi.inline_.instrs_.size(), false);
  }
  else
  {
    // allocate irs storage
    codeGen_.storageAllocationPass(512, 4 + numParameter);
  }

  codeGen_.popSymbolMap();

  Error::info() << "function " << name.getName(stream) << " " << (fi.size_) << " instrs "
                << (fi.isInlineFunction_ ? "inlined" : "");

  Token token = stream.readToken();
  Error::expect(token.getType() == Token::TOK_END) << stream << "expect 'END' at the end of a function";

  stream.skipWhiteSpaces();
}

void RTParser::parseArrayDecl(Stream& stream, const Stream::String& name)
{
  stream.read();

  uint32_t symName = codeGen_.findSymbolAsLink(name);
  Error::expect(symName == CodeGen::NoRef) << stream << "redefinition of symbol " << name;

  uint32_t arraySize = 1;
  codeGen_.addArrayDeclaration(name, arraySize);
  uint32_t arrayRef = codeGen_.findSymbolAsLink(name);

  codeGen_.instrMov(_Operand::createSymAccess(arrayRef, arraySize - 1), parseExpr(stream));

  while(stream.skipWhiteSpaces().peek() == ',')
  {
    stream.read();
    ++arraySize;
    codeGen_.resizeArray(name, arraySize);
    codeGen_.instrMov(_Operand::createSymAccess(arrayRef, arraySize - 1), parseExpr(stream));
  }

  Error::expect(stream.skipWhiteSpaces().read() == '}') << stream << "missing '}'";

  stream.skipWhiteSpaces();
}

void RTParser::parseStructDecl(Stream& stream)
{
  Token name = stream.readToken();
  Error::expect(name.getType() == Token::TOK_NAME) << stream << "expect struct name";

  Stream::String structName = name.getName(stream);

  Error::expect(stream.skipWhiteSpaces().read() == '{') << stream << "expect '{' after struct name";

  std::vector<Stream::String> fields;

  while(stream.skipWhiteSpaces().peek() != '}')
  {
    Token field = stream.readToken();
    Error::expect(field.getType() == Token::TOK_NAME) << stream << "expect field name";

    fields.push_back(field.getName(stream));

    Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';' after field";
  }

  stream.read();  // discard '}'

  codeGen_.addStructDeclaration(structName, fields);

  Error::expect(stream.skipWhiteSpaces().read() == ';') << stream << "missing ';' after struct declaration";

  stream.skipWhiteSpaces();
}

void RTParser::parseStructInstanceDecl(Stream& stream, uint32_t structTypeId, const Stream::String& name)
{
  codeGen_.addStructInstanceDeclaration(name, structTypeId);

  if(stream.skipWhiteSpaces().peek() != '{')
  {
    // declaration only, no initializer
    return;
  }

  stream.read();  // discard '{'

  uint32_t symRef = codeGen_.findSymbolAsLink(name);
  uint32_t fieldCount = codeGen_.getStructFieldCount(structTypeId);

  uint32_t field = 0;

  codeGen_.instrMov(_Operand::createSymAccess(symRef, field), parseExpr(stream));
  ++field;

  while(stream.skipWhiteSpaces().peek() == ',')
  {
    stream.read();
    Error::expect(field < fieldCount)
        << stream << "too many initializers for struct instance '" << name << "'";
    codeGen_.instrMov(_Operand::createSymAccess(symRef, field), parseExpr(stream));
    ++field;
  }

  Error::expect(field == fieldCount) << stream << "not enough initializers for struct instance '" << name
                                     << "'";

  Error::expect(stream.skipWhiteSpaces().read() == '}') << stream << "missing '}'";

  stream.skipWhiteSpaces();
}

void RTParser::parseStructPointerDecl(Stream& stream, uint32_t structTypeId, const Stream::String& name)
{
  // declares a plain pointer variable that reinterprets an arbitrary address (another
  // struct/array base, pointer arithmetic, or a raw constant) as the given struct type
  codeGen_.addStructPointerDeclaration(name, structTypeId);

  if(stream.skipWhiteSpaces().peek() != '=')
  {
    // declared but not yet initialized, can be assigned later like any other scalar
    return;
  }

  stream.read();  // discard '='

  _Operand initExpr = parseExpr(stream);

  codeGen_.instrMov(_Operand::createSymbol(name, 0), initExpr);
}

void RTParser::streamCallback(uint32_t line, bool invalidate)
{
  if(invalidate)
  {
    auto i = codeTolineMapping_.begin();
    for(; i != codeTolineMapping_.end();)
    {
      if(i->second > line)
      {
        i = codeTolineMapping_.erase(i);
      }
      else
      {
        ++i;
      }
    }

    return;
  }

  auto it = codeTolineMapping_.find(codeGen_.getCurCodeAddr() * 10);

  uint32_t subIndex = 0;
  while(it != codeTolineMapping_.end() && (it->first / 10) == codeGen_.getCurCodeAddr())
  {
    ++it;
    ++subIndex;
  }

  codeTolineMapping_.insert(std::make_pair(codeGen_.getCurCodeAddr() * 10 + subIndex, line));
}

void RTParser::codeMovedCallback(uint32_t startAddr, uint32_t size, uint32_t targetAddr)
{
  startAddr *= 10;
  targetAddr *= 10;

  auto i1 = codeTolineMapping_.lower_bound(std::min(startAddr + size, targetAddr));
  auto i2 = codeTolineMapping_.upper_bound(std::max(startAddr + size, targetAddr));
  if(i2 != codeTolineMapping_.end())
  {
    --i2;
  }
  std::map<uint32_t, uint32_t> t;
  t.insert(i1, i2);
  codeTolineMapping_.erase(i1, i2);

  for(auto& j : t)
  {
    int32_t value = j.first;

    if((j.second & NonMovableLineFlag) == 0 ||
       ((startAddr / 10) < (j.first / 10) && (targetAddr / 10) > (j.first / 10)))
    {
      if(targetAddr > startAddr)
      {
        value -= size * 10;
      }
      else
      {
        value += size * 10;
      }
    }

    codeTolineMapping_.insert(std::make_pair(value, j.second));
  }
}

void RTParser::markLineAsNoMovable()
{
  auto itLastElement = --(codeTolineMapping_.end());
  itLastElement->second |= NonMovableLineFlag;
}
