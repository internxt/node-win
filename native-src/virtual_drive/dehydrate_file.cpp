#include <windows.h>
#include "napi_extract_args.h"
#include "SyncRoot.h"

napi_value dehydrate_file(napi_env env, napi_callback_info info) {
    auto [rawPath] = napi_extract_args<1>(env, info);
    const wchar_t* path = rawPath.c_str();

    wprintf(L"Dehydration file init %ls\n", path);
    DWORD attrib = GetFileAttributesW(path);

    if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
        wprintf(L"Path is from a directory: %ls\n", path);
        return nullptr;
    }

    winrt::handle placeholder(CreateFileW(path, 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    LARGE_INTEGER length;
    GetFileSizeEx(placeholder.get(), &length);

    if (!(attrib & FILE_ATTRIBUTE_UNPINNED)) {
        wprintf(L"File is already dehydrated or pinned: %ls\n", path);
        return nullptr;
    }

    wprintf(L"Dehydrating file started %ls\n", path);
    HRESULT hr = CfDehydratePlaceholder(placeholder.get(), offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);

    if (SUCCEEDED(hr)) {
        wprintf(L"Dehydration finished %ls\n", path);
        return nullptr;
    }

    DWORD err = HRESULT_CODE(hr);

    if (err == ERROR_SHARING_VIOLATION || err == ERROR_CLOUD_FILE_IN_USE) {
        MessageBoxW(
            nullptr,
            L"Unable to free up space because the file is currently in use.\nPlease close the file and try again.",
            L"File in use",
            MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    }

    winrt::throw_hresult(hr);
}
