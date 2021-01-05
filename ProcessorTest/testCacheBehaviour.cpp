#include <mtest.h>
#include "SLProcessorTest.h"

class TestCacheBehaviour : public mtest::test
{  
};

MTEST(TestCacheBehaviour,test_that_fetch_stalls_after_goto_is_handled_correctly)
{
/*  qfp32_t value=7;
  
  uint32_t code[]=
  {
    SLCode::Goto::create(4,false),
    
    SLCode::Nop::create(),
    SLCode::Nop::create(),
    SLCode::Nop::create(),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::REG_IRS,SLCode::REG_RES,7,0),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.reset();
  proc.execute(3);
  proc.executeWithCodeStall(4);
  proc.execute(3);
  
  proc.expectThatMemIs(7,value);*/
}
