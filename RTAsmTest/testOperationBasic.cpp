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

MTEST(testOperationsBasic,test_that_log2_operation_works)
{
  RTProg testCode=RTASM(
    a=4096;
    c=log2(a);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",12);
}

MTEST(testOperationsBasic,test_that_log2_operation_const_works)
{
  RTProg testCode=RTASM(
    c=log2(128);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",7);
}

MTEST(testOperationsBasic,test_that_conversion_to_int_works)
{
  RTProg testCode=R"asm(
    a=99.3456;
    c=int(a);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99);
}

MTEST(testOperationsBasic,test_that_conversion_to_int_const_works)
{
  RTProg testCode=RTASM(
    c=int(99.3456);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99);
}

MTEST(testOperationsBasic,test_that_shift_operation_works)
{
  RTProg testCode=R"asm(
    a=99;
    b=7;
    c=shft(a,b);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99*128);
}

MTEST(testOperationsBasic,test_that_shift_operation_with_const_params_works)
{
  RTProg testCode=R"asm(
    c=shft(99,7);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99*128);
}


MTEST(testOperationsBasic,test_that_shift_operation_with_both_mem_works)
{
  RTProg testCode=R"asm(
    a0=0;
    a1=1;
    c=shft([a0],[a1]);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  _qfp32_t a=99;
  _qfp32_t b=7;
  tester.getProcessor().writeMemory(0,a.toRaw());
  tester.getProcessor().writeMemory(1,b.toRaw());
  
  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99*128);
}

MTEST(testOperationsBasic,test_that_operator_group2_both_mem_operand_fix_is_handled_automatically)
{
  //shift operation belongs to group2; this group cant be used with to mem and irs operands directly
  RTProg testCode=R"asm(
    b=7;
    a0=0;
    c=shft([a0],b);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  _qfp32_t a=99;
  tester.getProcessor().writeMemory(0,a.toRaw());
  
  tester.loadCode();
  tester.execute();
   
  tester.expectSymbol("c",99*128);
}