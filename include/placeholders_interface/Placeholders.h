#pragma once
#include <vector>
#include <string>
#include <PlaceholderInfo.h>

class Placeholders
{
public:
    static void CreateOne(
        _In_ PCWSTR fileName,
        _In_ PCWSTR fileIdentity,
        int64_t fileSize,
        DWORD fileIdentityLength,
        uint32_t fileAttributes,
        FILETIME creationTime,
        FILETIME lastWriteTime,
        FILETIME lastAccessTime,
        _In_ PCWSTR destPath);

    static void CreateEntry(
        _In_ PCWSTR itemName,
        _In_ PCWSTR itemIdentity,
        bool isDirectory,
        uint32_t itemSize,
        DWORD itemIdentityLength,
        uint32_t itemAttributes,
        FILETIME creationTime,
        FILETIME lastWriteTime,
        FILETIME lastAccessTime,
        _In_ PCWSTR destPath);

    static void UpdateSyncStatus(const std::wstring &filePath, bool syncState, bool isDirectory);
    static HRESULT UpdatePinState(const std::wstring &path, const PinState state);
    static CF_PLACEHOLDER_STATE GetPlaceholderState(const std::wstring& filePath);
    static std::vector<std::wstring> GetPlaceholderWithStatePending(const std::wstring& filePath);
    static bool IsFileValidForSync(const std::wstring& filePath);
    static bool ConvertToPlaceholder(const std::wstring& fullPath, const std::wstring& serverIdentity);
    static std::string GetFileIdentity(const std::wstring& filePath);
};