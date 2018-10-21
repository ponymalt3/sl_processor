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
  enum Operand {REG_AD0=0,REG_AD1,REG_IRS,REG_RES,IRS,DEREF_AD0,DEREF_AD1,INVALID_OP};
  enum Command {CMD_MOV=0,CMD_CMP,CMD_ADD,CMD_SUB,CMD_MUL,CMD_DIV,CMD_MAC,CMD_MAC_RES,CMD_LOG2=8,CMD_SHFT,CMD_INVALID=15};
  enum UnaryCommand {UNARY_NEG,UNARY_LOG2,UNARY_TRUNC};
  enum CmpMode {CMP_EQ=0,CMP_NEQ,CMP_LT,CMP_LE};

  enum {MUX1_RESULT=0,MUX1_MEM=1,MUX2_MEM=0,MUX2_IRS=1};
  enum {WBREG_AD0=REG_AD0,WBREG_AD1=REG_AD1,WBREG_IRS=REG_IRS,WBREG_NONE=3};
  
  struct IRS
  {
    static uint16_t getOffset(uint16_t instr)
    {
      return (instr>>2)&0x1FF;
    }
  
    static uint16_t patchOffset(uint16_t instr,uint16_t offset)
    {
      uint16_t patchedInstr=(instr&0xF803) + ((offset&0x1FF)<<2);
      return patchedInstr;
    }
  };

   //      MOV [IRS]                RESULT, LOOP  => code/4 A/1 B/1 OFFSET/9
  struct Mov
  {
    enum {Code1=0x8000,Bits1=3};//MOVIRS
    enum {Code2=0xF000,Bits2=9};//MOVDATA1
    enum {Code3=0xF080,Bits3=10};//MOVDATA2(2)

    static uint32_t create(Operand a,Operand b,uint32_t irsOffset=0,bool incAddr=false)
    {
      uint32_t offset=irsOffset&0x1FF;
      uint32_t incAD=incAddr;
      uint32_t muxA=(b==REG_RES)?MUX1_RESULT:MUX1_MEM;
      uint32_t muxB=(b==REG_IRS)?MUX2_IRS:MUX2_MEM;
      uint32_t muxAD0=(a==DEREF_AD0)?0:1;
      uint32_t muxAD1=(a==DEREF_AD0 || b==DEREF_AD0)?0:1;
      uint32_t wbReg=a;
      
      //copy instruction
      if((a == DEREF_AD0 || a == DEREF_AD1) && (b == DEREF_AD0 || b == DEREF_AD1))
      {
        //use same instruction as MOVDATA2(2) but other mux config
        muxAD0=(a==DEREF_AD0)?0:1;
        muxAD1=(b==DEREF_AD0)?0:1;
        return Code3 + (incAD<<4) + (MUX2_MEM<<3) + (muxAD1<<2) + (MUX1_MEM<<1) + (muxAD0<<0);
      }

      if(a == IRS)
      {
        //mov [IRS+offset] RESULT,LOOP
        return Code1+0x1000 + (incAD<<11) + (offset<<2) + (muxA<<1);
      }

      if(b == IRS)
      {
        //mov RESULT,LOOP,AD0,AD1 [IRS+offset]
        return Code1+0x0000 + (incAD<<11) + (offset<<2) + a;
      }

      if(a != DEREF_AD0 && a != DEREF_AD1)
      {
        //MOV RESULT, DATAx, LOOP  [DATAx]      => code/12 WBREG/2 ADDR1/1 INC/1 (A/1,B/1,ADDR0/1)
        return Code2 + (wbReg<<5) + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1);
      }

      //MOV [DATAx] RESULT,LOOP  => code/12 ADDR1/1 A/1 B/1 INC/1 (ADDR0/1)
      return Code3 + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1) + (muxAD0<<0);
      //can be used as nop or for inc ad
    }
  };

  struct Op
  {
    enum {Code1=0x0000,Bits1=1};
    enum {Code2=0xC000,Bits2=4};

    static uint32_t create(Operand a,Operand b,Command cmd,uint32_t irsOffset=0,bool incAddr=false,bool incAddr2=false)
    {
      //OP RESULT, [DATAx]  [IRS]       => code/1 ADDR0/1 A/1 OP/3 OFFSET/9 INC/1
      //OP RESULT, [DATAx]  [DATAx]      => code/8 ADDR0/1 A/1 ADDR1/1 OP/3 INC/2 (B/1)

      uint32_t muxAD0=(a==DEREF_AD0)?0:1;
      uint32_t muxAD1=(b==DEREF_AD0)?0:1;
      uint32_t muxA=(a==REG_RES)?MUX1_RESULT:MUX1_MEM;
      uint32_t muxB=MUX2_MEM;
      uint32_t incAD=incAddr;
      uint32_t incAD2=incAddr2;
      uint32_t offset=irsOffset&0x1FF;
      
      //fix bug
      if(muxA == MUX1_MEM && b != IRS)
      {
        //if both operands are mem then swap address increment signals
        uint32_t t=incAD;
        incAD=incAD2;
        incAD2=t;
      }
      
      bool cmdExt=(cmd>>3)&1;
      cmd=static_cast<SLCode::Command>(cmd&7);

      if(b == IRS)
      {
        //encode 4. bit of cmd in incAD of addr is not used
        if(muxA == MUX1_RESULT)
        {
          muxAD0=cmdExt;
        }
        
        return Code1 + (cmd<<12) + (incAD<<11) + (offset<<2) + (muxA<<1) + muxAD0;
      }
      else
      {
        return Code2 + (incAD2<<9) + (cmdExt<<8) + (cmd<<5) + (incAD<<4) + (muxB<<3) + (muxAD1<<2) + (muxA<<1) + muxAD0;
      }
    }
  };

  struct Cmp
  {
    enum {Code=0xD000,Bits=4};

    static uint32_t create(uint32_t irsOffset,CmpMode mode,bool disableExecuteFor3Cycles=false)
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
      //GOTO CONST       => code/6 CONST/10 (A/1) (MD/1)

      uint32_t muxA=MUX1_RESULT;
      uint32_t loopEnd=loopEndMarker;
      uint32_t pcAdjust=(relJump&0x3FF);

      return Code + (pcAdjust<<2) + (muxA<<1) + 1;
    }
    
    //absolute goto
    static uint32_t create()
    {
      //GOTO RESULT       => code/6

      return Code + 0;
    }
    
  };

  struct Load
  {
    enum {Code=0xB000,Bits=4};

    static uint32_t constDataValue1(uint32_t value)
    {
      return ((value&0x80000000)>>20)+((value&0x3FF80000)>>19);
    }

    static uint32_t constDataValue2(uint32_t value)
    {
      return ((value&0x40000000)>>19)+((value&0x0007FF00)>>8);
    }

    static uint32_t constDataValue3(uint32_t value)
    {
      return (value&0x000000FF);
    }

    static uint32_t create(uint32_t constData)
    {
      //LOAD RESULT CONST16, CONST32     => code/4 X/12

      //1-111111 11111--- -------- --------
      //12111111 11111222 22222222 --------
      //12111111 11112222 22222222 33333333

      //uint32_t constData1=constDataValue1(value);
      //uint32_t constData2=constDataValue2(value);
      //uint32_t constData3=constDataValue3(value);

      return Code + constData;
    }
    
    static uint32_t create1(uint32_t value) { return create(constDataValue1(value)); }
    static uint32_t create2(uint32_t value) { return create(constDataValue2(value)); }
    static uint32_t create3(uint32_t value) { return create(constDataValue3(value)); }
  };

  struct UnaryOp
  {
    enum {Code=0xF100,Bits=10};

    static uint32_t create(UnaryCommand ucmd)
    {
      //NEG RESULT         => code/16  (A/1)

      uint32_t muxA=MUX1_RESULT;
      
      bool isNeg=ucmd == UNARY_NEG;
      bool isTrunc=ucmd == UNARY_TRUNC;
      
      uint32_t cmd=CMD_INVALID;
      
      if(ucmd == UNARY_LOG2)
      {
        cmd=CMD_LOG2;
      }

      return Code + ((cmd&0x7)<<3) + (isTrunc<<2) + (muxA<<1) + isNeg;
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
  
  struct Loop
  {
    enum {Code=0xF140,Bits=10};

    enum {};

    static uint32_t create()
    {
      //LOOP         => code/10

      uint32_t muxA=MUX1_RESULT;

      return Code + (muxA<<1);
    }
  };
}

#endif /* SLPROCESSORINSTRDEF_H_ */
