%module OSX

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include "error.h"
#include "semaphore-posix.h"
#include "semaphore-sysv.h"
#include "flock.h"

#include <napi.h>
#include <errnoname.c>
#include <string>
#include <system_error>
%}

%include exception.i
%exception {
  try {
    $action
   } catch(std::system_error e) {
    throwJavaScriptException(e, info.Env());
  } catch(const char *e) {
    // you could thow with NAPI?
    SWIG_exception(SWIG_SystemError, e);
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}

%include "semaphore-posix.h"
%include "semaphore-sysv.h"
%nodefaultctor Flock;
%include "flock.h"
