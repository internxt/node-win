#include "stdafx.h"
#include "Callbacks.h"
#include <iostream>
#include <fstream>


void CALLBACK FetchDataCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.fetchDataCallback) {
        context->callbacks.fetchDataCallback(context->env);
    }
}

void CALLBACK ValidateDataCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.validateDataCallback) {
        context->callbacks.validateDataCallback(context->env);
    }
}

void CALLBACK CancelFetchDataCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.cancelFetchDataCallback) {
        context->callbacks.cancelFetchDataCallback(context->env);
    }
}

void CALLBACK FetchPlaceholdersCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.fetchPlaceholdersCallback) {
        context->callbacks.fetchPlaceholdersCallback(context->env);
    }
}

void CALLBACK CancelFetchPlaceholdersCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.cancelFetchPlaceholdersCallback) {
        context->callbacks.cancelFetchPlaceholdersCallback(context->env);
    }
}

void CALLBACK NotifyFileOpenCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyFileOpenCompletionCallback) {
        context->callbacks.notifyFileOpenCompletionCallback(context->env);
    }
}

void CALLBACK NotifyFileCloseCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyFileCloseCompletionCallback) {
        context->callbacks.notifyFileCloseCompletionCallback(context->env);
    }
}

void CALLBACK NotifyDehydrateCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyDehydrateCallback) {
        context->callbacks.notifyDehydrateCallback(context->env);
    }
}

void CALLBACK NotifyDehydrateCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyDehydrateCompletionCallback) {
        context->callbacks.notifyDehydrateCompletionCallback(context->env);
    }
}

void CALLBACK NotifyDeleteCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyDeleteCallback) {
        context->callbacks.notifyDeleteCallback(context->env);
    }
}

void CALLBACK NotifyDeleteCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyDeleteCompletionCallback) {
        context->callbacks.notifyDeleteCompletionCallback(context->env);
    }
}

void CALLBACK NotifyRenameCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyRenameCallback) {
        context->callbacks.notifyRenameCallback(context->env);
    }
}

void CALLBACK NotifyRenameCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.notifyRenameCompletionCallback) {
        context->callbacks.notifyRenameCompletionCallback(context->env);
    }
}

void CALLBACK NoneCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    CallbackContext* context = static_cast<CallbackContext*>(callbackInfo->CallbackContext);
    if (context->callbacks.noneCallback) {
        context->callbacks.noneCallback(context->env);
    }
}


// void CALLBACK DeleteDataNotificationCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// ) {
//     MessageBoxW(NULL, L"Delete Data Notification Callback triggered!", L"Callback Activated", MB_OK);
// }

// void CALLBACK FetchDataCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// ) {
//     // UNREFERENCED_PARAMETER(callbackInfo);
//     // UNREFERENCED_PARAMETER(callbackParameters);

//     if (callbackInfo != nullptr) {
//         // Asumiendo que callbackInfo tiene un miembro llamado 'SomeField':
//         std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
//     }

//     if (callbackParameters != nullptr) {
//         // Asumiendo que callbackParameters tiene un miembro llamado 'AnotherField':
//         std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
//     }
// }

// void CALLBACK FetchPlaceholdersCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// ) {
//     if (callbackInfo != nullptr) {
//         std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
//     }

//     if (callbackParameters != nullptr) {
//         std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
//     }
// }

// void CALLBACK CancelFetchDataCallback(
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// ) {
//     if (callbackInfo != nullptr) {
//         std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
//     }

//     if (callbackParameters != nullptr) {
//         std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
//     }
// }