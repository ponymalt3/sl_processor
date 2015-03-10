/*
 * Register.h
 *
 *  Created on: Jan 11, 2015
 *      Author: malte
 */

#ifndef REGISTER_H_
#define REGISTER_H_

class Clock
{
public:
  enum {Rising,Falling,Both};
  enum {Phase0=0,Phase180,Phase360};

  static Clock* instance()
  {
    if(instance_ == 0)
      instance=new Clock();

    return instance_;
  }

  class Register
  {
  public:
    void update()
    {
      value_=nextValue_;
    }

    uint32_t read() const { return value_; }
    void write(uint32_t value) { nextValue_=value; }

  protected:
    Register *linkRising_;
    Register *linkFalling_;
    bool linkedRising_;
    bool linkedFalling_;
    uint32_t value_;
    uint32_t nextValue_;
  };

  void onRisingEdge(Register *reg)
  {
    if(reg->linkedRising_)
          return;

    Register *prev=risingEdgeList_;
    risingEdgeList_=reg;
    reg->linkRising_=prev;
    reg->linkedRising_=true;
  }

  void onFallingEdge(Register *reg)
  {
    if(reg->linkedFalling_)
      return;

    Register *prev=fallingEdgeList_;
    fallingEdgeList_=reg;
    reg->linkFalling_=prev;
    reg->linkedFalling_=true;
  }

  void step(uint32_t phaseInc=Phase360)
  {
    if(phaseInc == Phase0)
      return;

    if(phase_ == Phase180)
    {
      while(fallingEdgeList_ != 0)
      {
        fallingEdgeList_->update();
        fallingEdgeList_->linkedFalling_=false;
        fallingEdgeList_=fallingEdgeList_->linkRising_;
      }
    }

    if((phase_ == Phase180 && phaseInc > Phase180) || (phase_ == Phase360))
    {
      while(risingEdgeList_ != 0)
      {
        risingEdgeList_->update();
        risingEdgeList_->linkedRising_=false;
        risingEdgeList_=risingEdgeList_->linkRising_;
      }
    }

    if(phase_ == Phase360 && phaseInc == Phase360)
    {
      while(fallingEdgeList_ != 0)
      {
        fallingEdgeList_->update();
        fallingEdgeList_->linkedFalling_=false;
        fallingEdgeList_=fallingEdgeList_->linkRising_;
      }
    }

    phase_=(phase_+phaseInc)%Phase360;
  }

protected:
  Clock()
  {
    phase_=Phase0;
    risingEdgeList_=0;
    fallingEdgeList_=0;
  }

  static Clock* instance_;

  Register *risingEdgeList_;
  Register *fallingEdgeList_;
  uint32_t phase_;
};

static Clock* Clock::instance_=0;


template<const uint32_t bits,uint32_t _ClkMode>
class register_t : public Clock::Register
{
public:
  register_t& operator=(uint32_t value)
  {
    value&=(1<<bits)-1;

    if(_ClkMode == Clock::Rising || _ClkMode == Clock::Both)
    {
      write(value);
      Clock::instance()->onRisingEdge(this);
    }

    if(_ClkMode == Clock::Falling || _ClkMode == Clock::Both)
    {
      write(value);
      Clock::instance()->onFallingEdge(this);
    }

    return *this;
  }

  uint32_t operator()(uint32_t j,uint32_t i=32)
  {
    if(i == 32)
      i=j;

    return (read()>>i)&((1<<(i-j+1))-1);
  }
};


#endif /* REGISTER_H_ */
