#include <unittest++/UnitTest++.h>
#include "SLProcessorTest.h"

SUITE(testMovMemX)
{
  TEST(MovAD0)
  {
    qfp32_t ad0=1000;
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
}