#include "RTAsmTest.h"
#include <mtest.h>

class testIfOperator : public mtest::test
{
};

MTEST(testIfOperator,test_that_operator_equal_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    a=4;
    b=0;
    if(a == x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_equal_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 10;
    a=4;
    b=0;
    if(a == x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}

MTEST(testIfOperator,test_that_operator_less_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 6;
    a=4;
    b=0;
    if(a < x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_less_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    a=4;
    b=0;
    if(a < x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}

MTEST(testIfOperator,test_that_operator_less_equal_1_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 7;
    a=4;
    b=0;
    if(a <= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_less_equal_2_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    a=4;
    b=0;
    if(a <= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_less_equal_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 2;
    a=4;
    b=0;
    if(a <= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}

MTEST(testIfOperator,test_that_operator_greater_equal_1_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x -3;
    a=4;
    b=0;
    if(a >= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_greater_equal_2_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    a=4;
    b=0;
    if(a >= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_greater_equal_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 20;
    a=4;
    b=0;
    if(a >= x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}

MTEST(testIfOperator,test_that_operator_greater_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x -2;
    a=4;
    b=0;
    if(a > x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_greater_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 10;
    a=4;
    b=0;
    if(a > x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}

MTEST(testIfOperator,test_that_operator_not_equal_execute_then_branch)
{
  RTProg testCode=RTASM(
    def x 30;
    a=4;
    b=0;
    if(a != x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(1).asUint);  
}

MTEST(testIfOperator,test_that_operator_not_equal_execute_else_branch)
{
  RTProg testCode=RTASM(
    def x 4;
    a=4;
    b=0;
    if(a != x)
      b=1;
    else
      b=2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  uint32_t offset=tester.getIRSAddrOfSymbol("b");
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(offset) == qfp32_t(2).asUint);  
}