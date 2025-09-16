#include <Windows.h>
#include "napi_extract_args.h"
#include "Placeholders.h"

napi_value get_file_identity_impl(napi_env env, napi_callback_info info) {
    auto [path] = napi_extract_args<1>(env, info);

    std::string fileIdentity = Placeholders::GetFileIdentity(path);
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), '\0'), fileIdentity.end());
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), ' '), fileIdentity.end());

    napi_value jsFileIdentity;
    napi_create_string_utf8(env, fileIdentity.c_str(), fileIdentity.length(), &jsFileIdentity);

    return jsFileIdentity;
}
