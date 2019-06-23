#pragma once

#include <functional>
#include <map>

class Peripherals
{
public:
  typedef std::function<uint32_t(uint32_t,uint32_t,bool)> handler_t;
  
  void addPeripheral(uint32_t baseAddr,uint32_t size,const handler_t &handler)
  {
    peripherals_.insert(std::make_pair(baseAddr,std::make_pair(size,handler)));
  }
  
  std::pair<bool,uint32_t> accessPeripheral(uint32_t addr,uint32_t data,bool write)
  {
    auto it=--(peripherals_.upper_bound(addr));

    if(it != peripherals_.end() && it->first >= addr && addr < (it->first+it->second.first))
    {
      return std::make_pair(true,it->second.second(addr-it->first,data,write));
    }
    
    return std::make_pair(false,0U);
  }
  
  void createPeripherals()
  {
    //Peripherals p;
    this->addPeripheral(0xF00100,4,[](uint32_t offset,uint32_t data,bool write){
      static uint32_t state=0x34aeb348;
      
      uint32_t stateOld=state;
      state=state^(state<<13)^(state>>17)^(state<<5);
      std::cout<<"access: "<<(offset)<<"  state: 0x"<<std::hex<<(state)<<"  old: 0x"<<(stateOld)<<std::dec<<"\n";
      
      switch(offset)
      {
        case 0: return 0x00000000+(stateOld>>8);
        case 1: return (((stateOld>>7)&1)<<31)+0x00000000+(stateOld>>8);
        case 2: return stateOld;
        case 3:
          if(write)
          {
            state=data;
          }
        default:
          ;
      }
      
      return 0U;
    });
  }
  
protected:
  std::map<uint32_t,std::pair<uint32_t,handler_t>> peripherals_;
};