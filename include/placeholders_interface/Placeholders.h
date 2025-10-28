#pragma once
#include <vector>
#include <string>
#include <PlaceholderInfo.h>

class Placeholders
{
public:
    static void MaintainIdentity(std::wstring &fullPath, PCWSTR fileIdentity, bool isDirectory);
    static void UpdateSyncStatus(const std::wstring &filePath, bool isDirectory);
    static void UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory);
    static FileState GetPlaceholderInfo(const std::wstring &directoryPath);
};