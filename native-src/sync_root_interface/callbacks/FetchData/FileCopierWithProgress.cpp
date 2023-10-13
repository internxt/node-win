#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include "FileCopierWithProgress.h"
#include "Utilities.h"
#include <filesystem>
namespace fs = std::filesystem;

// 100MB chunks
#define CHUNKSIZE (4096 * 25600)
// Arbitrary delay per chunk, again, so you can actually see the progress bar
// move
#define CHUNKDELAYMS 250

#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#define CF_SIZE_OF_OP_PARAM(field)                      \
        (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
         FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

HRESULT FileCopierWithProgress::TransferData(
    _In_ CF_CONNECTION_KEY connectionKey,
    _In_ LARGE_INTEGER transferKey,
    _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData,
    _In_ LARGE_INTEGER startingOffset,
    _In_ LARGE_INTEGER length,
    _In_ NTSTATUS completionStatus)
{
        CF_OPERATION_INFO opInfo = {0};
        CF_OPERATION_PARAMETERS opParams = {0};
        wprintf(L"[%04x:%04x] - TransferData\n", GetCurrentProcessId(), GetCurrentThreadId());
        opInfo.StructSize = sizeof(opInfo);
        opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
        opInfo.ConnectionKey = connectionKey;
        opInfo.TransferKey = transferKey;
        opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
        opParams.TransferData.CompletionStatus = completionStatus;
        opParams.TransferData.Buffer = transferData;
        opParams.TransferData.Offset = startingOffset;
        opParams.TransferData.Length = length;

        HRESULT hr = CfExecute(&opInfo, &opParams);
        if (FAILED(hr))
        {
                wprintf(L"Error in CfExecute(), HRESULT: %lx\n", hr);
        }
        printf("TransferData : %s\n", SUCCEEDED(hr) ? "Succeeded" : "Failed");

        return hr;
}