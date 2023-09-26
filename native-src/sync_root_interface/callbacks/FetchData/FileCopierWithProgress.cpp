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

struct READ_COMPLETION_CONTEXT
{
        OVERLAPPED Overlapped;
        CF_CALLBACK_INFO CallbackInfo;
        TCHAR FullPath[MAX_PATH];
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
    _In_ LPCWSTR serverFolder) // fake nube
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
                wprintf(L"[%04x:%04x] - CopyFromServerToClient failed\n", GetCurrentProcessId(), GetCurrentThreadId());
                TransferData(
                    lpCallbackInfo->ConnectionKey,
                    lpCallbackInfo->TransferKey,
                    NULL,
                    lpCallbackParameters->FetchData.RequiredFileOffset,
                    lpCallbackParameters->FetchData.RequiredLength,
                    STATUS_UNSUCCESSFUL);

                // error details
                winrt::throw_last_error();
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
                Utilities::ApplyTransferStateToFile((LPCWSTR)readContext->FullPath, readContext->CallbackInfo, total, completed);

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

                // This helper function tells the Cloud File API about the transfer,
                // which will copy the data to the local syncroot
                TransferData(
                    readContext->CallbackInfo.ConnectionKey,
                    readContext->CallbackInfo.TransferKey,
                    errorCode == 0 ? readContext->Buffer : NULL,
                    readContext->StartOffset,
                    Utilities::LongLongToLargeInteger(numberOfBytesTransfered),
                    errorCode);
                wprintf(L"[%04x:%04x] - Pass CfExecute\n");
                // Move the values in the read context to the next chunk
                readContext->StartOffset.QuadPart += numberOfBytesTransfered;
                readContext->RemainingLength.QuadPart -= numberOfBytesTransfered;

                // See if there is anything left to read
                if (readContext->RemainingLength.QuadPart > 0)
                {
                        wprintf(L"[%04x:%04x] - Pass readContext->RemainingLength.QuadPart > 0\n");
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

                        // In the event of ReadFileEx succeeding, the while loop
                        // will complete, this chunk is done, and whenever the OS
                        // has completed this new ReadFileEx again, then this entire
                        // method will be called again with the new chunk.
                        // In that case, the handle and buffer need to remain intact
                        if (!ReadFileEx(
                                readContext->Handle,
                                readContext->Buffer,
                                bytesToRead,
                                &readContext->Overlapped,
                                OverlappedCompletionRoutine))
                        {
                                // In the event the ReadFileEx failed,
                                // we want to loop through again to try and
                                // process this again
                                errorCode = GetLastError();
                                numberOfBytesTransfered = 0;

                                keepProcessing = true;
                        }
                }
                else
                {
                        wprintf(L"[%04x:%04x] - Free the buffer we are done\n");
                        // Close the read file handle and free the buffer,
                        // because we are done.
                        CloseHandle(readContext->Handle);
                        HeapFree(GetProcessHeap(), 0, readContext);
                }
        } while (keepProcessing);
}

