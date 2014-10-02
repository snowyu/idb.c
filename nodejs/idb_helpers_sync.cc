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
#include "utils.h"
#include "./idb_helpers_sync.h"

using v8::Number;
using v8::String;

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
static inline const char* ToCString(const v8::String::Utf8Value& value) {
  return *value;
}


NAN_METHOD(PutInFileSync) {
  NanScope();

  int l = args.Length();
  TIDBProcesses partitionKeyWay = dkFixed;
  sds attr  = NULL;
  sds key   = NULL;
  sds value = NULL;
  Local<Value> param;
  if (l >= 4) {
      param = args[3];
      if (!param->IsUndefined() && !param->IsNull()) {
          partitionKeyWay = (TIDBProcesses) param->Uint32Value();
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
  printf("PutInFileSync:key=%s, value=%s,attr=%s, partitionKeyWay=%d\n",key, value, attr, partitionKeyWay);
#endif
  l = value ? strlen(value):0;
  int result = iPutInFile(key, value, l, attr, partitionKeyWay);
  //printf("result=%d\n", result);
  sdsfree(key);
  sdsfree(value);
  sdsfree(attr);
  NanReturnValue(NanNew<Number>(result));
}


