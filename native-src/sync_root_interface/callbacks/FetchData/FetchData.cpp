#include "stdafx.h"
#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <FileCopierWithProgress.h>
#include <DownloadMutexManager.h>
#include <fstream>
#include <vector>
#include <utility> // para std::pai
#include <cfapi.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include "Utilities.h"
#include <locale>
#include <codecvt>
#include <filesystem>
#include <Placeholders.h>
#include <Logger.h>

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::mutex mtx_download;
inline std::condition_variable cv;
inline std::condition_variable cv_download;
inline bool ready = false;
inline bool ready_download = false;
inline bool callbackResult = false;
inline std::wstring fullServerFilePath;

inline size_t lastSize;

#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define CF_SIZE_OF_OP_PARAM(field)                  \
    (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
     FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

#define CHUNK_SIZE (4096 * 1024)

#define CHUNKDELAYMS 250

CF_CONNECTION_KEY connectionKey;
CF_TRANSFER_KEY transferKey;
LARGE_INTEGER fileSize;
LARGE_INTEGER requiredLength;
LARGE_INTEGER requiredOffset;
CF_CALLBACK_INFO g_callback_info;
std::wstring g_full_client_path;

inline bool load_finished = false;
inline size_t lastReadOffset = 0;
struct FetchDataArgs
{
    std::wstring fileIdentityArg;
};

void load_data()
{
    printf("load_data called");
}

void setup_global_tsfn_fetch_data(napi_threadsafe_function tsfn)
{
    g_fetch_data_threadsafe_callback = tsfn;
}

napi_value create_response(napi_env env, bool finished, int progress) {
    napi_value result_object;
    napi_create_object(env, &result_object);

    napi_value finished_value;
    napi_get_boolean(env, finished, &finished_value);
    napi_set_named_property(env, result_object, "finished", finished_value);

    napi_value progress_value;
    napi_create_int32(env, progress, &progress_value);
    napi_set_named_property(env, result_object, "progress", progress_value);

    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);

    napi_resolve_deferred(env, deferred, result_object);

    return promise;
}

std::string WStringToString(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();

    // int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    // std::string strTo(sizeNeeded, 0);
    // WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
    std::wstring filename = wstr;
    std::string utf8_filename = converter.to_bytes(filename);
    return utf8_filename;
}

size_t file_incremental_reading(napi_env env, const std::string &filename, size_t &dataSizeRead, bool final_step, float &progress, napi_value error_callback = nullptr)
{
    std::ifstream file;

    // Abre el archivo
    printf("filename: %s\n", filename.c_str());
    file.open(filename, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        Logger::getInstance().log("Error al abrir el archivo.", LogLevel::ERROR);
        return dataSizeRead; // Retorna el dataSizeRead sin cambios
    }

    file.clear();                 // Limpia los flags de error/end-of-file
    file.seekg(0, std::ios::end); // Va al final del archivo
    size_t newSize = file.tellg();

    size_t datasizeAvailableUnread = newSize - dataSizeRead;

    size_t growth = newSize - lastSize;

    try
    {

        if ((datasizeAvailableUnread > 0)) // && CHUNK_SIZE < datasizeAvailableUnread && !final_step) || (datasizeAvailableUnread > 0 && final_step)
        {                                  // Si el archivo ha crecido
            std::vector<char> buffer(CHUNK_SIZE);
            file.seekg(dataSizeRead);
            file.read(buffer.data(), CHUNK_SIZE);

            LARGE_INTEGER startingOffset, length;

            startingOffset.QuadPart = dataSizeRead; 

            LARGE_INTEGER chunkBufferSize;
            chunkBufferSize.QuadPart = min(datasizeAvailableUnread, CHUNK_SIZE);

            HRESULT hr = FileCopierWithProgress::TransferData(
                connectionKey,
                transferKey,
                buffer.data(),
                startingOffset,
                chunkBufferSize,
                STATUS_SUCCESS);

            dataSizeRead += chunkBufferSize.QuadPart;

            if (FAILED(hr))
            {
                wprintf(L"Error in FileCopierWithProgress::TransferData(), HRESULT: %lx\n", hr);
                load_finished = true;

                HRESULT hr = FileCopierWithProgress::TransferData(
                    connectionKey,
                    transferKey,
                    NULL,
                    requiredOffset,
                    requiredLength,
                    STATUS_UNSUCCESSFUL);
            }
            else
            {
                UINT64 totalSize = static_cast<UINT64>(fileSize.QuadPart);
                progress = static_cast<float>(dataSizeRead) / static_cast<float>(totalSize);
                Utilities::ApplyTransferStateToFile(g_full_client_path.c_str(), g_callback_info, totalSize, dataSizeRead);
                Sleep(CHUNKDELAYMS);
            }
        }
    }
    catch (...)
    {
        Logger::getInstance().log("Error al leer el archivo.", LogLevel::ERROR);
        HRESULT hr = FileCopierWithProgress::TransferData(
            connectionKey,
            transferKey,
            NULL,
            requiredOffset,
            requiredLength,
            STATUS_UNSUCCESSFUL);
    }

    file.close();
    lastSize = newSize;
    return dataSizeRead; 
}


napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        Logger::getInstance().log("This function must receive at least two arguments", LogLevel::ERROR);
        return create_response(env, false, 0);
    }

    napi_valuetype valueType;

    // Verificar el primer argumento: debería ser un booleano
    napi_typeof(env, argv[0], &valueType);
    if (valueType != napi_boolean)
    {
        Logger::getInstance().log("First argument should be boolean", LogLevel::ERROR);
        return create_response(env, false, 0);
    }

    bool response;
    napi_get_value_bool(env, argv[0], &response);

    napi_typeof(env, argv[1], &valueType);
    if (valueType != napi_string)
    {
        Logger::getInstance().log("Second argument should be string", LogLevel::ERROR);
        return create_response(env, false, 0);
    }

    callbackResult = response;

    size_t response_len;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &response_len);

    std::wstring response_wstr(response_len, L'\0');

    napi_get_value_string_utf16(env, argv[1], (char16_t *)response_wstr.data(), response_len + 1, &response_len);

    if (argc == 3)
    {
        napi_valuetype callbackType;
        napi_typeof(env, argv[2], &callbackType);
        if (callbackType != napi_function)
        {
            Logger::getInstance().log("Third argument should be a function", LogLevel::ERROR);
            return create_response(env, false, 0);
        }
    }

    fullServerFilePath = response_wstr;

    float progress;
    lastReadOffset = file_incremental_reading(env, WStringToString(fullServerFilePath), lastReadOffset, false, progress, argc == 3 ? argv[2] : nullptr);

    std::wstring file_path = fullServerFilePath.c_str();
    std::ifstream file(file_path, std::ios::binary);

    if (!file)
    {
        Logger::getInstance().log("This file couldn't be opened in realtime.", LogLevel::ERROR);
        return create_response(env, false, 0);
    }


    file.seekg(0, std::ios::end);
    LONG total_size = file.tellg();
    file.seekg(0, std::ios::beg);


    file.close();


    if (lastReadOffset == fileSize.QuadPart)
    {
        Logger::getInstance().log("File has been fully read.", LogLevel::DEBUG);
        lastReadOffset = 0;
        load_finished = true;
        Utilities::ApplyTransferStateToFile(g_full_client_path.c_str(), g_callback_info, fileSize.QuadPart, fileSize.QuadPart);
        Sleep(CHUNKDELAYMS);
        Placeholders::UpdatePinState(g_full_client_path.c_str(), PinState::AlwaysLocal);
    };

    {
        std::lock_guard<std::mutex> lock(mtx);

        if (load_finished)
        {
            ready = true;
            cv.notify_one();
        }
    }

    napi_value resultBool;
    napi_get_boolean(env, load_finished, &resultBool);

    napi_value progress_value;
    napi_create_double(env, progress, &progress_value);

    Logger::getInstance().log("fetch data result: " + std::to_string(load_finished), LogLevel::DEBUG);

    // napi_value result_object;
    // napi_create_object(env, &result_object);

    // napi_set_named_property(env, result_object, "finished", resultBool);
    // napi_set_named_property(env, result_object, "progress", progress_value);

    // if (load_finished)
    // {
    //     load_finished = false;
    // };

    // napi_value promise;
    // napi_deferred deferred;
    // napi_create_promise(env, &deferred, &promise);

    // napi_resolve_deferred(env, deferred, result_object);

    return create_response(env, load_finished, progress);
}

