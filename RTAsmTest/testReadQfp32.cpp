#include "RTAsmTest.h"
#include <mtest.h>

class testReadQfp32 : public mtest::test
{
};

MTEST(testReadQfp32,test_that_positive_minimum_value_is_ok)
{
  RTProg testAssign=RTASM(
    a=0.00000006;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(0.00000006).toRaw());  
  tester.expectMemoryAt(4,0.00000006);
}

MTEST(testReadQfp32,test_that_positive_value_for_exp_0_is_ok)
{
  RTProg testAssign=RTASM(
    a=30.99999994;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(30.99999994).toRaw()); 
  tester.expectMemoryAt(4,30.99999994); 
}

MTEST(testReadQfp32,test_that_positive_value_for_exp_1_is_ok)
{
  RTProg testAssign=RTASM(
    a=6034.33452;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(6034.33452).toRaw());  
  tester.expectMemoryAt(4,6034.33452); 
}

MTEST(testReadQfp32,test_that_positive_value_for_exp_2_is_ok)
{
  RTProg testAssign=RTASM(
    a=113530.23454;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(113530.23438).toRaw());  
  tester.expectMemoryAt(4,113530.23438); 
}

MTEST(testReadQfp32,test_that_positive_value_for_exp_3_is_ok)
{
  RTProg testAssign=RTASM(
    a=67823842;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(67823842).toRaw());
  tester.expectMemoryAt(4,67823842); 
}

MTEST(testReadQfp32,test_that_positive_max_value_is_ok)
{
  RTProg testAssign=RTASM(
    a=536870911;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(536870911).toRaw());
  tester.expectMemoryAt(4,536870911);
}

MTEST(testReadQfp32,test_that_negative_minimum_value_is_ok)
{
  RTProg testAssign=RTASM(
    a=-0.00000006;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-0.00000006).toRaw());
  tester.expectMemoryAt(4,-0.00000006);
}

MTEST(testReadQfp32,test_that_negative_value_for_exp_0_is_ok)
{
  RTProg testAssign=RTASM(
    a=-0.674395995;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-0.674395978).toRaw());
  tester.expectMemoryAt(4,-0.674395978);
}

MTEST(testReadQfp32,test_that_negative_value_for_exp_1_is_ok)
{
  RTProg testAssign=RTASM(
    a=-439.8743890;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-439.874389648).toRaw());
  tester.expectMemoryAt(4,-439.874389648); 
}

MTEST(testReadQfp32,test_that_negative_value_for_exp_2_is_ok)
{
  RTProg testAssign=RTASM(
    a=-656875.8754;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-656875.875).toRaw());
  tester.expectMemoryAt(4,-656875.875);   
}

MTEST(testReadQfp32,test_that_negative_value_for_exp_3_is_ok)
{
  RTProg testAssign=RTASM(
    a=-83465848;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-83465848).toRaw());
  tester.expectMemoryAt(4,-83465848); 
}

MTEST(testReadQfp32,test_that_negative_max_value_is_ok)
{
  RTProg testAssign=RTASM(
    a=-536870911;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getIRSAddrOfSymbol("a") == 4);
  
  tester.loadCode();
  tester.execute();
  
  EXPECT(tester.getProcessor().readMemory(4) == qfp32_t(-536870911).toRaw()); 
  tester.expectMemoryAt(4,-536870911);  
}

MTEST(testReadQfp32,test_that_qfp32_overflow_is_detected)
{
  RTProg testAssign=RTASM(
    a=-536870912;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 1);   
}

MTEST(testReadQfp32,test_that_only_one_load_instr_is_generated_for_exp0)
{
  RTProg testAssign=RTASM(
    a=-18.03125;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getCodeSize() == 2+16);//entry vector of size 16
}

MTEST(testReadQfp32,test_that_only_one_load_instr_is_generated_for_exp1)
{
  RTProg testAssign=RTASM(
    a=4000;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getCodeSize() == 2+16);//entry vector of size 16
}

MTEST(testReadQfp32,test_that_two_load_instr_is_generated)
{
  RTProg testAssign=RTASM(
    a=4001.00390625;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getCodeSize() == 3+16);//entry vector of size 16
}

MTEST(testReadQfp32,test_that_three_load_instr_is_generated)
{
  RTProg testAssign=RTASM(
    a=0.00000006;
  );
  
  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);  
  EXPECT(tester.getCodeSize() == 4+16);//entry vector of size 16
}

MTEST(testReadQfp32,test_that_hex_value_works)
{
  RTProg testCode=R"asm(
    a=0x19ABCDEF;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("a",0x19ABCDEF); 
}

MTEST(testReadQfp32,test_that_negative_hex_value_works)
{
  RTProg testCode=R"asm(
    a=-0x0EADBEEF;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("a",-0x0EADBEEF); 
}

MTEST(testReadQfp32,test_that_small_hex_value_works)
{
  RTProg testCode=R"asm(
    a=0x1F;
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("a",0x1F); 
}