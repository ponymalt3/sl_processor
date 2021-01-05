#include <mtest.h>
#include "ProcessorTest/SLProcessorTest.h"

// run all tests
int main(int argc, char **argv)
{
  LoadAndSimulateProcessor::getVdhlTestGenerator().enable("test.vector");
  mtest::runAllTests("*.*",mtest::enableColor);
  return 0;
}
