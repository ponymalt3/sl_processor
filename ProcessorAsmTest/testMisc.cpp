#include "RTAsmTest.h"
#include <mtest.h>

class testMisc : public mtest::test
{
};

MTEST(testMisc,test_that_cluster_test_prgm_runs_on_sim_also)
{
  RTProg testCode=R"asm(    
    function main2()
      a1=512+2;
      [a1]=[a1++]*23;
      t=0;
      loop(999)
        t=t+1;
      end
      t=t;
    end
    
    function main()
      a1=512+4;
      [a1]=[a1++]*99;
      t=0;
      loop(999)
        t=t+1;
      end
      t=t;
    end 
 
    function main1()
      a1=512+0;
      [a1]=[a1++]*7;
      t=0;
      loop(999)
        t=t+1;
      end
      t=t;
    end  
  )asm";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse(0,true).getNumErrors() == 0);
  tester.getProcessor().writeMemory(512,qfp32_t(77).toRaw());

  tester.loadCode();
  tester.execute();
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.expectMemoryAt(513,77.0*7);
}

MTEST(testMisc,test_that_loop_with_inc_write_to_ext_mem_works_correct)
{
  RTProg testCode=R"abc(
    function main()
      a1=512;
      [a1]=234;

      loop(3)
        [a1]=[a1++]+1;
      end
    end

    main();
  )abc";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
  
  tester.expectMemoryAt(512,234);
  tester.expectMemoryAt(513,235);
  tester.expectMemoryAt(514,236);
  tester.expectMemoryAt(515,237);
}

MTEST(testMisc,test_that_array_alloc_with_array_address_access_works)
{
  RTProg testCode=R"(
  function test(a,b,c,d)
    e=b;
    a1=d;
    [a1]=99;
    sum=10;
    a1=a+0*b+e*0;
    a1=d;
    [a1]=[a1]+sum;
  end
  
  a {1};
  b {1};
  array c 4;
  array d 4;

  e=1000+(test(a,b,c,d)*99+188)/23;#expr forces code move
  )";
  
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();  
  tester.execute();
  
  tester.expectSymbol("d",0,109);
}

MTEST(testMisc,test_that_math_lib_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(

    ## evaluate functions
    resultSin1=sin(0.5);
    resultSin2=sin(10);
    
    resultCos1=cos(0.5);
    resultCos2=cos(10);
    
    resultSqrt=sqrt(17);
    
    resultMod=mod(99,7);
    
    resultLn=ln(99);    
    resultExp=exp(13); 
    
    decl resultBM 2; ## array of size 2
    boxMuellerTransform(0.7,0.3452,resultBM);##-0.47561,0.69796  
    
    resultSin1=resultSin1;
    resultSin2=resultSin2;
    resultCos1=resultCos1;
    resultCos2=resultCos2;
    resultSqrt=resultSqrt;
    resultMod=resultMod;
    resultLn=resultLn;
    resultExp=resultExp;
   
  )");
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";
  std::cout<<"res: "<<(qfp32_t::initFromRaw(0x00008000U))<<"\n";
  std::cout<<"res: "<<(qfp32_t::initFromRaw(0x007b73a2U))<<"\n";

  tester.loadCode();
  tester.execute();
  
  tester.expectSymbol("resultSin1",0.47942554);//0.47943);
  tester.expectSymbol("resultSin2",-0.54402107);//-0.54402);
  tester.expectSymbol("resultCos1",0.87765401);//0.87758);
  tester.expectSymbol("resultCos2",-0.83917647);//-0.83907);
  tester.expectSymbol("resultSqrt",4.12310403);//4.1231);
  tester.expectSymbol("resultMod",1);
  tester.expectSymbol("resultLn",4.53794795);//4.5951);
  tester.expectSymbol("resultSqrt",4.12310403);//4.1231);
  tester.expectSymbol("resultExp",442319.42187500);//442413.392);
  tester.expectSymbol("resultBM",0,-0.51357740);//-0.47561);
  tester.expectSymbol("resultBM",1,0.75377464);//0.69796);
}

MTEST(testMisc,test_that_mem_lib_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  
  a0=0;
  [a0]=0;

  decl mem 8;
  pool=createMem(256,256);
  mem(0)=allocateMem(17,pool); 
  mem(1)=allocateMem(63,pool); 
  mem(2)=allocateMem(11,pool); 
  mem(3)=allocateMem(11,pool); 
  mem(4)=allocateMem(20,pool); 
  mem(5)=allocateMem(5,pool);  
  mem(6)=allocateMem(3,pool);  
  mem(7)=allocateMem(4,pool);  
  )");
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  std::cout<<"disasm:\n"<<(tester.getDisAsmString())<<"\n";

  tester.loadCode();
  tester.execute();
  
  tester.expectMemoryAt(256,256-142);
  tester.expectMemoryAt(257,0);
  
  tester.expectSymbolWithOffset("mem",0,495);
  tester.expectSymbolWithOffset("mem",1,431);
  tester.expectSymbolWithOffset("mem",2,419);
  tester.expectSymbolWithOffset("mem",3,407);
  tester.expectSymbolWithOffset("mem",4,386);
  tester.expectSymbolWithOffset("mem",5,380);
  tester.expectSymbolWithOffset("mem",6,376);
  tester.expectSymbolWithOffset("mem",7,371);
}