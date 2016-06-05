/*
 * SymbolMap.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "SymbolMap.h"
#include "ErrorHandler.h"

SymbolMap::_Symbol::_Symbol()
{
  strOffset_=0;
  strLength_=0;
  flagAllocated_=0;
  flagConst_=0;
  flagIsArray_=0;
  flagStayAllocated_=0;
  allocatedSize_=0;
  allocatedAddr_=0;
  lastAccess_=0;
  link_=InvalidLink;
}

SymbolMap::_Symbol::_Symbol(const Stream::String &str)
{
  strOffset_=str.getOffset();
  strLength_=str.getLength();
  flagAllocated_=0;
  flagConst_=0;
  flagIsArray_=0;
  flagStayAllocated_=0;
  allocatedSize_=0;
  allocatedAddr_=0;
  lastAccess_=0;
  link_=InvalidLink;
}

void SymbolMap::_Symbol::changeArraySize(uint32_t size)
{
  flagIsArray_=size!=0;
  allocatedSize_=size==0?1:size;
}

SymbolMap::SymbolMap(Stream &stream):Error(stream.getErrorHandler()), stream_(stream)
{
  symCount_=0;

  for(uint32_t i=0;i<(sizeof(hashTable_)/sizeof(hashTable_[0]));++i)
    hashTable_[i]=InvalidLink;
}

uint32_t SymbolMap::findSymbol(const Stream::String &str)
{
  return findSymbol(str,str.hash());
}

uint32_t SymbolMap::createSymbol(const Stream::String &str,uint32_t size)
{
  uint32_t hash=str.hash();

  uint32_t i=findSymbol(str,hash);
  Error::expect(i == InvalidLink) << stream_ << "symbol " << str << " already defined at ";//<< Error::LineNumber(stream_,symbols_[i].strOffset_);

  return insertSymbol(_Symbol(str),hash,size);
}

uint32_t SymbolMap::findOrCreateSymbol(const Stream::String &str,uint32_t size)
{
  uint32_t hash=str.hash();

  uint32_t i=findSymbol(str,hash);

  if(i != InvalidLink)
    return i;

  return insertSymbol(_Symbol(str),hash,size);
}

uint32_t SymbolMap::createSymbolNoToken(uint32_t size)
{
  return insertSymbol(_Symbol(),0,size);
}

uint32_t SymbolMap::createConst(const Stream::String &str,qfp32 value)
{
  uint32_t hash=str.hash();

  uint32_t i=findSymbol(str,hash);
  Error::expect(i == InvalidLink) << stream_ << "const "<< str << " already defined";

  _Symbol newSym=_Symbol(str);
  newSym.constValue_=value;
  newSym.flagConst_=1;

  return insertSymbol(newSym,hash);
}

uint32_t SymbolMap::createReference(const Stream::String &str,uint32_t irsOffset)
{
  uint32_t hash=str.hash();

  uint32_t i=findSymbol(str,hash);
  Error::expect(i == InvalidLink) << stream_ << "const " << str << " already defined at ";// << Error::LineNumber(stream_,symbols_[i].strOffset_);

  _Symbol newSym=_Symbol(str);
   newSym.flagAllocated_=1;
   newSym.allocatedAddr_=irsOffset;

   return insertSymbol(newSym,hash);
}

SymbolMap::_Symbol& SymbolMap::operator[](uint32_t i)
{
  Error::expect(i < symCount_) << stream_ << "symbol reference out of range" << (i) << ErrorHandler::FATAL;
  return symbols_[i];
}

uint32_t SymbolMap::insertSymbol(const _Symbol &sym,uint32_t hashIndex,uint32_t size)
{
  Error::expect(symCount_ < (sizeof(symbols_)/sizeof(symbols_[0]))) << stream_ << "no more symbol storage available" << ErrorHandler::FATAL;

  _Symbol &newSym=symbols_[symCount_];

  newSym=sym;

  hashIndex&=0xFF;
  newSym.link_=hashTable_[hashIndex];
  hashTable_[hashIndex]=symCount_;

  if(size > 0)
  {
    //is array
    newSym.flagIsArray_=1;
    newSym.allocatedSize_=size;
  }
  else
  {
    //single element
    newSym.allocatedSize_=1;
  }

  return symCount_++;
}

uint32_t SymbolMap::findSymbol(const Stream::String &str,uint32_t hashIndex)
{
  uint32_t cur=hashTable_[hashIndex&0xFF];
  while(cur != InvalidLink)
  {
    if(stream_.createStringFromToken(symbols_[cur].strOffset_,symbols_[cur].strLength_) == str)
      return cur;

    cur=symbols_[cur].link_;
  }

  return InvalidLink;
}

