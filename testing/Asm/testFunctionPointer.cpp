#include <mtest.h>

#include "RTAsmTest.h"

class testFunctionPointer : public mtest::test
{
};

MTEST(testFunctionPointer, test_that_a_bare_function_name_can_be_assigned_and_called_through)
{
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    fn = addOne;
    a = fn(10);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 11);
}

MTEST(testFunctionPointer, test_that_reassigning_the_pointer_calls_the_new_function)
{
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    function double(x)
      return x*2;
    end

    fn = addOne;
    r = fn(10);
    a0=200;
    [a0++]=r;

    fn = double;
    r = fn(10);
    [a0]=r;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectMemoryAt(200, 11);
  tester.expectMemoryAt(201, 20);
}

MTEST(testFunctionPointer, test_that_a_function_can_be_passed_as_a_parameter_and_called_indirectly)
{
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    function callIt(fn, v)
      return fn(v);
    end

    r = callIt(addOne, 41);
    a0=200;
    [a0]=r;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectMemoryAt(200, 42);
}

MTEST(testFunctionPointer, test_that_a_bare_indirect_call_statement_works)
{
  RTProg testCode = R"asm(
    function setFlag(addr)
      a0=addr;
      [a0]=77;
    end

    fn = setFlag;
    fn(200);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectMemoryAt(200, 77);
}

MTEST(testFunctionPointer, test_that_a_struct_field_can_hold_a_function_pointer)
{
  // calling directly through the field expression (h.fn(x)) isn't supported --
  // copy it into a plain local first, same as you'd do with any other
  // computed value that a struct-ref field read can't be chained with.
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    struct Handler { fn; payload; };

    function runHandler(Handler h)
      handler = h.fn;
      return handler(h.payload);
    end

    Handler h;
    h.fn = addOne;
    h.payload = 99;
    r = runHandler(h);
    a0=200;
    [a0]=r;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectMemoryAt(200, 100);
}

MTEST(testFunctionPointer, test_that_taking_the_address_of_an_inline_function_is_an_error)
{
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    fn = addOne;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse(0, false, 1000).getNumErrors() != 0);  // force inlining
}

MTEST(testFunctionPointer, test_that_numeric_array_indexing_still_works_alongside_indirect_calls)
{
  RTProg testCode = R"asm(
    function addOne(x)
      return x+1;
    end

    data {5,6,7};
    fn = addOne;

    r = fn(data(1));
    a0=200;
    [a0]=r;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectMemoryAt(200, 7);
}