// In a nutshell, it copies a file from the "server" to the
// "client" using the overlapped trickery of Windows to
// chunkatize the copy. This way you don't have to allocate
// a huge buffer.
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
        wprintf(L"[%04x:%04x] - CopyFromServerToClientWorker FILESIZE %lld \n", GetCurrentProcessId(), GetCurrentThreadId(), callbackInfo->FileSize.QuadPart);
        HANDLE serverFileHandle;

        std::wstring fullServerPath(serverFolder);
        std::wstring fullClientPath(callbackInfo->VolumeDosName);
        fullClientPath.append(callbackInfo->NormalizedPath);

        // fullServerPath and fullClientPath should have the same file name and extension
        if (!ArePathsEqual(fullClientPath.c_str(), fullServerPath.c_str()))
        {
                wprintf(L"[%04x:%04x] - Error names and extensions on server file and placeholder are not equal.\n", GetCurrentProcessId(), GetCurrentThreadId());
                TransferData(callbackInfo->ConnectionKey, callbackInfo->TransferKey, NULL, requiredFileOffset, requiredLength, STATUS_UNSUCCESSFUL);
                return;
        }

        // print fullServerPath and fullClientPath
        wprintf(L"[%04x:%04x] - fullServerPath: %s\n", GetCurrentProcessId(), GetCurrentThreadId(), fullServerPath.c_str());
        wprintf(L"[%04x:%04x] - fullClientPath: %s\n", GetCurrentProcessId(), GetCurrentThreadId(), fullClientPath.c_str());

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
        wprintf(L"[%04x:%04x] - Full path: %s\n", GetCurrentProcessId(), GetCurrentThreadId(), fullServerPath.c_str());
        serverFileHandle =
            CreateFileW(
                fullServerPath.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL);

        if (serverFileHandle == INVALID_HANDLE_VALUE)
        {
                HRESULT hr = NTSTATUS_FROM_WIN32(GetLastError());

                wprintf(L"[%04x:%04x] - Failed to open %s for read, hr %x\n",
                        GetCurrentProcessId(),
                        GetCurrentThreadId(),
                        fullServerPath.c_str(),
                        hr);

                winrt::check_hresult(hr);
        }

        // if size file is more than 3.8 GB, we use the chunk size, because we are using uint32_t
        // if filesize is less than 100MB, we use the file size
        chunkBufferSize = (ULONG)min(requiredLength.QuadPart, CHUNKSIZE);

        // if filesize is more than 1GB, we use the chunk size
        if (callbackInfo->FileSize.QuadPart > 1073741824)
        {
                wprintf(L"[%04x:%04x] - File size is more than 1GB, we use the chunk size\n", GetCurrentProcessId(), GetCurrentThreadId());
                chunkBufferSize = (ULONG)(CHUNKSIZE * 2);
        }

        // if filesize is more than 2GB, we use chunksize x3
        if (callbackInfo->FileSize.QuadPart > 2147483648)
        {
                wprintf(L"[%04x:%04x] - File size is more than 2GB, we use the chunk size x3\n", GetCurrentProcessId(), GetCurrentThreadId());
                chunkBufferSize = (ULONG)(CHUNKSIZE * 3);
        }

        // if filesize is more than 3GB, we use chunksize x4
        if (callbackInfo->FileSize.QuadPart > 3221225472)
        {
                wprintf(L"[%04x:%04x] - File size is more than 3GB, we use the chunk size x4\n", GetCurrentProcessId(), GetCurrentThreadId());
                chunkBufferSize = (ULONG)(CHUNKSIZE * 4);
        }

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

        // Tell the read completion context where to copy the chunk(s)
        wcsncpy_s(
            (wchar_t *)readCompletionContext->FullPath,
            _countof(readCompletionContext->FullPath), // Tamaño máximo del búfer de destino
            fullClientPath.data(),
            wcslen(fullClientPath.data()));

        // Set up the remainder of the overlapped stuff
        readCompletionContext->CallbackInfo = *callbackInfo;
        readCompletionContext->Handle = serverFileHandle;
        readCompletionContext->PriorityHint = priorityHint;
        readCompletionContext->Overlapped.Offset = requiredFileOffset.LowPart;
        readCompletionContext->Overlapped.OffsetHigh = requiredFileOffset.HighPart;
        readCompletionContext->StartOffset = requiredFileOffset;
        readCompletionContext->RemainingLength = requiredLength;
        readCompletionContext->BufferSize = chunkBufferSize;

        wprintf(L"[%04x:%04x] - Downloading data for %s, priority %d, offset %08x`%08x length %08x\n",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                readCompletionContext->FullPath,
                priorityHint,
                requiredFileOffset.HighPart,
                requiredFileOffset.LowPart,
                chunkBufferSize);

        // Initiate the read for the first chunk. When this async operation
        // completes (failure or success), it will call the OverlappedCompletionRoutine
        // above with that chunk. That OverlappedCompletionRoutine is responsible for
        // subsequent ReadFileEx calls to read subsequent chunks. This is only for the
        // first one
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
                // cierrta el handle del server
                CloseHandle(serverFileHandle);
                // libera heap
                HeapFree(GetProcessHeap(), 0, readCompletionContext);

                winrt::check_hresult(hr);
        }

        // delete file in fake server
        DeleteFileW(fullServerPath.c_str());
        wprintf(L"[%04x:%04x] - File deleted in fake server.\n", GetCurrentProcessId(), GetCurrentThreadId());
}

// TODO: likely, we will use this function to create the file in the fake server
void FileCopierWithProgress::CreateFileTemp(std::wstring fileIdentity)
{
        std::wstring serverFolder = L"C:\\Users\\<user>\\Desktop\\fakeserver";       // Ruta de la carpeta del servidor
        std::wstring fullServerPath = serverFolder + L"\\" + fileIdentity + L".txt"; // Ruta completa del archivo
        wprintf(L"Ruta completa del archivo: %s\n", fullServerPath.c_str());
        std::wofstream file(fullServerPath);
        if (file.is_open())
        {
                file << "Contenido del archivo" << std::endl;
                file.close();
                wprintf(L"Archivo creado correctamente.\n");
                // Agregar permisos de lectura y escritura al archivo
                if (SetFileAttributesW((LPCWSTR)fullServerPath.c_str(), FILE_ATTRIBUTE_NORMAL) != 0)
                {
                        wprintf(L"Permisos de lectura y escritura agregados correctamente.\n");
                }
                else
                {
                        wprintf(L"Error al agregar permisos de lectura y escritura.\n");
                }
        }
        else
        {
                wprintf(L"Error al crear el archivo.\n");
        }
}

bool FileCopierWithProgress::ArePathsEqual(const std::wstring &clientPath,
                                           const std::wstring &fakeServerPath)
{
        fs::path filePath1(clientPath);
        fs::path filePath2(fakeServerPath);
        std::wstring fileName1 = filePath1.filename().wstring();
        std::wstring fileName2 = filePath2.filename().wstring();
        std::wstring extension1 = filePath1.extension().wstring();
        std::wstring extension2 = filePath2.extension().wstring();
        if (fileName1 == fileName2 && extension1 == extension2)
        {
                wprintf(L"File names and extensions are equal.\n");
                return true;
        }
        else
        {
                wprintf(L"File names and extensions are not equal.\n");
                return false;
        }
}