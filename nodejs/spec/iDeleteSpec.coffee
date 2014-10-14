#console.log("DeleteSpec")

path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

gKey    = utils.getKeyPath('deleteTest')
gKey2   = utils.getRandomKey()


describe 'Delete a key/value from a directory', ->

    utils.clearDataDir()

    it 'delete an non-exists key from the folder synchronous', ->
        utils.testDeleteInFileSync(gKey2, false)
        attr  = utils.getRandomStr(5)
        utils.testDeleteInFileSync(gKey, attr, false)

    it 'should delete an exists key from the folder synchronous', ->
        value = utils.getRandomStr(16)
        attr  = utils.getRandomStr(5)
        utils.testPutInFileSync(gKey, value, attr)
        utils.testDeleteInFileSync(gKey, attr, true)
        utils.testDeleteInFileSync(gKey, true)

    it 'delete an non-exists key from the folder asynchronous', ->
        key  = utils.getRandomKey()
        utils.testDeleteInFileAsync(key, false)

    it 'delete an non-exists key\'s attr from the folder asynchronous', ->
        key  = utils.getRandomKey()
        attr  = utils.getRandomStr(5)
        utils.testDeleteInFileAsync(key, attr, false)
    it 'should delete an exists key from the folder asynchronous', ->
        key  = utils.getRandomKey()
        value = utils.getRandomStr(16)
        utils.testPutInFileSync(key, value)
        utils.testDeleteInFileAsync(key, true)
    it 'should delete an exists key\'s attr from the folder asynchronous', ->
        key  = utils.getRandomKey()
        value = utils.getRandomStr(16)
        attr  = utils.getRandomStr(5)
        utils.testPutInFileSync(key, value, attr)
        utils.testDeleteInFileAsync(key, attr, true)

    it 'should delete an exists alias key from the folder synchronous', ->
        key    = "realKey"
        value  = utils.getRandomStr(16)
        alias  = "aliasKey"
        utils.testPutInFileSync(path.join(gKey, key), value)
        utils.testCreateKeyAliasSync(gKey, key, alias)
        utils.testDeleteInFileSync(path.join(gKey, alias), true)

    it 'should delete an exists key with aliases together synchronous', ->
        key    = "realKey"
        value  = utils.getRandomStr(16)
        alias  = "aliasKey"
        alias2 = "aliasKey2"
        alias3 = "aliasKey3"
        utils.testPutInFileSync(path.join(gKey, key), value)
        #expect(utils.isFileExists(path.join(gKey, alias))).toBe false
        #utils.testPutInFileSync(path.join(gKey, key), value)
        utils.testCreateKeyAliasSync(gKey, key, alias)
        utils.testCreateKeyAliasSync(gKey, key, alias2)
        utils.testCreateKeyAliasSync(gKey, key, alias3)
        utils.testDeleteInFileSync(path.join(gKey, key), true)
        utils.testIsExistsInFileSync(path.join(gKey, alias), false)
        utils.testIsExistsInFileSync(path.join(gKey, alias2), false)
        utils.testIsExistsInFileSync(path.join(gKey, alias3), false)

