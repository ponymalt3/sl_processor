#include <mtest.h>
#include "SLProcessorTest.h"

class TestMov : public mtest::test
{
};

MTEST(TestMov,testMovToAD0RegFromResult)
{
  qfp32_t ad0=10;
  qfp32_t value=31.0625;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  
  proc.run(6);//include one cycle stall
  
  EXPECT(proc.readMemory(ad0) == value.asUint);
}

MTEST(TestMov,testMovToAD1RegFromResult)
{
  qfp32_t ad1=10;
  qfp32_t value=31.0625;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,0);
  
  proc.run(6);
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
}

MTEST(TestMov,MovToAD0FromResultAndInc)
{
  qfp32_t ad0=10;
  qfp32_t value=31.0625;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  proc.writeMemory(ad0+1,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(ad0) == value.asUint);
  EXPECT(proc.readMemory(ad0+1) == value.asUint);
}

MTEST(TestMov,MovToAD1FromResultAndInc)
{
  qfp32_t ad1=10;
  qfp32_t value=31.0625;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
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
  proc.writeMemory(ad1,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(ad1) == value.asUint);
  EXPECT(proc.readMemory(ad1) == value.asUint);
}

MTEST(TestMov,testMovToAD0FromResultStallsWhenAD0IsWrittenBefore)
{
  qfp32_t ad0=9;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),//make sure addr is only increment once
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  proc.writeMemory(ad0+1,0);
  proc.writeMemory(ad0+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0) == ad0.asUint);
  EXPECT(proc.readMemory(ad0+1) == ad0.asUint);
  EXPECT(proc.readMemory(ad0+2) == 0);
}

MTEST(TestMov,testMovToAD1FromResultStallsWhenAD1IsWrittenBefore)
{
  qfp32_t ad1=9;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),//make sure addr is only increment once
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,0);
  proc.writeMemory(ad1+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1) == ad1.asUint);
  EXPECT(proc.readMemory(ad1+1) == ad1.asUint);
  EXPECT(proc.readMemory(ad1+2) == 0);
}

MTEST(TestMov,testMovToAD0FromResultDoesNotStallWhenAD1IsWrittenBefore)
{
  qfp32_t ad0=9;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,0);
  proc.writeMemory(ad0+1,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(ad0) == ad0.asUint);
  EXPECT(proc.readMemory(ad0+1) == ad0.asUint);
}

MTEST(TestMov,testMovToAD1FromResultDoesNotStallWhenAD0IsWrittenBefore)
{
  qfp32_t ad1=9;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(ad1) == ad1.asUint);
  EXPECT(proc.readMemory(ad1+1) == ad1.asUint);
}

MTEST(TestMov,test_that_mov_to_Result_from_AD0_works_and_expect_stall)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad0,value.asUint);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(5) == value.asUint);
}

MTEST(TestMov,test_that_mov_to_Result_from_AD0_with_inc_works_and_expect_stall)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(ad0+1,0);
  proc.writeMemory(ad0+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0+1) == value.asUint);
  EXPECT(proc.readMemory(ad0+2) == 0);
}


MTEST(TestMov,test_that_mov_to_Result_from_AD1_works_and_expect_stall)
{
  qfp32_t ad1=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    //stall
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad1,value.asUint);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(5) == value.asUint);
}

MTEST(TestMov,test_that_mov_to_Result_from_AD1_with_inc_works_and_expect_stall)
{
  qfp32_t ad1=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value.asUint);
  proc.writeMemory(ad1+1,0);
  proc.writeMemory(ad1+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1+1) == value.asUint);
  EXPECT(proc.readMemory(ad1+2) == 0);
}

MTEST(TestMov,test_that_mov_to_Result_from_AD0_works_and_does_not_stall_when_AD1_is_written_before)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value.asUint);
  proc.writeMemory(ad0+1,0);
  proc.writeMemory(ad0+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0+1) == value.asUint);
  EXPECT(proc.readMemory(ad0+2) == 0);
}

MTEST(TestMov,test_that_mov_to_Result_from_AD1_works_and_does_not_stall_when_AD0_is_written_before)
{
  qfp32_t ad1=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::REG_RES,SLCode::REG_AD1,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value.asUint);
  proc.writeMemory(ad1+1,0);
  proc.writeMemory(ad1+2,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1+1) == value.asUint);
  EXPECT(proc.readMemory(ad1+2) == 0);
}


MTEST(TestMov,test_that_mov_to_Result_from_IRS_works)
{
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Mov::create(SLCode::REG_RES,SLCode::IRS,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.asUint);
  proc.writeMemory(6,0);
  
  proc.run(4);
  
  EXPECT(proc.readMemory(6) == value.asUint);
}

MTEST(TestMov,test_that_mov_to_IRS_from_Result_works)
{
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  
  proc.run(4);
  
  EXPECT(proc.readMemory(5) == value.asUint);
}

//NOT POSSIBLE ANYMORE: cause data mux for mem write is saved
MTEST(TestMov,test_that_mov_from_mem_AD0_to_mem_AD1_works_DISABLED)
{
  qfp32_t ad0=9;
  qfp32_t ad1=11;
  qfp32_t value1=-0.25;
  qfp32_t value2=13;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0),
   
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value1.asUint);
  proc.writeMemory(ad0+1,value1.asUint);
  proc.writeMemory(ad1,0);
  proc.writeMemory(ad1+1,0);
  
  proc.run(10);
  
  EXPECT(proc.readMemory(ad1+0) == value1.asUint);
  EXPECT(proc.readMemory(ad1+1) == value1.asUint);
}