#include <mtest.h>

#include "SLProcessorTest.h"

class TestMac : public mtest::test
{
};

// Single MAC: result = a * b  (using IRS as second operand)
MTEST(TestMac, testMacSingleTerm)
{
  qfp32_t a = 3.0;
  qfp32_t b = 4.0;

  uint32_t code[] = {
      SLCode::Load::create1(a.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 5),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 10),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, b);
  proc.writeMemory(10, 0U);

  proc.run(20);
  proc.expectThatMemIs(10, a * b);
}

// Two-term dot product: a0*b0 + a1*b1
// MACs are spaced apart by the address loads (≥3 cycle gap)
MTEST(TestMac, testMacTwoTerms)
{
  qfp32_t a0 = 2.0, b0 = 3.0;
  qfp32_t a1 = 4.0, b1 = 5.0;

  qfp32_t ad0_base = 5;
  qfp32_t ad1_base = 10;

  uint32_t code[] = {
      SLCode::Load::create1(ad0_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD0, SLCode::REG_RES),
      SLCode::Load::create1(ad1_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD1, SLCode::REG_RES),

      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 30),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, a0);
  proc.writeMemory(6, a1);
  proc.writeMemory(10, b0);
  proc.writeMemory(11, b1);
  proc.writeMemory(30, 0U);

  proc.run(30);
  proc.expectThatMemIs(30, a0 * b0 + a1 * b1);
}

// Four-term dot product
MTEST(TestMac, testMacDotProduct4)
{
  qfp32_t a[4] = {1.0, 2.0, 3.0, 4.0};
  qfp32_t b[4] = {5.0, 6.0, 7.0, 8.0};
  qfp32_t expected = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];

  qfp32_t ad0_base = 5;
  qfp32_t ad1_base = 20;

  uint32_t code[] = {
      SLCode::Load::create1(ad0_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD0, SLCode::REG_RES),
      SLCode::Load::create1(ad1_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD1, SLCode::REG_RES),

      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 50),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  for(int i = 0; i < 4; ++i)
  {
    proc.writeMemory(5 + i, a[i]);
    proc.writeMemory(20 + i, b[i]);
  }
  proc.writeMemory(50, 0U);

  proc.run(40);
  proc.expectThatMemIs(50, expected);
}

// MAC with negative values: (-3)*2 + 5*(-1) = -11
MTEST(TestMac, testMacNegativeValues)
{
  qfp32_t a0 = -3.0, b0 = 2.0;
  qfp32_t a1 = 5.0, b1 = -1.0;
  qfp32_t expected = a0 * b0 + a1 * b1;

  qfp32_t ad0_base = 5;
  qfp32_t ad1_base = 15;

  uint32_t code[] = {
      SLCode::Load::create1(ad0_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD0, SLCode::REG_RES),
      SLCode::Load::create1(ad1_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD1, SLCode::REG_RES),

      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 60),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, a0);
  proc.writeMemory(6, a1);
  proc.writeMemory(15, b0);
  proc.writeMemory(16, b1);
  proc.writeMemory(60, 0U);

  proc.run(30);
  proc.expectThatMemIs(60, expected);
}

// MAC_RES resets accumulator: two independent MAC sequences
// Verifies that mac_acc_reg1/reg2 are cleared after CMD_MAC_RES
MTEST(TestMac, testMacResetAfterMacRes)
{
  qfp32_t a0 = 3.0, b0 = 4.0;   // first sequence: 3*4 = 12
  qfp32_t a1 = 2.0, b1 = 5.0;   // second sequence: 2*5 = 10

  uint32_t code[] = {
      // --- sequence 1 ---
      SLCode::Load::create1(a0.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 5),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 20),  // store result1

      // --- sequence 2 (accumulator must be 0 here) ---
      SLCode::Load::create1(a1.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 6),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 21),  // store result2

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, b0);
  proc.writeMemory(6, b1);
  proc.writeMemory(20, 0U);
  proc.writeMemory(21, 0U);

  proc.run(40);
  proc.expectThatMemIs(20, a0 * b0);         // 12
  proc.expectThatMemIs(21, a1 * b1);         // 10 (not 22 = accumulator was reset!)
}

