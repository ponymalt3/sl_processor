#include <mtest.h>
#include "SLProcessorTest.h"

class TestOp : public mtest::test
{  
};

MTEST(TestOp,testOpWithOperandsResultAndIRS)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value2.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(6) == (value-value2).toRaw());
  proc.expectThatMemIs(6,value-value2);
}

MTEST(TestOp,testOpWithOperandsResultAndDATA0)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD0,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value2.toRaw());
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0,value-value2);
}

MTEST(TestOp,testOpWithOperandsResultAndDATA0AndIncAddr)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD0,SLCode::CMD_SUB,5,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value2.toRaw());
  proc.writeMemory(ad0+1,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0+1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0+1,value-value2);
}

MTEST(TestOp,testOpWithOperandsResultAndDATA1)
{
  qfp32_t ad1=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD1,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value2.toRaw());
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad1,value-value2);
}

MTEST(TestOp,testOpWithOperandsResultAndDATA1AndIncAddr)
{
  qfp32_t ad1=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD1,SLCode::CMD_SUB,0,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad1,value2.toRaw());
  proc.writeMemory(ad1+1,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1+1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad1+1,value-value2);
}

MTEST(TestOp,testOpWithOperandsDATA0WithAddrIncAndIRS)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::IRS,SLCode::CMD_SUB,5,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value2.toRaw());
  proc.writeMemory(ad0,value.toRaw());
  proc.writeMemory(ad0+1,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0+1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0+1,value-value2);
}

MTEST(TestOp,testOpWithOperandsDATA1WithAddrIncAndIRS)
{
  qfp32_t ad1=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    SLCode::Op::create(SLCode::DEREF_AD1,SLCode::IRS,SLCode::CMD_SUB,5,true),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(ad1,value2.toRaw());
  proc.writeMemory(ad1+1,0);
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad1+1) == (value2-value).toRaw());
  proc.expectThatMemIs(ad1+1,value2-value);
}



/////////////////////////////////////////  STALL  ////////////////////////////////////
MTEST(TestOp,testOpWithTwoMemoryOperandsAndNoIncExpectStall1)
{
  qfp32_t ad0=10;
  qfp32_t ad1=20;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    //load ad1
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::DEREF_AD1,SLCode::CMD_SUB),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad0,value.toRaw());
  proc.writeMemory(ad1,value2.toRaw());
  
  proc.run(10);
  
  EXPECT(proc.readMemory(5) == (value-value2).toRaw());
  proc.expectThatMemIs(5,value-value2);
}

MTEST(TestOp,testOpWithTwoMemoryOperandsAndNoIncExpectStall2)
{
  qfp32_t ad0=10;
  qfp32_t ad1=20;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    //load ad1
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0,SLCode::CMD_SUB),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,5),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,0);
  proc.writeMemory(ad0,value2.toRaw());
  proc.writeMemory(ad1,value.toRaw());
  
  proc.run(10);
  
  EXPECT(proc.readMemory(5) == (value-value2).toRaw());
  proc.expectThatMemIs(5,value-value2);
}

MTEST(TestOp,testOpWithMemoryAndIRSWithNoIncExpectStall)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD0,SLCode::IRS,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value2.toRaw());
  proc.writeMemory(ad0,value.toRaw());
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0,value-value2);
}

MTEST(TestOp,testOpWithResultAndMemoryWithNoIncExpectStall)
{
  qfp32_t ad0=10;
  qfp32_t value=-24.5;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::REG_RES,SLCode::DEREF_AD0,SLCode::CMD_SUB),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
 
  proc.writeMemory(ad0,value.toRaw());
  
  proc.run(8);
  
  EXPECT(proc.readMemory(ad0) == (ad0-value).toRaw());
  proc.expectThatMemIs(ad0,ad0-value);
}


