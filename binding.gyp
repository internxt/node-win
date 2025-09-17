{
  "targets": [
    {
      "msvs_windows_target_platform_version": "10.0.22621.0",
      "target_name": "addon",
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": "1",
          "AdditionalOptions": [
            "-std:c++latest",
            "/EHsc",
            "/await"
          ]
        }
      },
      "sources": [
        "native-src/logger/Logger.cpp",
        "native-src/logger/LoggerPath.cpp",
        "native-src/main.cpp",
        "native-src/placeholders_interface/PlaceHolderInfo.cpp",
        "native-src/placeholders_interface/Planceholders.cpp",
        "native-src/sync_root_interface/SyncRoot.cpp",
        "native-src/sync_root_interface/Utilities.cpp",
        "native-src/sync_root_interface/callbacks/Callbacks.cpp",
        "native-src/sync_root_interface/callbacks/CancelFetchData/CancelFetchDataCallback.cpp",
        "native-src/sync_root_interface/callbacks/FetchData/FetchData.cpp",
        "native-src/sync_root_interface/callbacks/FetchData/FileCopierWithProgress.cpp",
        "native-src/sync_root_interface/callbacks/FetchData/TransferContext.cpp",
        "native-src/virtual_drive/Wrappers.cpp",
        "native-src/virtual_drive/connect_sync_root.cpp",
        "native-src/virtual_drive/convert_to_placeholder.cpp",
        "native-src/virtual_drive/create_file_placeholder.cpp",
        "native-src/virtual_drive/create_folder_placeholder.cpp",
        "native-src/virtual_drive/get_file_identity.cpp",
        "native-src/virtual_drive/hydrate_file.cpp",
        "native-src/virtual_drive/register_sync_root/register_sync_root.cpp",
        "native-src/virtual_drive/register_sync_root/register_sync_root_wrapper.cpp"
      ],
      "include_dirs": [
        "include",
        "include/logger",
        "include/placeholders_interface",
        "include/sync_root_interface",
        "include/sync_root_interface/callbacks",
        "include/virtual_drive",
        "include/virtual_drive/register_sync_root"
      ],
      "libraries": [
        "-lCldApi.lib",
        "-lPropsys.lib"
      ]
    },
    {
      "target_name": "after_build",
      "type": "none",
      "dependencies": [
        "addon"
      ],
      "copies": [
        {
          "destination": "dist",
          "files": [
            "<(PRODUCT_DIR)/addon.node"
          ]
        }
      ]
    }
  ]
}