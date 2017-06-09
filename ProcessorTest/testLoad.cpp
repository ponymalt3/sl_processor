#include <mtest.h>
#include "SLProcessorTest.h"

class TestLoad : public mtest::test
{  
};

MTEST(TestLoad,Load0)
{
  qfp32_t value=-24.5;
  std::cout<<"value as uint: "<<std::hex<<(value.toRaw())<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create(SLCode::Load::constDataValue1(value.toRaw())),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(4);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
}

MTEST(TestLoad,Load1)
{
  qfp32_t value=-36;
  std::cout<<"value as uint: "<<std::hex<<(value.toRaw())<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create(SLCode::Load::constDataValue1(value.toRaw())),
    SLCode::Load::create(SLCode::Load::constDataValue2(value.toRaw())),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(5);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
}
  
MTEST(TestLoad,Load2)
{
  qfp32_t value=-360000.968438;
  std::cout<<"value as uint: "<<std::hex<<(value.toRaw())<<std::dec<<"\n";
  uint32_t code[]=
  {
    SLCode::Load::create(SLCode::Load::constDataValue1(value.toRaw())),
    SLCode::Load::create(SLCode::Load::constDataValue2(value.toRaw())),
    SLCode::Load::create(SLCode::Load::constDataValue3(value.toRaw())),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.run(6);
  
  EXPECT(proc.readMemory(10) == value.toRaw());
}