describe 'IsExists a key/value from a directory', ->
    path=require('path')
    fse=require('fs-extra')
    fs=require('graceful-fs')
    chance = new require('chance')()
    idb = require('../index')
    utils = require('./utils')

    dataDir = path.join(__dirname, 'tmp')
    charset = '中文你好abcdefghijklmnopqrstuvwxyz关键爱英明真光明浮现美丽宝贝'
    gKey    = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    gKey2   = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    fse.remove(dataDir)


    it 'should not be exists a key from the folder synchronous', ->
        value = utils.getRandomStr(16)
        utils.testIsExistsInFileSync(gKey, false)

    it 'should not be exists a key\'s attr from the folder synchronous', ->
        attr  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testIsExistsInFileSync(gKey, attr, false)

    it 'should be exists a key from the folder synchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value)
        utils.testIsExistsInFileSync(gKey, true)

    it 'should be exists a key\'s attr from the folder synchronous', ->
        attr  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testIsExistsInFileSync(gKey, attr, true)

    it 'should not be exists a key from the folder asynchronous', ->
        key = utils.getRandomStr(16)
        utils.testIsExistsInFileAsync(key, false)

    it 'should not be exists a key\'s attr from the folder asynchronous', ->
        attr  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testIsExistsInFileAsync(gKey, attr, false)

    it 'should be exists a key from the folder asynchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value)
        utils.testIsExistsInFileAsync(gKey, true)

    it 'should be exists a key\'s attr from the folder asynchronous', ->
        attr  = utils.getRandomStr(6)
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testIsExistsInFileAsync(gKey, attr, true)

