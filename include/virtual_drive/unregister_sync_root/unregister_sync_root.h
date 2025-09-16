#pragma once

#include <node_api.h>

HRESULT unregister_sync_root(const GUID &providerId);
HRESULT unregister_sync_root(const wchar_t *providerIdStr);
