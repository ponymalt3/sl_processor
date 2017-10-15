#include "RTAsmTest.h"
#include <mtest.h>

class testLoopFast : public mtest::test
{
};

MTEST(testLoopFast,test_that_fast_loop_is_generated_if_loop_body_is_not_complex)
{
  RTProg testCode=RTASM(
    a=0;
    loop(5)
      a=a+1;
    end
    a=a;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();  
  tester.execute();
  
  EXPECT(tester.getCodeSize() == 10) << "expect smaller code size";
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(5).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}