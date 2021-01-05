/*
 * BuddyAlloc.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */
 
 #pragma once

#ifndef ALLOC_H_
#define ALLOC_H_

#include <stdint.h>

class Allocator
{
public:
  Allocator(uint32_t size)
  {
    numBlocks_=(size+7)/8;
    blocks_=new uint8_t[numBlocks_];
    for(uint32_t i=0;i<numBlocks_;++i)
    {
      blocks_[i]=0x00;
    }
    
    firstFreeBlock_=0;
    lastUsedBlock_=0;
  }
  
  ~Allocator()
  {
    delete [] blocks_;
  }
  
  uint32_t allocate(uint32_t size,bool highestAddr=false,bool alignBlock=false)
  {
    if(size == 0)
    {
      return 0xFFFFFFFF;
    }
    
    uint32_t startBlock=firstFreeBlock_;
    
    if(highestAddr)
    {
      startBlock=lastUsedBlock_;
    }
    
    if(alignBlock)
    {
      while(startBlock < numBlocks_ && blocks_[startBlock] != 0x00) ++startBlock;
    }
    
    uint32_t addr=allocateFrom(startBlock,size);
    
    if(addr/8 == firstFreeBlock_)
    {
      firstFreeBlock_=+size/8;
      while(firstFreeBlock_ < numBlocks_ && blocks_[firstFreeBlock_] == 0xFF) ++firstFreeBlock_;
    }
    
    uint32_t lastAllocBlock=(addr+size-1)/8;
    if(lastAllocBlock > lastUsedBlock_)
    {
      lastUsedBlock_=lastAllocBlock;
    }
    
    return addr;
  }
  
  void release(uint32_t addr,uint32_t size)
  {
    if(addr/8 >= numBlocks_ || ((blocks_[addr/8]>>(addr&7))&1) == 0)
    {
      return;
    }
    
    uint32_t sizeInFirstBlock=min(8-(addr&7),size);
    uint32_t mask=(1<<sizeInFirstBlock)-1;
    
    blocks_[addr/8]&=~(mask<<(addr&7));
    
    size-=sizeInFirstBlock;
    uint32_t fullBlocks=size/8;
    for(uint32_t i=1;i<=fullBlocks;++i)
    {
      blocks_[addr/8+i]=0x00;
    }
    
    size-=fullBlocks*8;
    
    if(size)
    {
      blocks_[addr/8+fullBlocks+1]&=~((1<<size)-1);
    }    
    
    if(addr/8 < firstFreeBlock_)
    {
      firstFreeBlock_=addr/8;
    }
    
    while(lastUsedBlock_ > 0 && blocks_[lastUsedBlock_] == 0x00) --lastUsedBlock_;
  }
  
protected:
  uint32_t allocateFrom(uint32_t firstBlock,uint32_t size)
  {    
    for(uint32_t i=firstBlock;i<numBlocks_;)
    {
      if(blocks_[i] == 0xFF)
      {
        ++i;
        continue;
      }
      
      uint32_t pos=log2(blocks_[i]);
      uint32_t remainingSize=size-min(8-pos,size);
      uint32_t fullBlocks=remainingSize/8;
      
      uint32_t j=i+1;
      while(j<min(numBlocks_,i+1+fullBlocks) && blocks_[j] == 0x00) ++j;
      
      if(j < (i+1+fullBlocks))
      {
        //not enough free blocks
        i=j;
        continue;
      }
      
      remainingSize-=8*fullBlocks;
      
      if(remainingSize != 0 && (j >= numBlocks_ || blocks_[j]&((1<<remainingSize)-1))) 
      {
        //not enough free space
        i=j;
        continue;
      }
      
      blocks_[i]|=min(0xFF,(1<<min(size,8))-1)<<pos;
      
      //mark data as used
      for(uint32_t k=i+1;k<j;++k)
      {
        blocks_[k]=0xFF;
      }
      
      if(remainingSize)
      {
        blocks_[j]|=(1<<remainingSize)-1;
      }
      
      return i*8+pos;
    }
    
    return 0xFFFFFFFF;
  }
  
  uint32_t log2(uint8_t value)
  {
    uint32_t result=0;
    if(value >= 0x10) result+=4,value>>=4;
    if(value >= 0x04) result+=2,value>>=2;
    if(value >= 0x02) result+=1,value>>=1;
    return result+(value&1);
  }
  
  uint32_t min(uint32_t a,uint32_t b)
  {
    return a<b?a:b;
  }
  
  uint8_t *blocks_;
  uint32_t numBlocks_;
  uint32_t firstFreeBlock_;
  uint32_t lastUsedBlock_;
};

#endif /* ALLOC_H_ */
