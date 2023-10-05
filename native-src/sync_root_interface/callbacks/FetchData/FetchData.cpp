#include "stdafx.h"
#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <FileCopierWithProgress.h>
#include <fstream>
#include <vector>
#include <utility> // para std::pai
#include <cfapi.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include "Utilities.h"

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;
inline std::wstring fullServerFilePath;

#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define CF_SIZE_OF_OP_PARAM(field)                  \
    (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
     FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

#define CHUNK_SIZE (4096)

CF_CONNECTION_KEY connectionKey;
CF_TRANSFER_KEY transferKey;
LARGE_INTEGER fileSize;
LARGE_INTEGER requiredLength;
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
    wprintf(L"setup_global_tsfn_fetch_data called\n");
    g_fetch_data_threadsafe_callback = tsfn;
}

std::string WStringToString(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

size_t file_incremental_reading(const std::string &filename, size_t &dataSizeRead, bool final_step)
{
    printf("===================RESPONSE CALLBACK CALLED===================\n");
    std::ifstream file;

    // Abre el archivo
    file.open(filename, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return dataSizeRead; // Retorna el dataSizeRead sin cambios
    }

    file.clear();                 // Limpia los flags de error/end-of-file
    file.seekg(0, std::ios::end); // Va al final del archivo
    size_t newSize = file.tellg();

    size_t datasizeAvailableUnread = newSize - dataSizeRead;

    if ((datasizeAvailableUnread > 0 && CHUNK_SIZE < datasizeAvailableUnread && !final_step) || (datasizeAvailableUnread > 0 && final_step))
    { // Si el archivo ha crecido
        printf("============ENTER IN IF STATEMENT============\n");
        std::vector<char> buffer(CHUNK_SIZE);
        file.seekg(dataSizeRead);
        file.read(buffer.data(), CHUNK_SIZE);
        // std::cout.write(buffer.data(), datasizeAvailableUnread);

        LARGE_INTEGER startingOffset, length;

        startingOffset.QuadPart = dataSizeRead; // Desplazamiento desde el cual se leyeron los datos

        printf("connectionKey: %d\n", connectionKey);
        printf("transferKey: %d\n", transferKey);
        printf("startingOffset: %d\n", startingOffset.QuadPart);

        LARGE_INTEGER chunkBufferSize;
        chunkBufferSize.QuadPart = min(requiredLength.QuadPart, CHUNK_SIZE);

        FileCopierWithProgress::TransferData(
            connectionKey,
            transferKey,
            buffer.data(),
            startingOffset,
            chunkBufferSize,
            STATUS_SUCCESS);

        dataSizeRead += chunkBufferSize.QuadPart;
    }

    UINT64 totalSize = static_cast<UINT64>(fileSize.QuadPart);
    Utilities::ApplyTransferStateToFile(g_full_client_path.c_str(), g_callback_info, totalSize , dataSizeRead);

    file.close();
    return dataSizeRead; // Retorna el nuevo dataSizeRead
}

// response callback fn
napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        // Manejar error
        wprintf(L"This function must receive at least two arguments");
        return nullptr;
    }

    napi_valuetype valueType;

    // Verificar el primer argumento: debería ser un booleano
    napi_typeof(env, argv[0], &valueType);
    if (valueType != napi_boolean)
    {
        wprintf(L"First argument should be boolean\n");
        return nullptr;
    }

    bool response;
    napi_get_value_bool(env, argv[0], &response);

    // Verificar el segundo argumento: debería ser una cadenai
    napi_typeof(env, argv[1], &valueType);
    if (valueType != napi_string)
    {
        wprintf(L"Second argument should be string\n");
        return nullptr;
    }

    // std::lock_guard<std::mutex> lock(mtx);
    // ready = true;
    callbackResult = response;
    wprintf(L"response_callback_fn_fetch_data called\n");

    size_t response_len;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &response_len);

    std::wstring response_wstr(response_len, L'\0');

    napi_get_value_string_utf16(env, argv[1], (char16_t *)response_wstr.data(), response_len + 1, &response_len);

    wprintf(L"input path: %s .\n", response_wstr.c_str());

    fullServerFilePath = response_wstr;

    lastReadOffset = file_incremental_reading(WStringToString(fullServerFilePath), lastReadOffset, false);

    // size file in real time
    std::wstring file_path = fullServerFilePath.c_str();
    std::ifstream file(file_path, std::ios::binary);

    if (!file)
    {
        wprintf(L"[Error] No se pudo abrir el archivo en tiempo real.\n");
    }

    // Obtener el tamaño del archivo
    file.seekg(0, std::ios::end);
    LONG total_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Imprimir el tamaño del archivo
    wprintf(L"[Debug] El tamaño del archivo es: %lld bytes.\n", total_size);

    // Cerrar el archivo
    file.close();
    // end size file in real time

    // tamaño_archivo != fileSize.QuadPart
    if (total_size == fileSize.QuadPart)
    {
        wprintf(L"[Debug - total_size == fileSize.QuadPart] El tamaño del archivo es igual al fileSize.QuadPart.\n");
        while (true)
        {
            lastReadOffset = file_incremental_reading(WStringToString(fullServerFilePath), lastReadOffset, CHUNK_SIZE >= fileSize.QuadPart - lastReadOffset);
            if (lastReadOffset == fileSize.QuadPart)
            {
                printf("[Debug]File has been fully read.\n");
                load_finished = true;
                break;
            };
        }
    }

    if (lastReadOffset == fileSize.QuadPart)
    {
        printf("[Debug] File has been fully read.\n");
        load_finished = true;
    };

    {
        std::lock_guard<std::mutex> lock(mtx);

        // Si load_data() ha sido ejecutado 10 veces
        if (load_finished)
        {
            ready = true;
            cv.notify_one();
        }
    }

    return nullptr;
}

