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
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //jump
    SLCode::Goto::create(4,false),
    
    //should be skipped
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    
    //target
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0U);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(ad0) == value2.toRaw());
  proc.expectThatMemIs(ad0,value2);
}

MTEST(TestGoto,testSimpleBackwardJump)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    //load result reg
    SLCode::Load::create1(value2.toRaw()),
        
    //should be executed    
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.toRaw()),
    
    //jump
    SLCode::Goto::create(-2,false),
    
    //should not be executed
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0U);
  proc.writeMemory(ad0+1,0U);
  
  proc.run(12);
  
  EXPECT(proc.readMemory(ad0) == value.toRaw());
  EXPECT(proc.readMemory(ad0+1) == 0);
  
  proc.expectThatMemIs(ad0,value);
  proc.expectThatMemIs(ad0+1,0);
}

MTEST(TestGoto,testConditionalGotoWithConditionIsTrue)
{
  qfp32_t ad0=10;
  qfp32_t value=ad0;
  qfp32_t value2=31;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Goto::create(2,false),
    
    //should be skipped
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    
    //should be executed
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(ad0,0U);
  
  proc.run(10);
  
  EXPECT(proc.readMemory(ad0) == value2.toRaw());
  proc.expectThatMemIs(ad0,value2);
}

MTEST(TestGoto,testConditionalGotoWithConditionIsFalse)
{
  qfp32_t ad0=10;
  qfp32_t value=ad0-1.0f;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    
    SLCode::Cmp::create(5,SLCode::CmpMode::CMP_EQ),
    SLCode::Goto::create(5,false),
    
    //should be executed
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,//nop
    0xFFFF,//nop
    
    //should be skipped
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(ad0,0U);
  proc.writeMemory(ad0+1,0U);
  
  proc.run(10);
  
  EXPECT(proc.readMemory(ad0) == ad0.toRaw());
  EXPECT(proc.readMemory(ad0+1) == ad0.toRaw());
  proc.expectThatMemIs(ad0,ad0);
  proc.expectThatMemIs(ad0+1,ad0);
}
