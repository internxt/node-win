#include <windows.h>
#include "napi_extract_args.h"
#include "SyncRoot.h"

napi_value dehydrate_file(napi_env env, napi_callback_info info) {
    auto [path] = napi_extract_args<1>(env, info);

    SyncRoot::DehydrateFile(path.c_str());

    return nullptr;
}
