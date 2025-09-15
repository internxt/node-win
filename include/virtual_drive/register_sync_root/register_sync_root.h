#pragma once

#include <cfapi.h>
#include <Callbacks.h>
#include "stdafx.h"
#include <iostream>
#include <vector>

HRESULT register_sync_root(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId, const wchar_t *logoPath);