#include "RTAsmTest.h"
#include <mtest.h>

class testAssignment : public mtest::test
{
};

MTEST(testAssignment,test_that_def_const_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    def x 13;
    a=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(0) == qfp32_t(13).asUint);  
}

MTEST(testAssignment,test_that_ref_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    ref x 0;%param 0%
    a=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(1).getNumErrors() == 0);//use one parameter
  EXPECT(tester.getIRSAddrOfSymbol("a") == 1);
  
  qfp32_t expect=13;
  
  tester.getProcessor().writeMemory(0,expect.asUint);
  
  tester.loadCode();
  tester.execute();  
    
  EXPECT(tester.getProcessor().readMemory(1) == expect.asUint);  
}

MTEST(testAssignment,test_that_const_assigned_to_ref_works)
{
  RTProg testAssign=RTASM(
    ref x 0;%param 0%
    x=7;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(1).getNumErrors() == 0);//use one parameter
  
  tester.getProcessor().writeMemory(0,qfp32_t(13).asUint);
  
  tester.loadCode();
  tester.execute();  
  
  qfp32_t expect=7;
  EXPECT(tester.getProcessor().readMemory(0) == expect.asUint);  
}

MTEST(testAssignment,test_that_const_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    a=1;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 0);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=1;
  
  EXPECT(tester.getProcessor().readMemory(0) == expect.asUint);  
}

MTEST(testAssignment,test_that_var_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    a=1;
    b=a;
    a=0;%otherwise a is optimized away%
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  EXPECT(tester.getIRSAddrOfSymbol("b") == 1);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=1;  
  EXPECT(tester.getProcessor().readMemory(1) == expect.asUint);  
}

MTEST(testAssignment,test_that_def_const_assigned_to_array_works)
{
  RTProg testAssign=RTASM(
    def x 7;
    decl array 5;
    array(2)=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("array") == 1);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=7;  
  EXPECT(tester.getProcessor().readMemory(1+2) == expect.asUint);  
}

MTEST(testAssignment,test_that_ref_assigned_to_array_works)
{
  RTProg testAssign=RTASM(
    def x 1;
    decl array 5;
    array(2)=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(2).getNumErrors() == 0);//allocate 2 parameters
  EXPECT(tester.getIRSAddrOfSymbol("array") == 2);
  
  qfp32_t expect=7; 
  tester.getProcessor().writeMemory(1,expect.asUint);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(2+2) == expect.asUint);  
}

MTEST(testAssignment,test_that_array_assigned_to_ref_works)
{
  RTProg testAssign=RTASM(
    def x 1;
    decl array 5;
    array(4)=15;
    x=array(4);
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(2).getNumErrors() == 0);//allocate 2 parameters
  EXPECT(tester.getIRSAddrOfSymbol("array") == 2);
  
  tester.getProcessor().writeMemory(1,qfp32_t(7).asUint);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(1) == qfp32_t(15).asUint);  
}

MTEST(testAssignment,test_that_array_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    decl array 5;
    a=0;
    array(0)=15;
    x=array(4);
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(2).getNumErrors() == 0);//allocate 2 parameters
  EXPECT(tester.getIRSAddrOfSymbol("array") == 2);
  
  tester.getProcessor().writeMemory(1,qfp32_t(7).asUint);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(2+2) == qfp32_t(15).asUint);  
}

MTEST(testAssignment,test_that_const_assigned_to_addr_and_deref_addr_works)
{
  RTProg testAssign=RTASM(
    ad=10;
    [ad0]=17;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=17;  
  EXPECT(tester.getProcessor().readMemory(10) == expect.asUint);  
}




/*
MTEST(testSyntax,test_assign)
{
  RTProg testAssign=RTASM(
    def a 23;

    b=0.735;
    c=13;

  %with spaces%
    d = c ;

  %addr internal regs%
    a0=10;
    a1=20;
    a1=-b*c;
    a1=c;

  %deref addrs%
    [a0]=[a1];
    [a0]=[a0];
    [a0]=a;
    [a0]=[a1]+23*-a;

  %simple assignments%
    c=c*2;
    b=3.4594;
    b=c;
  );

  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);    
}*/