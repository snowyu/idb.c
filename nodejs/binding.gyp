{
  "targets": [
    {
      "target_name": "idb",
      "sources": [
        "idb.cc",
        "pi_est.cc",
        "sync.cc",
        "async.cc"
      ],
      'dependencies': [
        '../libidb.gyp:idb'
      ],
      "libraries": [],
      "include_dirs": ["<!(node -e \"require('nan')\")", "../src/"]
    }
  ]
}
