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
  
  EXPECT(tester.getCodeSize() == 10+16) << "expect smaller code size";//entry vector of size 16
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(5).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
    
  tester.expectSymbol("a",5);
}

MTEST(testLoopFast,test_that_fast_loop_works_with_big_values)
{
  RTProg testCode=RTASM(
    a=0;
    loop(99999)
      a=a+1;
    end
    a=a;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();  
  tester.execute();
  
  EXPECT(tester.getCodeSize() == 11+16) << "expect different code size";//entry vector of size 16
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(99999).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
    
  tester.expectSymbol("a",99999);
}

MTEST(testLoopFast,test_that_fast_loop_inside_another_loop_works)
{
  RTProg testCode=R"(
    loop(1)
      sum=0;
      b=99;
      a1=20;
      a0=21;
      ttt=1;
      loop(ttt)
        sum=sum+[a0++]*[a1++];
      end
    end
    sum=sum;
    ttt=ttt;)";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(qfp32_t::fromDouble(20),qfp32_t::fromDouble(99.0));
  tester.getProcessor().writeMemory(qfp32_t::fromDouble(21),qfp32_t::fromDouble(1.0));

  tester.loadCode();  
  tester.execute();
  
  tester.expectSymbol("sum",99);
}