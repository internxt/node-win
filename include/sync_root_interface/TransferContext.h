#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
#include "stdafx.h"
#include <cfapi.h>
#include "Logger.h"
#include "Utilities.h"
#include "Placeholders.h"
#include "FileCopierWithProgress.h"

struct TransferContext
{
    CF_CONNECTION_KEY connectionKey;
    CF_TRANSFER_KEY transferKey;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER requiredLength;
    LARGE_INTEGER requiredOffset;
    CF_CALLBACK_INFO callbackInfo;
    std::wstring path;

    size_t lastReadOffset = 0;
    size_t lastSize = 0;
    bool loadFinished = false;

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    std::wstring tmpPath;
};

std::shared_ptr<TransferContext> GetOrCreateTransferContext(
    CF_CONNECTION_KEY connKey,
    CF_TRANSFER_KEY transferKey);

void RemoveTransferContext(CF_TRANSFER_KEY transferKey);
