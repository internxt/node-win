#include <Callbacks.h>

struct NotifyDeleteArgs {
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

void NotifyDeleteCall(napi_env env, napi_value js_callback, void* context, void* data) {
  NotifyDeleteArgs* args = static_cast<NotifyDeleteArgs*>(data);

  std::u16string u16_targetPath(args->targetPathArg.begin(), args->targetPathArg.end());
  std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

  napi_value js_targetPathArg, js_fileIdentityArg;
  
  napi_create_string_utf16(env, u16_targetPath.c_str(), u16_targetPath.size(), &js_targetPathArg);
  napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);

  napi_value args_to_js_callback[2];
  args_to_js_callback[0] = js_targetPathArg;
  args_to_js_callback[1] = js_fileIdentityArg;

  napi_value undefined, result;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);

  delete args;
}