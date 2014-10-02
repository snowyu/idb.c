/*********************************************************************
 * iDB - Key/Value NOSQL Data Storage for Node.js
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include "utils.h"
#include "idb_helpers.h"
#include "./idb_helpers_async.h"

using v8::Function;
using v8::Local;
using v8::Null;
using v8::Number;
using v8::Value;

class PutInFileWorker : public NanAsyncWorker {
 public:
  PutInFileWorker(NanCallback *callback, sds key,
      sds value,
      sds attr,
      TIDBProcesses partitionKeyWay)
    : NanAsyncWorker(callback), key(key), value(value), attr(attr), partitionKeyWay(partitionKeyWay), result(0) {}
  ~PutInFileWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    int l = value ? sdslen(value) : 0;
#ifdef DEBUG
    printf("PutInFileAsync:key=%s, value=%s, attr=%s, partitionKeyWay=%d\n",key, value, attr, partitionKeyWay);
#endif
    result = iPutInFile(key, value, l, attr, partitionKeyWay);
    sdsfree(key);sdsfree(value);sdsfree(attr);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

    Local<Value> error;
    //String error = NULL; 
    
    if (result == 0) {
        error = NanNull();
    }
    else if (result == -1) {
        error = NanError("The same filename is exists, can not create folder to keep value", result);
    }
    else {
        error = NanError(idbErrorStr(result), result);
    }
    
    Local<Value> argv[] = {
        error
      //, NanNew<Number>(result)
    };

    //callback->Call(2, argv);
    callback->Call(1, argv);
  }

 private:
  sds key;
  sds attr;
  sds value;
  TIDBProcesses partitionKeyWay;
  int result;
};

// Asynchronous access to the function
NAN_METHOD(PutInFileAsync) {
  NanScope();

  int l = args.Length();
  TIDBProcesses partitionKeyWay = dkFixed;
  sds attr  = NULL;
  sds key   = NULL;
  sds value = NULL;
  Local<Value> param;
  if (l >= 5) {
      param = args[3];
      if (!param->IsUndefined() && !param->IsNull()) {
          partitionKeyWay = (TIDBProcesses) param->Uint32Value();
      }
  }
  if (l >= 4) {
      param = args[2];
      if (!param->IsUndefined() && !param->IsNull()) {
        attr = sdsnew(*NanUtf8String(param));
      }
  }
  if (l >= 3) {
      param = args[1];
      if (!param->IsUndefined() && !param->IsNull()) {
          value = sdsnew(*NanUtf8String(param));
      }
  }
  if (l >= 2) {
      param = args[0];
      if (!param->IsUndefined() && !param->IsNull()) {
          //this would error:
          //const char* s= *NanUtf8String(args[0]);
          //printf("async key=%s\n", s);
          //key = sdsnew(s);
          //printf("async key=%x,%s\n", key,key);
          key = sdsnew(*NanUtf8String(param));
        }
  }
  if (!key || l <= 1) {
      NanThrowTypeError("where my key argument value? u type nothing? or missing the callback?");
      NanReturnUndefined();
  }

  NanCallback *callback = new NanCallback(args[l-1].As<Function>());

  NanAsyncQueueWorker(new PutInFileWorker(callback, key, value, attr, partitionKeyWay));
  NanReturnUndefined();
}


class GetInFileWorker : public NanAsyncWorker {
 public:
  GetInFileWorker(NanCallback *callback, sds key, sds attr)
    : NanAsyncWorker(callback), key(key), attr(attr), value(NULL) {}
  ~GetInFileWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    value = iGetInFile(key, attr);
#ifdef DEBUG
    printf("GetInFileAsync:key=%s, value=%s, attr=%s\n",key, value, attr);
#endif
    sdsfree(key);sdsfree(attr);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

    Local<Value> result;
    if (value) {
        result = String::New(value);
        sdsfree(value);
    }
    else
        result = NanUndefined();
    
    Local<Value> argv[] = {
        NanNull()
      , result
    };

    callback->Call(2, argv);
  }

 private:
  sds key;
  sds attr;
  sds value;
};

// Asynchronous access to the function
NAN_METHOD(GetInFileAsync) {
  NanScope();

  int l = args.Length();
  sds attr  = NULL;
  sds key   = NULL;
  Local<Value> param;
  if (l >= 3) {
      param = args[1];
      if (!param->IsUndefined() && !param->IsNull()) {
          attr = sdsnew(*NanUtf8String(param));
      }
  }
  if (l >= 2) {
      param = args[0];
      if (!param->IsUndefined() && !param->IsNull()) {
          key = sdsnew(*NanUtf8String(param));
      }
  }
  if (!key || l <= 1) {
      NanThrowTypeError("where my key argument value? u type nothing? or missing the callback?");
      NanReturnUndefined();
  }

  NanCallback *callback = new NanCallback(args[l-1].As<Function>());

  NanAsyncQueueWorker(new GetInFileWorker(callback, key, attr));
  NanReturnUndefined();
}
