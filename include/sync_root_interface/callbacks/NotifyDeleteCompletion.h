#pragma once

#include <CallbacksBase.h>

struct NotifyDeleteCompletionArgs {
    std::wstring fileIdentity;
};

void CALLBACK NotifyDeleteCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);