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
        "native-src/sync_root_watcher/DirectoryWatcher.cpp",
        "native-src/sync_root_watcher/SyncRootWatcher.cpp",
        "native-src/sync_root_interface/callbacks/NotifyMessage/NofifyMessageCallback.cpp",
        "native-src/sync_root_interface/callbacks/CancelFetchData/CancelFetchDataCallback.cpp",
        "native-src/logger/Logger.cpp",
        "native-src/sync_root_interface/callbacks/FetchPlaceholder/FetchPlaceholder.cpp",
        "native-src/sync_root_interface/callbacks/NotifyFileAdded/NotifyFileAddedCallback.cpp",
        "native-src/virtual_drive/Wrappers.cpp",
        "native-src/placeholders_interface/Planceholders.cpp",
        "native-src/sync_root_interface/callbacks/NotifyDelete/NotifyDeleteCallback.cpp",
        "native-src/sync_root_interface/callbacks/FetchData/FileCopierWithProgress.cpp",
        "native-src/logger/LoggerPath.cpp",
        "native-src/sync_root_interface/SyncRoot.cpp",
        "native-src/sync_root_interface/callbacks/NotifyRename/NotifyRenameCallback.cpp",
        "native-src/sync_root_interface/callbacks/FetchData/FetchData.cpp",
        "native-src/main.cpp",
        "native-src/sync_root_interface/Utilities.cpp",
        "native-src/sync_root_interface/callbacks/Callbacks.cpp"
      ],
      "include_dirs": [
        "include/logger",
        "include",
        "include/sync_root_watcher",
        "include/placeholders_interface",
        "include/sync_root_interface",
        "include/sync_root_watcher",
        "include/virtual_drive",
        "include"
      ],
      "libraries": [
        "-lCldApi.lib",
        "-lPropsys.lib"
      ]
    }
  ]
}