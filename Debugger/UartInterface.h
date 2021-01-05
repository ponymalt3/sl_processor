#pragma once

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
