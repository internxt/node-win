#include "stdafx.h"
#include "Callbacks.h"
#include <iostream>
#include <fstream>

void CALLBACK DeleteDataNotificationCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    MessageBoxW(NULL, L"Delete Data Notification Callback triggered!", L"Callback Activated", MB_OK);
}

void CALLBACK FetchDataCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    // UNREFERENCED_PARAMETER(callbackInfo);
    // UNREFERENCED_PARAMETER(callbackParameters);

    if (callbackInfo != nullptr) {
        // Asumiendo que callbackInfo tiene un miembro llamado 'SomeField':
        std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
    }

    if (callbackParameters != nullptr) {
        // Asumiendo que callbackParameters tiene un miembro llamado 'AnotherField':
        std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
    }
}

void CALLBACK FetchPlaceholdersCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    if (callbackInfo != nullptr) {
        std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
    }

    if (callbackParameters != nullptr) {
        std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
    }
}

void CALLBACK CancelFetchDataCallback(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    if (callbackInfo != nullptr) {
        std::wcout << L"Callback Info, SomeField: " << callbackInfo << std::endl;
    }

    if (callbackParameters != nullptr) {
        std::wcout << L"Callback Parameters, AnotherField: " << callbackParameters << std::endl;
    }
}