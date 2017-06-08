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
  class Exception : public std::runtime_error
  {
  public:
    friend class ErrorHandler;    
    uint32_t getNumErrors() const { return numErrors_; }
    
  protected:
    Exception(uint32_t numErrors) : std::runtime_error("fatal error")
    {
      numErrors_=numErrors;
    }
    
    uint32_t numErrors_;    
  };
  
  Error(ErrorHandler &handler);

  ErrorHandler& expect(bool expr);
  uint32_t getNumErrors();// { return instance_.errors_; }
  
  ErrorHandler& getErrorHandler() { return handler_; }

protected:
  ErrorHandler &handler_;
};

#endif /* ERROR_H_ */
