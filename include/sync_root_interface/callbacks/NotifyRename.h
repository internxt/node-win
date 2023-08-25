#pragma once

#include <CallbacksBase.h>

struct NotifyRenameArgs {
    std::wstring newFileName;
    std::wstring fileIdentity;
};

void CALLBACK NotifyRenameCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);