// MAC with 3-cycle gap (minimal safe gap): two MACs separated by exactly 3 instructions
// This is the minimum gap for the 2-register feedback chain to work correctly.
// Pattern: MAC, MOV, MOV, MOV, MAC, MAC_RES (3 instructions between the two MACs)
MTEST(TestMac, testMacMinimalGap3)
{
  qfp32_t a0 = 2.0, b0 = 3.0;
  qfp32_t a1 = 4.0, b1 = 5.0;
  qfp32_t expected = a0 * b0 + a1 * b1;  // 6 + 20 = 26

  uint32_t code[] = {
      SLCode::Load::create1(a0.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 5),   // MAC term 0

      // 3-instruction gap (mov RES→IRS[99] three times, neutral for result)
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),

      SLCode::Load::create1(a1.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 6),   // MAC term 1

      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 30),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, b0);
  proc.writeMemory(6, b1);
  proc.writeMemory(30, 0U);

  proc.run(40);
  proc.expectThatMemIs(30, expected);
}

// MAC with Load-then-MAC pattern providing exactly 3 instructions gap
// Load a, MAC, (Load next_a, Mov, Mov = 3-gap), MAC, ...
// RES holds the loaded value when MAC fires; MOV to IRS[99] is neutral (a dummy store).
MTEST(TestMac, testMacLoadMacWith3Gap)
{
  qfp32_t a0 = 2.0, b0 = 3.0;
  qfp32_t a1 = 4.0, b1 = 5.0;
  qfp32_t a2 = -1.0, b2 = 6.0;
  qfp32_t expected = a0 * b0 + a1 * b1 + a2 * b2;  // 6 + 20 - 6 = 20

  // mem[5]=b0, mem[6]=b1, mem[7]=b2
  uint32_t code[] = {
      SLCode::Load::create1(a0.toRaw()),
      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 5),   // MAC1: a0*b0

      // 3-instruction gap (also loads a1 ready for MAC2):
      SLCode::Load::create1(a1.toRaw()),                         // gap 1, RES=a1
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),    // gap 2, neutral store
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),    // gap 3, neutral store

      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 6),   // MAC2: a1*b1

      // 3-instruction gap:
      SLCode::Load::create1(a2.toRaw()),                         // gap 1, RES=a2
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),    // gap 2
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 99),    // gap 3

      SLCode::Op::create(SLCode::REG_RES, SLCode::IRS, SLCode::CMD_MAC, 7),   // MAC3: a2*b2

      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 50),

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(5, b0);
  proc.writeMemory(6, b1);
  proc.writeMemory(7, b2);
  proc.writeMemory(50, 0U);

  proc.run(50);
  proc.expectThatMemIs(50, expected);
}

// Two independent MAC sequences back-to-back, verifying accumulator reset between them
// Tests that a long sequence (8 terms) works and the second sequence is independent
MTEST(TestMac, testMacTwoIndependentSequences)
{
  qfp32_t a[4] = {1.0, 2.0, 3.0, 4.0};
  qfp32_t b[4] = {1.0, 1.0, 1.0, 1.0};
  qfp32_t expected1 = 10.0;  // 1+2+3+4

  qfp32_t c[4] = {5.0, 6.0, 7.0, 8.0};
  qfp32_t d[4] = {1.0, 1.0, 1.0, 1.0};
  qfp32_t expected2 = 26.0;  // 5+6+7+8

  qfp32_t ad0_base = 10;
  qfp32_t ad1_base = 20;
  qfp32_t ad0_base2 = 30;
  qfp32_t ad1_base2 = 40;

  uint32_t code[] = {
      // sequence 1
      SLCode::Load::create1(ad0_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD0, SLCode::REG_RES),
      SLCode::Load::create1(ad1_base.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD1, SLCode::REG_RES),

      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 60),  // result1 → mem[60]

      // sequence 2 (accumulator must be 0 after MAC_RES above)
      SLCode::Load::create1(ad0_base2.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD0, SLCode::REG_RES),
      SLCode::Load::create1(ad1_base2.toRaw()),
      SLCode::Mov::create(SLCode::REG_AD1, SLCode::REG_RES),

      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::DEREF_AD0, SLCode::DEREF_AD1, SLCode::CMD_MAC, 0, true, true),
      SLCode::Op::create(SLCode::REG_RES, SLCode::REG_RES, SLCode::CMD_MAC_RES),
      SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 61),  // result2 → mem[61]

      0xFFFF,
      0xFFFF,
      0xFFFF,
  };

  LoadAndSimulateProcessor proc(code);
  for(int i = 0; i < 4; ++i)
  {
    proc.writeMemory(10 + i, a[i]);
    proc.writeMemory(20 + i, b[i]);
    proc.writeMemory(30 + i, c[i]);
    proc.writeMemory(40 + i, d[i]);
  }
  proc.writeMemory(60, 0U);
  proc.writeMemory(61, 0U);

  proc.run(80);
  proc.expectThatMemIs(60, expected1);
  proc.expectThatMemIs(61, expected2);
}
