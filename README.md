iDB for C
==============

a simple and efficient Structed-Column Oriented Key/Value Hierarchy Database Engine for 
C Language.


Directories and Files
=====================

README.md   -- this file.
deps\       -- the third-party files
  redis\    -- the emmbed iDB to redis
src\        -- the iDB Database Engine Source.
  idb.h     -- the iDB Database Engine head file
  idb.c     -- the iDB Database Engine source file
  idb_helpers.h -- the helper functions and constants for the iDB Database Engine head file
    Constants: IDB_KEY_TYPE_NAME, IDB_VALUE_NAME, XATTR_PREFIX
    Functions: IsDirValueExists, GetDirValue, SetDirValue
  isdk_utils.h  -- the general utils functions and constants.
    Constants: O_RW_RW_R__PERMS, O_RWXRWXR_XPERMS, O_EXCL_CREAT
    Functions: open_or_create_file, MoveDir, DeleteDir, DirectoryExists, IsDirValueExists,
               ForceDirectories, sdsJoinPathLen, UrlEncode, UrlDecode
  isdk_xattr.h  -- the xattr helper functions and constants
    functions: IsXattrExists, GetXattr, SetXattr
  deps\     -- the third-party files for source



