#include <mtest.h>
#include "SLProcessorTest.h"

class TestCmp : public mtest::test
{  
};

MTEST(TestCmp,testEqTrue)
{
  qfp32_t value=-24.5;
  qfp32_t compare=value;
  
  uint32_t code[]=
  {
    SLCode::Load::create(SLCode::Load::constDataValue1(value.asUint)),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.asUint);
  proc.writeMemory(10,0);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.asUint);
}

MTEST(TestCmp,testEqFalse)
{
  qfp32_t value=-24.5;
  qfp32_t compare=0;
  
  uint32_t code[]=
  {
    SLCode::Load::create(SLCode::Load::constDataValue1(value.asUint)),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.asUint);
  proc.writeMemory(10,0);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == 0);
}

MTEST(TestCmp,testBlockAddrIncIfCondNotMet)
{
  qfp32_t value=1;
  qfp32_t compare=30;
  qfp32_t ad0=10;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create(SLCode::Load::constDataValue1(ad0.asUint)),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //cmp
    SLCode::Load::create(SLCode::Load::constDataValue1(value.asUint)),
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    
    //write different value
    SLCode::Load::create(SLCode::Load::constDataValue1(compare.asUint)),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,compare.asUint);
  proc.writeMemory(11,0);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(10) == compare.asUint);
  EXPECT(proc.readMemory(11) == 0);//no write
}

// run all tests
int main(int argc, char **argv)
{
	mtest::runAllTests("TestCmp.*");
  return 0;
}
