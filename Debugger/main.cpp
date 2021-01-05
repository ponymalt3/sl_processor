#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "qfp32.h"

#include "Assembler/RTParser.h"
#include "Assembler/CodeGen.h"
#include "Assembler/RTProg.h"
#include "Assembler/DisAsm.h"

#include "UartInterface.h"
#include "SystemControl.h"
#include "IncludeResolver.h"

#include "Debugger.h"
#include "Simulator.h"
#include "CommonPrefixTree.h"

#include "termios.h"

uint32_t swap16(uint32_t value)
{
  return ((value&0xFF)<<8)+((value>>8)&0xFF);
}

uint32_t swap32(uint32_t value)
{
  uint32_t result=swap16(value&0xFFFF);
  result=(result<<16)+swap16(value>>16);
  return result;
}

enum {KEY_UP=165,KEY_DOWN=166,KEY_BACKSPACE=127};

int main(int argc, char **argv)
{
  bool compile=false;
  bool program=false;
  bool read=false;
  bool write=false;
  bool enableCores=false;
  bool debug=false;
  std::string arg0="";
  std::string arg1="";
  std::string arg2="";
  std::string debugTarget="";

  for(uint32_t i=1;i<argc;++i)
  {
    if(argv[i][0] == '-')
    {
      switch(argv[i][1])
      {
        case 'c' | 'C': compile=true; break;
        case 'p' | 'P': program=true; break;
        case 'r' | 'R': read=true; break;
        case 'w' | 'W': write=true; break;
        case 'e' | 'E': enableCores=true; break;
        case 'd' | 'D':
          debug=true;
          if(argc > (i+1))
          {
            debugTarget=argv[i+1];
            ++i;
          }
          break;
        case 'h':
          std::cout<<"-c compile input output\n"
                     "-p write compiled program to fpga\n"
                     "-t execute test case specified in input\n"
                     "-d sim/target start debugging after compile\n";
          break;
        default:
          std::cout<<"unknown switch '"<<(argv[i]+1)<<"'\n";
          return -1;
      }
    }
    else
    {
      if(arg0.length() == 0)
      {
        arg0=argv[i];
      }
      else if(arg1.length() == 0)
      {
        arg1=argv[i];
      }
      else if(arg2.length() == 0)
      {
        arg2=argv[i];
      }
      else
      {
        std::cout<<"unexpected argument '"<<(argv[i])<<"'\n";
        return -1;
      }
    }
  }

  UartInterface uart("/dev/ttyUSB0");
  SystemControl control(&uart);

  if(!control.checkConnection() && (program || read || write || enableCores))
  {
    std::cout<<"System not connected/connection problem\n";
    return -1;
  }

  if(program || compile)
  {
    IncludeResolver ir(arg0);
    ir.storeResolvedCode("resolved");

    if(ir.getResolvedCode().size() == 0)
    {
      return -1;
    }

    auto filesAsLineVectors=ir.getFilesAsLineVectors();
    auto lineToFileMap=ir.getLineToFileMap();
    std::cout<<"line to file map\n";
    for(auto &i : lineToFileMap)
    {
      std::cout<<"asm "<<(i.first)<<"  =>  "<<(i.second.first)<<"  with offset: "<<(i.second.second)<<"\n";
    }

    RTProg prog(ir.getResolvedCode().c_str());
    Stream s(prog);
    CodeGen gen(s,6);

    RTParser parser(gen);
    parser.parse(s);
    gen.generateEntryVector(1,4);

    std::cout<<"Num Instrs: "<<(gen.getCurCodeAddr())<<"\n";

    if(Error(prog.getErrorHandler()).getNumErrors() == 0)
    {
      gen.storageAllocationPass(512,0);
    }
    else
    {
      return -1;
    }

    uint16_t *code=new uint16_t[gen.getCurCodeAddr()];
    for(uint32_t i=0;i<gen.getCurCodeAddr();++i)
    {
      code[i]=gen.getCodeAt(i);
    }


    if(debug)
    {
      DebuggerInterface *debIfc=0;

      if(debugTarget == "sim")
      {
        debIfc=new Simulator(4096,512,512);
      }
      else if(debugTarget == "target")
      {

      }
      else
      {
        std::cout<<"invalid debug target\n";
        return -1;
      }

      Debugger deb(debIfc,
                   gen.getDefaultSymbols(),
                   gen.getFunctions(),
                   parser.getLineMapping(),
                   ir.getLineToFileMap(),
                   ir.getFilesAsLineVectors(),
                   code,
                   gen.getCurCodeAddr(),
                   0x80000000);

      deb.loadCode();

      termios t;
      tcgetattr(0,&t);
      t.c_lflag&=(~ICANON) & (~ECHO);
      tcsetattr(0,TCSANOW,&t);

      auto x=DisAsm::getLinesFromCode(code,gen.getCurCodeAddr());
      std::cout<<"DisAsm:\n";
      for(uint32_t i=0;i<x.size();++i)
      {
        std::cout<<(i)<<". "<<(x[i])<<"\n";
      }
      std::cout<<"\n";

      CommonPrefixTree cpt;
      cpt.insert("show ");
      cpt.insert("run ");
      cpt.insert("quit");
      cpt.insert("show callstack");
      cpt.insert("show mem[");
      cpt.insert("step");

      for(auto &i : ir.getFilesAsLineVectors())
      {
        cpt.insert(std::string("run ") + i.first + ":");
      }

      std::string result;
      std::vector<std::string> prevCmds;
      do
      {
        std::cout<<"> ";
        bool newCmd=true;
        uint32_t index=prevCmds.size();
        std::string cmd;
        std::string cmdTmp;
        int16_t ch=getchar();
        while(ch != '\n' || cmd.length() == 0)
        {
          if(ch == 27)
          {
            ch=getchar();
            ch=getchar()+100;
          }

          switch(ch)
          {
          case KEY_BACKSPACE:
            if(cmd.length() > 0)
            {
              cmd=cmd.substr(0,cmd.length()-1);
              std::cout<<"\b \b";
            }
            break;

          case KEY_UP:
            if(prevCmds.size() == 0)
            {
              break;
            }
            for(uint32_t i=0;i<cmd.length();++i) std::cout<<"\b \b";
            if(index > 0)
            {
              --index;
            }
            cmd=prevCmds[index];
            std::cout<<cmd;
            break;

          case KEY_DOWN:
            for(uint32_t i=0;i<cmd.length();++i) std::cout<<"\b \b";
            if(index < (prevCmds.size()-1))
            {
              ++index;
              cmd=prevCmds[index];
            }
            else
            {
              index=prevCmds.size();
              cmd=cmdTmp;
            }
            std::cout<<cmd;
            break;

          case '\t':
          {
            std::string extend=cpt.getCommonPrefix(cmd);
            std::cout<<extend;
            cmd+=extend;
            cmdTmp=cmd;
            break;
          }

          default:
            if(ch != '\n')
            {
              cmd.push_back(ch);
              std::cout<<(uint8_t(ch));
            }
            cmdTmp=cmd;
          }

          ch=getchar();
        }

        prevCmds.push_back(cmd);
        result=deb.command(cmd);
        std::cout<<"\n"<<(result)<<"\n";
      }
      while(result.length() > 0);
    }

    delete [] code;

    if(!program && !debug)
    {
      return 0;
    }

    if(!control.resetCores(0xFFFFFFFF))
    {
      std::cout<<"cannot reset cores\n";
      return -1;
    }

    uint32_t numWords=gen.getCurCodeAddr()+3;
    uint32_t *buffer=new uint32_t[numWords];

    memset(buffer,0xFF,numWords*4);

    for(uint32_t i=0;i<gen.getCurCodeAddr();++i)
    {
      buffer[i]=swap16(gen.getCodeAt(i))<<16;
    }

    std::cout<<"first code word: "<<std::hex<<(buffer[0])<<std::dec<<"\n";

    if(!control.write(80*1024/4,buffer,numWords))
    {
      std::cout<<"write code memory failed\n";
      delete [] buffer;
      return -1;
    }

    std::cout<<"program complete! "<<(numWords*4)<<" bytes written\n";
    delete [] buffer;

    return 0;
  }

  if(read)
  {
    uint32_t addr=0;
    {
      std::istringstream iss(arg0);
      iss>>addr;
    }

    uint32_t length=0;
    {
      std::istringstream iss(arg1);
      iss>>length;
    }

    std::cout<<"read request\n  addr: "<<(addr)<<"\n  length: "<<(length)<<"\n";

    uint32_t numWords=((length+3)&(~3))/4;
    uint32_t *buffer=new uint32_t[numWords];

    if(!control.read(addr,buffer,numWords))
    {
      delete [] buffer;
      std::cout<<"  failed\n";
      return -1;
    }

    std::cout<<"  complete! "<<(numWords*4)<<" bytes read\n";

    std::ostream *stream=&(std::cout);

    if(arg2.length() > 0)
    {
      std::fstream *f=new std::fstream(arg2,std::ios::out|std::ios::binary);

      if(!f->is_open())
      {
        std::cout<<"cannot open file "<<(arg2)<<"\n";
        delete f;
        delete [] buffer;
        return -1;
      }

      stream=f;
    }

    for(uint32_t i=0;i<numWords;++i)
    {
      (*stream)<<(qfp32_t::initFromRaw(swap32(buffer[i])))<<"\n";
    }

    if(stream != &(std::cout))
    {
      delete stream;
    }

    delete [] buffer;

    return 0;
  }

  if(write)
  {
    uint32_t addr=0;
    {
      std::istringstream iss(arg0);
      iss>>addr;
    }

    uint32_t length=0;
    {
      std::istringstream iss(arg1);
      iss>>length;
    }

    std::cout<<"write request\n  addr: "<<(addr)<<"\n  length: "<<(length)<<"\n";

    if(arg2.find_first_not_of("0123456789.") != std::string::npos)
    {
      //is a filename
      std::fstream f(arg2,std::ios::in|std::ios::binary);

      if(!f.is_open())
      {
        std::cout<<"cannot open file "<<(arg2)<<"\n";
        return -1;
      }

      uint32_t *buffer=new uint32_t[length];

      uint32_t i=0;
      for(i=0;i<length && !f.eof();++i)
      {
        double data=0;
        f>>data;
        buffer[i]=swap32(qfp32_t(data).getAsRawUint());
        std::cout<<"  write "<<(qfp32_t(data))<<"\n";
      }

      if(!control.write(addr,buffer,i))
      {
        delete [] buffer;
        std::cout<<"  failed\n";
        return -1;
      }

      std::cout<<"  complete! "<<(i*4)<<" bytes written\n";

      delete [] buffer;
    }
    else
    {
      double data=0;
      std::istringstream iss(arg2);
      iss>>data;

      uint32_t buffer=swap32(qfp32_t(data).getAsRawUint());
      if(!control.write(addr,&buffer,1))
      {
        std::cout<<"  failed\n";
        return -1;
      }

      std::cout<<"  complete! "<<(4)<<" bytes written\n";
    }

    return 0;
  }

  if(enableCores)
  {
    uint32_t mask=0;
    std::istringstream iss(arg0);
    iss>>std::hex>>mask;

    std::cout<<"mask: "<<(mask)<<"\n";

    if(!control.enableCoreMask(mask))
    {
      std::cout<<"enable cores failed\n";
      return -1;
    }

    uint32_t numCores=0;
    for(uint32_t i=0;i<16;++i)
    {
      if(mask&(1<<i))
      {
        ++numCores;
      }
    }

    std::cout<<(numCores)<<" cores enabled\n";

    return 0;
  }

	return 0;
}
