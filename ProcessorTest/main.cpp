#include <mtest.h>
#include "SLProcessorTest.h"

class TestGoto : public mtest::test
{  
};

MTEST(TestGoto,testSimpleForwardJump)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //jump
    SLCode::Goto::create(4,false),
    
    //should be skipped
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    
    //target
    SLCode::Load::create1(value2.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory((int32_t)ad0,0);
  
  proc.run(9);
  
  EXPECT(proc.readMemory((int32_t)ad0) == value2.asUint);
}

MTEST(TestGoto,testSimpleBackwardJump)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //load result reg
    SLCode::Load::create1(value2.asUint),
        
    //should be executed    
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.asUint),
    
    //jump
    SLCode::Goto::create(0x200+2,false),
    
    //should not be executed
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Load::create1(value2.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory((int32_t)ad0,0);
  proc.writeMemory((int32_t)ad0+1,0);
  
  proc.run(12);
  
  EXPECT(proc.readMemory((int32_t)ad0) == value.asUint);
  EXPECT(proc.readMemory((int32_t)ad0+1) == 0);
}

MTEST(TestGoto,testConditionalGoto)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
        
    //should be executed
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    //jump
    SLCode::Goto::create(-3,false),
    
    //should not be executed
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Load::create1(value2.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory((int32_t)ad0,0);
  proc.writeMemory((int32_t)ad0+1,0);
  
  proc.run(9);
  
  EXPECT(proc.readMemory((int32_t)ad0) == value.asUint);
  EXPECT(proc.readMemory((int32_t)ad0+1) == 0);
}

// run all tests
int main(int argc, char **argv)
{
	mtest::runAllTests("TestGoto.testSimpleBackwardJump");
  return 0;
}
