#console.log("Append")

path   = require('path')
fse    = require('fs-extra')
fs     = require('graceful-fs')
chance = new require('chance')()
idb    = require('../index')
utils  = require('./utils')

gKey    = utils.getKeyPath("appendKey")
gKey2   = utils.getRandomKey()


## xit will be called after all too.
utils.clearDataDir()

describe 'Append a key/value to a directory', ->

    it 'should be add a key to the folder synchronous', ->
        value = utils.getRandomStr(16)
        utils.testAppendInFileSync(gKey, value)

    xit 'should be update a key to the folder synchronous', ->
        value = utils.getRandomStr(20)
        utils.testAppendInFileSync(gKey, value)

    xit 'should be update a key attr to the folder synchronous', ->
        value = utils.getRandomStr(20)
        attr = utils.getRandomStr(10)
        utils.testAppendInFileSync(gKey, value, attr)

    xit 'should be add a key to the folder asynchronous', ->
        value = utils.getRandomStr(16)
        utils.testAppendInFileAsync(gKey2, value)

    xit 'should be update a key to the folder asynchronous', ->
        value = utils.getRandomStr(20)
        utils.testAppendInFileAsync(gKey2, value)

    xit 'should be update a key attr to the folder asynchronous', ->
        value = utils.getRandomStr(20)
        attr = utils.getRandomStr(10)
        utils.testAppendInFileAsync(gKey2, value, attr)

    xit 'should be a creation error when the same file exists', ->
         # test the same name file exists for the key
        key = path.join(gKey, "2")
        fse.ensureFileSync(key)
        expect idb.appendInFileSync(key)
            .toBe idb.IDB_ERR_DUP_FILE_NAME

