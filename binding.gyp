{
  "targets": [{
    "target_name": "sysv-semaphore",
    "sources": [ "src/error.cpp", "src/token.cpp", "src/semaphore-sysv.cpp", "src/main.cpp" ],
    "include_dirs": ["node_modules/node-addon-api", "src-vendor/errnoname", "/usr/include", "src"],
    "cflags_cc": ["-fexceptions", "-frtti", "-std=c++17", "-pthread" ],
    "conditions": [
      ["OS=='mac'", {
        "CXX":"/usr/bin/clang++",
        "LINK": "/usr/bin/clang++",
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
          "GCC_ENABLE_CPP_RTTI": "YES",
          "CLANG_CXX_LIBRARY": "libc++",
          "MACOSX_DEPLOYMENT_TARGET": "14.4.1",
          "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
        }
      }]
    ],
    "rules": [
      {
        "rule_name": "swig",
        "extension": "i",
        "inputs": ["<(RULE_INPUT_PATH)"],
        "outputs": ["<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).cpp"],
        "action": ["swig", "-c++", "-javascript", "-napi", "-I./src", "-o", "<@(_outputs)", "<@(_inputs)"],
        "process_outputs_as_sources": 1
      }
    ],
    "sources": [
      "src/error.cpp",
      "src/token.cpp",
      "src/semaphore-sysv.cpp",
      "src/main.i"
    ]
  }]
}
