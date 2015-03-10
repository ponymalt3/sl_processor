/*
 * SymbolMap.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#ifndef SYMBOLMAP_H_
#define SYMBOLMAP_H_

#include <stdint.h>
#include "Stream.h"

class SymbolMap
{
public:
  enum {InvalidLink=0xFFFF};
  struct _Symbol
  {
    _Symbol();
    _Symbol(const Stream::String &str);

    void updateLastAccess(uint32_t codeAddr) { lastAccess_=codeAddr; }
    void changeArraySize(uint32_t size);

    uint16_t strOffset_;
    uint16_t strLength_ : 8;
    uint16_t flagAllocated_ : 1;
    uint16_t flagConst_ : 1;
    uint16_t flagUseAddrAsArraySize_ : 1;
    uint16_t link_;
    union
    {
      struct
      {
        uint16_t allocatedAddr_;
        uint16_t lastAccess_;
      };
      qfp32 constValue_;
    };
  };

  SymbolMap(Stream &stream);

  uint32_t findSymbol(const Stream::String &str);
  uint32_t createSymbol(const Stream::String &str,uint32_t size=0);
  uint32_t findOrCreateSymbol(const Stream::String &str,uint32_t size=0);
  uint32_t createSymbolNoToken(uint32_t size);
  uint32_t createConst(const Stream::String &str,qfp32 value);
  uint32_t createReference(const Stream::String &str,uint32_t irsOffset);

  _Symbol& operator[](uint32_t i);

protected:
  uint32_t insertSymbol(const _Symbol &sym,uint32_t hashIndex=0,uint32_t size=0);
  uint32_t findSymbol(const Stream::String &str,uint32_t hashIndex);

  _Symbol symbols_[200];
  uint16_t hashTable_[256];
  uint16_t symCount_;
  Stream &stream_;
};

#endif /* SYMBOLMAP_H_ */