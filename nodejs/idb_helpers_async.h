/*********************************************************************
 * iDB - Key/Value NOSQL Data Storage for Node.js
 *
 * Copyright (c) 2014 Riceball LEE
 *
 * MIT License
 ********************************************************************/

#ifndef IDB_HELPERS_ASYNC_H_
#define IDB_HELPERS_ASYNC_H_

#include <node.h>
#include <nan.h>

NAN_METHOD(PutInFileAsync);
NAN_METHOD(GetInFileAsync);
NAN_METHOD(IsExistsInFileAsync);
NAN_METHOD(IncrByInFileAsync);
NAN_METHOD(DeleteInFileAsync);
NAN_METHOD(AppendInFileAsync);

#endif  // IDB_HELPERS_ASYNC_H_
