extern "C" {
    void CALLBACK FetchDataCallback_C(
        _In_ CF_CALLBACK_INFO* callbackInfo,
        _In_ CF_CALLBACK_PARAMETERS* callbackParameters
    );

    void CALLBACK FetchPlaceholdersCallback_C(
        _In_ CF_CALLBACK_INFO* callbackInfo,
        _In_ CF_CALLBACK_PARAMETERS* callbackParameters
    );
}