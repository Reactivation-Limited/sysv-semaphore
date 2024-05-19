#include <napi.h>
#include <system_error>

void throwJavaScriptError(std::system_error &e, Napi::Env env);
