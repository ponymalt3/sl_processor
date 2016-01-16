#include <mtest.h>
#include "SLProcessorTest.h"

class TestMemXOp : public mtest::test
{
  //op RESULT AD1
  //op MEM AD1
};


MTEST(TestMemXOp,test_that_sub_AD1_from_Result_works)
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
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD1,SLCode::CMD_SUB),
    //stall 1 cycle
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value2.asUint);
  proc.writeMemory(5,0);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(5) == (value-value2).asUint);
}

MTEST(TestMemXOp,test_that_sub_AD1_from_AD0_works)
{
  qfp32_t ad0=10;
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  qfp32_t value2=-7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::DEREF_AD1,SLCode::CMD_SUB),
    //stall one cycle
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(ad1,value2.asUint);
  proc.writeMemory(5,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(5) == (value-value2).asUint);
}

MTEST(TestMemXOp,test_that_sub_AD1_from_Result_with_inc_works)
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
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD1,SLCode::CMD_SUB,0,true),
    //stall 1 cycle
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value2.asUint);
  proc.writeMemory(ad1+1,0);
  
  proc.run(9);
  
  EXPECT(proc.readMemory(ad1+1) == (value-value2).asUint);
}

MTEST(TestMemXOp,test_that_sub_AD1_from_AD0_with_inc_works)
{
  qfp32_t ad0=10;
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  qfp32_t value2=-7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::DEREF_AD1,SLCode::CMD_SUB,0,false,true),
    //stall one cycle
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(ad1,value2.asUint);
  proc.writeMemory(ad1+1,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(ad1+1) == (value-value2).asUint);
}

MTEST(TestMemXOp,test_that_sub_AD1_from_AD0_stalls_while_external_write_is_in_progress)
{
  qfp32_t ad0=10;
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  qfp32_t value2=-7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Load::create1(value2.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    //stall one cycle
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::DEREF_AD1,SLCode::CMD_SUB,0,false,true),
    //stall one cycle
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,0);
  
  proc.run(12);
  
  EXPECT(proc.readMemory(ad1+1) == (value-value2).asUint);
}


MTEST(TestMemXOp,test_that_sub_AD1_from_AD0_stalls_while_external_mem_is_stalled)
{
  qfp32_t ad0=10;
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
  qfp32_t value2=-7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(ad1.asUint),
    SLCode::Load::create2(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //stall one cycle
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::DEREF_AD1,SLCode::CMD_SUB,0,false,true),
    //stall one cycle
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,0);
  
  proc.reset();
  proc.executeWithMemExtStall(9);
  
  proc.writeMemory(ad1,value2.asUint);
  
  proc.execute(3);  
  
  EXPECT(proc.readMemory(ad1+1) == (value-value2).asUint);
}