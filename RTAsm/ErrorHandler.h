#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

#include <stdint.h>
#include "Stream.h"

class ErrorHandler
{
public:
  friend class Error;
  
  enum Type {FATAL};
  
  ErrorHandler();
  
  ErrorHandler& operator<<(const char *str);
  ErrorHandler& operator<<(char c);
  ErrorHandler& operator<<(const Stream::String &str);
  ErrorHandler& operator<<(uint32_t value);
  ErrorHandler& operator<<(const Stream &stream);
  ErrorHandler& operator<<(Type type);
  
protected:
  void printHeader();

  bool isFault_;
  bool linePrinted_;
  bool newLinePending_;
  uint32_t line_;
  uint32_t errors_;
};

#endif