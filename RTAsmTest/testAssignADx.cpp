#include "RTAsmTest.h"
#include <mtest.h>

class testAssignADx : public mtest::test
{
};

MTEST(testAssignADx,test_that_def_const_assigned_to_addr0_and_deref_addr0_works)
{
  RTProg testAssign=RTASM(
    def x 10;
    a0=x;
    [a0]=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(10) == qfp32_t(10).asUint);  
}

MTEST(testAssignADx,test_that_ref_assigned_to_addr0_and_deref_addr0_works)
{
  RTProg testAssign=RTASM(
    ref x 0;
    a0=x;
    [a0]=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse(1).getNumErrors() == 0);
  
  qfp32_t value=21;  
  tester.getProcessor().writeMemory(0,value.asUint);
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(value) == value.asUint);  
}

MTEST(testAssignADx,test_that_const_assigned_to_addr0_and_deref_addr0_works)
{
  RTProg testAssign=RTASM(
    a0=10;
    [a0]=17;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  qfp32_t value=17;
  EXPECT(tester.getProcessor().readMemory(10) == value.asUint);  
}

MTEST(testAssignADx,test_that_var_assigned_to_addr0_and_deref_addr0_works)
{
  RTProg testAssign=RTASM(
    a=23;
    a0=a;
    [a0]=a;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  qfp32_t value=23;
  EXPECT(tester.getProcessor().readMemory(value) == value.asUint);  
}

MTEST(testAssignADx,test_that_array_assigned_to_addr0_and_deref_addr0_works)
{
  RTProg testAssign=RTASM(
    decl array 3;
    
    array(0)=2;
    array(1)=3;
    
    a0=array(0);
    [a0]=array(1);
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(2) == qfp32_t(3).asUint);  
}

MTEST(testAssignADx,test_that_def_const_assigned_to_addr1_and_deref_addr1_works)
{
  RTProg testAssign=RTASM(
    def x 10;
    a1=x;
    [a1]=x;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(10) == qfp32_t(10).asUint);  
}

MTEST(testAssignADx,test_that_var_assigned_to_deref_addr0_with_inc_works)
{
  RTProg testAssign=RTASM(
    a=7;
    a0=5;
    [a0++]=a;
    [a0]=99;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(7).asUint); 
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(99).asUint); 
}

MTEST(testAssignADx,test_that_var_assigned_to_deref_addr1_with_inc_works)
{
  RTProg testAssign=RTASM(
    a=7;
    a1=5;
    [a1++]=a;
    [a1]=99;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(7).asUint); 
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(99).asUint); 
}

MTEST(testAssignADx,test_that_deref_addr0_assigned_to_deref_addr1_with_both_inc_works)
{
  RTProg testAssign=RTASM(
    a0=7;
    a1=5;
    [a0]=700;
    [a1++]=[a0++];
    [a0]=27;
    [a1]=43;
  );
    
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(7) == qfp32_t(700).asUint);
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(700).asUint);
  EXPECT(tester.getProcessor().readMemory(8) == qfp32_t(27).asUint);
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(43).asUint); 
}

MTEST(testAssignADx,test_that_var_assigned_to_deref_addr0_without_inc_works)
{
  RTProg testAssign=RTASM(
    a=7;
    a0=5;
    [a0]=a;
    [a0]=99;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(5) == qfp32_t(99).asUint);
}