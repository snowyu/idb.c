describe 'adjust iDB MaxPageSize options', ->
    path=require('path')
    fse=require('fs-extra')
    fs=require('fs')
    chance = new require('chance')()
    idb = require('../index')

    dataDir = path.join(__dirname, 'tmp')
    gKey    = path.join(dataDir, 'mykey')
    #fse.remove(dataDir)

    it 'should be two dirs when set the MaxPageSize is 2', ->
        expect(idb.setMaxPageSize(2)).toBe true
        expect(idb.putInFileSync(path.join(gKey, "1"))).toBe 0
        expect(idb.putInFileSync(path.join(gKey, "2"))).toBe 0
        result = idb.putInFileSync(path.join(gKey, "3"))
        expect(result).toBe idb.IDB_ERR_PART_FULL
        idb.putInFileSync(path.join(gKey, "13"))
        idb.putInFileSync(path.join(gKey, "14"))



