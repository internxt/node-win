#pragma once
class Placeholders
{
public:
    static void CreateOne(
        _In_ PCWSTR fileName,
        _In_ PCWSTR fileIdentity,
        uint32_t fileSize,
        DWORD fileIdentityLength,
        uint32_t fileAttributes,
        FILETIME creationTime,
        FILETIME lastWriteTime,
        FILETIME lastAccessTime,
        _In_ PCWSTR destPath);
};
