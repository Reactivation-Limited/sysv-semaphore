%module OSX

%{
#define NAPI_ENABLE_CPP_EXCEPTIONS
#include <napi.h>
#include <errnoname.c>
#include <string>
#include <system_error>
#include "semaphore-posix.h"
#include "semaphore-sysv.h"
#include "flock.h"
%}

%include exception.i
%exception {
  try {
    $action
   } catch(std::system_error& e) {
    const int code  = e.code().value();
    std::string what = e.what();
    std::string syscall = what.substr(0, what.find(':'));
    std::string message = errnoname(code);
    message += what.substr(what.find(':'));
    message += ", ";
    message += syscall;
    Napi::Error jsError = Napi::Error::New(info.Env(), message);
    jsError.Set("errno", Napi::Number::New(info.Env(), -code));
    jsError.Set("code", Napi::String::New(info.Env(), errnoname(code)));
    jsError.Set("syscall", Napi::String::New(info.Env(), syscall));
    jsError.ThrowAsJavaScriptException();
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
