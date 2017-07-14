/*
 * BuddyAlloc.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */
 
 #pragma once

#ifndef BUDDYALLOC_H_
#define BUDDYALLOC_H_

#include <stdint.h>

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
  enum {NumLevel=9,BitMapSize=(NumLevel>5)?3+(1<<(NumLevel-4)):NumLevel};

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

#endif /* BUDDYALLOC_H_ */
