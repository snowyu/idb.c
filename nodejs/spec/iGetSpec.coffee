describe 'Get a key/value from a directory', ->
    path=require('path')
    fse=require('fs-extra')
    fs=require('fs')
    chance = new require('chance')()
    idb = require('../index')

    dataDir = path.join(__dirname, 'tmp')
    charset = '中文你好abcdefghijklmnopqrstuvwxyz关键爱英明真光明浮现美丽宝贝'
    gKey    = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    gKey2   = path.join(dataDir, 'u'+chance.string({pool: charset, length: 5}))
    fse.remove(dataDir)

    testGetInFileSync = (key, attr, value)->
        expect(idb.getInFileSync(key, attr)).toBe value
    testGetInFileAsync = (key, attr, value)->
        idb.getInFile key, attr, (err, result)->
            expect(err).toBe null
            expect(result).toBe value
            asyncSpecDone()
        asyncSpecWait()

    testPutInFileSync = (key, value, attr, partitionKeyWay)->
        expect(idb.putInFileSync(key, value, attr, partitionKeyWay)).toBe 0
        attr = '.value' unless attr
        result = fs.readFileSync(path.join(key,attr), 'utf8')
        expect(result).toBe value

    testPutInFileAsync = (key, value, attr, partitionKeyWay)->
        idb.putInFile key, value, attr, partitionKeyWay, (err)->
            expect(err).toBe null
            attr = '.value' unless attr
            result = fs.readFileSync(path.join(key,attr), 'utf8')
            expect(result).toBe value
            asyncSpecDone()
        asyncSpecWait()

    it 'should be get a key from the folder synchronous', ->
        value = chance.string({pool: charset, length: 16})
        testPutInFileSync(gKey, value)
        testGetInFileSync(gKey, null, value)

    it 'should be get a key\'s attr from the folder synchronous', ->
        attr  = chance.string({pool: charset, length: 5})
        value = chance.string({pool: charset, length: 16})
        testPutInFileSync(gKey, value, attr)
        testGetInFileSync(gKey, attr, value)

    it 'should be get a key from the folder asynchronous', ->
        value = chance.string({pool: charset, length: 16})
        testPutInFileSync(gKey, value)
        testGetInFileAsync(gKey, null, value)

    it 'should be get a key\'s attr from the folder asynchronous', ->
        attr  = chance.string({pool: charset, length: 5})
        value = chance.string({pool: charset, length: 16})
        testPutInFileSync(gKey, value, attr)
        testGetInFileAsync(gKey, attr, value)


