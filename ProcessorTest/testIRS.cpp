#include <mtest.h>
#include "SLProcessorTest.h"

class TestIRS : public mtest::test
{  
};

MTEST(TestIRS,test_that_irs_addr_is_changed)
{
  int32_t offset=10;
  qfp32_t irs=15;
  qfp32_t value=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,offset),
    
    //change irs
    SLCode::Load::create1(irs.toRaw()),
    SLCode::Mov::create(SLCode::REG_IRS,SLCode::REG_RES,0,0),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,offset),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(offset,0);
  proc.writeMemory(irs+offset,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(offset) == value.toRaw());
  EXPECT(proc.readMemory(irs+offset) == value.toRaw());
  
  proc.expectThatMemIs(offset,value);
  proc.expectThatMemIs(irs+offset,value);
}

MTEST(TestIRS,test_that_irs_addr_is_changed_with_big_value)
{
  int32_t offset=10;
  qfp32_t irs=496;
  qfp32_t value=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,offset),
    
    //change irs
    SLCode::Load::create1(irs.toRaw()),
    SLCode::Mov::create(SLCode::REG_IRS,SLCode::REG_RES,0,0),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,offset),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(offset,0);
  proc.writeMemory(irs+offset,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(offset) == value.toRaw());
  EXPECT(proc.readMemory(irs+offset) == value.toRaw());
  
  proc.expectThatMemIs(offset,value);
  proc.expectThatMemIs(irs+offset,value);
}

MTEST(TestIRS,test_that_irs_access_after_irs_changed_is_stalled)
{
  int32_t offset=10;
  qfp32_t irs=496;
  qfp32_t ad0=7;
  qfp32_t value=22;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    //change irs
    SLCode::Load::create1(irs.toRaw()),
    SLCode::Mov::create(SLCode::REG_IRS,SLCode::REG_RES),
    
    //instr  must stall cause irs reg is used
    SLCode::Mov::create(SLCode::REG_RES,SLCode::IRS,offset),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(offset,(value/qfp32_t(2)).toRaw());
  proc.writeMemory(irs+offset,value.toRaw());
  
  proc.run(9);
  
  EXPECT(proc.readMemory(ad0) == value.toRaw());
  proc.expectThatMemIs(ad0,value);
}
