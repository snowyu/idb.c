/*********************************************************************
 * iDB binding for Nodejs
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include "./sync.h"
#include "./async.h"

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;

// Expose synchronous and asynchronous access to our
// Estimate() function
void InitAll(Handle<Object> exports) {
  exports->Set(NanNew<String>("putInFileSync"),
    NanNew<FunctionTemplate>(PutInFileSync)->GetFunction());

  exports->Set(NanNew<String>("putInFile"),
    NanNew<FunctionTemplate>(PutInFileAsync)->GetFunction());
}

NODE_MODULE(idb, InitAll)
