#include "RTAsmTest.h"
#include <mtest.h>

class testWhileLoop : public mtest::test
{
};

MTEST(testWhileLoop,test_that_simple_while_works)
{
  RTProg testCode=R"asm(
    a=10;
    b=1;
    while(a > 0)
      b=b*2;
      a=a-1;
    end
    b=b;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("b",1024);
}

MTEST(testWhileLoop,test_that_while_loop_with_continue_works)
{
  RTProg testCode=R"asm(
    a=10;
    b=1;
    while(a > 0)
    
      a=a-1;
      
      if(a > 5)
        continue;
      end
      
      b=b+1;
    end 
    b=b;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("b",7);
}

MTEST(testWhileLoop,test_that_while_loop_with_break_works)
{
  RTProg testCode=R"asm(
    a=10;
    b=1;
    while(a > 0)
    
      if(b >= 6)
        break;
      end
      
      b=b+1;
      a=a-1;
    end 
    b=b;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("b",6);
}

MTEST(testWhileLoop,test_that_while_loop_with_multiple_conditions_works)
{
  RTProg testCode=R"asm(
    a=10;
    b=100;
    x=1;
    while((a > 0 and b > 10) or x == 1)
    
      if(b <= 25)
        x=0;
      end
      
      b=b/2;
      a=a-1;
    end 
    b=b;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("x",0);
  tester.expectSymbol("a",6);
  tester.expectSymbol("b",6.25);
}