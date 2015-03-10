/*
 * Error.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */

#include "Error.h"
#include <assert.h>
#include <iostream>

Error Error::instance_;

Error& Error::expect(bool expr)
{
  if(instance_.newLinePending_)
    std::cout<<"\n";

  instance_.newLinePending_=false;
  instance_.isFault_=!expr;
  return instance_;
}

Error& Error::operator<<(const char *str)
{
  if(!isFault_)
    return *this;

  printHeader();
  std::cout<<"  "<<str;
  return *this;
}

Error& Error::operator<<(const Stream::String &str)
{
  if(!isFault_)
    return *this;

  printHeader();

  std::cout<<"  ";
  for(uint32_t i=0;i<str.getLength();++i)
    std::cout<<str[i];

  return *this;
}

Error& Error::operator<<(uint32_t value)
{
  if(!isFault_)
    return *this;

  printHeader();
  std::cout<<"  "<<value;
  return *this;
}

Error& Error::operator<<(const Stream &stream)
{
  line_=stream.getCurrentLine();
  linePrinted_=false;
  return *this;
}

Error& Error::operator<<(Type type)
{
  if(type == FATAL)
    assert(isFault_ == false);

  return *this;
}

uint32_t Error::getNumErrors()
{
  return instance_.errors_;
}

Error::Error()
{
  isFault_=false;
  linePrinted_=false;
  newLinePending_=false;
  line_=0;
  errors_=0;
}

void Error::printHeader()
{
  if(!linePrinted_)
    std::cout<<"at "<<(line_)<<":\n";

  linePrinted_=true;
  newLinePending_=true;
}

