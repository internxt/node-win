#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <FileCopierWithProgress.h>
#include <fstream>
#include <vector>
#include <utility> // para std::pai

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;
inline std::wstring fullServerFilePath;

#define CF_SIZE_OF_OP_PARAM(field)                  \
    (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
     FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

struct FetchDataArgs
{
    std::wstring fileIdentityArg;
};

std::pair<std::vector<char>, size_t> readFileToBuffer(const std::wstring &filePath)
{
    // Abre el archivo en modo binario y mueve el puntero al final
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        // Manejar el error
        throw std::runtime_error("Could not open file");
    }

    // Obtén el tamaño del archivo
    std::streamsize size = file.tellg();

    // Mueve el puntero al inicio del archivo
    file.seekg(0, std::ios::beg);

    // Crea un vector para almacenar el contenido del archivo
    std::vector<char> buffer(size);

    // Lee el archivo al buffer
    if (!file.read(buffer.data(), size))
    {
        // Manejar el error
        throw std::runtime_error("Could not read file");
    }

    return {buffer, static_cast<size_t>(size)};
}

bool createFileFromBuffer(const std::string &filePath, const std::vector<char> &buffer)
{
    std::ofstream outputFile(filePath, std::ios::binary);
    if (!outputFile)
    {
        std::cerr << "No se pudo crear el archivo." << std::endl;
        return false;
    }

    // Escribir el contenido del búfer en el archivo
    outputFile.write(buffer.data(), buffer.size());

    if (!outputFile)
    {
        std::cerr << "Error al escribir en el archivo." << std::endl;
        return false;
    }

    outputFile.close();
    std::cout << "Archivo creado exitosamente." << std::endl;
    return true;
}

void setup_global_tsfn_fetch_data(napi_threadsafe_function tsfn)
{
    wprintf(L"setup_global_tsfn_fetch_data called\n");
    g_fetch_data_threadsafe_callback = tsfn;
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

    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    callbackResult = response;
    wprintf(L"response_callback_fn_fetch_data called\n");

    size_t response_len;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &response_len);

    std::wstring response_wstr(response_len, L'\0');

    napi_get_value_string_utf16(env, argv[1], (char16_t *)response_wstr.data(), response_len + 1, &response_len);

    wprintf(L"input path: %s .\n", response_wstr);

    fullServerFilePath = response_wstr;

    cv.notify_one();

    return nullptr;
}

void HydrateFile(_In_ CONST CF_CALLBACK_INFO *lpCallbackInfo,
                 _In_ CONST CF_CALLBACK_PARAMETERS *lpCallbackParameters, const std::wstring &syncRootPath, const std::wstring &fakeServerFilePath)
{
    // FileCopierWithProgress::CopyFromServerToClient(lpCallbackInfo, lpCallbackParameters, L"C:\\Users\\User\\Desktop\\fakeserver");
    wprintf(L"HydrateFile called\n");
    wprintf(L"fullServerPath: %s .\n", fakeServerFilePath.c_str());
    wprintf(L"fullClientPath: %s .\n", syncRootPath.c_str());

    // leer archivo
    auto [buffer, size] = readFileToBuffer(fakeServerFilePath); // path fake server file
    wprintf(L"size: %d .\n", size);
    wprintf(L"buffer: %s .\n", buffer.data());

    // crear archivo en escritorio desde el buffer
    createFileFromBuffer("C:\\Users\\User\\Desktop\\nuevoarchivo.txt", buffer);

    CF_OPERATION_INFO opInfo = {0};
    CF_OPERATION_PARAMETERS opParams = {0};

    opInfo.StructSize = sizeof(opInfo);
    opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
    opInfo.ConnectionKey = lpCallbackInfo->ConnectionKey;
    opInfo.TransferKey = lpCallbackInfo->TransferKey;
    opInfo.CorrelationVector = lpCallbackInfo->CorrelationVector;
    opInfo.RequestKey = lpCallbackInfo->RequestKey;
    // opInfo.SyncStatus =

    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
    opParams.TransferData.Flags = CF_OPERATION_TRANSFER_DATA_FLAG_NONE;
    opParams.TransferData.CompletionStatus = STATUS_SUCCESS;
    opParams.TransferData.Buffer = (LPCVOID)buffer.data();
    // LARGE_INTEGER largeIntoffset;
    // largeIntoffset.QuadPart = static_cast<LONGLONG>(0);
    // opParams.TransferData.Offset = largeIntoffset; // lpCallbackParameters->FetchData.RequiredFileOffset;

    LARGE_INTEGER largeInt;
    largeInt.QuadPart = static_cast<LONGLONG>(size);
    opParams.TransferData.Length = largeInt;
    // opParams.TransferData.Length = (LARGE_INTEGER)sizeof(buffer);
    // imprimir opinfo y opparams
    wprintf(L" detils opInfo and opParams: %s - %s - %s - %s - %s .\n", opInfo.StructSize);

    HRESULT hr = CfExecute(&opInfo, &opParams);

    if (FAILED(hr))
    {
        wprintf(L"Failed to execute CF_OPERATION_TYPE_TRANSFER_DATA\n");
        // error details
        wprintf(L"hr: %d .\n", hr);
        return;
    }
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

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false;
    }

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

    if (callbackResult)
    {
        HydrateFile(callbackInfo, callbackParameters, fullClientPath, fullServerFilePath);
    }
    else
    {
        wprintf(L"File %s has been dehydrated.\n", fileIdentityStr.c_str());
    }

    std::lock_guard<std::mutex> lock(mtx);
    ready = false; // Reset ready
}