void HydrateFile(_In_ CONST CF_CALLBACK_INFO *lpCallbackInfo,
                 _In_ CONST CF_CALLBACK_PARAMETERS *lpCallbackParameters, const std::wstring &syncRootPath, const std::wstring &fakeServerFilePath)
{
    FileCopierWithProgress::CopyFromServerToClient(lpCallbackInfo, lpCallbackParameters, fakeServerFilePath.c_str());
}

void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    wprintf(L"notify_fetch_data_call called\n");
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
        fprintf(stderr, "Failed to get undefined value.\n");
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
        fprintf(stderr, "Failed to call JS function.\n");
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
        fprintf(stderr, "Failed to create threadsafe function.\n");
        return;
    }
    wprintf(L"Threadsafe function created.\n");
    setup_global_tsfn_fetch_data(tsfn_fetch_data);
}

void CALLBACK fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    wprintf(L"Callback fetch_data_callback_wrapper called\n");
    // get callbackinfo
    wprintf(L"fileId = %s\n", callbackInfo->FileIdentity);

    connectionKey = callbackInfo->ConnectionKey;
    transferKey = callbackInfo->TransferKey;
    fileSize = callbackInfo->FileSize;
    requiredLength = callbackParameters->FetchData.RequiredLength;
    g_callback_info = *callbackInfo;

    std::wstring fullClientPath(callbackInfo->VolumeDosName);
        fullClientPath.append(callbackInfo->NormalizedPath);

    g_full_client_path = fullClientPath;

    wprintf(L"Full path: %s\n", fullClientPath.c_str());

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    FetchDataArgs *args = new FetchDataArgs();
    args->fileIdentityArg = fileIdentityStr;
    wprintf(L"Callback fetch_data_callback_wrapper called\n");
    wprintf(L"g_fetch_data_threadsafe_callback = %s\n", g_fetch_data_threadsafe_callback);
    napi_status status = napi_call_threadsafe_function(g_fetch_data_threadsafe_callback, args, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        wprintf(L"Callback called unsuccessfully.\n");
    };

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }

    wprintf(L"FINISH\n");

    // std::lock_guard<std::mutex> lock(mtx);
    lastReadOffset = 0;
    ready = false; // Reset ready
}