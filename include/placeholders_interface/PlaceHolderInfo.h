#pragma once

#include "stdafx.h"
#include <optional>

enum class SyncState
{
    /* status that occurs when an error occurs while reading the status*/
    Undefined = -1,
    NotInSync = 0,
    InSync = 1,
};

enum class PinState
{
    /* The pin state is derived from the state of the parent folder. For example new remote files start out in this state, following the state of their parent folder. This state is used purely for resetting pin states to their derived value. The effective state for an item will never be "Inherited". */
    Inherited = 0,
    /* The file shall be available and up to date locally. Also known as "pinned". Pinned dehydrated files shall be hydrated as soon as possible. */
    AlwaysLocal = 1,
    /* File shall be a dehydrated placeholder, filled on demand. Also known as "unpinned". Unpinned hydrated files shall be dehydrated as soon as possible. If a unpinned file becomes hydrated (such as due to an implicit hydration where the user requested access to the file's data) its pin state changes to Unspecified. */
    OnlineOnly = 2,
    /* The user hasn't made a decision. The client or platform may hydrate or dehydrate as they see fit. New remote files in unspecified directories start unspecified, and dehydrated (which is an arbitrary decision).*/
    Unspecified = 3,
    /* The file will never be synced to the cloud. Useful for ignored files to indicate to the OS the file will never besynced */
    Excluded = 4,
};

class PlaceHolderInfo
{
public:
    using Deleter = void (*)(CF_PLACEHOLDER_BASIC_INFO *);

    PlaceHolderInfo();
    PlaceHolderInfo(CF_PLACEHOLDER_BASIC_INFO *data, Deleter deleter);

    inline CF_PLACEHOLDER_BASIC_INFO *get() const noexcept { return _data.get(); }
    inline CF_PLACEHOLDER_BASIC_INFO *operator->() const noexcept { return _data.get(); }
    inline explicit operator bool() const noexcept { return static_cast<bool>(_data); }

    std::optional<PinState> pinState() const;
    std::optional<SyncState> syncState() const;
    std::optional<LARGE_INTEGER> FileId() const;
    std::optional<BYTE> FileIdentity() const;

private:
    std::unique_ptr<CF_PLACEHOLDER_BASIC_INFO, Deleter> _data;
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

std::string pinStateToString(PinState state);
std::string syncStateToString(SyncState state);
CF_PIN_STATE pinStateToCfPinState(PinState state);