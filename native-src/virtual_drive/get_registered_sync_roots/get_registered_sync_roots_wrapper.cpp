#include <Windows.h>
#include <locale>
#include <codecvt>
#include "SyncRoot.h"

std::string WStringToUTF8(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

napi_value get_registered_sync_roots_wrapper(napi_env env, napi_callback_info args) {
    std::vector<SyncRoots> roots = SyncRoot::GetRegisteredSyncRoots();

    napi_value jsArray;
    napi_status status = napi_create_array_with_length(env, roots.size(), &jsArray);
    if (status != napi_ok)
        throw std::runtime_error("Error creating the array");

    for (size_t i = 0; i < roots.size(); i++) {
        napi_value jsObj;
        status = napi_create_object(env, &jsObj);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the object");

        std::string id = WStringToUTF8(roots[i].id);
        napi_value napiId;
        status = napi_create_string_utf8(env, id.c_str(), id.size(), &napiId);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the string id");
        napi_set_named_property(env, jsObj, "id", napiId);

        std::string path = WStringToUTF8(roots[i].path);
        napi_value napiPath;
        status = napi_create_string_utf8(env, path.c_str(), path.size(), &napiPath);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the string path");
        napi_set_named_property(env, jsObj, "path", napiPath);

        std::string displayName = WStringToUTF8(roots[i].displayName);
        napi_value napiDisplayName;
        status = napi_create_string_utf8(env, displayName.c_str(), displayName.size(), &napiDisplayName);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the string displayName");
        napi_set_named_property(env, jsObj, "displayName", napiDisplayName);

        std::string version = WStringToUTF8(roots[i].version);
        napi_value napiVersion;
        status = napi_create_string_utf8(env, version.c_str(), version.size(), &napiVersion);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the string version");
        napi_set_named_property(env, jsObj, "version", napiVersion);

        status = napi_set_element(env, jsArray, i, jsObj);
        if (status != napi_ok)
            throw std::runtime_error("Error setting the element in the array");
    }

    return jsArray;
}
