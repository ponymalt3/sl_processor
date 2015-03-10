/*
 * RTAsm.h
 *
 *  Created on: Jan 21, 2015
 *      Author: malte
 */

#ifndef RTASM_H_
#define RTASM_H_

#include <assert.h>
#include <stdint.h>

uint32_t min(uint32_t a,uint32_t b)
{
  return a<b?a:b;
}


uint32_t log2(uint32_t value)
{
  uint32_t result=0;

  result+=(value&0xFFFF0000)?16:0;
  result+=(value&0xFF00FF00)?8:0;
  result+=(value&0xF0F0F0F0)?4:0;
  result+=(value&0xCCCCCCCC)?2:0;
  result+=(value&0xAAAAAAAA)?1:0;

  return result;
}

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

template<uint32_t NumLevel>
class BuddyAlloc
{
public:
  enum {BitMapSize=(NumLevel<=4)?5+(1<<(NumLevel-5)):1+NumLevel};

  BuddyAlloc(uint32_t base,uint32_t size)
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

  uint32_t allocate(uint32_t size)
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

  void release(uint32_t addr)
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

protected:
  class Level
  {
  public:
    void init(uint32_t size,uint32_t *map)
    {
      size_=size;
      map_=map;
      freePos_=size;//all reserved

      for(uint32_t i=0;i<(size+31)/32;++i)
        map_[i]=0;
    }

    bool avail() const { return freePos_ < size_; }

    uint32_t allocate()
    {
      uint32_t pos=freePos_;
      toggleBit(pos);
      updateFreePos(pos);
      return pos;
    }

    bool releaseAndCombine(uint32_t pos)
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

    void release(uint32_t pos)
    {
      toggleBit(pos);
      freePos_=min(freePos_,pos);
    }

    void updateFreePos(uint32_t startFromPos)
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

    void clearBit(uint32_t pos)
    {
      map_[pos>>5]&=~(1<<(pos&0x1F));
    }

    uint32_t getBit(uint32_t pos)
    {
      return (map_[pos>>5]>>(pos&0x1F))&1;
    }

    void toggleBit(uint32_t pos)
    {
      map_[pos>>5]^=1<<(pos&0x1F);
    }

  protected:
    uint16_t freePos_;
    uint16_t size_;
    uint32_t *map_;
  };

  uint32_t base_;
  uint32_t minBlockSize_;
  uint32_t minBlockSizeLg2_;
  Level level_[NumLevel+1];
  uint32_t map_[BitMapSize];
};

/*
template<uint32_t NumBlocks>
class BuddyAllocSingleBlock8 : public BuddyAlloc<NumBlocks>
{
public:
  BuddyAllocSingleBlock8(uint32_t base,uint32_t size)
    : BuddyAlloc(base,size)
  {
    assert(size/NumBlocks == 8);

    for(uint32_t i=0;i<NumBlocks;++i)
      bitBlocks_[i]=0;

    activeBlock_=NumBlocks;
  }

  uint32_t allocate(uint32_t size)
  {
    if(activeBlock_ < NumBlocks)

    uint32_t neededBlocks=(size+blocksSize_-1)/blocksSize_;
    for(uint32_t i=0;i<NumBlocks;++i)
    {
      if(blocks_[i].free_ && blocks_[i].skipBlocks_ >= neededBlocks)
      {
        while(blocks_[i].skipBlocks_ > neededBlocks)
        {
          uint32_t skipBlocks=blocks_[i].skipBlocks_;

          //split blocks
          blocks_[i].skipBlocks_=skipBlocks/2;
          blocks_[i+skipBlocks/2].skipBlocks_=skipBlocks/2;
          blocks_[i+skipBlocks/2].free_=1;
        }

        blocks_[i].free_=0;
        return base_+(i*blocksSize_);
      }
    }

    return -1;
  }

  void release(uint32_t addr)
  {
    uint32_t byteAddr=(addr-base_)/8;
    uint32_t bitAddr=(addr-base_)%8;

    if((bitBlocks_[byteAddr]>>bitAddr)&1)
    {
      //clear bit
      bitBlocks_[byteAddr]&=~(1<<bitAddr);
      if(bitBlocks_[byteAddr] == 0)
        BuddyAlloc<NumBlocks>::release(base_+(byteAddr));
    }
    else
      BuddyAlloc<NumBlocks>::release(addr);
  }

protected:
  uint8_t bitBlocks_[NumBlocks];
  uint32_t activeBlock_;
};
*/


#endif /* RTASM_H_ */
