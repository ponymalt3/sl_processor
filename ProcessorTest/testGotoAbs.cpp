#include <mtest.h>
#include "SLProcessorTest.h"

class TestGotoAbs : public mtest::test
{  
};

MTEST(TestGotoAbs,test_that_goto_with_abs_addr_works)
{
  qfp32_t ad0=10;
  qfp32_t target=6;
  qfp32_t value=-24.5;
  qfp32_t value2=11;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    //jump
    SLCode::Load::create1(target.toRaw()),
    SLCode::Goto::create(),
    
    //should be skipped
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,true),
    
    //target
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(ad0) == value2.toRaw());
}

MTEST(TestGotoAbs,test_that_goto_flushes_pipeline_and_prevent_addr_inc)
{
  qfp32_t ad0=10;
  qfp32_t target=5;
  qfp32_t value=-24.5;
  qfp32_t value2=11;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    //jump
    SLCode::Load::create1(target.toRaw()),
    SLCode::Goto::create(),
    
    //should be skipped
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,true),
    
    //target
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  
  proc.run(10);
  
  EXPECT(proc.readMemory(ad0) == value2.toRaw());
}
