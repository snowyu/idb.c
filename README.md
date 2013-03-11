iDB for C Low-Level Functions Library
=====================================

THe KISS Efficient Structed-Column Oriented Key/Value Hierarchy data store
Library.


Directories and Files
---------------------

README.md   -- this file.
deps\       -- the third-party files
  redis\    -- the emmbed iDB to redis
src\        -- the iDB Database Engine Source.
  idb.h     -- the iDB Database Engine head file
  idb.c     -- the iDB Database Engine source file
  idb_helpers.h -- the helper functions and constants for the iDB Database Engine head file
    Constants: IDB_KEY_TYPE_NAME, IDB_VALUE_NAME, XATTR_PREFIX
    Options: IDBMaxPageCount, IDBDuplicationKeyProcess, IDBPartitionFullProcess
    Functions: 
        Operations(Attributes): iPut/iGet, iIsExists, iDeleleAttr
        Operations(Key): iDelete, iAlias, iKeyExists, iSubkeys, iSubkeyCount, iSubkeyWalk
        Helpers: IsDirValueExists, GetDirValue, SetDirValue
  isdk_utils.h  -- the general utils functions and constants.
    Constants: O_RW_RW_R__PERMS, O_RWXRWXR_XPERMS, O_EXCL_CREAT
    Functions: open_or_create_file, MoveDir, DeleteDir, DirectoryExists, IsDirValueExists,
               ForceDirectories, sdsJoinPathLen, UrlEncode, UrlDecode
  isdk_xattr.h  -- the xattr helper functions and constants
    functions: IsXattrExists, GetXattr, SetXattr
  deps\     -- the third-party files for source


Make it
-------

        cd build
        cmake ..
        make

Usages
------

All the iDB data is stored in the specified directories(aDir).

The iDB keys and values are strings, only strings. And they are encoded in UTF-8.
The iDB "key" can be hierarchical tree structure like directory. The hierarchy
delimiter of key is character "/"(path).

The iDB store two kinds of data through out a key:

* Attributes
* Subkeys

A key can have multiple attributes. One of the most special attribute
should be the "value" attribute, which is used to store the value of the "key".