/////////////////////////////////////////  INCREMENT  //////////////////////////
MTEST(TestOp,testOpWithTwoMemoryOperandsAndIncOpA)
{
  qfp32_t ad0=10;
  qfp32_t ad1=20;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    //load ad1
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0,SLCode::CMD_SUB,0,true,false),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value2.toRaw());
  proc.writeMemory(ad1,value.toRaw());
  proc.writeMemory(ad1+1,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(ad0) == (value-value2).toRaw());
  EXPECT(proc.readMemory(ad1+1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0,value-value2);
  proc.expectThatMemIs(ad1+1,value-value2);
}

MTEST(TestOp,testOpWithTwoMemoryOperandsAndIncOpB)
{
  qfp32_t ad0=10;
  qfp32_t ad1=20;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    //load ad1
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0,SLCode::CMD_SUB,0,false,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value2.toRaw());
  proc.writeMemory(ad1,value.toRaw());
  proc.writeMemory(ad0+1,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(ad0+1) == (value-value2).toRaw());
  EXPECT(proc.readMemory(ad1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0+1,value-value2);
  proc.expectThatMemIs(ad1,value-value2);
}

MTEST(TestOp,testOpWithTwoMemoryOperandsAndIncBoth)
{
  qfp32_t ad0=10;
  qfp32_t ad1=20;
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    //load ad0
    SLCode::Load::create1(ad0.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD0,SLCode::REG_RES),
    //load ad1
    SLCode::Load::create1(ad1.toRaw()),
    SLCode::Mov::create(SLCode::REG_AD1,SLCode::REG_RES),
    
    //should stall one cycle
    
    SLCode::Op::create(SLCode::DEREF_AD1,SLCode::DEREF_AD0,SLCode::CMD_SUB,0,true,true),
    SLCode::Mov::create(SLCode::DEREF_AD0,SLCode::REG_RES),
    SLCode::Mov::create(SLCode::DEREF_AD1,SLCode::REG_RES),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(ad0,value2.toRaw());
  proc.writeMemory(ad1,value.toRaw());
  proc.writeMemory(ad0+1,0);
  proc.writeMemory(ad1+1,0);
  
  proc.run(11);
  
  EXPECT(proc.readMemory(ad0+1) == (value-value2).toRaw());
  EXPECT(proc.readMemory(ad1+1) == (value-value2).toRaw());
  proc.expectThatMemIs(ad0+1,value-value2);
  proc.expectThatMemIs(ad1+1,value-value2);
}


/////////////////// OPERATIONS //////////////////////
MTEST(TestOp,testOpAddition)
{
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_ADD,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(6);
  
  EXPECT(proc.readMemory(6) == (value2+value).toRaw());
  proc.expectThatMemIs(6,value2+value);
}

MTEST(TestOp,testOpSubstract)
{
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_SUB,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(6);
  
  EXPECT(proc.readMemory(6) == (value2-value).toRaw());
  proc.expectThatMemIs(6,value2-value);
}

MTEST(TestOp,testOpMultiply)
{
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_MUL,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(7);
  
  EXPECT(proc.readMemory(6) == (value2*value).toRaw());
  proc.expectThatMemIs(6,value2*value);
}

MTEST(TestOp,testOpDivide)
{
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value2.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_DIV,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(36);
  
  EXPECT(proc.readMemory(6) == (value2/value).toRaw());
  proc.expectThatMemIs(6,value2/value);
}

MTEST(TestOp,testOpShift)
{
  qfp32_t value=-24.5;
  qfp32_t value2=7;
  
  uint32_t code[]=
  {
    SLCode::Load::create1(value.toRaw()),
    SLCode::Op::create(SLCode::REG_RES,SLCode::IRS,SLCode::CMD_SHFT,5),
    SLCode::Mov::create(SLCode::IRS,SLCode::REG_RES,6),

    0xFFFF,
    0xFFFF,
    0xFFFF
  };
  
  LoadAndSimulateProcessor proc(code);
  
  proc.writeMemory(5,value2.toRaw());
  proc.writeMemory(6,0);
  
  proc.run(5);
  
  proc.expectThatMemIs(6,_qfp32_t(-24.5*128));
}
