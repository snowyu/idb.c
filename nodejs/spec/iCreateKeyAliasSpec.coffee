path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

gKey    = utils.getKeyPath("alias")
gKey2   = utils.getRandomKey()
utils.clearDataDir()

describe 'Create a Key Alias from a directory', ->

    it 'try to create an alias from a non-exists key synchronous', ->
        key    = utils.getRandomStr(5)
        alias  = utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias, idb.IDB_ERR_KEY_NOT_EXISTS)

    it 'create an alias from a key synchronous', ->
        key    = "mainKey"
        value  = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join(gKey, key), value)
        alias  = "alias.1"+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)
        alias  = "alias.2"+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)

    it 'create an alias from a key with child path synchronous', ->
        p      = "mykey"
        key    = path.join p, "other",utils.getRandomStr(5)
        value  = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join(gKey, key), value)
        alias  = path.join p, "another", utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)

