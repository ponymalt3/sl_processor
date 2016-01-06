#include <mtest.h>
#include "SLProcessorTest.h"

class testMovMemX : public mtest::test
{
};

MTEST(testMovMemX,MovAD0)
{
  qfp32_t ad1=1000;
  qfp32_t value=31.5;
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
  
  proc.run(7);//include one cycle stall
  
  EXPECT(proc.readMemory(10) == value.asUint);
}
  
MTEST(testMovMemX,MovAD1AndInc)
{
  qfp32_t ad1=10;
  qfp32_t value=31.0625;
  std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create(ad1.asUint),
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
  
  proc.run(9);
  
  EXPECT(proc.readMemory(10) == value.asUint);
  EXPECT(proc.readMemory(11) == value.asUint);
}