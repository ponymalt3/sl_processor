#include "RTAsmTest.h"
#include <mtest.h>

class testArrayDecl : public mtest::test
{
};

MTEST(testArrayDecl,test_that_array_with_consts_works)
{
  RTProg testCode=R"asm(
    a {1,2,3,4,5};
    b=a(2);  
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("b",3);
  tester.expectSymbolWithOffset("a",0,1);
  tester.expectSymbolWithOffset("a",1,2);
  tester.expectSymbolWithOffset("a",2,3);
  tester.expectSymbolWithOffset("a",3,4);
  tester.expectSymbolWithOffset("a",4,5);
}

MTEST(testArrayDecl,test_that_array_with_symbols_works)
{
  RTProg testCode=R"asm(
    a=99;
    b=3;
    c=4;
    arr {1,a,3,b*4+1,5,c};
    
    ok=0;
    if(arr(0) == 1 and 
       arr(1) == 99 and 
       arr(2) == 3 and 
       arr(3) == 13 and 
       arr(4) == 5 and
       arr(5) == c)
      ok=1;
    end
    ok=ok;
    )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
  tester.expectSymbolWithOffset("arr",0,1);
  tester.expectSymbolWithOffset("arr",1,99);
  tester.expectSymbolWithOffset("arr",2,3);
  tester.expectSymbolWithOffset("arr",3,13);
  tester.expectSymbolWithOffset("arr",4,5);
  tester.expectSymbolWithOffset("arr",5,4);
}

MTEST(testArrayDecl,test_that_array_sizeof_operator_works)
{
  RTProg testCode=R"asm(
    a {1,2,3,4,5};
    
    size=sizeof(a);
    sum=0;
    a0=a;
    loop(size)
      sum=sum+[a0++];
    end
    sum=sum;     
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(0,0.0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("sum",15);
  tester.expectSymbol("size",5);
}

MTEST(testArrayDecl,test_that_array_redeclaration_is_an_error)
{
  RTProg testCode=R"asm(
    a=99;
    a {1,2,3,4,5};
    b=a(2);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 2);
}

MTEST(testArrayDecl,test_that_array_stays_allocated_if_address_is_loaded)
{
  RTProg testCode=R"asm(
  function ttt(a,b)
    s=0;
    a0=b;
    loop(a)
      s=s+[a0++];
    end
    return s;
  end
  
  size=12;
  weights {0,0,0,0,0,0,0,0,0,0,0,0};
  xxx {99,99,99};
    
  x=ttt(size,weights);
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("x",0);
}