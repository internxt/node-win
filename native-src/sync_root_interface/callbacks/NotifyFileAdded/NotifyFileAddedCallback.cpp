#include "Callbacks.h"
#include "DirectoryWatcher.h"
#include "Logger.h"
#include <codecvt>
inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;
inline std::wstring server_identity;
#include <filesystem>
#include <Placeholders.h>

struct FetchDataArgs
{
    std::wstring fileIdentityArg;
};

napi_value response_callback_fn_added(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
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

    bool confirmation_response;
    napi_get_value_bool(env, argv[0], &confirmation_response);

    // Verificar el segundo argumento: debería ser un string
    napi_typeof(env, argv[1], &valueType);
    if (valueType != napi_string)
    {
        wprintf(L"Second argument should be string\n");
        return nullptr;
    }

    size_t response_len;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &response_len);

    std::wstring response_wstr(response_len, L'\0');

    napi_get_value_string_utf16(env, argv[1], (char16_t *)response_wstr.data(), response_len + 1, &response_len);

    // wprintf(L"input path: %s .\n", response_wstr.c_str());

    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    callbackResult = confirmation_response;
    server_identity = response_wstr.c_str();

    cv.notify_one();

    return nullptr;
}

void notify_file_added_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    napi_status status;
    FetchDataArgs *args = static_cast<FetchDataArgs *>(data);
    napi_value js_string_path, js_response_callback_fn, undefined, result;

    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_string_path);

    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_added, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_string_path, js_response_callback_fn};

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
        Logger::getInstance().log("Failed to call JS function.", LogLevel::ERROR);
        return;
    }
    delete args;
}

// TODO: move to utils
std::string wstringToString(const std::wstring &wstr)
{
    // Configurar la conversión a UTF-8
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    // Utilizar la conversión
    return converter.to_bytes(wstr);
}

void register_threadsafe_notify_file_added_callback(FileChange &change, const std::string &resource_name, napi_env env, InputSyncCallbacksThreadsafe input)
{
    std::wstring *dataToSend = new std::wstring(change.type == NEW_FILE ? change.path : (change.path + L"\\"));
    napi_status status = napi_call_threadsafe_function(input.notify_file_added_threadsafe_callback, dataToSend, napi_tsfn_blocking);

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }
    HANDLE placeholder;

    if (!std::filesystem::exists(change.path))
    {
        Logger::getInstance().log("File does not exist", LogLevel::ERROR);
        return;
    };

    try
    {

        if (change.type == NEW_FOLDER)
        {
            placeholder = CreateFileW(
                change.path.c_str(),
                FILE_LIST_DIRECTORY | WRITE_DAC,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                nullptr);
        }
        else if (change.type == NEW_FILE)
        { //||  change.type == MODIFIED_FILE) {
            placeholder = CreateFileW(
                change.path.c_str(),
                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
        }

        // winrt::handle placeholder(CreateFileW(change.path.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        const std::wstring idStr = server_identity;
        LPCVOID idStrLPCVOID = static_cast<LPCVOID>(idStr.c_str());
        DWORD idStrByteLength = static_cast<DWORD>(idStr.size() * sizeof(wchar_t));

        if (callbackResult)
        {
            try
            {
                Logger::getInstance().log("Convert to placeholder in sync" + Logger::fromWStringToString(change.path), LogLevel::INFO);
                Sleep(100);
                HRESULT hr = CfConvertToPlaceholder(placeholder, idStrLPCVOID, idStrByteLength, CF_CONVERT_FLAG_MARK_IN_SYNC, nullptr, nullptr);
                // show error
                if (FAILED(hr) || hr != S_OK)
                {
                    Logger::getInstance().log("Error converting to placeholder, ConvertToPlaceholder failed,", LogLevel::ERROR);
                }

                if (change.type == NEW_FILE) {
                    Placeholders::UpdatePinState(change.path.c_str(), PinState::AlwaysLocal);
                };
                CloseHandle(placeholder);
            }
            catch (...)
            {
                Logger::getInstance().log("Error converting to placeholder, CloseHandle failed.", LogLevel::ERROR);
            }
        }
    }
    catch (...)
    {
        Logger::getInstance().log("Error converting to placeholder", LogLevel::ERROR);
    }

    // if (!callbackResult) {
    //     wprintf(L"not in sync\n");
    //     winrt::StorageProviderItemProperty prop;
    //     prop.Id(1);
    //     prop.Value(L"Value1");
    //     prop.IconResource(L"imageres.dll,-1402");

    //     std::filesystem::path fullPath(change.path.c_str());
    //     std::wstring directory = fullPath.parent_path().wstring();
    //     std::wstring filename = fullPath.filename().wstring();

    //     // this is adding the custom state over the existing one
    //     Utilities::ApplyCustomOverwriteStateToPlaceholderFile(directory.c_str(), filename.c_str(), prop);
    // }

    if (status != napi_ok)
    {
        Logger::getInstance().log("Unable to call notify_file_added_threadsafe_callback", LogLevel::ERROR);
        napi_throw_error(env, NULL, "Unable to call notify_file_added_threadsafe_callback");
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false; // Reset ready
    }
};