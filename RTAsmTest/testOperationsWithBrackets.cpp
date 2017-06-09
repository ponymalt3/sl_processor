#include "RTAsmTest.h"
#include <mtest.h>

class testOperationsWithBrackets : public mtest::test
{
};

MTEST(testOperationsWithBrackets,test_that_bracket_precedence_is_correct_with_const)
{
  RTProg testCode=RTASM(
    a=(1+4)/5;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(1).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_precedence_is_correct_with_vars)
{
  RTProg testCode=RTASM(
    a=1;
    b=4;
    c=5;
    d=(a+b)/c;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("d")) == qfp32_t(1).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("d")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_work_with_const)
{
  RTProg testCode=RTASM(
    a=-(1+4);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(-5).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_work_with_vars)
{
  RTProg testCode=RTASM(
    a=1;
    b=4;
    c=-(a+b);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(-5).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_in_const_sum_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=a+-(1+4)+1;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
    std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";
    
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(0).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_in_sum_with_vars_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=1;
    c=4;
    d=a+-(b+c)+b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("d")) == qfp32_t(0).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("d")));
}

MTEST(testOperationsWithBrackets,test_that_multiple_brackets_with_const_term_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(a-1)*(1+4);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(15).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_multiple_brackets_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(a-1)*(1+a);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(15).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_depth_2_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(1+a*(1+a))/7;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(3).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_depth_3_work)
{
  RTProg testCode=RTASM(
    a=4;
    b=(2+a*(1+(a-1)*2))/5;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(6).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
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
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(-2).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
}

MTEST(testOperationsWithBrackets,test_that_bracket_negate_with_const_works)
{
  RTProg testCode=RTASM(
    a=5*-(8+1);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(-45).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
}