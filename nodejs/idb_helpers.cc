/*********************************************************************
 * iDB - Key/Value NOSQL Data Storage for Node.js
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include "idb_helpers.h"
#include "./idb_helpers_sync.h"
#include "./idb_helpers_async.h"

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;
using v8::Number;

// Expose synchronous and asynchronous access to our
// Estimate() function
void InitAll(Handle<Object> exports) {
  exports->Set(NanNew<String>("dkStopped"),
    NanNew<Number>(dkStopped));
  exports->Set(NanNew<String>("dkIgnored"),
    NanNew<Number>(dkStopped));
  exports->Set(NanNew<String>("IDB_OK"),
    NanNew<Number>(IDB_OK));
  exports->Set(NanNew<String>("IDB_ERR_PART_FULL"),
    NanNew<Number>(IDB_ERR_PART_FULL));
  exports->Set(NanNew<String>("IDB_ERR_PART_DUP_KEY"),
    NanNew<Number>(IDB_ERR_PART_DUP_KEY));
  exports->Set(NanNew<String>("IDB_ERR_INVALID_UTF8"),
    NanNew<Number>(IDB_ERR_INVALID_UTF8));

  exports->Set(NanNew<String>("errorStr"),
    NanNew<FunctionTemplate>(ErrorStrSync)->GetFunction());
  exports->Set(NanNew<String>("setMaxPageSize"),
    NanNew<FunctionTemplate>(SetMaxPageSizeSync)->GetFunction());

  exports->Set(NanNew<String>("putInFileSync"),
    NanNew<FunctionTemplate>(PutInFileSync)->GetFunction());
  exports->Set(NanNew<String>("putInFile"),
    NanNew<FunctionTemplate>(PutInFileAsync)->GetFunction());

  exports->Set(NanNew<String>("getInFileSync"),
    NanNew<FunctionTemplate>(GetInFileSync)->GetFunction());
  exports->Set(NanNew<String>("getInFile"),
    NanNew<FunctionTemplate>(GetInFileAsync)->GetFunction());
}

NODE_MODULE(idb, InitAll)
