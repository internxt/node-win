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
        "native-src/sync_root_interface/callbacks/FetchPlaceholder/FetchPlaceholder.cpp",
        "native-src/sync_root_interface/callbacks/NotifyDelete/NotifyDeleteCallback.cpp",
        "native-src/sync_root_interface/callbacks/NotifyRename/NotifyRenameCallback.cpp",
        "native-src/sync_root_interface/callbacks/NotifyRename/RenameTransferContext.cpp",
        "native-src/virtual_drive/Wrappers.cpp"
      ],
      "include_dirs": [
        "include",
        "include/logger",
        "include/placeholders_interface",
        "include/sync_root_interface",
        "include/sync_root_interface/callbacks",
        "include/types",
        "include/virtual_drive"
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