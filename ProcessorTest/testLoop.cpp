#include <mtest.h>
#include "SLProcessorTest.h"

class TestLoop : public mtest::test
{  
};

MTEST(TestLoop,test_that_minimal_overhead_loop_works)
{
  qfp32_t ad0=12;
  qfp32_t loops=13;
  qfp32_t value=0;
  qfp32_t result=65+78;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(loops.toRaw()),
    SLCode::Loop::create(),
    SLCode::Load::create1(value.toRaw()),
    
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD0,SLCode::CMD_ADD,0,true),
    
    //jump
    SLCode::Goto::create(-1,false),
    
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  for(uint32_t i=0;i<(int32_t)loops;++i)
  {
    proc.writeMemory(ad0+qfp32_t::fromDouble(i),qfp32_t::fromDouble(i+5).toRaw());
  }
  
  proc.run(76);
  
  EXPECT(proc.readMemory(10) == result.toRaw());
  proc.expectThatMemIs(10,result);
}

MTEST(TestLoop,test_that_minimal_overhead_with_param_zero_executes_once)
{
  qfp32_t ad0=12;
  qfp32_t loops=0;
  qfp32_t value=0;
  qfp32_t result=1;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(loops.toRaw()),
    SLCode::Loop::create(),
    SLCode::Load::create1(value.toRaw()),
    
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD0,SLCode::CMD_ADD,0,true),
    
    //jump
    SLCode::Goto::create(-1,false),
    
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);

  proc.writeMemory(ad0,qfp32_t::fromDouble(1).toRaw());
  
  proc.run(11);
  
  EXPECT(proc.readMemory(10) == result.toRaw());
  proc.expectThatMemIs(10,result);
}