#pragma once

#include "stdafx.h"
#include <optional>

struct FileState
{
    std::string placeholderId;
    CF_PIN_STATE pinState;
};

class FileHandle
{
public:
    using Deleter = void (*)(void *);

    FileHandle();
    FileHandle(void *data, Deleter deleter);

    inline void *get() const { return _data.get(); }
    inline explicit operator bool() const noexcept { return static_cast<bool>(_data); }

private:
    std::unique_ptr<void, void (*)(void *)> _data;
};

FileHandle handleForPath(const std::wstring &path);
