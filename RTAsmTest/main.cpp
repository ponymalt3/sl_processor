#include "RTAsmTest.h"
#include <mtest.h>

// run all tests
int main(int argc, char **argv)
{
  /*
  
  RTProg testCode=R"abc(
    ref proc_id 4;
    def mem_size 512;
    def shared_size 2048;
    
    a1=512+proc_id;
    [a1]=proc_id;
    tt=0;
    loop(99999)
      tt=tt+1;
    end
    tt=tt;
  )abc";
  
  Stream s(testCode);
  CodeGen codeGen(s);
  RTParser parser(codeGen);
  
  parser.parse(s);
  if(Error(testCode.getErrorHandler()).getNumErrors() == 0)
  {
    codeGen.storageAllocationPass(512,0);
  }
  else
  {
    std::cout<<"found "<<(Error(testCode.getErrorHandler()).getNumErrors())<<" errors\n";
  }
  
  uint16_t code[512];
    
  std::cout<<"\ncode:\n";
  for(uint32_t i=0;i<codeGen.getCurCodeAddr();++i)
  {
    code[i]=codeGen.getCodeAt(i);
    std::cout<<std::dec<<(i)<<") "<<std::hex<<(code[i])<<"\n";
  }
  
  std::cout<<std::dec<<"\n";
  std::cout<<"disasm:\n"<<(DisAsm::getStringFromCode(code,codeGen.getCurCodeAddr()))<<"\n";
   */

  LoadAndSimulateProcessor::getVdhlTestGenerator().enable("test.vector");
  mtest::runAllTests("*.*",mtest::enableColor);//**);
  return 0;
}