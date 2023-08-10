#include "stdafx.h"
#include <node_api.h>
#include "Placeholders.h"
// #include "CloudMirror/CloudMirror/stdafx.h"
// #include "CloudMirror/CloudMirror/FileCopierWithProgress.h"
// namespace demo {

napi_value Method(napi_env env, napi_callback_info args) {
  napi_value greeting;
  napi_status status;

  status = napi_create_string_utf8(env, "world", NAPI_AUTO_LENGTH, &greeting);
  if (status != napi_ok) return nullptr;
  return greeting;
}

napi_value CreatePlaceholderFile(napi_env env, napi_callback_info args) {
  size_t argc = 8;  // Number of expected arguments
  napi_value argv[8];  // Array to hold argument values
  
  // Get the argument values from the callback info
  napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

  if (argc < 8) {
    napi_throw_error(env, nullptr, "Insufficient arguments passed to CreatePlaceholderFile");
    return nullptr;
  }

  LPCWSTR fileName;
  size_t fileNameLength;
  napi_get_value_string_utf16(env, argv[0], nullptr, 0, &fileNameLength);
  fileName = new WCHAR[fileNameLength + 1];
  napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(fileName)), fileNameLength + 1, nullptr);

  LPCWSTR fileIdentity;
  size_t fileIdentityLength;
  napi_get_value_string_utf16(env, argv[1], nullptr, 0, &fileIdentityLength);
  fileIdentity = new WCHAR[fileIdentityLength + 1];
  napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(fileIdentity)), fileIdentityLength + 1, nullptr);

  uint32_t fileSize;
  napi_get_value_uint32(env, argv[2], &fileSize);

  uint32_t fileAttributes;
  napi_get_value_uint32(env, argv[3], &fileAttributes);

  // Extract creationTime, lastWriteTime, and lastAccessTime if they are provided
  // You can use napi_get_value_int64 or napi_get_value_bigint_* functions to get these values
  FILETIME creationTime, lastWriteTime, lastAccessTime;

  // Create a dummy FILETIME for the creationTime
  creationTime.dwLowDateTime = 12345678; // Set appropriate values here
  creationTime.dwHighDateTime = 87654321;

  // Create a dummy FILETIME for the lastWriteTime
  lastWriteTime.dwLowDateTime = 98765432; // Set appropriate values here
  lastWriteTime.dwHighDateTime = 23456789;

  // Create a dummy FILETIME for the lastAccessTime
  lastAccessTime.dwLowDateTime = 34567890; // Set appropriate values here
  lastAccessTime.dwHighDateTime = 78901234;

  LPCWSTR destPath;
  size_t destPathLength;
  napi_get_value_string_utf16(env, argv[7], nullptr, 0, &destPathLength);
  destPath = new WCHAR[destPathLength + 1];
  napi_get_value_string_utf16(env, argv[7], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(destPath)), destPathLength + 1, nullptr);

  // Call the function
  Placeholders::CreateOne(
    fileName,
    fileIdentity,
    fileSize,
    fileIdentityLength,
    fileAttributes,
    creationTime,
    lastWriteTime,
    lastAccessTime,
    destPath
  );

  // Clean up allocated memory
  delete[] fileName;
  delete[] fileIdentity;
  delete[] destPath;

  // Return a napi_value if needed
  napi_value result;
  napi_get_boolean(env, true, &result);
  return result;
}

napi_value init(napi_env env, napi_value exports) {
  napi_property_descriptor desc = {
    "createPlaceholderFile",
    nullptr,
    CreatePlaceholderFile,
    nullptr,
    nullptr,
    nullptr,
    napi_default,
    nullptr
  };

  napi_status defineStatus = napi_define_properties(env, exports, 1, &desc);
  if (defineStatus != napi_ok) {
    napi_throw_error(env, nullptr, "Failed to define function");
    return nullptr;
  }

  // napi_status status;
  // napi_value fn;

  // status = napi_create_function(env, nullptr, 0, Method, nullptr, &fn);
  // if (status != napi_ok) return nullptr;

  // status = napi_set_named_property(env, exports, "hello", fn);
  // if (status != napi_ok) return nullptr;
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)

// }  // namespace demo