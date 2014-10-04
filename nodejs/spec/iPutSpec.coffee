describe 'Put a key/value to a directory', ->
    path   = require('path')
    fse    = require('fs-extra')
    fs     = require('graceful-fs')
    chance = new require('chance')()
    idb    = require('../index')
    utils  = require('./utils')

    dataDir = path.join(__dirname, 'tmp')
    charset = '中文你好abcdefghijklmnopqrstuvwxyz关键爱英明真光明浮现美丽宝贝'
    gKey    = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    gKey2   = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    fse.remove(dataDir)


    it 'should be add a key to the folder synchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value)

    it 'should be update a key to the folder synchronous', ->
        value = utils.getRandomStr(20)
        utils.testPutInFileSync(gKey, value)

    it 'should be update a key attr to the folder synchronous', ->
        value = utils.getRandomStr(20)
        attr = utils.getRandomStr(10)
        utils.testPutInFileSync(gKey, value, attr)

    it 'should be add a key to the folder asynchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileAsync(gKey2, value)

    it 'should be update a key to the folder asynchronous', ->
        value = utils.getRandomStr(20)
        utils.testPutInFileAsync(gKey2, value)

    it 'should be update a key attr to the folder asynchronous', ->
        value = utils.getRandomStr(20)
        attr = utils.getRandomStr(10)
        utils.testPutInFileAsync(gKey2, value, attr)

    it 'should be a creation error when the same file exists', ->
         # test the same name file exists for the key
        key = path.join(gKey, "2")
        fse.ensureFileSync(key)
        expect idb.putInFileSync(key)
            .toBe idb.IDB_ERR_DUP_FILE_NAME

