/*
 * SLCodeDef.h
 *
 *  Created on: Apr 8, 2015
 *      Author: malte
 */

#ifndef SLCODEDEF_H_
#define SLCODEDEF_H_

namespace SLCode
{
  enum Operand {REG_AD0=0,REG_AD1,REG_LOOP,REG_RES,IRS,DEREF_AD0,DEREF_AD1};
  enum Command {CMD_MOV=0,CMD_UNUSED,CMD_ADD,CMD_SUB,CMD_MUL,CMD_DIV,CMD_MAC,CMD_MAC_RES};
  enum CmpMode {CMP_EQ=0,CMP_NEQ,CMP_GT,CMP_LE};

  enum {MUX1_RESULT=0,MUX1_MEM=1,MUX2_MEM=0,MUX2_LOOP=1};

   //      MOV [IRS]                RESULT, LOOP  => code/4 A/1 B/1 OFFSET/9
  struct Mov
  {
    enum {Code1=0x8000,Bits=3};//MOVIRS
    enum {Code2=0xF000,Bits=9};//MOVDATA1
    enum {Code3=0xF080,Bits=10};//MOVDATA2(2)

    static uint32_t create(Operand a,Operand b,uint32_t irsOffset,bool incAddr)
    {
      uint32_t offset=irsOffset&0x1FF;
      uint32_t incAD=incAddr;
      uint32_t muxA=b==REG_RES?MUX1_RESULT:MUX1_MEM;
      uint32_t muxB=b==REG_LOOP?MUX2_LOOP:MUX2_MEM;
      uint32_t muxAD0=(a==DEREF_AD0)?0:1;
      uint32_t muxAD1=(b==DEREF_AD0)?0:1;
      uint32_t wbReg=a;

      if(a == Operand::IRS)
      {
        //mov [IRS+offset] RESULT,LOOP
        return Code1+0x1000 + (incAD<<11) + (offset<<2) + (muxA<<1);
      }

      if(b == Operand::IRS)
      {
        //mov RESULT,LOOP,AD0,AD1 [IRS+offset]
        return Code1+0x0000 + (incAD<<11) + (offset<<2) + a;
      }

      if(a == Operand::DEREF_AD0 || a == Operand::DEREF_AD1)
      {
        //MOV [DATAx] RESULT,LOOP  => code/12 ADDR1/1 A/1 B/1 INC/1 (ADDR0/1)
        return Code2 + (wbReg<<5) + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1);
        //can be used as nop or for inc ad
      }

      return Code3 + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1);
    }
  };

  struct Op
  {
    enum {Code1=0x0000,Bits=1};
    enum {Code2=0xC000,Bits=4};

    static uint32_t create(Operand a,Operand b,Command cmd,uint32_t irsOffset=0,bool incAddr=false,bool incAddr2=false)
    {
      //OP RESULT, [DATAx]  [IRS]       => code/1 ADDR0/1 A/1 OP/3 OFFSET/9 INC/1
      //OP RESULT, [DATAx]  [DATAx]      => code/8 ADDR0/1 A/1 ADDR1/1 OP/3 INC/2 (B/1)

      uint32_t muxAD0=(a==REG_AD0)?0:1;
      uint32_t muxAD1=(b==REG_AD0)?0:1;
      uint32_t muxA=(a==REG_RES)?MUX1_RESULT:MUX1_MEM;
      uint32_t muxB=MUX2_MEM;
      uint32_t incAD=incAddr;
      uint32_t incAD2=incAddr2;
      uint32_t offset=irsOffset&0x1FF;

      if(b == IRS)
      {
        return Code1 + (cmd<<12) + (incAD<<11) + (offset<<2) + (muxA<<1) + muxAD0;
      }
      else
      {
        return Code2 + (incAD2<<8) + (cmd<<5) + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1) + muxAD0;
      }
    }
  };

  struct Cmp
  {
    enum {Code=0xD000,Bits=4};

    static uint32_t create(Operand a,uint32_t irsOffset,CmpMode mode,bool disableExecuteFor3Cycles=false)
    {
      //CMP RESULT  [IRS]        => code/4 MODE/2 NOX_CY/1

      uint32_t noXCycles=disableExecuteFor3Cycles;
      uint32_t offset=irsOffset&0x1FF;

      return Code + (noXCycles<<11) + (offset<<2) + mode;
    }
  };

  struct Goto
  {
    enum {Code=0xA000,Bits=4};

    static uint32_t create(int32_t relJump,bool loopEndMarker)
    {
      //GOTO RESULT, CONST       => code/6 CONST/10 (A/1) (MD/1)

      uint32_t muxA=MUX1_RESULT;
      uint32_t loopEnd=loopEndMarker;
      uint32_t pcAdjust=relJump&0x3FF;

      return Code + (pcAdjust<<2) + (muxA<<1) + loopEnd;
    }
  };

  struct Load
  {
    enum {Code=0xB000,Bits=4};

    static uint32_t constDataValue1(uint32_t value)
    {
      return ((value&0x80000000)>>22)+((value&0x1FF00000)>>20);
    }

    static uint32_t constDataValue2(uint32_t value)
    {
      return ((value&0x60000000)>>15)+((value&0x000FFF80)>>6);//bit0 == 0
    }

    static uint32_t constDataValue3(uint32_t value)
    {
      return (value&0x0000007F)<<1;//bit0 == 0
    }

    static uint32_t create(uint32_t value)
    {
      //LOAD RESULT CONST16, CONST32     => code/4 MODE/2 X/10

      //1--11111 1111---- -------- --------
      //12211111 11112222 22222222 2-------
      //12211111 11112222 22222222 23333333

      uint32_t constData1=constDataValue1(value);
      uint32_t constData2=constDataValue2(value);
      uint32_t constData3=constDataValue3(value);

      uint32_t mode=0;

      if(constData2)
        ++mode;

      if(constData3)
        ++mode;

      return Code + (constData1<<2) + mode;
    }
  };

  struct Neg
  {
    enum {Code=0xF100,Bits=10};

    static uint32_t create()
    {
      //NEG RESULT         => code/16  (A/1)

      uint32_t muxA=MUX1_RESULT;

      return Code + (muxA<<1);
    }
  };

  struct SignalWait
  {
    enum {Code=0xF0C0,Bits=10};

    enum {SIG_ONLY,WAIT_SIG=4,WAIT_SIG2,WAIT_SIG_AND_LOCK};

    static uint32_t create()
    {
      //WAIT/SIGNAL         => code/10 MODE/3 (A/1)

      uint32_t muxA=MUX1_RESULT;
      uint32_t mode=SIG_ONLY;

      return Code + (mode<<2) + (muxA<<1);
    }
  };
}

#endif /* SLPROCESSORINSTRDEF_H_ */
