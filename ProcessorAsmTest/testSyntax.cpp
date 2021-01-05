#include "RTAsmTest.h"
#include <mtest.h>

class testSyntax : public mtest::test
{
};

MTEST(testSyntax,test_basic_features)
{
  RTProg test=RTASM(
    def const 1000;
    decl arr 10;
    ref r 4;

    d=0;
    abc=0;
    a=1;
    b=2;
    c=3;
    e=666;
    abcdef = -(a*b+c);
    test3=-(-a);
    test4=(((-a+b)*c)+d)/e;

    test5=0;

    if(a > b)
      test5=1;
    else
      test5=2;
    end

    y=-999.5123456789;
    loop(c+2*3)
      y=y+i;
    end

  );
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 1);
}

MTEST(testSyntax,test_basic_exprs)
{
  RTProg testExpr=RTASM(
    def a 100;
    def b 2;
    def c 3;

    d=4;
    e=5;

    test1=0;
    test2=-(a*b+c);
    test3=-(-a);
    test4=(((-a+b)*c)+d)/e;
    test5=((a+b)*c+5)*(d*(a+b));
    test6=-99;
    test7=-b;

    %test brackets%
    test_b1=(a*(b+c));
    test_b2=(a*((b+c)));

    %test sub/add replacement%
    test_as1=a+-b*c;
    test_as2=a+-b-c;
  );
  
  RTProgTester tester(testExpr);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testSyntax,test_assign)
{
  RTProg testAssign=RTASM(
    def a 23;

    b=0.735;
    c=13;

  %with spaces%
    d = c ;

  %addr internal regs%
    a0=10;
    a1=20;
    a1=-b*c;
    a1=c;

  %deref addrs%
    [a0]=[a1];
    [a0]=[a0];
    [a0]=a;
    [a0]=[a1]+23*-a;

  %simple assignments%
    c=c*2;
    b=3.4594;
    b=c;
  );

  RTProgTester tester(testAssign);
  EXPECT(tester.parse().getNumErrors() == 0);    
}

MTEST(testSyntax,test_const_values_cant_be_assigned)
{
  RTProg code=RTASM(
    def a 23;

  %cant assign values to consts%
    a=1;
  );
  
  //expect errors
  RTProgTester tester(code);
  EXPECT(tester.parse().getNumErrors() != 0); 
}

MTEST(testSyntax,test_addr_reg_cant_be_read)
{
  RTProg code=RTASM(
  %cant read a0/a1%
    b=a1;
  );
  
  RTProgTester tester(code);
  EXPECT(tester.parse().getNumErrors() != 0); 
}

MTEST(testSyntax,test_non_existing_var_cant_be_read)
{
  RTProg code=RTASM(
  %cant read non existing values%
    c=d;
  );

  RTProgTester tester(code);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testSyntax,test_if_construct)
{
  RTProg testIf=RTASM(
    def a 100;

    b=2;
    c=3;
    d=4;

    result=0;

    if(d < c)
      result=1;
    end

    if(d > 0.5777)
      result=result+10;
    end

    if(d <= 17.4)
      result=result+100;
    end

    if(d == d)
      result=result+1000;
    end

    if(d != c)
      result=result+10000;
    end

    if(d >= (a+b)*c)
      result2=100000;
    end

    e=99;
    f=45;
    g=-1000.7654321;

    if(d == d)
      b=b+10;
      c=c+10;
      d=d-1;
    else
      b=b+1000;
      c=c+1111;
      d=-100;
    end

    if(d != d)
      e=0;
    else
      e=b;
      f=c;
      g=(d*10+5)*-1;
    end

    if( a > b )
      if(c < d)
        result = 0 ;
      end
    else
      result = 0 ;
    end

  );

  RTProgTester tester(testIf);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testSyntax,test_loop_construct)
{
  RTProg testLoop=RTASM(
    a=10;

    def b 3;

    sum=0;
    sum2=0;

    loop(a*3+9)
      loop(5)
        loop(b)
          loop(1)
            loop(0) %should also work same as loop(1)%
              sum=sum+i+j+k+l+m;
            end
          end
        end
      end
      sum2=sum2+i;
    end
  );

  RTProgTester tester(testLoop);
  EXPECT(tester.parse().getNumErrors() == 0);    
}

MTEST(testSyntax,test_array)
{
  RTProg testDef=RTASM(
    def a 10;
    ref param0 4;

    decl arr 3;

    arr(0)=0;
    arr(1)=1;
    arr(2)=2;

    sum=0;
    a0=arr;
    loop(3)
      sum=sum+[a0++];
    end
  );

  RTProgTester tester(testDef);
  EXPECT(tester.parse().getNumErrors() == 0);    
}

MTEST(testSyntax,test_empty_branch)
{
  RTProg testSyntax=RTASM(
    a=10;

    if(a == 10)
    else
      a=99;
    end
  );
  
  RTProgTester tester(testSyntax);
  EXPECT(tester.parse().getNumErrors() == 0);    
}

MTEST(testSyntax,test_that_comment_at_the_end_works)
{
  RTProg testComment=RTASM(
    b=2;%test%
  );
  
  RTProgTester tester(testComment);
  EXPECT(tester.parse().getNumErrors() == 0);  
}  

MTEST(testSyntax,test_that_store_const_in_irs_works)
{
  RTProg test=RTASM(
    def cons -31;
    a=cons;
  );
  
  //a should be allocted to irs address 1
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testSyntax,test_that_store_const2_in_irs_works)
{
  RTProg test=RTASM(
    def cons 199;
    a=cons;
  );
  
  //a should be allocted to irs address 1
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testSyntax,test_that_store_const3_in_irs_works)
{
  RTProg test=RTASM(
    def cons 400000000;
    a=cons;
  );
  
  //a should be allocted to irs address 1
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 0);
}

MTEST(testSyntax,test_that_refs_are_only_allowed_at_irs_addr_higher_than_4_expect_errors)
{
  RTProg test=R"(
    ref a 0;
    ref b 1;
    ref c 2;
    ref d 3;
    f=a+b+c+d;
  )";
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 4);
}

MTEST(testSyntax,test_that_loop_index_cant_be_written_inside_loop_expect_errors)
{
  RTProg test=R"(
    loop(1)
      i=10;
      k=99; % should be ok
    end
  )";
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 5);
}

MTEST(testSyntax,test_that_different_loop_index_cant_be_written_inside_loop_expect_errors)
{
  RTProg test=R"(
    loop(1)
      loop(1)
        i=0;
        k=99;
      end
    end
  )";
  
  RTProgTester tester(test);
  EXPECT(tester.parse().getNumErrors() == 7);
}
