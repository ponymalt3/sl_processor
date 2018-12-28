#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

#include "qfp32.h"

#include "RTAsm/RTParser.h"
#include "RTAsm/CodeGen.h"
#include "RTAsm/RTProg.h"

#include <regex>

std::string resolveIncludes(const std::string &filename)
{
  std::cout<<"filename: "<<(filename)<<"\n";
  std::fstream f(filename,std::ios::in);
  std::string data;
  
  f.seekg(0,std::ios::end);
  uint32_t size=f.tellg();
  f.seekg(0,std::ios::beg);
  
  data.reserve(size);
  f.read(const_cast<char*>(data.c_str()),size);
  
  std::smatch m;
  std::regex regex("include\\((.*)\\)");
  while(std::regex_match(data,m,regex))
  {
    std::string toReplace=m[0].str();
    std::string replaceData=resolveIncludes(filename + "/" + m[1].str());
    data.replace(data.find(toReplace),toReplace.size(),replaceData);
  }
  
  return data;
}


#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

class UartInterface
{
public:
  UartInterface(const std::string &device)
  {
    handle_=open(device.c_str(),O_RDWR | O_NOCTTY);
    
    termios config;
    tcgetattr(handle_,&config);
    config.c_cflag=B115200 | CS8 | CREAD | CLOCAL;
    config.c_oflag=0;
    config.c_iflag=0;
    config.c_lflag=0;
    tcsetattr(handle_,TCSANOW,&config);
    tcflush(handle_,TCIOFLUSH);
  }
  
  UartInterface(UartInterface &&mv)
  {
    handle_=mv.handle_;
    mv.handle_=-1;
  }
  
  ~UartInterface()
  {
    if(handle_ != -1)
    {
      close(handle_);
    }
  }
  
  int32_t write(const uint8_t *data,uint32_t size)
  {
    /*for(uint32_t i=0;i<size;++i)
    {
      ::write(handle_,data+i,1);
    }
    return 0;*/
    return ::write(handle_,data,size);
  }
  
  int32_t read(uint8_t *data,uint32_t size)
  {
    return ::read(handle_,data,size);
  }
  
protected:
  int handle_;
};


class SystemControl
{
public:
  enum {CMD_READ=0x81,CMD_WRITE=0x82,CMD_CORE=0x03,CMD_PING=0x04};
  
  SystemControl(UartInterface *uart) : io_(uart)
  {
    coreEn_=0;
  }
  
  bool checkConnection()
  {
    uint8_t cmd=CMD_PING;
    io_->write(&cmd,1);
    
    uint8_t read=0;
    io_->read(&read,1);
    
    return read == cmd;
  }
  
  bool enableCore(uint32_t coreId)
  {
    coreEn_|=1<<coreId;
    return updateState(coreEn_,0);
  }
  
  bool enableCoreMask(uint32_t mask)
  {
    coreEn_=mask;
    return updateState(coreEn_,0);
  }
  
  bool disableCore(uint32_t coreId)
  {
    coreEn_&=~(1<<coreId);
    return updateState(coreEn_,0);
  }
  
  bool resetCores(uint32_t coreMask)
  {
    coreEn_&=~coreMask;
    return updateState(coreEn_,coreMask);
  }
  
  bool write(uint32_t addr,const uint32_t *data,uint32_t size)
  {
    uint8_t buffer[]=
    {
      CMD_WRITE,
      (addr>>8)&0xFF,addr&0xFF,
      (size>>8)&0xFF,size&0xFF
    };
    
    io_->write(buffer,sizeof(buffer));
    io_->write((const uint8_t*)data,size*4);
    
    std::cout<<"write complete... "<<std::endl;
    uint8_t read=0;
    io_->read(&read,1);
    
    return read == CMD_WRITE;
  }
  
  bool read(uint32_t addr,uint32_t *data,uint32_t size)
  {
    uint8_t buffer[]=
    {
      CMD_READ,
      (addr>>8)&0xFF,addr&0xFF,
      (size>>8)&0xFF,size&0xFF
    };
    
    io_->write(buffer,sizeof(buffer));
    
    uint32_t bytesRead=0;
    while(bytesRead < size*4)
    {
      bytesRead+=io_->read(((uint8_t*)data)+bytesRead,size*4-bytesRead);
    }
    
    if(bytesRead != size*4)
    {
      return false;
    }
    
    uint8_t read=0;
    io_->read(&read,1);
    
    return read == CMD_READ;
  }
  
protected:
  bool updateState(uint32_t enable,uint32_t reset)
  {
    uint8_t buffer[]=
    {
      CMD_CORE,
      reset&0xFF,
      enable&0xFF
    };
    
    io_->write(buffer,sizeof(buffer));
    
    uint8_t read=0;
    io_->read(&read,1);
    
    return read == CMD_CORE; 
  }
  
  UartInterface *io_;
  uint32_t coreEn_;
};

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