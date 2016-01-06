#include <mtest.h>
#include "SLProcessorTest.h"

class TestMov : public mtest::test
{
};

MTEST(TestMov,MovAD0)
{
  qfp32_t ad0=10;
  qfp32_t value=31.5;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(6);//include one cycle stall
  
  EXPECT(proc.readMemory(10) == value.asUint);
}

MTEST(TestMov,MovAD1)
{
  qfp32_t ad1=10;
  qfp32_t value=-31.5;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(6);
  
  EXPECT(proc.readMemory(10) == value.asUint);
}

MTEST(TestMov,MovAD0AndInc)
{
  qfp32_t ad0=10;
  qfp32_t value=31.0625;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    //SLCode::Load::create(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(10) == value.asUint);
  EXPECT(proc.readMemory(11) == value.asUint);
}

MTEST(TestMov,MovAD1AndInc)
{
  qfp32_t ad1=10;
  qfp32_t value=31.0625;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
    SLCode::Load::create1(value.asUint),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,1),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(10) == value.asUint);
  EXPECT(proc.readMemory(11) == value.asUint);
}

MTEST(TestMov,testMovAD0StallWhenAD0Written)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),//make sure addr is only increment once
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(9,value.asUint);
  proc.writeMemory(11,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(10) == value.asUint);
  EXPECT(proc.readMemory(11) == 0);
}

MTEST(TestMov,testMovAD1StallWhenAD1Written)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,0),//make sure addr is only increment once
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(9,value.asUint);
  proc.writeMemory(11,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(10) == value.asUint);
  EXPECT(proc.readMemory(11) == 0);
}

MTEST(TestMov,testMovAD0DoesNotStallWhenAD1Written)
{
  qfp32_t ad0=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad0.asUint),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(9,value.asUint);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(10) == value.asUint);
}

MTEST(TestMov,testMovAD1DoesNotStallWhenAD0Written)
{
  qfp32_t ad1=9;
  qfp32_t value=-0.25;

  uint32_t code[]=
  {
    SLCode::Load::create1(ad1.asUint),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
    SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD1,0,1),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };

  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(9,value.asUint);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(10) == value.asUint);
}