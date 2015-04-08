/*
 * SLArithUnit.h
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#ifndef SLARITHUNIT_H_
#define SLARITHUNIT_H_

#include "SLProcessor.h"

class SLArithUnit
{
public:
  SLArithUnit();

  SLProcessor::_MUnit comb(const SLProcessor::_DecodeEx &decEx);
  void update(const SLProcessor::_DecodeEx &decEx,const SLProcessor::_MUnit &comb);

protected:
  struct _PendingOp
  {
    uint32_t result_;
    uint32_t cmd_ : 8;
  };

  _PendingOp pipeline_[32];
  _PendingOp macPipeline_[2];
  uint32_t curCycle_;

  uint32_t state_ : 4;
  _qfp32_t dataRegister_[2];

  struct _SumMode
  {
    uint32_t muxA_ : 1;//0 regular input; 1 internal reg
    uint32_t muxB_ : 1;
    uint32_t muxC_ : 1;//mux for mul into reg B
    uint32_t enRegA_ : 1;//enable int reg
    uint32_t enRegB_ : 1;
    uint32_t clearRegA_ : 1;
    uint32_t clearRegB_ : 1;
    uint32_t enAdd_ : 1;//enable addition
    uint32_t complete_ : 1;
    uint32_t ready_ : 1;
  };

  uint32_t sumModeState_;
  uint32_t activeOp_;
  uint32_t pendingOp_;
};





#endif /* SLARITHUNIT_H_ */
