#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <FileCopierWithProgress.h>
#include <fstream>
#include <vector>
#include <utility> // para std::pai
#include <cfapi.h>
#include <iostream>
#include <chrono>

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

inline int load_data_count = 0;
inline size_t lastIncrementalReadSize = 0;

struct FetchDataArgs
{
    std::wstring fileIdentityArg;
};

void load_data() {
    printf("load_data called");
}

void setup_global_tsfn_fetch_data(napi_threadsafe_function tsfn)
{
    wprintf(L"setup_global_tsfn_fetch_data called\n");
    g_fetch_data_threadsafe_callback = tsfn;
}

std::string WStringToString(const std::wstring &wstr) {
    if (wstr.empty())
        return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

size_t file_incremental_reading(const std::string& filename, size_t& previousSize) {
    std::ifstream file;

    // Abre el archivo
    file.open(filename, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return previousSize;  // Retorna el previousSize sin cambios
    }

    file.clear(); // Limpia los flags de error/end-of-file
    file.seekg(0, std::ios::end); // Va al final del archivo
    size_t newSize = file.tellg();

    size_t growth = newSize - previousSize;

    if (growth > 0) { // Si el archivo ha crecido
        std::vector<char> buffer(growth);
        file.seekg(previousSize);
        file.read(buffer.data(), growth);
        std::cout.write(buffer.data(), growth);
    } else if (newSize < previousSize) {
        // Si el archivo se ha truncado o reiniciado, esto podría ser una consideración para manejar.
        std::cerr << "El archivo parece haber sido truncado o reiniciado." << std::endl;
    }

    file.close();
    previousSize = newSize; // Actualiza el previousSize para reflejar el nuevo tamaño
    return previousSize;    // Retorna el nuevo previousSize
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

    // Verificar el segundo argumento: debería ser una cadena
    napi_typeof(env, argv[1], &valueType);
    if (valueType != napi_string)
    {
        wprintf(L"Second argument should be string\n");
        return nullptr;
    }

    //std::lock_guard<std::mutex> lock(mtx);
    //ready = true;
    callbackResult = response;
    wprintf(L"response_callback_fn_fetch_data called\n");

    size_t response_len;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &response_len);

    std::wstring response_wstr(response_len, L'\0');

    napi_get_value_string_utf16(env, argv[1], (char16_t *)response_wstr.data(), response_len + 1, &response_len);

    wprintf(L"input path: %s .\n", response_wstr.c_str());

    fullServerFilePath = response_wstr;

    lastIncrementalReadSize = file_incremental_reading(WStringToString(fullServerFilePath), lastIncrementalReadSize);

    {
        std::lock_guard<std::mutex> lock(mtx);
        
        // Incrementa el contador de ejecuciones
        load_data_count++;

        // Si load_data() ha sido ejecutado 10 veces
        if (load_data_count >= 10) {
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

    std::wstring fullClientPath(callbackInfo->VolumeDosName);
    fullClientPath.append(callbackInfo->NormalizedPath);
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

    // std::lock_guard<std::mutex> lock(mtx);
    lastIncrementalReadSize = 0;
    ready = false; // Reset ready
}