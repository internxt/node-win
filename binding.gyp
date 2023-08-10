{
  "targets": [
    {
      "target_name": "addon",
      'msvs_settings': {
        'VCCLCompilerTool': {
          'ExceptionHandling': '1',
          'AdditionalOptions': [ '-std:c++17', '/EHsc' ],
        },
      },
      "sources": [ "src/hello.cc", "src/Utilities.cpp", "src/Placeholders.cpp" ],
      "include_dirs": [ "include" ],
      "libraries": [
        "-lCldApi.lib"
      ],
    }
  ]
}