path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

dataDir = path.join(__dirname, 'tmp')
gKey    = path.join(dataDir, 'u'+utils.getRandomStr(5))
gKey2   = path.join(dataDir, 'u'+utils.getRandomStr(5))
fse.remove(dataDir)

describe 'Delete a key/value from a directory', ->

    it 'delete an non-exists key from the folder synchronous', ->
        utils.testDeleteInFileSync(gKey, false)
        attr  = utils.getRandomStr(5)
        utils.testDeleteInFileSync(gKey, attr, false)

    it 'should delete an exists key from the folder synchronous', ->
        value = utils.getRandomStr(16)
        attr  = utils.getRandomStr(5)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testDeleteInFileSync(gKey, attr, true)
        utils.testDeleteInFileSync(gKey, true)

    it 'delete an non-exists key from the folder asynchronous', ->
        key  = utils.getRandomStr(5)
        utils.testDeleteInFileAsync(key, false)
    it 'delete an non-exists key\'s attr from the folder asynchronous', ->
        key  = utils.getRandomStr(5)
        attr  = utils.getRandomStr(5)
        utils.testDeleteInFileAsync(key, attr, false)
    it 'should delete an exists key from the folder asynchronous', ->
        key  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(key, value)
        utils.testDeleteInFileAsync(key, true)
    it 'should delete an exists key\'s attr from the folder asynchronous', ->
        key  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        attr  = utils.getRandomStr(5)
        utils.testPutInFileSync(key, value, attr)
        utils.testDeleteInFileAsync(key, attr, true)

