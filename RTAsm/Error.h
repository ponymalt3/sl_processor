/*
 * Error.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */
#ifndef ERROR_H_
#define ERROR_H_

#include <stdint.h>
#include "Stream.h"

class Error
{
public:
  enum Type {FATAL};

  static Error& expect(bool expr);

  Error& operator<<(const char *str);
  Error& operator<<(const Stream::String &str);
  Error& operator<<(uint32_t value);
  Error& operator<<(const Stream &stream);
  Error& operator<<(Type type);

  static uint32_t getNumErrors();// { return instance_.errors_; }

protected:
  Error();
  void printHeader();

  static Error instance_;

  bool isFault_;
  bool linePrinted_;
  bool newLinePending_;
  uint32_t line_;
  uint32_t errors_;
};

#endif /* ERROR_H_ */
