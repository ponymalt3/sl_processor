#include <mtest.h>
#include "SLProcessorTest.h"

class TestBugs : public mtest::test
{  
};

MTEST(TestBugs,testOpWithOperandsResultAndIRS)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value2.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(6) == (value-value2).toRaw());
  proc.expectThatMemIs(6,value-value2);
}

// run all tests
int main(int argc, char **argv)
{
	mtest::runAllTests("*.*");
  return 0;
}
