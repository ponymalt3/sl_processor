#include "RTAsmTest.h"
#include <mtest.h>

class testFunction : public mtest::test
{
};

MTEST(testFunction,test_that_parameter_symbols_are_function_local_only)
{
  RTProg testCode=RTASM(
    function test(param1,param2)
      x=param1+param2;
    end
    
    var=99;
    
    function test2(paramx,paramy)
      x=var+param2;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testFunction,test_that_simple_function_without_return_works)
{
  RTProg testCode=RTASM(
    function test()
    end
    
    x=1;
    test();
    x=3;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(3).toRaw());
  tester.expectSymbol("x",3);
}

MTEST(testFunction,test_that_simple_function_with_return_works)
{
  RTProg testCode=RTASM(
    function test()
      return;
    end
    
    x=1;
    test();
    x=3;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(3).toRaw());
  tester.expectSymbol("x",3);
}

MTEST(testFunction,test_load_array_addr_in_function_works)
{
  RTProg testCode=RTASM(
    function test()
      decl arr 6;
      a0=arr+1;
      arr(1)=99;
      [a0]=1;
      return arr(1);
    end
    x=test();
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(1).toRaw());
  tester.expectSymbol("x",1);  
}

MTEST(testFunction,test_function_return_ends_the_function_correctly)
{
  RTProg testCode=RTASM(
    function test(p)
      if(p == 1)
        return 99;
      end
      
      return 11;
    end
    x=test(1);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(99).toRaw());
  tester.expectSymbol("x",99);  
}

MTEST(testFunction,test_function_return_correct_value)
{
  RTProg testCode=RTASM(
    function test()
      return 17;
    end
    x=test();
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(17).toRaw());
  tester.expectSymbol("x",17);
}

MTEST(testFunction,test_function_with_parameter_return_correct_value)
{
  RTProg testCode=RTASM(
    function test(p1,p2)
      return p1+p2;
    end
    x=test(19,23);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(42).toRaw());
  tester.expectSymbol("x",42);  
}

MTEST(testFunction,test_function_with_function_call_works)
{
  RTProg testCode=RTASM(
    function yyy(x,z)
      return x*z;
    end
    
    function test(p1,p2)
      return yyy(p1,p2)+p2;
    end
    
    x=test(19,23);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(19*23+23).toRaw());
  tester.expectSymbol("x",19*23+23);  
}

MTEST(testFunction,test_function_with_local_storage_works)
{
  RTProg testCode=RTASM(
    function test(p1,p2)
      decl arr 2;
      t=99;
      arr(0)=1;
      arr(1)=2;
      y=t-10*arr(1)-arr(0);
      return t+y+p2;
    end
    
    x=test(19,23);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(99-20-1+99+23).toRaw());
  tester.expectSymbol("x",99-20-1+99+23); 
}

MTEST(testFunction,test_recursive_function_works)
{
  RTProg testCode=RTASM(
    function fib(n)
      if(n == 1)
        return 1;
      end
      return n+fib(n-1);
    end
    
    x=fib(5);
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("x")) == qfp32_t(15).toRaw());
  tester.expectSymbol("x",15);
}