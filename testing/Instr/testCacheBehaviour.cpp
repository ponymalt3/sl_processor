#include <mtest.h>

#include "SLProcessorTest.h"

class TestCacheBehaviour : public mtest::test
{
};

MTEST(TestCacheBehaviour, test_that_fetch_stalls_after_goto_is_handled_correctly)
{
  qfp32_t value = 7;

  uint32_t code[] = {SLCode::Goto::create(4, false),

                     0xFFFF,
                     0xFFFF,
                     0xFFFF,

                     SLCode::Load::create1(value.toRaw()),
                     SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 7, 0),

                     0xFFFF,
                     0xFFFF,
                     0xFFFF};

  LoadAndSimulateProcessor proc(code);

  proc.reset();
  proc.execute(3);
  proc.executeWithCodeStall(4);
  proc.execute(3);

  proc.expectThatMemIs(7, value);
}

MTEST(TestCacheBehaviour, test_that_code_stall_around_entry_vector_goto_does_not_corrupt_target)
{
  qfp32_t target = 10;
  qfp32_t marker = 42;

  // entry-vector sequence, matches CodeGen::generateEntryVector exactly
  uint32_t code[] = {SLCode::Load::create1(target.toRaw()),  // addr0
                     SLCode::Load::create2(target.toRaw()),  // addr1
                     SLCode::Goto::create(),                 // addr2 (absolute jump via RESULT)

                     0xFFFF,
                     0xFFFF,
                     0xFFFF,
                     0xFFFF,
                     0xFFFF,
                     0xFFFF,
                     0xFFFF,  // addr3..9 padding

                     // target (addr10)
                     SLCode::Load::create1(marker.toRaw()),                    // addr10
                     SLCode::Load::create2(marker.toRaw()),                    // addr11
                     SLCode::Mov::create(SLCode::IRS, SLCode::REG_RES, 7, 0),  // addr12: mem[7]=marker

                     0xFFFF,
                     0xFFFF,
                     0xFFFF};

  LoadAndSimulateProcessor proc(code);
  proc.writeMemory(7, 0U);
  proc.reset();

  proc.execute(3);
  proc.executeWithCodeStall(3);
  proc.execute(25);

  EXPECT(proc.readMemory(7) == marker.toRaw());
  proc.expectThatMemIs(7, marker);
}
