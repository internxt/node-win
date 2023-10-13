// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
#include <iostream>
#include <fstream>

#pragma once

class FileCopierWithProgress
{
public:
    static HRESULT TransferData(
        _In_ CF_CONNECTION_KEY connectionKey,
        _In_ LARGE_INTEGER transferKey,
        _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData,
        _In_ LARGE_INTEGER startingOffset,
        _In_ LARGE_INTEGER length,
        _In_ NTSTATUS completionStatus);

};