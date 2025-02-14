#include "stdafx.h"
#include <Callbacks.h>
#include <Logger.h>
#include <Placeholders.h>
#include <string>
#include <filesystem>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include "RenameTransferContext.h"

napi_threadsafe_function g_notify_rename_tsfn = nullptr;
napi_value response_callback_fn(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1)
        return nullptr;
    bool response;
    napi_get_value_bool(env, argv[0], &response);

    void* data;
    napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data);
    RenameContext* renameCtx = static_cast<RenameContext*>(data);
    {
        Logger::getInstance().log("response_callback_fn: Setting callback result.", LogLevel::DEBUG, FOREGROUND_BLUE);
        std::lock_guard<std::mutex> lock(renameCtx->mtx);
        renameCtx->ready = true;
        renameCtx->callbackResult = response;
    }
    renameCtx->cv.notify_one();
    return nullptr;
}

void notify_rename_call(napi_env env, napi_value js_callback, void* context, void* data)
{
    RenameContext* renameCtx = static_cast<RenameContext*>(data);
    napi_value js_targetPathArg, js_fileIdentityArg, undefined, result;
    std::u16string u16_targetPath(renameCtx->targetPath.begin(), renameCtx->targetPath.end());
    std::u16string u16_fileIdentity(renameCtx->fileIdentity.begin(), renameCtx->fileIdentity.end());

    napi_status status = napi_create_string_utf16(env, u16_targetPath.c_str(), u16_targetPath.size(), &js_targetPathArg);
    if (status != napi_ok)
        return;
    status = napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);
    if (status != napi_ok)
        return;
    napi_value js_response_callback_fn;
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn, renameCtx, &js_response_callback_fn);

    napi_value args_to_js_callback[3] = { js_targetPathArg, js_fileIdentityArg, js_response_callback_fn };

    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok)
        return;
    status = napi_call_function(env, undefined, js_callback, 3, args_to_js_callback, &result);
    if (status != napi_ok)
        Logger::getInstance().log("Failed to call JS function in notifyRenameCallback.", LogLevel::ERROR);
}

void register_threadsafe_notify_rename_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name(resource_name.begin(), resource_name.end());
    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_value notify_rename_value;
    napi_status status_ref = napi_get_reference_value(env, input.notify_rename_callback_ref, &notify_rename_value);
    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notify_rename_value, &valuetype);
    if (type_status != napi_ok || valuetype != napi_function)
    {
        fprintf(stderr, "notify_rename_value is not a function.\n");
        abort();
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_rename_value,
        nullptr,
        resource_name_value,
        0,
        1,
        nullptr,
        nullptr,
        nullptr,
        notify_rename_call,
        &g_notify_rename_tsfn);
    if (status != napi_ok)
    {
        const napi_extended_error_info *errorInfo = nullptr;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        abort();
    }
}

void CALLBACK notify_rename_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;
    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));
    PCWSTR targetPathArg = callbackParameters->Rename.TargetPath;

    auto renameCtx = GetOrCreateRenameContext(callbackInfo->ConnectionKey, callbackInfo->TransferKey);
    renameCtx->targetPath = std::wstring(targetPathArg);
    renameCtx->fileIdentity = fileIdentityStr;

    napi_status status = napi_call_threadsafe_function(g_notify_rename_tsfn, renameCtx.get(), napi_tsfn_blocking);
    if (status != napi_ok)
        wprintf(L"Callback called unsuccessfully.\n");

    CF_OPERATION_PARAMETERS opParams = {0};
    {
        std::unique_lock<std::mutex> lock(renameCtx->mtx);
        while (!renameCtx->ready)
            renameCtx->cv.wait(lock);
    }
    opParams.AckRename.CompletionStatus = renameCtx->callbackResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_RENAME;
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(&opInfo, &opParams);
     if (FAILED(hr))
        wprintf(L"Error in CfExecute() rename action, HRESULT: %lx\n", hr);
    {
        std::lock_guard<std::mutex> lock(renameCtx->mtx);
        renameCtx->ready = false;
    }
    printf("Mark item as async: %ls\n", targetPathArg);
    WCHAR systemPath[MAX_PATH];
    if (!GetWindowsDirectoryW(systemPath, sizeof(systemPath) / sizeof(WCHAR)))
    {
        wprintf(L"Error al obtener el directorio de Windows: %d\n", GetLastError());
        return;
    }
    std::wstring driveLetter(systemPath, 2);
    std::wstring absolutePath = targetPathArg;
    if (PathFileExistsW(absolutePath.c_str()) == FALSE)
        absolutePath = driveLetter + L"\\" + targetPathArg;
    bool isDirectory = std::filesystem::is_directory(absolutePath);
   
    RemoveRenameContext(callbackInfo->TransferKey);
}