#pragma once

#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#include <Shellapi.h>
#include <wrl/client.h>
#include <Shobjidl_core.h>
#include <string>

void SetThumbnail(const std::wstring& filePath, const std::wstring& thumbnailPath);
