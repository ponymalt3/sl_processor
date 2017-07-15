#include "RTAsmTest.h"
#include <mtest.h>

class testFunctionDecl : public mtest::test
{
};

MTEST(testFunctionDecl,test_that_simple_empty_function_works)
{
  RTProg testCode=RTASM(
    function test()
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testFunctionDecl,test_that_function_with_parameters_works)
{
  RTProg testCode=RTASM(
    function test(param1,param2)
      x=param2+param2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testFunctionDecl,test_that_multiple_functions_with_parameters_works)
{
  RTProg testCode=RTASM(
    function test(param1,param2)
      x=param2+param2;
    end
    
    function test2(paramx,paramy)
      x=paramy+paramx;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testFunctionDecl,test_that_parameter_symbols_are_function_local_only)
{
  RTProg testCode=RTASM(
    function test(param1,param2)
      x=param1+param2;
    end
    
    function test2(paramx,paramy)
      x=paramy+param2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testFunctionDecl,test_that_function_with_void_return_works)
{
  RTProg testCode=RTASM(
    function test(param1,param2)
      x=param2+param2;
      return;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testFunctionDecl,test_that_function_with_expr_return_works)
{
  RTProg testCode=RTASM(
    ref stack 0;
    stack=0;
    
    function test(param1,param2)
      return param1+param2;
    end
    
    x=test(1,2);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  std::cout<<(tester.getDisAsmString())<<"\n";
  tester.getProcessor().writeMemory(tester.getIRSAddrOfSymbol("x"),qfp32_t(0).toRaw());

  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(3).toRaw());
}