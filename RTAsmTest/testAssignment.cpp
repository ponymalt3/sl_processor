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
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(13).toRaw());  
}

MTEST(testAssignment,test_that_ref_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    ref x 4;%param 0%
    a=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(1).getNumErrors() == 0);//use one parameter
  EXPECT(tester.getIRSAddrOfSymbol("a") == 5);
  
  qfp32_t expect=13;
  
  tester.getProcessor().writeMemory(4,expect.toRaw());
  
  tester.loadCode();
  tester.execute();  
    
  EXPECT(tester.getProcessor().readMemory(5) == expect.toRaw());  
}

MTEST(testAssignment,test_that_const_assigned_to_ref_works)
{
  RTProg testAssign=RTASM(
    ref x 9;%param 0%
    x=7;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(1).getNumErrors() == 0);//use one parameter
  
  tester.getProcessor().writeMemory(9,qfp32_t(13).toRaw());
  
  tester.loadCode();
  tester.execute();  
  
  qfp32_t expect=7;
  EXPECT(tester.getProcessor().readMemory(9) == expect.toRaw());  
}

MTEST(testAssignment,test_that_const_assigned_to_var_works)
{
  RTProg testAssign=RTASM(
    a=1;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=1;
  
  EXPECT(tester.getProcessor().readMemory(4) == expect.toRaw());  
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
  
  EXPECT(tester.getIRSAddrOfSymbol("b") == 5);//first 4 words reserved for calls
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=1;  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == expect.toRaw());  
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
  EXPECT(tester.getIRSAddrOfSymbol("array") == 8);
  
  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=7;  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("array")+2) == expect.toRaw());  
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
  tester.getProcessor().writeMemory(1,expect.toRaw());
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(addr+2) == expect.toRaw());  
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
  
  tester.getProcessor().writeMemory(1,qfp32_t(7).toRaw());
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(1) == qfp32_t(15).toRaw());  
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
  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);//first 4 words reserved for calls
  EXPECT(tester.getIRSAddrOfSymbol("array") == 8);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(15).toRaw());  
}

MTEST(testAssignment,test_that_array_base_addr_load_works)
{
  RTProg testAssign=RTASM(
    a=0;
    decl array 4;
    array(1)=5;
    a0=array+1;
    [a0]=1;
    a=1;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  EXPECT(tester.getIRSAddrOfSymbol("array") == 8);

  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("array")+1) == qfp32_t(1).toRaw()); 
}