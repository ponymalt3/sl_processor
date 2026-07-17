// Standalone codegen for cluster cocotb tests: compiles an RT-assembler source file
// (with the 4-core entry vector, main1()..main4()) and dumps the resulting code words
// as hex, one per line. Never calls execute()/loadCode() -- the single-core ISS's
// ext-memory bound is therefore irrelevant, this only compiles.

#include "RTAsmTest.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    std::cerr << "usage: gen_code <resolved-source-file>\n";
    return 1;
  }

  std::ifstream f(argv[1]);
  if(!f.is_open())
  {
    std::cerr << "could not open '" << argv[1] << "'\n";
    return 1;
  }

  std::stringstream ss;
  ss << f.rdbuf();

  RTProg prog(ss.str());
  RTProgTester tester(prog);

  // RTProgTester::parse() logs debug info (function list etc.) to std::cout; redirect it
  // away so only the hex dump below reaches stdout for the caller to parse.
  std::stringstream discard;
  std::streambuf* savedCoutBuf = std::cout.rdbuf(discard.rdbuf());
  auto err = tester.parse(0, true);  // generateFullEntryVector=true -> generateEntryVector(4,4)
  std::cout.rdbuf(savedCoutBuf);

  if(err.getNumErrors() != 0)
  {
    std::cerr << "compile failed with " << err.getNumErrors() << " error(s)\n";
    return 1;
  }

  for(uint32_t i = 0; i < tester.getCodeSize(); ++i)
  {
    printf("%04x\n", tester.getCodeAt(i));
  }

  return 0;
}
