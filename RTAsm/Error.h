/*
 * Error.h
 *
 *  Created on: Mar 7, 2015
 *      Author: malte
 */
#ifndef ERROR_H_
#define ERROR_H_

#include <stdint.h>
#include <stdexcept>

class ErrorHandler;

class Error
{
public:
  Error(ErrorHandler &handler);

  ErrorHandler& expect(bool expr);
  uint32_t getNumErrors();// { return instance_.errors_; }
  
  ErrorHandler& getErrorHandler() { return handler_; }

protected:
  ErrorHandler &handler_;
};

#endif /* ERROR_H_ */
