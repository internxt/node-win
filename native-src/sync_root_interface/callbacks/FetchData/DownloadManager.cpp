#include "DownloadManager.h"

DownloadManager& DownloadManager::getInstance() {
    static DownloadManager instance;
    return instance;
}


DownloadManager::DownloadManager() = default;
DownloadManager::~DownloadManager() = default;


std::shared_ptr<DownloadContext> DownloadManager::CreateDownloadContext(napi_env env, napi_value js_callback) {
    auto context = std::make_shared<DownloadContext>();
    CreateFetchDataTsfn(env, js_callback, context);

    std::lock_guard<std::mutex> lock(mutex_);
    downloadContexts_.emplace(context->connectionKey.Internal, context);
    return context;
}


std::shared_ptr<DownloadContext> DownloadManager::FindDownloadContext(const DownloadContext::CF_CONNECTION_KEY& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = downloadContexts_.find(key.Internal);
    if (it != downloadContexts_.end()) {
        return it->second;
    }
    return nullptr;
}

void DownloadManager::RemoveDownloadContext(const DownloadContext::CF_CONNECTION_KEY& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    downloadContexts_.erase(key.Internal);
}

void DownloadManager::CreateFetchDataTsfn(napi_env env, napi_value js_callback, std::shared_ptr<DownloadContext> context) {
    napi_value resource_name;
    napi_create_string_utf8(env, "FetchDataTsfn", NAPI_AUTO_LENGTH, &resource_name);

    napi_status status = napi_create_threadsafe_function(
        env,
        js_callback,
        nullptr,
        resource_name,
        0,
        1,
        nullptr,
        nullptr,
        context.get(),
        FetchDataTsfnCallback,
        &context->fetchDataTsfn
    );

    if (status != napi_ok) {
        Logger.getInstance().log("Failed to create threadsafe function", LogLevel::ERROR);
    }
}

void DownloadManager::FetchDataTsfnCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    auto* downloadContext = static_cast<DownloadContext*>(context);
    auto* args = static_cast<FetchDataArgs*>(data);

    napi_value undefined;
    napi_get_undefined(env, &undefined);

    napi_value callbackArgs[2];
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(args->fileIdentityArg.c_str()), args->fileIdentityArg.length(), &callbackArgs[0]);
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_fetch_data, downloadContext, &callbackArgs[1]);

    napi_call_function(env, undefined, js_callback, 2, callbackArgs, nullptr);

    delete args;
}
