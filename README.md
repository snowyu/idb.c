iDB for C Low-Level Functions Library
=====================================

THe KISS Efficient Structed-Column Oriented Key/Value Hierarchy data store
Library.


Directories and Files
---------------------

* README.md   -- this file.
* deps\       -- the third-party files
  * redis\    -- the emmbed iDB to redis
* src\        -- the iDB Database Engine Source.
  * idb.h     -- the iDB Database Engine head file
  * idb.c     -- the iDB Database Engine source file
  * idb_helpers.h -- the helper functions and constants for the iDB Database Engine head file
    * Constants: IDB_KEY_TYPE_NAME, IDB_VALUE_NAME, XATTR_PREFIX
    * Options: 
      *   IDBMaxPageCount: the max page size limit for the iDB, less than or equ 0 means disable it.
      *   IDBDuplicationKeyProcess: howto process the duplication keys when list subkeys
          *    dkIgnored: DO NOT add the duplication key into list.
          *    dkFixed: remove the left duplication key.
          *    dkStopped: stopped and raise error.
          *    dkReserved: add the duplication key into list too.
      *   IDBPartitionFullProcess: the available process way when putting local parition full(see iPutInXXX).
          *   dkIgnored: force to put the key in prev full dir.
          *   dkStopped: stopped and raise error.
    * Functions: 
      *   Operations(Attributes): 
          * iPutInFile/iPutInXattr, 
          * iGetInFile/iGetInXattr, 
          * iDeleteInFile/iDeleteInXattr
          * iIsExistsInFile/iIsExistsInXattr
          * iPut/iGet/iDelete/iIsExists(..., aStorageType) -- will be deprecated maybe
      *   Operations(Key): iKeyDelete, iKeyAlias, iKeyIsExists
      *   Operations(Subkey): iSubkeys, iSubkeyTotal, iSubkeyCount, iSubkeyWalk
  * isdk_utils.h  -- the general utils functions and constants.
    * Constants: O_RW_RW_R__PERMS, O_RWXRWXR_XPERMS, O_EXCL_CREAT
    * Functions: open_or_create_file, MoveDir, DeleteDir, DirectoryExists, IsDirValueExists,
               ForceDirectories, sdsJoinPathLen, UrlEncode, UrlDecode
  * isdk_xattr.h  -- the xattr helper functions and constants
    * functions: IsXattrExists, GetXattr, SetXattr
  * deps\     -- the third-party files for source


Make it
-------

        mkdir build
        cd build
        cmake ..
        make

Usages
------

All the iDB data is stored in the specified directories(aDir).

The iDB keys and values are strings, only strings. And they are encoded in UTF-8.
The iDB "key" can be hierarchical tree structure like directory. The hierarchy
delimiter of key is character "/"(path) which is called a key path.

The iDB store two kinds of data through out a key:

* Attributes
* Subkeys

A key can have multiple attributes. One of the most special attribute
should be the "value" attribute, which is used to store the value of the "key".


Limits
------

The Different File System would be different. So be care of your choose.

* the key name length           = the directory(file) name length. 
* the key path depth            = the path depth in the file system.
* the max subkeys count limits  = the max file count in a directory
  * enable the IDBMaxPageCount can exceed the limits.


