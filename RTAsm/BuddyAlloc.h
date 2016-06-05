/*
 * BuddyAlloc.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef BUDDYALLOC_H_
#define BUDDYALLOC_H_

#include <algorithm>
#include <iostream>

template<uint32_t NumLevel>
class BuddyAlloc
{
public:
  /*
   * 2    1w
   * 4    1w
   * 8    1w
   * 16   1w
   * 32   1w
   * 64   2w
   * 128  4w
   * 256  8w
   * 512  16w
   */
  enum {BitMapSize=(NumLevel>5)?3+(1<<(NumLevel-4)):NumLevel};

  BuddyAlloc(uint32_t base,uint32_t size);
  uint32_t allocate(uint32_t size);
  void release(uint32_t addr,uint32_t size);

protected:
  class Level
  {
  public:
    void init(uint32_t size,uint32_t *map);

    bool avail() const { return freePos_ < size_; }

    uint32_t allocate();

    bool releaseAndCombine(uint32_t pos);
    void release(uint32_t pos);

    void updateFreePos(uint32_t startFromPos);

    void clearBit(uint32_t pos) { map_[pos>>5]&=~(1<<(pos&0x1F)); }
    uint32_t getBit(uint32_t pos) const { return (map_[pos>>5]>>(pos&0x1F))&1; }
    void toggleBit(uint32_t pos) { map_[pos>>5]^=1<<(pos&0x1F); }
    
  protected:
    uint16_t freePos_;
    uint16_t size_;
    uint32_t *map_;
  };

  static uint32_t log2(uint32_t value);
  
  uint32_t base_;
  uint32_t minBlockSize_;
  uint32_t minBlockSizeLg2_;
  Level level_[NumLevel];
  uint32_t map_[BitMapSize];
  bool topBlockFree_;
};

template<uint32_t NumLevel>
BuddyAlloc<NumLevel>::BuddyAlloc(uint32_t base,uint32_t size)
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

template<uint32_t NumLevel>
uint32_t BuddyAlloc<NumLevel>::allocate(uint32_t size)
{
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


template<uint32_t NumLevel>
void BuddyAlloc<NumLevel>::release(uint32_t addr,uint32_t size)
{
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

template<uint32_t NumLevel>
void BuddyAlloc<NumLevel>::Level::init(uint32_t size,uint32_t *map)
{
  size_=size;
  map_=map;
  freePos_=size;//all reserved

  for(uint32_t i=0;i<(size+31)/32;++i)
    map_[i]=0;
}

template<uint32_t NumLevel>
uint32_t BuddyAlloc<NumLevel>::Level::allocate()
{
  uint32_t pos=freePos_;
  toggleBit(pos);
  updateFreePos(pos);
  return pos;
}

template<uint32_t NumLevel>
bool BuddyAlloc<NumLevel>::Level::releaseAndCombine(uint32_t pos)
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

template<uint32_t NumLevel>
void BuddyAlloc<NumLevel>::Level::release(uint32_t pos)
{
  toggleBit(pos);
  freePos_=std::min((uint32_t)freePos_,pos);
}

template<uint32_t NumLevel>
void BuddyAlloc<NumLevel>::Level::updateFreePos(uint32_t startFromPos)
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

template<uint32_t NumLevel>
uint32_t BuddyAlloc<NumLevel>::log2(uint32_t value)
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

#endif /* BUDDYALLOC_H_ */
