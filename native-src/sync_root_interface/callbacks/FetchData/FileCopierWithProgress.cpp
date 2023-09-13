#include "stdafx.h"
#include "FileCopierWithProgress.h"

#define CHUNKSIZE 409600
#define CHUNKDELAYMS 250
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#define CF_SIZE_OF_OP_PARAM(field)                  \
    (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
     FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

struct READ_COMPLETION_CONTEXT
{
    OVERLAPPED Overlapped;
    CF_CALLBACK_INFO CallbackInfo;
    wchar_t FullPath[MAX_PATH];
    HANDLE Handle;
    CHAR PriorityHint;
    LARGE_INTEGER StartOffset;
    LARGE_INTEGER RemainingLength;
    ULONG BufferSize;
    BYTE Buffer[1];
};

// This entire class is static

void FileCopierWithProgress::CopyFromServerToClient(
    _In_ CONST CF_CALLBACK_INFO *lpCallbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *lpCallbackParameters,
    _In_ LPCWSTR serverFolder)
{
    try
    {
        CopyFromServerToClientWorker(
            lpCallbackInfo,
            lpCallbackInfo->ProcessInfo,
            lpCallbackParameters->FetchData.RequiredFileOffset,
            lpCallbackParameters->FetchData.RequiredLength,
            lpCallbackParameters->FetchData.OptionalFileOffset,
            lpCallbackParameters->FetchData.OptionalLength,
            lpCallbackParameters->FetchData.Flags,
            lpCallbackInfo->PriorityHint,
            serverFolder);
    }
    catch (...)
    {
        TransferData(
            lpCallbackInfo->ConnectionKey,
            lpCallbackInfo->TransferKey,
            NULL,
            lpCallbackParameters->FetchData.RequiredFileOffset,
            lpCallbackParameters->FetchData.RequiredLength,
            STATUS_UNSUCCESSFUL);
    }
}

void FileCopierWithProgress::TransferData(
    _In_ CF_CONNECTION_KEY connectionKey,
    _In_ LARGE_INTEGER transferKey,
    _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData,
    _In_ LARGE_INTEGER startingOffset,
    _In_ LARGE_INTEGER length,
    _In_ NTSTATUS completionStatus)
{
    CF_OPERATION_INFO opInfo = {0};
    CF_OPERATION_PARAMETERS opParams = {0};

    opInfo.StructSize = sizeof(opInfo);
    opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
    opInfo.ConnectionKey = connectionKey;
    opInfo.TransferKey = transferKey;
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
    opParams.TransferData.CompletionStatus = STATUS_SUCCESS; // completionStatus;
    opParams.TransferData.Buffer = transferData;
    opParams.TransferData.Offset = startingOffset;
    opParams.TransferData.Length = length;

    // print before
    wprintf(L"[%04x:%04x] - opInfo.StructSize is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opInfo.StructSize);
    wprintf(L"[%04x:%04x] - opInfo.Type is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opInfo.Type);
    wprintf(L"[%04x:%04x] - opInfo.ConnectionKey is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opInfo.ConnectionKey);
    wprintf(L"[%04x:%04x] - opInfo.TransferKey is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opInfo.TransferKey);
    wprintf(L"[%04x:%04x] - opParams.ParamSize is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opParams.ParamSize);
    wprintf(L"[%04x:%04x] - opParams.TransferData.CompletionStatus is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opParams.TransferData.CompletionStatus);
    wprintf(L"[%04x:%04x] - opParams.TransferData.Buffer is %s\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opParams.TransferData.Buffer);
    wprintf(L"[%04x:%04x] - opParams.TransferData.Offset is %d\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opParams.TransferData.Offset);
    wprintf(L"[%04x:%04x] - opParams.TransferData.Length is %d\n",

            GetCurrentProcessId(),
            GetCurrentThreadId(),
            opParams.TransferData.Length);
    // print after
    // winrt::check_hresult(CfExecute(&opInfo, &opParams));
    // catch error of CfExecute
    HRESULT hr = CfExecute(&opInfo, &opParams);
    if (FAILED(hr))
    {
        wprintf(L"[%04x:%04x] - Failed to execute transfer data, hr %x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                hr);
    }
    else
    {
        wprintf(L"[%04x:%04x] - Transfer data executed\n",
                GetCurrentProcessId(),
                GetCurrentThreadId());
    }
}

LPCWSTR TCharToWChar(const TCHAR *tcharString)
{
#ifdef UNICODE
    return tcharString;
#else
    static WCHAR wcharString[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, tcharString, -1, wcharString, MAX_PATH);
    return wcharString;
#endif
}

void WINAPI FileCopierWithProgress::OverlappedCompletionRoutine(
    _In_ DWORD errorCode,
    _In_ DWORD numberOfBytesTransfered,
    _Inout_ LPOVERLAPPED overlapped)
{
    READ_COMPLETION_CONTEXT *readContext =
        CONTAINING_RECORD(overlapped, READ_COMPLETION_CONTEXT, Overlapped);

    // There is the possibility that this code will need to be retried, see end of loop
    auto keepProcessing{false};
    wprintf(L"[%04x:%04x] - OverlappedCompletionRoutine called ");
    do
    {
        // Determine how many bytes have been "downloaded"
        if (errorCode == 0)
        {
            if (!GetOverlappedResult(readContext->Handle, overlapped, &numberOfBytesTransfered, TRUE))
            {
                errorCode = GetLastError();
            }
        }

        //
        // Fix up bytes transfered for the failure case
        //
        if (errorCode != 0)
        {
            wprintf(L"[%04x:%04x] - Async read failed for %s, Status %x\n",
                    GetCurrentProcessId(),
                    GetCurrentThreadId(),
                    readContext->FullPath,
                    NTSTATUS_FROM_WIN32(errorCode));

            numberOfBytesTransfered = (ULONG)(min(readContext->BufferSize, readContext->RemainingLength.QuadPart));
        }

        assert(numberOfBytesTransfered != 0);

        // Simulate passive progress. Note that the completed portion
        // should be less than the total or we will end up "completing"
        // the hydration request prematurely.
        LONGLONG total = readContext->CallbackInfo.FileSize.QuadPart + readContext->BufferSize;
        LONGLONG completed = readContext->StartOffset.QuadPart + readContext->BufferSize;

        // Update the transfer progress
        Utilities::ApplyTransferStateToFile(readContext->FullPath, readContext->CallbackInfo, total, completed);

        // Slow it down so we can see it happening
        Sleep(CHUNKDELAYMS);

        // Complete whatever range returned
        wprintf(L"[%04x:%04x] - Executing download for %s, Status %08x, priority %d, offset %08x`%08x length %08x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                readContext->FullPath,
                NTSTATUS_FROM_WIN32(errorCode),
                readContext->PriorityHint,
                readContext->StartOffset.HighPart,
                readContext->StartOffset.LowPart,
                numberOfBytesTransfered);
        wprintf(L"[%04x:%04x] - TransferData called inside of OverlappedCompletionRoutine");
        wprintf(L"[%04x:%04x] - readContext->Buffer is %s\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                readContext->Buffer);
        // This helper function tells the Cloud File API about the transfer,
        // which will copy the data to the local syncroot
        TransferData(
            readContext->CallbackInfo.ConnectionKey,
            readContext->CallbackInfo.TransferKey,
            errorCode == 0 ? readContext->Buffer : NULL,
            readContext->StartOffset,
            Utilities::LongLongToLargeInteger(numberOfBytesTransfered),
            errorCode);

        // Move the values in the read context to the next chunk
        readContext->StartOffset.QuadPart += numberOfBytesTransfered;
        readContext->RemainingLength.QuadPart -= numberOfBytesTransfered;

        // See if there is anything left to read
        if (readContext->RemainingLength.QuadPart > 0)
        {
            // Cap it at chunksize
            DWORD bytesToRead = (DWORD)(min(readContext->RemainingLength.QuadPart, readContext->BufferSize));

            // And call ReadFileEx to start the next chunk read
            readContext->Overlapped.Offset = readContext->StartOffset.LowPart;
            readContext->Overlapped.OffsetHigh = readContext->StartOffset.HighPart;

            wprintf(L"[%04x:%04x] - Downloading data for %s, priority %d, offset %08x`%08x length %08x\n",
                    GetCurrentProcessId(),
                    GetCurrentThreadId(),
                    readContext->FullPath,
                    readContext->PriorityHint,
                    readContext->Overlapped.OffsetHigh,
                    readContext->Overlapped.Offset,
                    bytesToRead);
            wprintf(L"[%04x:%04x] - ReadFileEx called inside of OverlappedCompletionRoutine");
            if (!ReadFileEx(
                    readContext->Handle,
                    readContext->Buffer,
                    bytesToRead,
                    &readContext->Overlapped,
                    OverlappedCompletionRoutine))
            {
                wprintf(L"[%04x:%04x] - ReadFileEx false");
                errorCode = GetLastError();
                numberOfBytesTransfered = 0;

                keepProcessing = true;
            }
        }
        else
        {
            CloseHandle(readContext->Handle);
            HeapFree(GetProcessHeap(), 0, readContext);
        }
    } while (keepProcessing);
}

void FileCopierWithProgress::CopyFromServerToClientWorker(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_opt_ CONST CF_PROCESS_INFO *processInfo,
    _In_ LARGE_INTEGER requiredFileOffset,
    _In_ LARGE_INTEGER requiredLength,
    _In_ LARGE_INTEGER /*optionalFileOffset*/,
    _In_ LARGE_INTEGER /*optionalLength*/,
    _In_ CF_CALLBACK_FETCH_DATA_FLAGS /*fetchFlags*/,
    _In_ UCHAR priorityHint,
    _In_ LPCWSTR serverFolder)
{
    HANDLE serverFileHandle;

    std::wstring fullServerPath(serverFolder);
    fullServerPath.append(L"\\");
    fullServerPath.append(reinterpret_cast<wchar_t const *>(callbackInfo->FileIdentity));
    wprintf(L"[%04x:%04x] - Full server path is %s\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            fullServerPath.c_str());
    std::wstring fullClientPath(callbackInfo->VolumeDosName);
    fullClientPath.append(callbackInfo->NormalizedPath);
    wprintf(L"[%04x:%04x] - Full client path is %s\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            fullClientPath.c_str());
    READ_COMPLETION_CONTEXT *readCompletionContext;
    DWORD chunkBufferSize;

    wprintf(L"[%04x:%04x] - Received data request from %s for %s%s, priority %d, offset %08x`%08x length %08x`%08x\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            (processInfo && processInfo->ImagePath) ? processInfo->ImagePath : L"UNKNOWN",
            callbackInfo->VolumeDosName,
            callbackInfo->NormalizedPath,
            priorityHint,
            requiredFileOffset.HighPart,
            requiredFileOffset.LowPart,
            requiredLength.HighPart,
            requiredLength.LowPart);

    int length = WideCharToMultiByte(CP_UTF8, 0, fullClientPath.c_str(), -1, NULL, 0, NULL, NULL);
    char *serverPathUtf8 = new char[length];
    WideCharToMultiByte(CP_UTF8, 0, fullClientPath.c_str(), -1, serverPathUtf8, length, NULL, NULL);
    wprintf(L"[%04x:%04x] - Opening %s for read\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            fullServerPath.c_str(),
            serverPathUtf8);

    serverFileHandle =
        CreateFile(
            serverPathUtf8,
            GENERIC_READ,
            0, // FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, // FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);                // no template file

    wprintf(L"[%04x:%04x] - CreateFile called\n",
            GetCurrentProcessId(),
            GetCurrentThreadId());
    if (serverFileHandle == INVALID_HANDLE_VALUE)
    {
        DWORD dwError = GetLastError();
        wprintf(L"Error al abrir el archivo: %u\n", dwError);

        HRESULT hr = NTSTATUS_FROM_WIN32(GetLastError());

        wprintf(L"[%04x:%04x] - Failed to open %s for read, hr %x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                fullServerPath.c_str(),
                hr);

        winrt::check_hresult(hr);
    }
    wprintf(L"[%04x:%04x] - Pass INVALID_HANDLE_VALUE\n",
            GetCurrentProcessId(),
            GetCurrentThreadId());
    // Allocate the buffer used in the overlapped read.
    chunkBufferSize = (ULONG)min(requiredLength.QuadPart, CHUNKSIZE);

    readCompletionContext = (READ_COMPLETION_CONTEXT *)
        HeapAlloc(
            GetProcessHeap(),
            0,
            chunkBufferSize + FIELD_OFFSET(READ_COMPLETION_CONTEXT, Buffer));

    if (readCompletionContext == NULL)
    {
        HRESULT hr = E_OUTOFMEMORY;

        wprintf(L"[%04x:%04x] - Failed to allocate read buffer for %s, hr %x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                fullServerPath.c_str(),
                hr);

        CloseHandle(serverFileHandle);
        winrt::check_hresult(hr);
    }
    // wchar_t wcharString[10000];
    // size_t lengthfp = _tcslen(readCompletionContext->FullPath) + 1;
    // size_t convertedChars = 0;
    // mbstowcs_s(&convertedChars, wcharString, lengthfp, readCompletionContext->FullPath, lengthfp - 1);
    // Tell the read completion context where to copy the chunk(s)
    //    wcsncpy(readCompletionContext->FullPath, fullClientPath.data(), wcslen(fullClientPath.data()));
    wcscpy(readCompletionContext->FullPath, fullClientPath.data());

    wprintf(L"[%04x:%04x] - fullClientPath.data() is %s\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            fullClientPath.data());

    wprintf(L"[%04x:%04x] - readCompletionContext->FullPath is %s\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            readCompletionContext->FullPath);

    // Set up the remainder of the overlapped stuff
    readCompletionContext->CallbackInfo = *callbackInfo;
    readCompletionContext->Handle = serverFileHandle;
    readCompletionContext->PriorityHint = priorityHint;
    readCompletionContext->Overlapped.Offset = requiredFileOffset.LowPart;
    readCompletionContext->Overlapped.OffsetHigh = requiredFileOffset.HighPart;
    readCompletionContext->StartOffset = requiredFileOffset;
    readCompletionContext->RemainingLength = requiredLength;
    readCompletionContext->BufferSize = chunkBufferSize;
    // readCompletionContext->Buffer = new BYTE[chunkBufferSize];

    wprintf(L"[%04x:%04x] - Downloading data for %s, priority %d, offset %08x`%08x length %08x\n",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            readCompletionContext->FullPath,
            priorityHint,
            requiredFileOffset.HighPart,
            requiredFileOffset.LowPart,
            chunkBufferSize);

    if (!ReadFileEx(
            serverFileHandle,
            readCompletionContext->Buffer,
            chunkBufferSize,
            &readCompletionContext->Overlapped,
            OverlappedCompletionRoutine))
    {
        HRESULT hr = NTSTATUS_FROM_WIN32(GetLastError());
        wprintf(L"[%04x:%04x] - Failed to perform async read for %s, Status %x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                fullServerPath.c_str(),
                hr);

        CloseHandle(serverFileHandle);
        HeapFree(GetProcessHeap(), 0, readCompletionContext);

        winrt::check_hresult(hr);
    }

    delete[] serverPathUtf8;
}
