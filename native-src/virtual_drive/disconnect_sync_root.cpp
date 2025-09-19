#include <windows.h>
#include "napi_extract_args.h"
#include "SyncRoot.h"

napi_value disconnect_sync_root(napi_env env, napi_callback_info info) {
    auto [syncRootPath] = napi_extract_args<1>(env, info);

    HRESULT result = SyncRoot::DisconnectSyncRoot(syncRootPath.c_str());

    return nullptr;
}
