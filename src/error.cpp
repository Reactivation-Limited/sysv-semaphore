#include "error.h"

#include <errnoname.h>
#include <string>

void throwJavaScriptException(std::system_error &e, Napi::Env env) {
  const int code = e.code().value();
  std::string what = e.what();
  std::string syscall = what.substr(0, what.find(':'));
  std::string message = errnoname(code);
  message += what.substr(what.find(':'));
  message += ", ";
  message += syscall;
  Napi::Error jsError = Napi::Error::New(env, message);
  jsError.Set("errno", Napi::Number::New(env, code));
  jsError.Set("code", Napi::String::New(env, errnoname(code)));
  jsError.Set("syscall", Napi::String::New(env, syscall));
  jsError.ThrowAsJavaScriptException();
}
