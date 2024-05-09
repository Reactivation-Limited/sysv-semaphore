{
  "make_global_settings": [
    ["CXX","/usr/bin/clang++"],
    ["LINK","/usr/bin/clang++"],
  ],
  "targets": [{
    "target_name": "OSX",
    "sources": [ "src/semaphore.cpp", "src/flock.cpp", "OSX.cxx" ],
    "cflags_cc!": ["-fno-exceptions"],
    "cflags_cc": ["-fexceptions"],
    'include_dirs': ['src-vendor/errnoname'],
    "conditions": [
      ["OS=='mac'", {
        "OTHER_CPLUSPLUSFLAGS" : [ "-std=c++17", "-stdlib=libc++" ],
        "OTHER_LDFLAGS": [ "-stdlib=libc++" ],
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
          "GCC_ENABLE_CPP_RTTI": "YES",
          "MACOSX_DEPLOYMENT_TARGET": "14.4.1",
          "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
        }
      }]
    ]
  }],
  "link_settings": {
    "libraries": [
      "-lerrnoname"
    ],
  }
}
