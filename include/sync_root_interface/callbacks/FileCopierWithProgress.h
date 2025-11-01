#include <iostream>
#include <fstream>

#pragma once

class FileCopierWithProgress
{
public:
    static void TransferData(
        _In_ CF_CONNECTION_KEY connectionKey,
        _In_ LARGE_INTEGER transferKey,
        _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData,
        _In_ LARGE_INTEGER startingOffset,
        _In_ LARGE_INTEGER length,
        _In_ NTSTATUS completionStatus);
};