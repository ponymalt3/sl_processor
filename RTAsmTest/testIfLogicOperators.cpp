#include "RTAsmTest.h"
#include <mtest.h>

class testIfLogicOperators : public mtest::test
{
};

MTEST(testIfLogicOperators,test_that_and_operator_evaluate_to_true)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    
    ok=0;
    if(a == b and b < c)
      ok=1;
    else
      ok=0;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_and_operator_evaluate_to_false)
{
  RTProg testCode=RTASM(
    def a 4;
    def b 4;
    def c 5;
    
    ok=0;
    if(a == b and b > c)
      ok=0;
    else
      ok=1;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_or_operator_evaluate_to_true)
{
  RTProg testCode=RTASM(
    def a 4;
    def b 4;
    def c 5;
    
    ok=0;
    if(a != b or b < c)
      ok=1;
    else
      ok=0;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_or_operator_evaluate_to_false)
{
  RTProg testCode=RTASM(
    def a 4;
    def b 4;
    def c 5;
    
    ok=0;
    if(a != b or b > c)
      ok=0;
    else
      ok=1;
    end
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_simple_and_or_mix_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a == b and (b > c or c != d or e < d) and a < e) 
      ok=1;
    else
      ok=0;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_complex_and_or_mix_works)
{
  RTProg testCode=RTASM(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    a0=31;
    
    [a0]=a;
    
    ok=0;
    if(((a+3)+5)*2 != (b*6) or ((b-2) < c and (a == 3 or a == 4) and d == d and e > d) or [a0++] > e) 
      ok=1;
    else
      ok=0;
    end
    
    [a0]=e;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
  tester.expectMemoryAt(31,7);
}

MTEST(testIfLogicOperators,test_that_short_circuit_evaluation_in_and_expr_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    a0=31;
    
    [a0]=a;
    
    ok=0;
    if(a == b and (b > c or c == d or e < d) and [a0++] < e) 
      ok=0;
    else
      ok=1;
    end
    
    [a0]=e;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
  tester.expectMemoryAt(31,7);
}

MTEST(testIfLogicOperators,test_that_short_circuit_evaluation_in_or_expr_works)
{
  RTProg testCode=RTASM(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    a0=31;
    
    [a0]=a;
    
    ok=0;
    if(a != b or (b < c and c != d and e > d) or [a0++] < e) 
      ok=1;
    else
      ok=0;
    end
    
    [a0]=e;
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
  tester.expectMemoryAt(31,7);
}

MTEST(testIfLogicOperators,test_that_sub_group_or_as_last_expr_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a == b and (b > c or c == d or e < d)) 
      ok=0;
    else
      ok=1;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_sub_group_and_as_last_expr_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a != b or (b < c and c != d and e < d)) 
      ok=0;
    else
      ok=1;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_and_with_and_group_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a == b and (b < c and c == d) and e > d) 
      ok=0;
    else
      ok=1;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_and_with_and_group_as_last_expr_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a == b and b < c and (c != d and e < d)) 
      ok=0;
    else
      ok=1;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}


MTEST(testIfLogicOperators,test_that_or_with_or_group_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a != b or (b > c or c != d) or e < d) 
      ok=1;
    else
      ok=0;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}

MTEST(testIfLogicOperators,test_that_or_with_or_group_as_last_expr_works)
{
  RTProg testCode=R"asm(
    def a 4;
    def b 4;
    def c 5;
    def d 6;
    def e 7;
    
    ok=0;
    if(a != b or b > c or (c == d or e > d)) 
      ok=1;
    else
      ok=0;
    end
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("ok",1);
}