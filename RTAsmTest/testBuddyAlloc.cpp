#include <mtest.h>
#include "RTAsm/BuddyAlloc.h"

#include <list>

class testBuddyAlloc : public mtest::test
{
public:
  testBuddyAlloc():testee_(0,512)
  {
    guard0_=0xFFFFFFFF;
    guard1_=0xFFFFFFFF;
  }
  
protected:
  uint32_t guard0_;
  BuddyAlloc<9> testee_;
  uint32_t guard1_;
};

MTEST(testBuddyAlloc,test_that_alloc_smallest_block_works)
{
  uint32_t a=testee_.allocate(1);
  
  EXPECT(a == 0);
}

MTEST(testBuddyAlloc,test_that_multiple_allocs_works)
{
  uint32_t a=testee_.allocate(1);
  uint32_t b=testee_.allocate(17);
  uint32_t c=testee_.allocate(3);
  uint32_t d=testee_.allocate(41);
  uint32_t e=testee_.allocate(8); 
  uint32_t f=testee_.allocate(7);  
  
  EXPECT(a == 0);
  EXPECT(b == 32);
  EXPECT(c == 4);
  EXPECT(d == 64);
  EXPECT(e == 8);
  EXPECT(f == 16);
}

MTEST(testBuddyAlloc,test_that_alloc_fails_if_block_is_too_big)
{
  uint32_t a=testee_.allocate(1);
  uint32_t b=testee_.allocate(257); 
  
  EXPECT(a == 0);
  EXPECT(b == -1);
}

MTEST(testBuddyAlloc,test_that_free_blocks_will_be_collapsed)
{
  uint32_t a=testee_.allocate(1);
  uint32_t b=testee_.allocate(17);
  uint32_t c=testee_.allocate(3);
  uint32_t d=testee_.allocate(41);
  uint32_t e=testee_.allocate(8); 
  uint32_t f=testee_.allocate(7);

  testee_.release(a,1);
  testee_.release(e,8);
  testee_.release(c,3);
  
  //now a 16 size block should be available at 0
  uint32_t g=testee_.allocate(16);
  
  EXPECT(g == 0);
}

MTEST(testBuddyAlloc,test_that_free_blocks_will_be_collapsed_correctly_with_descending_sizes)
{
  uint32_t a=testee_.allocate(11);
  uint32_t b=testee_.allocate(9);
  uint32_t c=testee_.allocate(1);

  testee_.release(a,11);//16
  testee_.release(b,9);//16
  testee_.release(c,1);//1
  
  EXPECT(testee_.allocate(256) == 0);
  EXPECT(testee_.allocate(256) == 256);
}

MTEST(testBuddyAlloc,test_that_free_blocks_collapse_works_correctly_with_smallest_size)
{
  uint32_t a=testee_.allocate(1);
  uint32_t b=testee_.allocate(0);
  
  testee_.release(a,1);
  testee_.release(b,0);
  
  EXPECT(testee_.allocate(256) == 0);
  EXPECT(testee_.allocate(256) == 256);
}

MTEST(testBuddyAlloc,test_that_free_blocks_will_be_collapsed_with_lots_of_allocs)
{
  mtest::random rnd;
  
  struct _Alloc
  {
    uint32_t addr_;
    uint32_t size_;
  };
   
  std::list<_Alloc> blocks;
  
  for(uint32_t i=0;i<32;++i)
  {
    uint32_t size=rnd.getUint32()%16;
    uint32_t addr=testee_.allocate(size);
    
    blocks.push_back({addr,size});
  }
  
  for(auto &i : blocks)
  {
    testee_.release(i.addr_,i.size_);
  }
  
  EXPECT(testee_.allocate(256) == 0);
  EXPECT(testee_.allocate(256) == 256);
}

MTEST(testBuddyAlloc,test_that_alloc_too_big_block_dont_crash)
{
  uint32_t a=testee_.allocate(1000);
  EXPECT(a == -1);
}

MTEST(testBuddyAlloc,test_that_release_too_big_block_dont_crash)
{
  testee_.release(0,1000);
}

