#include <Windows.h>
#include "napi_extract_args.h"
#include "Placeholders.h"

napi_value convert_to_placeholder_impl(napi_env env, napi_callback_info info) {
    auto [path, placeholderId] = napi_extract_args<std::wstring, std::wstring>(env, info);

    Placeholders::ConvertToPlaceholder(path.c_str(), placeholderId.c_str());

    return nullptr;
}
