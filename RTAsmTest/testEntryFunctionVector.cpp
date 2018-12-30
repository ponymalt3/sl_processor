#include "RTAsmTest.h"
#include <mtest.h>

class testEntryFunctionVector : public mtest::test
{
};

MTEST(testEntryFunctionVector,test_that_main_function_is_called_by_default)
{
  RTProg test=R"asm(
    function a()
      a0=99;
      [a0]=1;
    end
    
    function b()
      a0=99;
      [a0]=2;
    end
    
    function main()
      a0=99;
      [a0]=98;
    end
  )asm";
  
  RTProgTester tester(test);
  EXPECT(tester.parse(0,true).getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.loadCode();
  tester.execute();  
    
  tester.expectMemoryAt(99,98.0);
}

MTEST(testEntryFunctionVector,test_that_specific_function_is_called_when_available)
{
  RTProg test=R"asm(
    function a()
      a0=99;
      [a0]=1;
    end
    
    function main()
      a0=99;
      [a0]=98;
    end
    
    function main2()
      a0=99;
      [a0]=300;
    end
    
    function main1()
      a0=99;
      [a0]=200;
    end
  )asm";
  
  RTProgTester tester(test);
  EXPECT(tester.parse(0,true).getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.loadCode();
  tester.execute();  
    
  tester.expectMemoryAt(99,200.0);
}