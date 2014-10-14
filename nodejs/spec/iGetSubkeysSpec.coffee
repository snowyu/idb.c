#console.log("Subkeys")

path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

gKey    = utils.getRandomKey()


describe 'Get Subkeys', ->

    utils.clearDataDir()

    it 'try to get subkeys from a non-exists key synchronous', ->
        key    = utils.getRandomKey()
        # return -1 means error occur.
        utils.testGetSubkeyCountSync(key, -1)
        utils.testGetSubkeysSync(key)

    it 'get subkeys from a key synchronous', ->
        subkey1 = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join gKey, subkey1, value)
        subkey2 = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join gKey, subkey2, value)
        subkey3 = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join gKey, subkey3, value)
        alias   = utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, subkey3, alias)

        utils.testGetSubkeyTotalSync(gKey, 4)
        utils.testGetSubkeysSync(gKey, [subkey1,subkey2,subkey3, alias])


