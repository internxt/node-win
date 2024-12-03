#include "stdafx.h"
#include <node_api.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>

struct DownloadContext {
    CF_CONNECTION_KEY connectionKey;
    CF_TRANSFER_KEY transferKey;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER requiredLength;
    LARGE_INTEGER requiredOffset;
    CF_CALLBACK_INFO callbackInfo;
    std::wstring fullClientPath;

    napi_threadsafe_function fetchDataTsfn;

    size_t lastReadOffset = 0;

    DownloadContext() = default;

    ~DownloadContext() {
        if (fetchDataTsfn != nullptr) {
            napi_release_threadsafe_function(fetchDataTsfn, napi_tsfn_release);
        }
    }
};

struct FetchDataArgs {
    std::wstring fileIdentityArg;
};
