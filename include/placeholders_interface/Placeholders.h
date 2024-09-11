#pragma once
#include <vector>
#include <string>
#include <PlaceholderInfo.h>

enum class PlaceholderAttribute
{
    OTHER = 0,
    NOT_PINNED = 1,
    PINNED = 2,
};

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

    static void MaintainIdentity(std::wstring &fullPath, PCWSTR fileIdentity, bool isDirectory);

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
    static CF_PLACEHOLDER_STATE GetPlaceholderState(const std::wstring &filePath);
    static std::vector<std::wstring> GetPlaceholderWithStatePending(const std::wstring &filePath);
    static bool IsFileValidForSync(const std::wstring &filePath);
    static bool ConvertToPlaceholder(const std::wstring &fullPath, const std::wstring &serverIdentity);
    static std::string GetFileIdentity(const std::wstring &filePath);
    static void UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory);
    static PlaceholderAttribute GetAttribute(const std::wstring &filePath);
    static bool IsTempFile(const std::wstring &filePath);
};