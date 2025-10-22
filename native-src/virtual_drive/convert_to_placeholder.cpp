#include <Windows.h>
#include "napi_extract_args.h"
#include "Placeholders.h"

napi_value convert_to_placeholder_impl(napi_env env, napi_callback_info info) {
    auto [path, serverIdentity] = napi_extract_args<std::wstring, std::wstring>(env, info);

    std::wstring wPath(path.begin(), path.end());
    std::wstring wServerIdentity(serverIdentity.begin(), serverIdentity.end());

    Placeholders::ConvertToPlaceholder(wPath, wServerIdentity);

    return nullptr;
}
