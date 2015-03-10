/*
 * SLPGroup.h
 *
 *  Created on: Jan 19, 2015
 *      Author: malte
 */

#ifndef SLPGROUP_H_
#define SLPGROUP_H_

#include "SLProcessor.h"
#include <assert.h>

class SLGroup
{
public:
  SLGroup(uint32_t codeMemSize,uint32_t processorMemSize)
  : code_(codeMemSize)
  , codePort_(code_)
  , localMem0_(processorMemSize)
  , localMem1_(processorMemSize)
  , localMem2_(processorMemSize)
  , localMem3_(processorMemSize)
  {
    codeMemSize_=codeMemSize;
    processorMemSize_=processorMemSize;

    cores_[0]=new _Core(localMem0_,code_,localMem1_);
    cores_[1]=new _Core(localMem1_,code_,localMem0_);
    cores_[2]=new _Core(localMem2_,code_,localMem3_);
    cores_[3]=new _Core(localMem3_,code_,localMem2_);
  }

  ~SLGroup()
  {
    for(uint32_t i=0;i<4;++i)
      delete cores_[i];
  }

  void writeCode(uint32_t addr,uint16_t data);

  uint32_t readData(uint32_t addr);
  void writeData(uint32_t addr,uint32_t data);

  uint32_t getState();

  void run(uint32_t cpuId);
  void signal(uint32_t flags);

protected:
  void update()
  {
    for(uint32_t i=0; i<4;++i)
      cores_[i]->processor_.update(cores_[i]->access_);
  }

  uint32_t codeMemSize_;
  uint32_t processorMemSize_;

  struct _Core
  {
    _Core(Mem &localMem,Mem &code,Mem &extMem)
    : localExt_(extMem)
    , code_(code)
    , processor_(localMem,localExt_,code_)
    {
    }

    MemPort localExt_;
    MemPort code_;
    SLProcessor processor_;
    SLProcessor::_ExternMemAccess access_;
  };

  Mem code_;
  MemPort codePort_;

  Mem localMem0_;
  Mem localMem1_;
  Mem localMem2_;
  Mem localMem3_;

  _Core *cores_[4];
};

void SLGroup::writeCode(uint32_t addr,uint16_t data)
{
  assert(addr < codeMemSize_-1);
  codePort_.write(addr,data);
  update();
  codePort_.update();
}

uint32_t SLGroup::readData(uint32_t addr)
{
  assert(addr < (4*codeMemSize_-3));

  uint32_t procId=(addr/processorMemSize_)^1;

  cores_[procId]->access_.addr_=addr;
  cores_[procId]->access_.data_=0;
  cores_[procId]->access_.rw_=0;
  cores_[procId]->access_.en_=1;

  update();

  cores_[procId]->access_.en_=0;

  return cores_[procId]->access_.data_;
}

void SLGroup::writeData(uint32_t addr,uint32_t data)
{
  assert(addr < (4*codeMemSize_-3));

  uint32_t procId=(addr/processorMemSize_)^1;

  cores_[procId]->access_.addr_=addr;
  cores_[procId]->access_.data_=data;
  cores_[procId]->access_.rw_=1;
  cores_[procId]->access_.en_=1;

  update();

  cores_[procId]->access_.en_=0;
}

uint32_t SLGroup::getState()
{
  uint32_t state=0;
  for(int32_t i=3;i>=0;--i)
    state=(state<<1)+cores_[i]->processor_.isRunning();

  return state;
}

void SLGroup::run(uint32_t cpuId)
{

}

void SLGroup::signal(uint32_t flags)
{

}

#endif /* SLPGROUP_H_ */
