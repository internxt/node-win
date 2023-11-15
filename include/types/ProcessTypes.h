#pragma once

enum class ProcessErrorName
{
    NOT_EXISTS,
    NO_PERMISSION,
    NO_INTERNET,
    NO_REMOTE_CONNECTION,
    BAD_RESPONSE,
    EMPTY_FILE,
    FILE_TOO_BIG,
    UNKNOWN
};

enum class FileOperationError
{
    UPLOAD_ERROR,
    DOWNLOAD_ERROR,
    RENAME_ERROR,
    DELETE_ERROR,
    METADATA_READ_ERROR
};