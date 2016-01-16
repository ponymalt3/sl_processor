#include <mtest.h>
#include "SLProcessorTest.h"

class TestMemXMov : public mtest::test
{
  //mov RESULT AD1
  //mov AD1 RESULT
};

MTEST(TestMemXMov,test_that_mov_to_Result_from_AD1_works)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    //stall 1 cycle
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value.asUint);
  proc.writeMemory(5,0);
  
  proc.run(8);//include one cycle stall
  
  EXPECT(proc.readMemory(5) == value.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_AD1_from_Result_works)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_Result_from_AD1_with_inc_works)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    //stall 1 cycle
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,value.asUint);
  proc.writeMemory(ad1+1,0);
  
  proc.run(9);//include one cycle stall
  
  EXPECT(proc.readMemory(5) == value.asUint);
  EXPECT(proc.readMemory(ad1+1) == value.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_AD1_from_Result_with_inc_works)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
  EXPECT(proc.readMemory(ad1+1) == value.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_Result_from_AD1_stalls_while_external_write_is_in_progress)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  qfp32_t value2=-7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),

    SLCode::Load::create1(value.asUint),    
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,true),
    //stall 1 cycle
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,value2.asUint);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
  EXPECT(proc.readMemory(5) == value2.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_Result_from_AD1_works_while_external_mem_is_stalled)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    //stall 1 cycle
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,0);
  
  proc.reset();
  proc.executeWithMemExtStall(7);
  
  proc.writeMemory(ad1,value.asUint);
  
  proc.execute(2);  
  
  EXPECT(proc.readMemory(5) == value.asUint);
}

MTEST(TestMemXMov,test_that_mov_to_AD1_from_Result_works_while_external_mem_is_stalled)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),

    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,0);
  
  proc.reset();
  proc.executeWithMemExtStall(8);
  
  proc.writeMemory(ad1,0);
  
  proc.execute(1);  
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
}