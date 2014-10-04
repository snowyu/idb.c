/*********************************************************************
 * iDB - Key/Value NOSQL Data Storage for Node.js
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include <string>
#include <errno.h>
#include "idb_helpers.h"
#include "utils.h"
#include "./idb_helpers_sync.h"

using namespace std;
using namespace v8;

//using v8::Number;
//using v8::String;

// synchronous access to the functions
// create a v8:array:
// v8::Local<v8::Array> myArray = NanNew<v8::Array>(arrayCount);
// for(unsigned int i = 0; i < arrayCount; i++) {
//   myArray->Set(i, NanNew<String>("mystring"));
// }
/*
v8::Handle<v8::Value> obj(args[0]);

 if(obj->IsArray()){

     v8::Local<v8::Array> arr= v8::Local<v8::Array>::Cast(args[0]);
     v8::String::Utf8Value key(arr->Get(0));
     v8::String::Utf8Value value(arr->Get(1));

 }

Local<Array> arr= Local<Array>::Cast(args[0]);
printf("size %d\n",arr->Length());
Local<Value> item = arr->Get(0);

 */

//the same as the *NanUtf8String:
/*
static inline const char* ToCString(const v8::String::Utf8Value& value) {
  return *value;
}
*/

NAN_METHOD(ErrorStrSync) {
  NanScope();
  if (args.Length() >= 1) {
      Local<Value> param;
      param = args[0];
      //if (!param->IsUndefined() && !param->IsNull()) {
      if (param->IsNumber()) {
          int errNo = param->Uint32Value();
          const char* s = idbErrorStr(errNo);
          Local<Value> result = String::New(s);
          NanReturnValue(result);
      }
  }
  else {
      //
      NanThrowTypeError("where my errno argument value? u type nothing?");
  }
  NanReturnUndefined();
}
NAN_METHOD(SetMaxPageSizeSync) {
  NanScope();
  bool result = false;
  if (args.Length() >= 1) {
      Local<Value> param;
      param = args[0];
      //if (!param->IsUndefined() && !param->IsNull()) {
      if (param->IsNumber()) {
          IDBMaxPageCount = param->Uint32Value();
          result = true;
      }
  }
  else {
      //
      NanThrowTypeError("where my MaxPageSize argument value? u type nothing?");
      NanReturnUndefined();
  }
  NanReturnValue(NanNew<Boolean>(result));
}


NAN_METHOD(PutInFileSync) {
  NanScope();

  int l = args.Length();
  TIDBProcesses partitionFull = dkStopped;
  sds attr  = NULL;
  sds key   = NULL;
  sds value = NULL;
  Local<Value> param;
  if (l >= 4) {
      param = args[3];
      if (param->IsNumber()) {
          partitionFull = (TIDBProcesses) param->Uint32Value();
      }
  }
  if (l >= 3) {
      param = args[2];
      if (!param->IsUndefined() && !param->IsNull()) {
          attr = sdsnew(*NanUtf8String(param));
          //string s = ValueToString(args[2]);
          //attr = s.c_str();
          //String::Utf8Value v(args[2]);
          //attr  = ToCString(v);
          //printf("attr=%s, %x\n", attr, attr);
      }
  }
  if (l >= 2) {
      param = args[1];
      if (!param->IsUndefined() && !param->IsNull()) {
          value = sdsnew(*NanUtf8String(param));
          //string s = ValueToString(args[1]);
          //value = s.c_str();
          //printf("value=%s\n", value);
          //value = *NanUtf8String(args[1]);
      }
  }
  if (l >= 1) {
      param = args[0];
      if (!param->IsUndefined() && !param->IsNull()) {
          //i found zmalloc can not alloc memory and copy the *NanUtf8String(args[1]) directly.
          //only convert to std:string zmalloc can work now.
          //I found the reason now:
          //  const char *s = *NanUtf8String(args[0]);
          //  printf("%s\n", s);
          //  const char *s2 = *NanUtf8String(args[1]);
          //  printf("%s, %s\n", s, s2);
          //  the s content is changed and equ s2!!!
          //key   = *NanUtf8String(args[0]);
          //string s = ValueToString(args[0]);
          //key = s.c_str();
          //this is would error too:
          //      const char* s= *NanUtf8String(args[0]);
          //      printf("async key=%s\n", s);
          //      key = sdsnew(s);
          //      printf("async key=%s\n", key); the key is no any or wrong content!!!
          //      only this is correction:
          key = sdsnew(*NanUtf8String(param));
      }
  }

  if (!key)
  {
      NanThrowTypeError("where my key argument value? u type nothing?");
      NanReturnUndefined();
  }
  //printf("%s\n",key);
  //printf("args.length=%lu, attr=%x,%s\n",l, attr,attr);
  if (key == NULL) {
      NanThrowTypeError("why can not malloc space for keypath?");
      NanReturnUndefined();
  }
#ifdef DEBUG
  printf("PutInFileSync:key=%s, value=%s,attr=%s, partitionFull=%d\n",key, value, attr, partitionFull);
#endif
  l = value ? strlen(value):0;
  int result = iPutInFile(key, value, l, attr, partitionFull);
  //printf("result=%d\n", result);
  sdsfree(key);
  sdsfree(value);
  sdsfree(attr);
  NanReturnValue(NanNew<Number>(result));
}

