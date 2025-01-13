#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <node_api.h>
#include "stdafx.h"
#include "Logger.h"

void response_callback_fn_fetch_data(napi_env env, napi_callback_info info);

struct FetchDataArgs {
    std::wstring fileIdentityArg;
};

struct DownloadContext {
    struct CF_CONNECTION_KEY {
        UINT64 Internal;
    } connectionKey;
    napi_threadsafe_function fetchDataTsfn;
};

class DownloadManager {
public:
    static DownloadManager& getInstance();

    std::shared_ptr<DownloadContext> CreateDownloadContext(napi_env env, napi_value js_callback);

    std::shared_ptr<DownloadContext> FindDownloadContext(const DownloadContext::CF_CONNECTION_KEY& key);

    void RemoveDownloadContext(const DownloadContext::CF_CONNECTION_KEY& key);

private:
    DownloadManager();
    ~DownloadManager();
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

    std::unordered_map<UINT64, std::shared_ptr<DownloadContext>> downloadContexts_;
    std::mutex mutex_;

    void CreateFetchDataTsfn(napi_env env, napi_value js_callback, std::shared_ptr<DownloadContext> context);

    static void FetchDataTsfnCallback(napi_env env, napi_value js_callback, void* context, void* data);
};

#endif 
