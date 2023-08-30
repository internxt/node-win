{
  "targets": [
    {
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
        "native-src/virtual_drive/Wrappers.cpp",
        "native-src/sync_root_interface/Utilities.cpp",
        "native-src/sync_root_interface/callbacks/NotifyDeleteCompletion.cpp",
        "native-src/sync_root_watcher/SyncRootWatcher.cpp",
        "native-src/main.cpp",
        "native-src/sync_root_interface/callbacks/NotifyRename.cpp",
        "native-src/sync_root_watcher/DirectoryWatcher.cpp",
        "native-src/placeholders_interface/Planceholders.cpp",
        "native-src/sync_root_interface/SyncRoot.cpp"
      ],
      "include_dirs": [
        "include/sync_root_interface",
        "include/sync_root_interface/callbacks",
        "include/sync_root_watcher",
        "include/placeholders_interface",
        "include",
        "include/virtual_drive"
      ],
      "libraries": [
        "-lCldApi.lib"
      ]
    }
  ]
}