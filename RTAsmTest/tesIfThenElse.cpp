#include "RTAsmTest.h"
#include <mtest.h>

class testIfThenElse : public mtest::test
{
};

MTEST(testIfThenElse,test_that_then_branch_only_works_and_execute)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=5;
    if(x == 4)
      ok=1;
    end
    c=99;
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_then_branch_only_works_and_not_execute_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=1;
    if(x == 5)
      ok=5;
    end
    c=99;    
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_then_else_branches_works_and_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=1;
    if(x == 4)
      ok=1;
    else
      ok=5;
    end
    c=99;
    d=ok; 
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_then_else_branches_works_and_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=1;
    if(x == 5)
      ok=5;
    else
      ok=1;
    end
    c=99;  
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_empty_then_branch_works_when_executing_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=1;
    if(x == 4)
    else
      ok=5;
    end
    c=99; 
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_empty_then_branch_works_when_executing_else_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=5;
    if(x == 5)
    else
      ok=1;
    end
    c=99; 
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_empty_else_branch_works_when_executing_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=5;
    if(x == 4)
      ok=1;
    else
    end
    c=99;
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}

MTEST(testIfThenElse,test_that_empty_else_branch_works_when_executing_else_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    ok=1;
    if(x == 5)
      ok=5;
    else
    end
    c=99; 
    d=ok;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).asUint);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(99).asUint);
}