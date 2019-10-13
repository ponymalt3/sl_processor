#include <mtest.h>
#include "SLProcessorTest.h"

class TestUnaryOps : public mtest::test
{  
};

MTEST(TestUnaryOps,test_that_negate_Result_works)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_NEG),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(5) == (-value).toRaw());
  proc.expectThatMemIs(5,-value);
}

MTEST(TestUnaryOps,test_that_small_number_trunc_works)
{
  qfp32_t value=-24.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_TRUNC),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(5);
  
  proc.expectThatMemIs(5,_qfp32_t(-24.0));
}

MTEST(TestUnaryOps,test_that_mid_number_trunc_works)
{
  qfp32_t value=2004.9;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Load::create2(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_TRUNC),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(6);
  
  proc.expectThatMemIs(5,_qfp32_t(2004.0));
}

MTEST(TestUnaryOps,test_that_large_number_trunc_works)
{
  qfp32_t value=200004.123;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Load::create2(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_TRUNC),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(6);
  
  proc.expectThatMemIs(5,_qfp32_t(200004.0));
}

MTEST(TestUnaryOps,test_that_very_large_number_trunc_works)
{
  qfp32_t value=-20000004.999;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Load::create2(value.toRaw()),
    SLCode::Load::create3(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_TRUNC),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(7);
  
  proc.expectThatMemIs(5,_qfp32_t(-20000004.0));
}

MTEST(TestUnaryOps,test_that_trunc_to_zero_removes_sign_bit)
{
  qfp32_t value=-0.99;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_TRUNC),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(5);
  
  proc.expectThatMemIs(5,_qfp32_t(0.0));
}


MTEST(TestUnaryOps,test_that_log2_works)
{
  qfp32_t value=4096;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_LOG2),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(6);
  
  proc.expectThatMemIs(5,_qfp32_t(12.0));
}

MTEST(TestUnaryOps,test_that_log2_with_numbers_below_one_works)
{
  qfp32_t value=0.7*0.693147181;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::UnaryOp::create(SLCode::UNARY_LOG2),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0U);
  
  proc.run(6);
  
  proc.expectThatMemIs(5,_qfp32_t(-1.12499994));
}