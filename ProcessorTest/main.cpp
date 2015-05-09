#include <unittest++/UnitTest++.h>
#include "SLProcessorTest.h"

SUITE(TestLoad)
{
  TEST(Load0)
  {
    qfp32_t value=-24.5;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(value.asUint),
      SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(5);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }

  TEST(Load1)
  {
    qfp32_t value=-36;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(value.asUint),
      SLCode::Load::constDataValue2(value.asUint),
      SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(6);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }
  
  TEST(Load2)
  {
    qfp32_t value=-360000.968438;
    std::cout<<"value as uint: "<<std::hex<<(value.asUint)<<std::dec<<"\n";
    uint32_t code[]=
    {
      SLCode::Load::create(value.asUint),
      SLCode::Load::constDataValue3(value.asUint),
      SLCode::Load::constDataValue2(value.asUint),
      SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,10,0),
      0xFFFF,
      0xFFFF,
      0xFFFF
    };
    
    LoadAndSimulateProcessor proc(code);
    
    proc.run(7);
    
    CHECK(proc.readMemory(10) == value.asUint);
  }
}

#include "testMov.cpp"


// run all tests
int main(int argc, char **argv)
{
	return UnitTest::RunAllTests();
}
