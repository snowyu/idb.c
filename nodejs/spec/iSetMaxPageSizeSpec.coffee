path   = require('path')
fse    = require('fs-extra')
fs     = require('graceful-fs')
chance = new require('chance')()
idb    = require('../index')
utils  = require('./utils')

dataDir = path.join(__dirname, 'tmp')
gKey    = path.join(dataDir, utils.getRandomStr(5))

describe 'adjust iDB MaxPageSize options', ->

    fse.remove(dataDir)

    it 'should be two dirs when set the MaxPageSize is 2', ->
        expect(idb.setMaxPageSize(2)).toBe true
        utils.testPutInFileSync(path.join(gKey, "1"))
        utils.testPutInFileSync(path.join(gKey, "2"))

        key = path.join(gKey, "3")
        utils.testPutInFileSync(key, null, null, idb.dkStopped, idb.IDB_ERR_PART_FULL)
        utils.testIsExistsInFileSync(key, false)
        #result = idb.putInFileSync(key)
        #expect(result).toBe idb.IDB_ERR_PART_FULL
        expect(fs.existsSync(key)).toBe false
        utils.testPutInFileAsync(key, null, null, idb.dkStopped, idb.IDB_ERR_PART_FULL)

    it 'can force to save the key even though the partition is full', ->
        #dkIgnore: force to save the key even though the partition is full.
        key = path.join(gKey, "3")
        utils.testPutInFileSync(key, utils.getRandomStr(20), null, idb.dkIgnored)
        utils.testIsExistsInFileSync(key, true)

        key = path.join(gKey, "5")
        utils.testPutInFileAsync(key, utils.getRandomStr(20), null, idb.dkIgnored)
        #result = idb.putInFileSync(key, null, null, idb.dkIgnored)
        #expect(result).toBe idb.IDB_OK, 'the putInFileSync is not ok'
        #expect(fs.existsSync(key)).toBe true, "create the key folder is failed"

    it 'should be partition key now for the partition full.', ->
        expect idb.putInFileSync(path.join(gKey, "13"))
            .toBe idb.IDB_OK
        key = path.join(gKey, ".1", "3")
        expect(fs.existsSync(key)).toBe true
        expect idb.putInFileSync(path.join(gKey, "14"))
            .toBe idb.IDB_OK
        expect(fs.existsSync(key)).toBe true
        expect idb.putInFileSync(path.join(gKey, "15"))
            .toBe idb.IDB_ERR_PART_FULL
        key = path.join(gKey, ".1", "5")
        expect(fs.existsSync(key)).toBe false


