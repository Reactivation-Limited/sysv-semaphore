%module OSX

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include <napi.h>
#include <errnoname.c>
#include "semaphore.h"
#include "flock.h"
#include "mode.h"
%}

%include exception.i
%exception {
  try {
    $action
  } catch(const char *e) {
    // you could thow with NAPI?
    SWIG_exception(SWIG_SystemError, e);
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}

%include "semaphore.h"
%nodefaultctor Flock;
%include "flock.h"
%include "mode.h"
