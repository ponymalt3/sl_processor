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
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  tester.expectSymbol("a",expect);
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
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  tester.expectSymbol("a",expect);
}

MTEST(testOperationsBasic,test_that_var_ref_operation_works)
{
  RTProg testCode=RTASM(
    ref x 8;
    b=4;
    a=b/x;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(8,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5); 
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  tester.expectSymbol("a",expect);
}

MTEST(testOperationsBasic,test_that_ref_const_sym_operation_works)
{
  RTProg testCode=RTASM(
    ref x 9;
    def y 5;
    a=x/y;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.getProcessor().writeMemory(9,qfp32_t(4).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  tester.expectSymbol("a",expect);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  tester.expectSymbol("a",expect);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(6).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(6,6);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).toRaw());
  tester.getProcessor().writeMemory(8,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(30).toRaw());
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(5,30);
  tester.expectMemoryAt(8,31);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).toRaw());
  tester.getProcessor().writeMemory(8,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).toRaw());
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(6,30);
  tester.expectMemoryAt(8,31);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).toRaw());
  tester.getProcessor().writeMemory(8,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(30).toRaw());
  EXPECT(tester.getProcessor().readMemory(9) == qfp32_t(31).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(5,30);
  tester.expectMemoryAt(9,31);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(4).toRaw());
  tester.getProcessor().writeMemory(8,qfp32_t(5).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).toRaw());
  EXPECT(tester.getProcessor().readMemory(9) == qfp32_t(31).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(6,30);
  tester.expectMemoryAt(9,31);
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
  
  tester.getProcessor().writeMemory(5,qfp32_t(5).toRaw());
  tester.getProcessor().writeMemory(8,qfp32_t(4).toRaw());

  tester.loadCode();
  tester.execute();
   
  qfp32_t expect=qfp32_t(4)/qfp32_t(5);
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw());
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(30).toRaw());
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(31).toRaw());
  tester.expectSymbol("a",expect);
  tester.expectMemoryAt(6,30);
  tester.expectMemoryAt(8,31);
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
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(9).toRaw());
  tester.expectSymbol("c",9);
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
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(-1).toRaw());
  tester.expectSymbol("c",-1);
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
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(20).toRaw());
  tester.expectSymbol("c",20);
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
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == expect.toRaw());
  tester.expectSymbol("c",expect);
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
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("c")) == qfp32_t(-4).toRaw());
  tester.expectSymbol("c",-4);
}