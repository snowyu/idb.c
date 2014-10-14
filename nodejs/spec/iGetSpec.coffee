#console.log("GetSpec")

path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

gKey    = utils.getRandomKey()
gKey2   = utils.getRandomKey()

describe 'Get a key/value from a directory', ->

    utils.clearDataDir()

    it 'should be get a key from the folder synchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value)
        utils.testGetInFileSync(gKey, value)

    it 'should be get a key\'s attr from the folder synchronous', ->
        attr  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testGetInFileSync(gKey, attr, value)

    it 'should be get a key from the folder asynchronous', ->
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value)
        utils.testGetInFileAsync(gKey, value)

    it 'should be get a key\'s attr from the folder asynchronous', ->
        attr  = utils.getRandomStr(5)
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testGetInFileAsync(gKey, attr, value)


