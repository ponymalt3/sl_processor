#include "RTAsmTest.h"
#include <mtest.h>

class testMisc : public mtest::test
{
};

MTEST(testMisc,test_that_cluster_test_prgm_runs_on_sim_also)
{
  RTProg testCode=R"abc(
    ref proc_id 4;
    def mem_size 512;
    def shared_size 2048;
    
    a1=512+proc_id*2;
    [a1]=proc_id;
    [a1]=[a1++]+99;
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
  
  tester.expectMemoryAt(514,_qfp32_t(1));
  tester.expectMemoryAt(515,_qfp32_t(100));
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

  tester.loadCode();
  tester.execute();
  
  tester.expectMemoryAt(512,234);
  tester.expectMemoryAt(513,235);
  tester.expectMemoryAt(514,236);
  tester.expectMemoryAt(515,237);
}


MTEST(testMisc,test_that_math_lib_works_correct)
{
  RTProg testCode=R"abc(
    def pi 3.141592654;
    
        function mod(a,b)
      div=a/b;
      return a-(int(div)*b);
    end

    function sqrt(x)
      s=shft(1.0,log2(x));
      s=2*s/(s*s+x);

      ## use 1/sqrt(x)
      loop(6)
        s=0.5*s*(3-x*s*s);
      end

      return s*x;
    end

    function sin(x)
      x=mod(x,2*pi);
      
      if(x > pi)
        x=-x+pi;
      end
      
      x2=x*x;

      ## taylor approximation
      t=x;
      x=x*x2;
      s=t-x*(1/6);
      x=x*x2;
      s=s+x*(1/120);
      x=x*x2;
      s=s-x*(1/5040);
      x=x*x2;
      s=s+x*(1/362880);

      return s;
    end

    function cos(x)
      return sin(x+0.5*pi);
    end

    function ln(x)
      return log2(x)*0.693147181;
    end

    function exp(x)
      
      if(x > 20.1)
        return 536190464;
      end
      
      if(x < -16.6)
        return 0;
      end
        
      x=x*1.442695041;
      ix=int(x);

      fx=x-ix;

      ## taylor approximation of 2^fraction
      t=0.693147181*fx;
      
      x=t;
      s=1+x;
      x=x*t;
      s=s+x*(1/2);
      x=x*t;
      s=s+x*(1/6);
      x=x*t;
      s=s+x*(1/24);
      
      return s*shft(1,ix);
      
    end
    
    function boxMuellerTransform(u1,u2,outAddr)
      a0=outAddr;
      t=sqrt(-2*ln(u1));
      t2=2*pi*u2;
      [a0++]=t*cos(t2);
      [a0++]=t*sin(t2);
    end

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
   
  )abc";
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
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