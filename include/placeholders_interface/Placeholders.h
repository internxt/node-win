#pragma once
#include <vector>
#include <string>
#include <PlaceholderInfo.h>

struct PlaceholderResult {
    bool success;
    std::wstring errorMessage;
};

class Placeholders
{
public:
    static PlaceholderResult CreateOne(
        const wchar_t *fileName,
        const wchar_t *fileIdentity,
        int64_t fileSize,
        LARGE_INTEGER creationTime,
        LARGE_INTEGER lastWriteTime,
        LARGE_INTEGER lastAccessTime,
        const wchar_t *destPath);

    static void MaintainIdentity(std::wstring &fullPath, PCWSTR fileIdentity, bool isDirectory);

    static PlaceholderResult CreateEntry(
        const wchar_t *itemName,
        const wchar_t *itemIdentity,
        LARGE_INTEGER creationTime,
        LARGE_INTEGER lastWriteTime,
        LARGE_INTEGER lastAccessTime,
        const wchar_t *destPath);

    static void ForceShellRefresh(const std::wstring &path);
    static void UpdateSyncStatus(const std::wstring &filePath, bool syncState, bool isDirectory);
    static HRESULT UpdatePinState(const std::wstring &path, const PinState state);
    static PlaceholderResult ConvertToPlaceholder(const std::wstring &fullPath, const std::wstring &serverIdentity);
    static std::string GetFileIdentity(const std::wstring &filePath);
    static void UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory);
    static FileState GetPlaceholderInfo(const std::wstring &directoryPath);
};