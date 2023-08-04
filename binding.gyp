{
  "targets": [
    {
      "target_name": "addon",
      # "msbuild_settings": {
      #   "Props": [ "./CustomConfig.props" ]
      # },
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': [ '-std:c++17', ],
        },
      },
      "sources": [ "hello.cc" ]
    }
  ]
}