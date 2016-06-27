#include "RTAsmTest.h"
#include <mtest.h>

class testOperationsBasic : public mtest::test
{
};

MTEST(testOperationsBasic,test_that_const_const_operation_works)
{
  RTProg testCode=RTASM(
    a=4/5;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
}

MTEST(testOperationsBasic,test_that_const_var_operation_works)
{
  RTProg testCode=RTASM(
    b=5;
    a=4/b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
}

MTEST(testOperationsBasic,test_that_var_ref_operation_works)
{
  RTProg testCode=RTASM(
    ref x 4;
    b=4;
    a=b/x;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(4,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5); 
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
}

MTEST(testOperationsBasic,test_that_ref_const_sym_operation_works)
{
  RTProg testCode=RTASM(
    ref x 4;
    def y 5;
    a=x/y;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(4,qfp32_t(4).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
}








MTEST(testOperationsBasic,test_that_const_ad0_operation_works)
{
  RTProg testCode=RTASM(
    x=4;
    a0=5;
    a=x/[a0];
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
}

MTEST(testOperationsBasic,test_that_const_ad0_with_inc_operation_works)
{
  RTProg testCode=RTASM(
    x=4;
    a0=5;
    a=x/[a0++];
    [a0]=6;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(6).asUint);
}

MTEST(testOperationsBasic,test_that_ad0_ad1_operation_works)
{
  RTProg testCode=RTASM(
    a0=5;
    a1=8;
    a=[a0]/[a1];
    [a0]=30;
    [a1]=31;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).asUint);
  tester.getProcessor().writeMemory(8,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(30).asUint);
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).asUint);
}

MTEST(testOperationsBasic,test_that_ad0_with_inc_ad1_operation_works)
{
  RTProg testCode=RTASM(
    a0=5;
    a1=8;
    a=[a0++]/[a1];
    [a0]=30;
    [a1]=31;
  );;
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).asUint);
  tester.getProcessor().writeMemory(8,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).asUint);
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).asUint);
}

MTEST(testOperationsBasic,test_that_ad0_ad1_with_inc_operation_works)
{
  RTProg testCode=RTASM(
    a0=5;
    a1=8;
    a=[a0]/[a1++];
    [a0]=30;
    [a1]=31;
  );;
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).asUint);
  tester.getProcessor().writeMemory(8,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(30).asUint);
  EXPECT(tester.getProcessor().readMemory(9) == qfp32_t(31).asUint);
}

MTEST(testOperationsBasic,test_that_ad0_with_inc_ad1_with_inc_operation_works)
{
  RTProg testCode=RTASM(
    a0=5;
    a1=8;
    a=[a0++]/[a1++];
    [a0]=30;
    [a1]=31;
  );;
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).asUint);
  tester.getProcessor().writeMemory(8,qfp32_t(5).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).asUint);
  EXPECT(tester.getProcessor().readMemory(9) == qfp32_t(31).asUint);
}

MTEST(testOperationsBasic,test_that_ad1_ad0_with_inc_operation_works)
{
  RTProg testCode=RTASM(
    a0=5;
    a1=8;
    a=[a1]/[a0++];
    [a0]=30;
    [a1]=31;
  );;
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).asUint);
  tester.getProcessor().writeMemory(8,qfp32_t(4).asUint);

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.asUint);
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).asUint);
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).asUint);
}









MTEST(testOperationsBasic,test_that_var_var_addition_works)
{
  RTProg testCode=RTASM(
    a=4;
    b=5;
    c=a+b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(9).asUint);
}

MTEST(testOperationsBasic,test_that_var_var_substraction_works)
{
  RTProg testCode=RTASM(
    a=4;
    b=5;
    c=a-b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(-1).asUint);
}

MTEST(testOperationsBasic,test_that_var_var_multiplication_works)
{
  RTProg testCode=RTASM(
    a=4;
    b=5;
    c=a*b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(20).asUint);
}

MTEST(testOperationsBasic,test_that_var_var_division_works)
{
  RTProg testCode=RTASM(
    a=4;
    b=5;
    c=a/b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
 
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == expect.asUint);
}

MTEST(testOperationsBasic,test_that_var_negate_works)
{
  RTProg testCode=RTASM(
    a=4;
    c=-a;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(-4).asUint);
}