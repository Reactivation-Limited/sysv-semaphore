%module OSX

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include <napi.h>
#include <errnoname.c>
#include "src/semaphore.h"
#include "src/flock.h"
#include "src/mode.h"
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

%include "src/mode.h"
%include "src/semaphore.h"
%nodefaultctor Flock;
%include "src/flock.h"
