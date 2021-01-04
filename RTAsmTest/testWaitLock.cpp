#include "RTAsmTest.h"
#include <mtest.h>

class testWaitLock : public mtest::test
{
};

MTEST(testWaitLock,test_that_lock_fails_if_loop_is_used)
{
  RTProg testCode=R"(
    buslock {
      a0=100;
      loop(10)
        [a0++]=7;
      end
    }
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}


MTEST(testWaitLock,test_that_lock_fails_if_return_is_used)
{
  RTProg testCode=R"(
    function f()
      buslock {
        a0=100;
        if([a0] == 17)
          return;
        end
      }
    end
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testWaitLock,test_that_lock_fails_if_function_is_called)
{
  RTProg testCode=R"(
    function f()
    end
    
    buslock {
      a0=100;
      if([a0] == 17)
        f();
      end
    }
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testWaitLock,test_that_lock_fails_if_too_many_instrs)
{
  RTProg testCode=R"(
    buslock {
      t {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
      a0=100;
    }
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testWaitLock,test_that_lock_works_correctly)
{
  RTProg testCode=R"(
    a=0;
    buslock {
      a0=100;
      if([a0] == 17)
        [a0]=18;
      end
    }
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.getProcessor().writeMemory(100,17.0); 
  
  tester.loadCode();
  tester.execute();
  
  tester.expectMemoryAt(100,18); 
}