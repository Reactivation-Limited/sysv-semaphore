%module main

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include "error.h"
#include "semaphore-sysv.h"

#include <napi.h>
#include <errnoname.c>
#include <string>
#include <system_error>
%}

%include exception.i
%exception {
  try {
    $action
  } catch(std::system_error &e) {
    throwJavaScriptError(e, info.Env());
    SWIG_fail;
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}

%include "token.h"
%include "semaphore-sysv.h"
