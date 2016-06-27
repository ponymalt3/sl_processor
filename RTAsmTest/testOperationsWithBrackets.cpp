#include "RTAsmTest.h"
#include <mtest.h>

class testOperationsWithBrackets : public mtest::test
{
};

MTEST(testOperationsWithBrackets,test_that_bracket_precedence_is_correct)
{
  RTProg testCode=RTASM(
    a=(1+4)/5;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(1).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_work)
{
  RTProg testCode=RTASM(
    a=-(1+4);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(-5).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_in_sum_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=a+-(1+4)+1;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(0).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_multiple_brackets_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(a-1)*(1+4);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(15).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_depth_2_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(1+a*(1+4))/7;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(3).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_depth_3_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(2+a*(1+(4-1)*2))/5;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(6).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_negate_inside_brackets_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=2+(-a);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(-2).asUint)
     << "read value is: " << qfp32_t::initFromRawData(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}