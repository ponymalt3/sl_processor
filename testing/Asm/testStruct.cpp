#include <mtest.h>

#include "RTAsmTest.h"

class testStruct : public mtest::test
{
};

MTEST(testStruct, test_that_struct_decl_with_initializer_works)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {3,4};

    a=p.x;
    b=p.y;
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 3);
  tester.expectSymbol("b", 4);
  tester.expectSymbolWithOffset("p", 0, 3);
  tester.expectSymbolWithOffset("p", 1, 4);
}

MTEST(testStruct, test_that_struct_field_write_works)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {1,2};
    p.x=99;
    c=p.x;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("c", 99);
  tester.expectSymbolWithOffset("p", 0, 99);
  tester.expectSymbolWithOffset("p", 1, 2);
}

MTEST(testStruct, test_that_uninitialized_struct_decl_works)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p;
    p.x=5;
    p.y=6;
    sum=p.x+p.y;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("sum", 11);
}

MTEST(testStruct, test_that_struct_field_index_matches_plain_array_index)
{
  // struct instances are plain arrays with named field sugar, so numeric indexing still works
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {7,8};
    a=p(0);
    b=p(1);
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 7);
  tester.expectSymbol("b", 8);
}

MTEST(testStruct, test_that_struct_passed_by_reference_to_function_mutates_caller_data)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    function movePoint(Point p, dx)
      p.x=p.x+dx;
    end

    Point pt {10,20};
    movePoint(pt,5);
    result=pt.x;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("result", 15);
  tester.expectSymbolWithOffset("pt", 0, 15);
}

MTEST(testStruct, test_that_struct_reference_reading_two_fields_in_one_expr_works)
{
  // exercises that the a1 address is recomputed for every single field access instead of being cached
  RTProg testCode = R"asm(
    struct Point { x; y; };

    function sumPoint(Point p)
      return p.x+p.y;
    end

    Point pt {7,8};
    total=sumPoint(pt);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("total", 15);
}

MTEST(testStruct, test_that_struct_reference_via_a1_does_not_clobber_manual_a0_array_iteration)
{
  // struct field access always uses a1; a manually driven a0 array walk in the same
  // function must keep working across repeated struct-ref field reads inside the loop
  RTProg testCode = R"asm(
    struct Point { x; y; };

    function sumArrayWithOffset(Point p, arr, size)
      s=0;
      a0=arr;
      loop(size)
        s=s+[a0++]+p.x;
      end
      return s+p.y;
    end

    Point pt {1,2};
    data {10,20,30};
    total=sumArrayWithOffset(pt,data,3);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("total", 10 + 20 + 30 + 3 * 1 + 2);
}

MTEST(testStruct, test_that_struct_reference_field_access_survives_a_function_call_in_between)
{
  // a1 is a plain scratch register with no call-boundary save/restore. touchA1() clobbers
  // a1 while reading its own struct parameter 'other'; the field address for 'p' must be
  // recomputed after the call returns, not assumed to still be valid from before the call.
  RTProg testCode = R"asm(
    struct Point { x; y; };

    function touchA1(Point q)
      return q.x;
    end

    Point other {77,88};
    data {5,6};
    ref Point p = data;

    a=p.x;
    touchA1(other);
    b=p.y;
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 5);
  tester.expectSymbol("b", 6);
}

MTEST(testStruct, test_that_struct_pointer_cast_of_array_base_address_works)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    data {11,22};
    ref Point p = data;

    a=p.x;
    b=p.y;
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 11);
  tester.expectSymbol("b", 22);
}

MTEST(testStruct, test_that_struct_pointer_cast_with_offset_arithmetic_works)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    data {1,2,3,4};
    ref Point p = data+2;

    a=p.x;
    b=p.y;
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 3);
  tester.expectSymbol("b", 4);
}

MTEST(testStruct, test_that_writing_through_struct_pointer_mutates_underlying_array)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    data {5,6};
    ref Point p = data;
    p.x=99;
    c=data(0);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("c", 99);
  tester.expectSymbolWithOffset("data", 0, 99);
  tester.expectSymbolWithOffset("data", 1, 6);
}

MTEST(testStruct, test_that_struct_pointer_can_be_declared_and_assigned_separately)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    data {7,8};
    ref Point p;
    p=data;

    a=p.x;
    a=a;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 7);
}

MTEST(testStruct, test_that_struct_pointer_cast_is_unchecked_across_struct_types)
{
  // casting is a raw reinterpret: no type enforcement (unlike passing structs to function parameters)
  RTProg testCode = R"asm(
    struct Point { x; y; };
    struct Vec2 { u; v; };

    data {3,4};
    ref Vec2 q = data;

    a=q.u;
    b=q.v;
    a=a;
    b=b;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();
  tester.execute();

  tester.expectSymbol("a", 3);
  tester.expectSymbol("b", 4);
}

MTEST(testStruct, test_that_struct_redefinition_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };
    struct Point { a; b; };
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testStruct, test_that_unknown_field_access_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {1,2};
    z=p.zzz;
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testStruct, test_that_too_few_initializers_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {1};
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testStruct, test_that_too_many_initializers_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    Point p {1,2,3};
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testStruct, test_that_wrong_struct_type_as_function_argument_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };
    struct Vec { a; b; c; };

    function f(Point p)
      z=p.x;
    end

    Vec v {1,2,3};
    f(v);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}

MTEST(testStruct, test_that_numeric_index_on_struct_reference_parameter_is_an_error)
{
  RTProg testCode = R"asm(
    struct Point { x; y; };

    function f(Point p)
      z=p(0);
    end

    Point pt {1,2};
    f(pt);
  )asm";

  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() != 0);
}
