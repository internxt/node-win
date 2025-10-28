#include <windows.h>
#include "napi_extract_args.h"
#include "stdafx.h"

napi_value dehydrate_file(napi_env env, napi_callback_info info)
{
    auto [path] = napi_extract_args<std::wstring>(env, info);

    DWORD attrib = GetFileAttributesW(path.c_str());

    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        throw std::runtime_error("Cannot dehydrate folder");
    }

    winrt::handle fileHandle(CreateFileW(path.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    LARGE_INTEGER length;
    GetFileSizeEx(fileHandle.get(), &length);

    winrt::check_hresult(CfDehydratePlaceholder(
        fileHandle.get(),
        offset,
        length,
        CF_DEHYDRATE_FLAG_NONE,
        nullptr));

    return nullptr;
}
