#include "RTAsmTest.h"
#include <mtest.h>

class testLoop : public mtest::test
{
};

MTEST(testLoop,test_that_loop_and_inc_and_const_count_works)
{
  RTProg testCode=RTASM(
    a=0;
    loop(5)
      a=a+1;
    end
    a=a;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(5).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
    
  tester.expectSymbol("a",5);
}

MTEST(testLoop,test_that_loop_with_index_access_and_const_count_works)
{
  RTProg testCode=RTASM(
    a=0;
    loop(5)
      a=a+i;
    end
    a=a;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(10).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
    
  tester.expectSymbol("a",10);
}

MTEST(testLoop,test_that_loop_with_index_access_and_var_count_works)
{
  RTProg testCode=RTASM(
    a=0;
    b=5;
    loop(b)
      a=a+i;
    end
    a=a;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")) == qfp32_t(10).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("a")));
    
  tester.expectSymbol("a",10);
}

MTEST(testLoop,test_that_loop_in_loop_with_index_access_and_var_count_works)
{
  RTProg testCode=RTASM(
    v=0;
    a=3;
    b=5;
    loop(a)
      loop(b)
        v=10*(i+1)+(j+1);
      end
    end
    v=v;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")) == qfp32_t(35).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")));
    
  tester.expectSymbol("v",35);
}

MTEST(testLoop,test_that_loop_continue_works)
{
  RTProg testCode=RTASM(
    v=1;
    t=3;
    loop(t)
      if(i == 1)
        continue;
      end
      v=v+i;
    end
    v=v;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")) == qfp32_t(3).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")));
    
  tester.expectSymbol("v",3);
}

MTEST(testLoop,test_that_loop_break_works)
{
  RTProg testCode=RTASM(
    v=0;
    loop(3)
      if(i == 2)
        break;
      end
      v=v+1;
    end
    v=v;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")) == qfp32_t(2).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")));
    
  tester.expectSymbol("v",2);
}

MTEST(testLoop,test_that_loop_in_loop_with_continue_and_break_works)
{
  RTProg testCode=RTASM(
    v=0;
    loop(5)%0 2%
      if(i == 1)
        continue;
      end
      if(i == 3)
        break;
      end
        
      loop(7)%0 1 3%
        if(j == 2)
          continue;
        end
          
        v=v+(i+1)*j;
        
        if(j == 3)
          break;
        end
      end
    end
    v=v;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")) == qfp32_t(16).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")));
    
  tester.expectSymbol("v",16);
}

MTEST(testLoop,test_that_loop_with_zero_loop_count_executes_one_time)
{
  RTProg testCode=RTASM(
    v=0;
    loop(0)
      v=v+1;
    end
    v=v;%needed otherwise execute stops after first iteration%
  );
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
   
  EXPECT(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")) == qfp32_t(1).toRaw())
    << "read value is: " << qfp32_t::initFromRaw(tester.getProcessor().readMemory(tester.getIRSAddrOfSymbol("v")));
    
  tester.expectSymbol("v",1);
}

MTEST(testLoop,test_that_operation_with_loop_index_and_array_base_and_irs_works)
{
  RTProg testCode=R"(
    array a 4;
    b=2;
    loop(1)
      a0=a+(i/b);
      [a0]=99;
    end
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbolWithOffset("a",0,99);
}

MTEST(testLoop,test_that_automatic_variable_locate_in_loop_works)
{
  RTProg testCode=R"(
  x=1;
  loop(2)
    a=x;
    b=2*a;
    c=3*b;
    d=4*c;
  end
  a=a;
  b=b;
  c=c;
  d=d;
  )";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("a",1);
  tester.expectSymbol("b",2);
  tester.expectSymbol("c",6);
  tester.expectSymbol("d",24);
}