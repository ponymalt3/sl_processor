#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

#include "qfp32.h"

#include "RTAsm/RTParser.h"
#include "RTAsm/CodeGen.h"
#include "RTAsm/RTProg.h"

#include "UartInterface.h"
#include "SystemControl.h"

#include <regex>

std::string resolveIncludes(const std::string &filename)
{
  std::cout<<"filename: "<<(filename)<<"\n";
  std::fstream f(filename,std::ios::in);
  
  if(!f.is_open())
  {
    return "";
  }
  
  std::string data;
  
  f.seekg(0,std::ios::end);
  uint32_t size=f.tellg();
  f.seekg(0,std::ios::beg);
  
  data.resize(size);
  f.read(const_cast<char*>(data.c_str()),size);

  std::string path="";
  std::string::size_type slashPos=filename.rfind('/');
  if(slashPos != std::string::npos)
  {
    path=filename.substr(0,slashPos);
  }
  
  std::regex regex("include\\((.*)\\)");
  std::smatch m;
  while(std::regex_search(data,m,regex))
  {
    std::string toReplace=m[0].str();
    std::string replaceData=resolveIncludes(path + "/" + m[1].str());
    data.replace(data.find(toReplace),toReplace.size(),replaceData);
  }
  
  return data;
}

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

int main(int argc, char **argv)
{
  bool compile=false;
  bool program=false;
  bool read=false;
  bool write=false;
  bool enableCores=false;
  std::string arg0="";
  std::string arg1="";
  std::string arg2="";
  
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
        case 'h':
          std::cout<<"-c compile input output\n-p write compiled program to fpga\n-t execute test case specified in input\n-d dump all memory to stdout\n";
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
    std::string data=resolveIncludes(arg0);
    
    RTProg prog(data.c_str());
    Stream s(prog);
    CodeGen gen(s);
    
    RTParser parser(gen);
    parser.parse(s);
    
    if(Error(prog.getErrorHandler()).getNumErrors() == 0)
    {
      gen.storageAllocationPass(512,0);
    }
    else
    {
      return -1;
    }
    
    if(!program)
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
