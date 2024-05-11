{
  "make_global_settings": [
    ["CXX","/usr/bin/clang++"],
    ["LINK","/usr/bin/clang++"],
  ],
  "targets": [{
    "target_name": "OSX",
    "sources": [ "src/semaphore.cpp", "src/flock.cpp", "src/OSX.cpp" ],

    'include_dirs': ["node_modules/node-addon-api", "src-vendor/errnoname"],
    "conditions": [
      ["OS=='mac'", {
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
          "GCC_ENABLE_CPP_RTTI": "YES",
          "CLANG_CXX_LIBRARY": "libc++",
          "MACOSX_DEPLOYMENT_TARGET": "14.4.1",
          "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
        }
      }]
    ]
  }]
}
