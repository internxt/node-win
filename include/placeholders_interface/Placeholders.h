#pragma once
#include <vector>
#include <string>
#include <PlaceholderInfo.h>

class Placeholders
{
public:
    static void MaintainIdentity(std::wstring &fullPath, PCWSTR fileIdentity, bool isDirectory);
    static void ForceShellRefresh(const std::wstring &path);
    static void UpdateSyncStatus(const std::wstring &filePath, bool isDirectory);
    static HRESULT UpdatePinState(const std::wstring &path, const PinState state);
    static std::string GetFileIdentity(const std::wstring &filePath);
    static void UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory);
    static FileState GetPlaceholderInfo(const std::wstring &directoryPath);
};