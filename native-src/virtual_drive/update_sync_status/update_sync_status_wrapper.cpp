#include <windows.h>
#include "Placeholders.h"

napi_value update_sync_status_wrapper(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    std::unique_ptr<wchar_t[]> widePath(new wchar_t[pathLength + 1]);
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(widePath.get()), pathLength + 1, nullptr);

    bool inputSyncState, isDirectory;
    napi_get_value_bool(env, argv[1], &inputSyncState);
    napi_get_value_bool(env, argv[2], &isDirectory);

    Placeholders::UpdateSyncStatus(widePath.get(), inputSyncState, isDirectory);

    return nullptr;
}
