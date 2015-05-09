#include <unittest++/UnitTest++.h>
#include "SLProcessorTest.h"

SUITE(TestMov)
{
  TEST(MovAD0)
  {
    qfp32_t ad0=10;
    qfp32_t value=31.5;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad0.asUint),
      SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(7);//include one cycle stall
    
    CHECK(proc.readMemory(10) == value.asUint);
  }
  
  TEST(MovAD1)
  {
    qfp32_t ad1=10;
    qfp32_t value=-31.5;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad1.asUint),
      SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(7);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }

  TEST(MovAD0AndInc)
  {
    qfp32_t ad0=10;
    qfp32_t value=31.0625;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad0.asUint),
      SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,1),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(9);
    
    CHECK(proc.readMemory(10) == value.asUint);
    CHECK(proc.readMemory(11) == value.asUint);
  }
  
  TEST(MovAD1AndInc)
  {
    qfp32_t ad0=10;
    qfp32_t value=31.0625;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad0.asUint),
      SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES,0,0),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,1),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES,0,1),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(6);
    
    CHECK(proc.readMemory(10) == value.asUint);
    CHECK(proc.readMemory(11) == value.asUint);
  }
  
  TEST(testMovAD0StallWhenAD0Written)
  {
    qfp32_t ad0=9;
    qfp32_t value=-0.25;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad0.asUint),
      SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
      SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,1),
      SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
 
    LoadAndSimulateProcessor proc(code);
    
    proc.writeMemory(9,value.asUint);
    
    proc.run(8);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }
  
  TEST(testMovAD0MemoryDataForward)
  {
    qfp32_t ad0=10;
    qfp32_t value=-11.875;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(ad0.asUint),
      SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES,0,0),
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES,0,0),
      SLCode::Mov::create(SLCode::REG_RES,SLCode::DEREF_AD0,0,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };

    LoadAndSimulateProcessor proc(code);
   
    proc.run(8);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }
}