#pragma concept

#include "UartInterface.h"

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