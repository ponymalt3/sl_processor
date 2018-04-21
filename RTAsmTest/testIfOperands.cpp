#include "RTAsmTest.h"
#include <mtest.h>

class testIfOperands : public mtest::test
{
};

MTEST(testIfOperands,test_that_const_const_operand_works)
{
  RTProg testCode=RTASM(
    def x 4;
    def y 4;
    ok=5;
    if(x == y)
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")));
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfOperands,test_that_const_ref_operand_works)
{
  RTProg testCode=RTASM(
    def x 4;
    ref y 0;
    ok=5;
    if(x == y)
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse(1).getNumErrors() == 0);
  tester.getProcessor().writeMemory(0,qfp32_t(4).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw());  
  tester.expectSymbol("ok",1);
}

MTEST(testIfOperands,test_that_ref_var_operand_works)
{
  RTProg testCode=RTASM(
    ref x 0;
    y=4;
    ok=5;
    if(x == y)
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse(1).getNumErrors() == 0);
  tester.getProcessor().writeMemory(0,qfp32_t(4).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw()); 
  tester.expectSymbol("ok",1); 
}

MTEST(testIfOperands,test_that_var_var_operand_works)
{
  RTProg testCode=RTASM(
    x=4;
    y=4;
    ok=5;
    if(x == y)
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw());
  tester.expectSymbol("ok",1);
}

MTEST(testIfOperands,test_that_var_deref_a0_operand_works)
{
  RTProg testCode=RTASM(
    x=4;
    a0=4;
    ok=5;
    if(x == [a0])
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  tester.getProcessor().writeMemory(4,qfp32_t(4).toRaw());

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw());
  tester.expectSymbol("ok",1);
}

MTEST(testIfOperands,test_that_deref_a0_deref_a1_with_inc_operand_works)
{
  RTProg testCode=RTASM(
    a0=10;
    a1=5;
    ok=7;
    if([a0] == [a1++]) 
      ok=1;
    else
      ok=0;
    end
    [a1]=6;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  tester.getProcessor().writeMemory(5,qfp32_t(4).toRaw());
  tester.getProcessor().writeMemory(10,qfp32_t(4).toRaw());
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw());
  EXPECT(tester.getProcessor().readMemory(6) == qfp32_t(6).toRaw());
  tester.expectSymbol("ok",1);
  tester.expectMemoryAt(6,6);
}

MTEST(testIfOperands,test_that_array_as_operand_works)
{
  RTProg testCode=RTASM(
    decl arr 10;
    ok=5;
    arr(5)=1;
    if(arr(5) == 1) 
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("ok")) == qfp32_t(1).toRaw());
  tester.expectSymbol("ok",1);
}