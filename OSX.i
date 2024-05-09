%module OSX

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include "semaphore.h"
#include "src/flock.h"
#include "node_modules/node-addon-api/napi.h"
#include <errnoname/errnoname.c>
%}

%include exception.i
%exception {
  try {
    $action
  } catch(const char *e) {
    SWIG_exception(SWIG_SystemError, e);
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}

%nodefaultctor Flock;
%include "src/flock.h"
