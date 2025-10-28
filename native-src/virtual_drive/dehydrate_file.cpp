#include <windows.h>
#include "napi_extract_args.h"
#include "stdafx.h"

napi_value dehydrate_file(napi_env env, napi_callback_info info)
{
    auto [rawPath] = napi_extract_args<std::wstring>(env, info);
    const wchar_t *path = rawPath.c_str();

    DWORD attrib = GetFileAttributesW(path);

    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        throw std::runtime_error("Cannot dehydrate folder");
    }

    winrt::handle fileHandle(CreateFileW(path, 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

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
}
