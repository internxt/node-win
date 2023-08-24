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
        "native-src/main.cpp",
        "native-src/sync_root_watcher/DirectoryWatcher.cpp",
        "native-src/sync_root_interface/SyncRoot.cpp",
        "native-src/virtual_drive/Wrappers.cpp",
        "native-src/placeholders_interface/Planceholders.cpp",
        "native-src/sync_root_watcher/SyncRootWatcher.cpp",
        "native-src/sync_root_interface/Utilities.cpp",
        "native-src/sync_root_interface/Callbacks.cpp"
      ],
      "include_dirs": [
        "include/virtual_drive",
        "include",
        "include/sync_root_watcher",
        "include/placeholders_interface",
        "include/sync_root_interface"
      ],
      "libraries": [
        "-lCldApi.lib"
      ]
    }
  ]
}