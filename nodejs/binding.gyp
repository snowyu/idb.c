{
  "targets": [
    {
      "target_name": "idb",
      "sources": [
        "idb_helpers.cc",
        "idb_helpers_sync.cc",
        "idb_helpers_async.cc"
      ],
      'dependencies': [
        '../libidb.gyp:idb'
      ],
      "libraries": [],
      "include_dirs": ["<!(node -e \"require('nan')\")", "../src/"]
    }
  ]
}
