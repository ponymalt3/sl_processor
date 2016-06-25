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
  EXPECT(tester.getIRSAddrOfSymbol("array") == 0);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=7;  
  EXPECT(tester.getProcessor().readMemory(0+2) == expect.asUint);  
}

MTEST(testAssignment,test_that_ref_assigned_to_array_works)
{
  RTProg testAssign=RTASM(
    ref x 1;
    decl array 5;
    array(2)=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(2).getNumErrors() == 0);//allocate 2 parameters
  
  uint32_t addr=tester.getIRSAddrOfSymbol("array");
  EXPECT(addr == 8);
  
  qfp32_t expect=7; 
  tester.getProcessor().writeMemory(1,expect.asUint);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(addr+2) == expect.asUint);  
}

MTEST(testAssignment,test_that_array_assigned_to_ref_works)
{
  RTProg testAssign=RTASM(
    ref x 1;
    decl array 5;
    array(4)=15;
    x=array(4);
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(2).getNumErrors() == 0);//allocate 2 parameters
  EXPECT(tester.getIRSAddrOfSymbol("array") == 8);
  
  tester.getProcessor().writeMemory(1,qfp32_t(7).asUint);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(1) == qfp32_t(15).asUint);  
}

MTEST(testAssignment,test_that_array_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    decl array 4;
    a=0;
    array(0)=15;
    a=array(0);
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 0);
  EXPECT(tester.getIRSAddrOfSymbol("array") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(0) == qfp32_t(15).asUint);  
}

MTEST(testAssignment,test_that_array_base_addr_load_works)
{
  RTProg testAssign=RTASM(
    decl array 4;
    array(0)=5;
    a0=array;
    [a0]=1;
  );
  ;
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  EXPECT(tester.getIRSAddrOfSymbol("array") == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(0) == qfp32_t(1).asUint);  
}