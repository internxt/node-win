#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
#include "stdafx.h"

struct RenameContext {
    CF_CONNECTION_KEY connectionKey;
    CF_TRANSFER_KEY transferKey;
    std::wstring targetPath;
    std::wstring fileIdentity;

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    bool callbackResult = false;
};

std::shared_ptr<RenameContext> GetOrCreateRenameContext(
    CF_CONNECTION_KEY connKey,
    CF_TRANSFER_KEY transferKey);

void RemoveRenameContext(CF_TRANSFER_KEY transferKey);
