#include "RTAsmTest.h"
#include <mtest.h>

class testOperationsExtended : public mtest::test
{
};

MTEST(testOperationsExtended,test_that_const_sum_works)
{
  RTProg testCode=RTASM(
    a=4+5+6+13+99;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(127).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",127);
}

MTEST(testOperationsExtended,test_that_mixed_sum_works)
{
  RTProg testCode=RTASM(
    a0=13;
    b=7;
    c=-3;
    a=4+b+[a0]+c;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(13,qfp32_t(119).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(127).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",127);
}

MTEST(testOperationsExtended,test_that_substraction_chain_with_negates_works)
{
  RTProg testCode=RTASM(
    a0=13;
    b=7;
    c=-3;
    a=4-(-3)-[a0]-c--b;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(13,qfp32_t(-110).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(127).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));

  tester.expectSymbol("a",127);
}

MTEST(testOperationsExtended,test_that_operator_precedence_is_correct_with_const)
{
  RTProg testCode=RTASM(
    a=1+4/5/6*7+-9;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(1)+qfp32_t(4)/qfp32_t(5)/qfp32_t(6)*qfp32_t(7)-qfp32_t(9);
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == expect.toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",expect);
}

MTEST(testOperationsExtended,test_that_operator_precedence_is_correct_with_vars)
{
  RTProg testCode=RTASM(
    a=1;
    b=4;
    c=5;
    d=6;
    e=7;
    f=9;
    g=a+b/c/d*e+-f;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  qfp32_t expect=qfp32_t(1)+qfp32_t(4)/qfp32_t(5)/qfp32_t(6)*qfp32_t(7)-qfp32_t(9);
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("g")) == expect.toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("g")));
     
  tester.expectSymbol("g",expect);
}

MTEST(testOperationsExtended,test_that_negate_with_const_works)
{
  RTProg testCode=RTASM(
    a=5*-8;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(-40).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",-40);
}

MTEST(testOperationsExtended,test_that_negate_with_var_works)
{
  RTProg testCode=RTASM(
    a=8;
    b=5*-a;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
    std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";


  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")) == qfp32_t(-40).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("b")));
     
  tester.expectSymbol("b",-40);
}

MTEST(testOperationsExtended,test_that_negate_with_mem_works)
{
  RTProg testCode=RTASM(
    a0=13;
    a=5*-[a0];
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(13,qfp32_t(8).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(-40).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",-40);
}

//internally for optimization operands should be swapped
class testOperationsSpecialBehaviour : public mtest::test
{
};

MTEST(testOperationsSpecialBehaviour,test_that_add_var_const_works)
{
  RTProg testCode=RTASM(
    a=5;
    b=a+6;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(11).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",11);
}

MTEST(testOperationsSpecialBehaviour,test_that_mul_var_const_works)
{
  RTProg testCode=RTASM(
    a=5;
    b=a*8;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(40).toRaw())
     << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
     
  tester.expectSymbol("a",40);
}