/*
 * Error.cpp
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */
 
#include "Error.h"
#include "ErrorHandler.h"
#include <assert.h>
#include <iostream>

ErrorHandler::ErrorHandler()
{
  isFault_=false;
  linePrinted_=false;
  newLinePending_=false;
  line_=0xFFFFFFFF;
  errors_=0;
}

ErrorHandler& ErrorHandler::operator<<(const char *str)
{
  if(!isFault_)
    return *this;

  printHeader();
  std::cout<<str;
  return *this;
}

ErrorHandler& ErrorHandler::operator<<(char c)
{
  if(!isFault_)
    return *this;

  printHeader();

  std::cout<<"'"<<(c)<<"' (0x"<<std::hex<<(uint32_t(c)&0xFF)<<std::dec<<")";
  
  return *this;
}

ErrorHandler& ErrorHandler::operator<<(const Stream::String &str)
{
  if(!isFault_)
    return *this;

  printHeader();

  std::cout<<"'";
  for(uint32_t i=0;i<str.getLength();++i)
    std::cout<<str[i];

  std::cout<<"'";
  return *this;
}

ErrorHandler& ErrorHandler::operator<<(uint32_t value)
{
  if(!isFault_)
    return *this;

  printHeader();
  std::cout<<value;
  return *this;
}

ErrorHandler& ErrorHandler::operator<<(const Stream &stream)
{
  uint32_t line=stream.getCurrentLine();
  
  if(line_ != line)
  {
    linePrinted_=false;
  }
  
  line_=line;
  return *this;
}

ErrorHandler& ErrorHandler::operator<<(Type type)
{
  if(type == FATAL)
  {
    if(isFault_)
      throw std::runtime_error("fatal error");
  }

  return *this;
}

void ErrorHandler::printHeader()
{
  if(!linePrinted_)
    std::cout<<"at "<<(line_)<<":\n";

  linePrinted_=true;
  newLinePending_=true;
}



Error::Error(ErrorHandler &handler):handler_(handler)
{
}

ErrorHandler& Error::expect(bool expr)
{
  if(handler_.newLinePending_)
    std::cout<<"\n";

  handler_.newLinePending_=false;
  handler_.isFault_=!expr;
  
  if(handler_.isFault_)
  {
    ++(handler_.errors_);
  }
  
  return handler_;
}

ErrorHandler& Error::info()
{
  if(handler_.newLinePending_)
    std::cout<<"\n";

  handler_.newLinePending_=false;
  handler_.isFault_=true;//force output but dont increment error count
  handler_.linePrinted_=true;
  return handler_;
}

uint32_t Error::getNumErrors()
{
  return handler_.errors_;
}