#include <mtest.h>
#include "SLProcessorTest.h"

class TestNeg : public mtest::test
{  
};

MTEST(TestNeg,test_that_negate_Result_works)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.asUint),
    SLCode::Neg::create(),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(5) == (-value).asUint);
}