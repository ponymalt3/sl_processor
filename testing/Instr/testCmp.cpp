#include <mtest.h>
#include "SLProcessorTest.h"

class TestCmp : public mtest::test
{  
};

MTEST(TestCmp,testEqualExpectTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=value;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
  proc.expectThatMemIs(10,value);
}

MTEST(TestCmp,testEqualExpectFalse)
{
  qfp32_t value=-24.5;
  qfp32_t compare=0;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
  proc.expectThatMemIs(10,0);
}

MTEST(TestCmp,testNotEqualExpectTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_NEQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
  proc.expectThatMemIs(10,value);
}

MTEST(TestCmp,testNotEqualExpectFalse)
{
  qfp32_t value=-24.5;
  qfp32_t compare=value;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_NEQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
  proc.expectThatMemIs(10,0);
}

MTEST(TestCmp,testLessEqualExpectTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LE),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
  proc.expectThatMemIs(10,value);
}

MTEST(TestCmp,testLessEqualWithBothOperandsEqualExpectTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=value;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LE),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
  proc.expectThatMemIs(10,value);
}

MTEST(TestCmp,testLessEqualExpectFalse)
{
  qfp32_t value=24.5;
  qfp32_t compare=3;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LE),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
  proc.expectThatMemIs(10,0);
}

MTEST(TestCmp,testLessThanExpectTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LT),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
  proc.expectThatMemIs(10,value);
}

MTEST(TestCmp,testLessThanWithBothOperandsEqualExpectFalse)
{
  qfp32_t value=-24.5;
  qfp32_t compare=value;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LT),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
  proc.expectThatMemIs(10,0);
}

MTEST(TestCmp,testLessThanExpectFalse)
{
  qfp32_t value=24.5;
  qfp32_t compare=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_LT),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(10,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
  proc.expectThatMemIs(10,0);
}

MTEST(TestCmp,testBlockAddrIncIfCondNotMet)
{
  qfp32_t value=1;
  qfp32_t compare=30;
  qfp32_t ad0=10;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //cmp
    SLCode::Load::create1(value.toRaw()),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    
    //write different value
    SLCode::Load::create1(compare.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.toRaw());
  proc.writeMemory(11,0U);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(10) == compare.toRaw());
  EXPECT(proc.readMemory(11) == 0);//no write
  
  proc.expectThatMemIs(10,compare);
  proc.expectThatMemIs(11,0);
}