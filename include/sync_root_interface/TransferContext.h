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

struct TransferContext
{
    CF_CONNECTION_KEY connectionKey;
    CF_TRANSFER_KEY transferKey;
    CF_CALLBACK_INFO callbackInfo;

    LARGE_INTEGER fileSize;
    LARGE_INTEGER requiredLength;
    LARGE_INTEGER requiredOffset;
    std::wstring path;

    bool ready = false;

    std::mutex mtx;
    std::condition_variable cv;
};

std::shared_ptr<TransferContext> GetOrCreateTransferContext(CF_CONNECTION_KEY connKey, CF_TRANSFER_KEY transferKey);

void RemoveTransferContext(CF_TRANSFER_KEY transferKey);
