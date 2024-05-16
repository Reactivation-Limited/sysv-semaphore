#include <napi.h>
#include <system_error>

void throwJavaScriptException(std::system_error &e, Napi::Env env);