void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    napi_status status;
    FetchDataArgs *args = static_cast<FetchDataArgs *>(data);
    napi_value js_fileIdentityArg, undefined, result;

    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);

    napi_value js_response_callback_fn;
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_fetch_data, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_fileIdentityArg, js_response_callback_fn};

    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to get undefined value.\n", LogLevel::ERROR);
        return;
    }

    // {
    //     std::lock_guard<std::mutex> lock(mtx);
    //     ready = false;
    // }

    std::unique_lock<std::mutex> lock(mtx);
    ready = false;

    status = napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to call JS function.", LogLevel::ERROR);
        return;
    }

    delete args;
}

void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function tsfn_fetch_data;
    napi_value fetch_data_value;
    napi_status status_ref = napi_get_reference_value(env, input.fetch_data_callback_ref, &fetch_data_value);

    napi_status status = napi_create_threadsafe_function(
        env,
        fetch_data_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_fetch_data_call,
        &tsfn_fetch_data);

    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to create threadsafe function.\n", LogLevel::ERROR);
        return;
    }
    setup_global_tsfn_fetch_data(tsfn_fetch_data);
}

void CALLBACK fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    connectionKey = callbackInfo->ConnectionKey;
    transferKey = callbackInfo->TransferKey;
    fileSize = callbackInfo->FileSize;
    requiredLength = callbackParameters->FetchData.RequiredLength;
    requiredOffset = callbackParameters->FetchData.RequiredFileOffset;
    g_callback_info = *callbackInfo;

    std::wstring fullClientPath(callbackInfo->VolumeDosName);
    fullClientPath.append(callbackInfo->NormalizedPath);

    g_full_client_path = fullClientPath;

    Logger::getInstance().log("Full download path: " + Logger::fromWStringToString(fullClientPath), LogLevel::INFO);

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    FetchDataArgs *args = new FetchDataArgs();
    args->fileIdentityArg = fileIdentityStr;
    wprintf(L"Callback fetch_data_callback_wrapper called\n");
    wprintf(L"g_fetch_data_threadsafe_callback = %s\n", g_fetch_data_threadsafe_callback);
    if (g_fetch_data_threadsafe_callback == nullptr)
    {
        wprintf(L"Callback fetch_data_callback_wrapper called but g_fetch_data_threadsafe_callback is null\n");
        return;
    }
    napi_status status = napi_call_threadsafe_function(g_fetch_data_threadsafe_callback, args, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        Logger::getInstance().log("Callback called unsuccessfully.\n", LogLevel::ERROR);
    };

    {
        std::unique_lock<std::mutex> lock(mtx);
        Logger::getInstance().log("Mutex await call.\n", LogLevel::DEBUG);
        while (!ready)
        {
            cv.wait(lock);
        }
    }

    Logger::getInstance().log("Hydration Completed\n", LogLevel::INFO);

    // DownloadMutexManager& mutexManager = DownloadMutexManager::getInstance();
    // mutexManager.setReady(true);

    // std::lock_guard<std::mutex> lock(mtx);
    lastReadOffset = 0;
    ready = false; // Reset ready
}