NAN_METHOD(GetInFileSync) {
  NanScope();

  int l = args.Length();
  sds attr  = NULL;
  sds key   = NULL;
  Local<Value> param;
  if (l >= 1) {
      param = args[0];
      if (!param->IsUndefined() && !param->IsNull()) {
          key = sdsnew(*NanUtf8String(param));
      }
  }

  if (!key)
  {
      NanThrowTypeError("where my key argument value? u type nothing?");
      NanReturnUndefined();
  }
  if (l >= 2) {
      param = args[1];
      if (!param->IsUndefined() && !param->IsNull()) {
          attr = sdsnew(*NanUtf8String(param));
      }
  }
  sds value = iGetInFile(key, attr);
#ifdef DEBUG
  printf("GetInFileSync: value=%s, key=%s, attr=%s\n", value, key, attr);
#endif
  Local<Value> result;
  if (value)
      result = String::New(value);
  else
      result = NanUndefined();

  //result = String::New(value);
  sdsfree(key);sdsfree(attr);sdsfree(value);

  NanReturnValue(result);
}

NAN_METHOD(IsExistsInFileSync) {
  NanScope();

  int l = args.Length();
  sds attr  = NULL;
  sds key   = NULL;
  Local<Value> param;
  if (l >= 1) {
      param = args[0];
      if (param->IsString()) {
          key = sdsnew(*NanUtf8String(param));
      }
  }

  if (!key)
  {
      NanThrowTypeError("where my key value? u type nothing?");
      NanReturnUndefined();
  }
  if (l >= 2) {
      param = args[1];
      if (param->IsString()) {
          attr = sdsnew(*NanUtf8String(param));
      }
  }
  bool result = iIsExistsInFile(key, attr);

  sdsfree(key);sdsfree(attr);

  NanReturnValue(NanNew<Boolean>(result));
}

NAN_METHOD(IncrByInFileSync) {
  NanScope();

  int l = args.Length();
  sds attr  = NULL;
  sds key   = NULL;
  int64_t value = 1;
  bool vDone = false;
  TIDBProcesses partitionFull = dkStopped;
  Local<Value> param;
  if (l >= 1) {
      param = args[0];
      if (param->IsString()) {
          key = sdsnew(*NanUtf8String(param));
      }
  }
  if (!key)
  {
      NanThrowTypeError("where my key value? u type nothing?");
      NanReturnUndefined();
  }
  if (l >= 2) {
      param = args[1];
      if (param->IsNumber()) {
          value = param->Int32Value();
          vDone = true;
      }
      else if (param->IsString()) {
          attr = sdsnew(*NanUtf8String(param));
      }
  }
  if (l >= 3) {
      param = args[2];
      if (param->IsString()) {
          attr = sdsnew(*NanUtf8String(param));
      }
      else if (param->IsNumber()) {
          if (vDone)
              partitionFull = (TIDBProcesses) param->Uint32Value();
          else
              value = param->Int32Value();
      }
  }
  if (l >= 4) {
      param = args[3];
      if (param->IsNumber()) {
          partitionFull = (TIDBProcesses) param->Uint32Value();
      }
  }
#ifdef DEBUG
printf("IncrByInFileSync:key=%s,value=%lld,attr=%s,partitionFull=%d\n", key,value,attr,partitionFull);
#endif
  int64_t result = iIncrByInFile(key, value, attr, partitionFull);
#ifdef DEBUG
  printf("IncrByInFileSync:result=%lld,errno=%d\n", result, errno);
#endif
  sdsfree(key);sdsfree(attr);
  if (errno) {
      NanThrowError(idbErrorStr(errno), errno);
      NanReturnUndefined();
  }

  NanReturnValue(NanNew<Number>(result));
}

