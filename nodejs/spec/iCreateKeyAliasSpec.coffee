#console.log("keyalias")

path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

gKey    = utils.getKeyPath("alias")
gKey2   = utils.getKeyPath("alias2")


describe 'Create a Key Alias from a directory', ->

    #beforeEach ->
    utils.clearDataDir()

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

    it 'should not create a duplication alias key synchronous', ->
        key    = "mainKey2"
        value  = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join(gKey, key), value)
        alias  = "alias.1"+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)
        utils.testCreateKeyAliasSync(gKey, key, alias, idb.IDB_ERR_KEY_ALREADY_EXISTS)

    it 'create an alias from a key with child path synchronous', ->
        p      = "mykey"
        key    = path.join p, "other",utils.getRandomStr(5)
        value  = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join(gKey, key), value)
        alias  = path.join p, "another", utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)

    it 'create an alias from a partitional key synchronous', ->
        expect(idb.setMaxPageSize(2)).toBe true
        rkey = path.join(gKey2, "first")
        utils.testPutInFileSync(path.join(rkey, "1"))
        utils.testPutInFileSync(path.join(rkey, "2"))

        partkey = path.join(rkey, "13")
        expect idb.putInFileSync(partkey)
            .toBe idb.IDB_OK
        key = path.join(rkey, ".1", "3")
        expect(fs.existsSync(key)).toBe true
        alias  = "alias.1."+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey2, path.join("first","13"), alias)

        alias  = "alias.2."+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey2, path.join("first","13"), alias)

    it 'should not create a duplication alias from partitional key synchronous', ->
        key    = "dupKey"
        value  = utils.getRandomStr(5)
        utils.testPutInFileSync(path.join(gKey, key), value)
        alias  = "alias.1"+utils.getRandomStr(5)
        utils.testCreateKeyAliasSync(gKey, key, alias)
        utils.testCreateKeyAliasSync(gKey, key, alias, idb.IDB_ERR_KEY_ALREADY_EXISTS)

    #it "restore the MaxPageSize off", ->
    #    expect(idb.setMaxPageSize(0)).toBe true

