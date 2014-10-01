/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2014 NAN contributors
 *
 * MIT License <https://github.com/rvagg/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <node.h>
#include <nan.h>
#include "idb_helpers.h"
#include "./sync.h"

using v8::Number;

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
NAN_METHOD(PutInFileSync) {
  NanScope();

  int l = args.Length();
  TIDBProcesses partitionKeyWay = dkFixed;
  char *attr  = NULL;
  const char *key   = NULL;
  char *value = NULL;
  if (l >= 4) partitionKeyWay = (TIDBProcesses) args[3]->Uint32Value();
  if (l >= 3) attr  = *NanUtf8String(args[2]);
  if (l >= 2) value = *NanUtf8String(args[1]);
  if (l >= 1) {
      key   = *NanUtf8String(args[0]);
  }
  else {
      NanThrowTypeError("where my key argument? u type nothing?");
      NanReturnUndefined();
  }
  printf("%s\n",key);
  printf("%d\n",strlen(key));
  sds s = sdsnew(key);
  if (s == NULL) {
      NanThrowTypeError("why can not malloc space for keypath?");
      NanReturnUndefined();
  }
  printf("OKsds:%d, %s, key=%s\n",l, key, s);
  l = value ? strlen(value):0;
  int result = iPutInFile(s, value, l, attr, partitionKeyWay);
  printf("result=%d\n", result);
  sdsfree(s);
  NanReturnValue(NanNew<Number>(result));
}
