/*********************************************************************
 * iDB - Key/Value NOSQL Data Storage for Node.js
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include "./idb_helpers_sync.h"
#include "./idb_helpers_async.h"

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;

// Expose synchronous and asynchronous access to our
// Estimate() function
void InitAll(Handle<Object> exports) {
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
