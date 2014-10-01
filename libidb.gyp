# This file is used with the GYP meta build system.
# http://code.google.com/p/gyp
# To build try this:
#   svn co http://gyp.googlecode.com/svn/trunk gyp
#   ./gyp/gyp -f make --depth=`pwd` libidb.gyp
#   make
#   ./out/Debug/test

{
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      # TODO: hoist these out and put them somewhere common, because
      #       RuntimeLibrary MUST MATCH across the entire project
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      }
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
      },
      'VCLibrarianTool': {
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
      },
    },
  },

  'targets': [
    {
      'variables': { 'target_arch%': 'ia32' }, # default for node v0.6.x
      'target_name': 'idb',
      'product_prefix': 'lib',
      'type': 'static_library',
      'sources': [
        "src/idb_helpers.c",
        "src/isdk_utils.c",
        "src/isdk_sds.c",
        "src/isdk_string.c",
        "src/isdk_xattr.c",
        "src/deps/sds.c",
        "src/deps/zmalloc.c",
        "src/deps/utf8proc.c",
      ],
      'defines': [
        'PIC',
        'USE_JEMALLOC'
      ],
      'include_dirs': [
        '/usr/local/include',
        './src',
        './src/deps',
      ],
      'link_settings': {
          'libraries': [
              '-ljemalloc -L/usr/local/lib'
          ]
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '/usr/local/include',
          './src',
          './src/deps',
        ],
        'conditions': [
          ['OS=="win"', {
            'defines': [
              'IDB_STATIC'
            ]
          }]
        ],
      },
    },

  ]
}
