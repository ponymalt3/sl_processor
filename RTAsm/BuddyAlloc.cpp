#include "BuddyAlloc.h"
#include <algorithm>

BuddyAlloc::BuddyAlloc(uint32_t base,uint32_t size)
{
  base_=base;
  minBlockSize_=size/(1<<NumLevel);
  minBlockSizeLg2_=log2(minBlockSize_);

  uint32_t mapOffset=0;
  for(uint32_t i=1;i<NumLevel+1;++i)
  {
    level_[NumLevel-i].init(1<<i,map_+mapOffset);
    mapOffset+=((1<<i)+31)/32;
  }
  
  topBlockFree_=true;
}

uint32_t BuddyAlloc::allocate(uint32_t size)
{
  if(size > (1<<(NumLevel-1+minBlockSizeLg2_)))
  {
    return -1;
  }
    
  if(size < minBlockSize_)
    size=minBlockSize_;

  uint32_t index=log2(size)-minBlockSizeLg2_;

  uint32_t curLevel=index;
  while(!level_[curLevel].avail())
  {
    if(++curLevel >= NumLevel)
    {
      if(topBlockFree_ == false)
      {
        return -1;
      }
      
      //set available blocks
      level_[NumLevel-1].release(0);
      level_[NumLevel-1].release(1);
      curLevel=NumLevel-1;
      topBlockFree_=false;
      break;
    }
  }

  //split blocks
  while(index < curLevel)
  {
    uint32_t pos=level_[curLevel].allocate();
    level_[curLevel-1].release(2*pos);
    level_[curLevel-1].release(2*pos+1);
    --curLevel;
  }

  return level_[index].allocate()*(1<<(index+minBlockSizeLg2_));
}


void BuddyAlloc::release(uint32_t addr,uint32_t size)
{
  if(size > (1<<(NumLevel-1+minBlockSizeLg2_)))
  {
    return;
  }
  
  if(size < minBlockSize_)
    size=minBlockSize_;
    
  int32_t curLevel=log2(size);
  uint32_t pos=addr>>(curLevel+minBlockSizeLg2_);

  //combine
  while(curLevel < NumLevel && level_[curLevel].releaseAndCombine(pos))
  {
    pos>>=1;
    ++curLevel;
  }
  
  if(curLevel == NumLevel)
  {
    topBlockFree_=true;
  }
}

void BuddyAlloc::Level::init(uint32_t size,uint32_t *map)
{
  size_=size;
  map_=map;
  freePos_=size;//all reserved

  for(uint32_t i=0;i<(size+31)/32;++i)
    map_[i]=0;
}

uint32_t BuddyAlloc::Level::allocate()
{
  uint32_t pos=freePos_;
  toggleBit(pos);
  updateFreePos(pos);
  return pos;
}

bool BuddyAlloc::Level::releaseAndCombine(uint32_t pos)
{
  if(getBit(pos^1) == 1)//is free
  {
    clearBit(pos^1);

    if((pos&(~1)) <= freePos_)
      updateFreePos(pos&(~1));

    return true;
  }
  else
  {
    release(pos);
    return false;
  }
}

void BuddyAlloc::Level::release(uint32_t pos)
{
  toggleBit(pos);
  freePos_=std::min((uint32_t)freePos_,pos);
}

void BuddyAlloc::Level::updateFreePos(uint32_t startFromPos)
{
  uint32_t mapIndex=startFromPos>>5;

  while(mapIndex < ((size_+31)/32))
  {
    if(map_[mapIndex] != 0)
    {
      uint32_t lsb=map_[mapIndex]&(~(map_[mapIndex]-1));
      freePos_=mapIndex*32+log2(lsb);
      return;
    }
    
    ++mapIndex;
  }
  
  freePos_=size_;
}

uint32_t BuddyAlloc::log2(uint32_t value)
{
  uint32_t origValue=value;
  
  uint32_t result=0;
  
  if(value&0xFFFF0000)
  {
    result+=16;
    value>>=16;
  }
  
  if(value&0x0000FF00)
  {
    result+=8;
    value>>=8;
  }
  
  if(value&0x000000F0)
  {
    result+=4;
    value>>=4;
  }
  
  if(value&0x0000000C)
  {
    result+=2;
    value>>=2;
  }
  
  if(value&0x0000002)
  {
    result+=1;
    value>>=1;
  }
  
  return result + ((origValue != (1<<result))?1:0);//round up
}