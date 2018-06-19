#include "RTAsmTest.h"
#include <mtest.h>

class testMisc : public mtest::test
{
};

MTEST(testMisc,test_that_local_mem_is_written_to_ext_mem)
{
  RTProg testCode=R"abc(
    ref proc_id 4;
    def mem_size 512;
    def shared_size 2048;
    
    a1=512+proc_id;
    [a1]=proc_id;
    tt=0;
    loop(999)
      tt=tt+1;
    end
    tt=tt;
  )abc";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  tester.getProcessor().writeMemory(4,qfp32_t(1).toRaw());

  tester.loadCode();
  tester.execute();
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.expectMemoryAt(513,0);
}