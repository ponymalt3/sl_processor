/*
 * SLArithUnit.h
 *
 *  Created on: Mar 26, 2015
 *      Author: malte
 */

#ifndef SLARITHUNIT_H_
#define SLARITHUNIT_H_

#include "SLProcessorStructs.h"

class SLArithUnit
{
public:
  SLArithUnit();

  void reset();

  _MUnit comb(const _DecodeEx &decEx);
  void update(const _DecodeEx &decEx,const _MUnit &comb,uint32_t en);

protected:
  struct _PendingOp
  {
    uint32_t result_;
    uint32_t cmd_ : 8;
  };

  _PendingOp pipeline_[32];
  uint32_t curCycle_;

  uint32_t activeOp_;
  uint32_t pendingOp_;
};

#endif /* SLARITHUNIT_H_ */
