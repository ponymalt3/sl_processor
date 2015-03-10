/*
 * BuddyAlloc.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef BUDDYALLOC_H_
#define BUDDYALLOC_H_

#include <algorithm>

template<uint32_t NumLevel>
class BuddyAlloc
{
public:
  /*
   * 1    1w
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
  enum {BitMapSize=(NumLevel<=4)?5+(1<<(NumLevel-5)):1+NumLevel};

  BuddyAlloc(uint32_t base,uint32_t size);
  uint32_t allocate(uint32_t size);
  void release(uint32_t addr);

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
    uint32_t getBit(uint32_t pos) { return (map_[pos>>5]>>(pos&0x1F))&1; }
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
  Level level_[NumLevel+1];
  uint32_t map_[BitMapSize];
};

template<uint32_t NumLevel>
BuddyAlloc<NumLevel>::BuddyAlloc(uint32_t base,uint32_t size)
{
  base_=base;
  minBlockSize_=size/(1<<NumLevel);
  minBlockSizeLg2_=log2(minBlockSize_);

  uint32_t mapOffset=0;
  for(uint32_t i=0;i<NumLevel+1;++i)
  {
    level_[NumLevel-i].init(1<<i,map_+mapOffset);
    mapOffset+=((1<<i)+31)/32;
  }
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
    if(++curLevel > NumLevel)
      return -1;
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
void BuddyAlloc<NumLevel>::release(uint32_t addr)
{
  int32_t curLevel=NumLevel;
  while(curLevel >= 0)
  {
    uint32_t pos=addr>>(curLevel+minBlockSizeLg2_);

    if(level_[curLevel].getBit(pos) == 0)//used
    {
      //combine
      while(curLevel <= NumLevel && !level_[curLevel].releaseAndCombine(pos))
      {
        pos>>=1;
        ++curLevel;
      }
    }
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

    if((pos&(~1)) <= (freePos_+1))
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

  while(mapIndex < size_)
  {
    if(map_[mapIndex] != 0)
    {
      uint32_t lsb=map_[mapIndex]&(~(map_[mapIndex]-1));
      freePos_=log2(lsb);
      break;
    }
  }
}

template<uint32_t NumLevel>
uint32_t BuddyAlloc<NumLevel>::log2(uint32_t value)
{
  uint32_t result=0;

  result+=(value&0xFFFF0000)?16:0;
  result+=(value&0xFF00FF00)?8:0;
  result+=(value&0xF0F0F0F0)?4:0;
  result+=(value&0xCCCCCCCC)?2:0;
  result+=(value&0xAAAAAAAA)?1:0;

  return result;
}

#endif /* BUDDYALLOC_H_ */
