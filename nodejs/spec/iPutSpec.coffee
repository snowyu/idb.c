describe 'Put a key/value to a directory', ->
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

    testPutInFileSync = (key, value, attr, partitionKeyWay)->
        expect(idb.putInFileSync(key, value, attr, partitionKeyWay)).toBe 0
        attr = '.value' unless attr
        result = fs.readFileSync(path.join(key,attr), 'utf8')
        expect(result).toBe value
    testPutInFileAsync = (key, value, attr, partitionKeyWay)->
        idb.putInFile key, value, attr, partitionKeyWay, (err)->
            isStopped = true
            expect(err).toBe null
            attr = '.value' unless attr
            result = fs.readFileSync(path.join(key,attr), 'utf8')
            expect(result).toBe value
            asyncSpecDone()
        asyncSpecWait()
        ###
        isStopped = false
        vResult = 1
        runs ->
            isStopped = false
            idb.putInFile key, value, attr, partitionKeyWay, (err)->
                isStopped = true
                vResult = err
        waitsFor ->
            return isStopped
        , "Waiting for putInFile Async result", 1000
        runs ->
            expect(isStopped).toBe true
            expect(vResult).toBe null
            attr = '.value' unless attr
            result = fs.readFileSync(path.join(key,attr), 'utf8')
            expect(result).toBe value
        ###

    it 'should be add a key to the folder synchronous', ->
        value = chance.string({pool: charset, length: 16})
        testPutInFileSync(gKey, value)

    it 'should be update a key to the folder synchronous', ->
        value = chance.string({pool: charset, length: 20})
        testPutInFileSync(gKey, value)

    it 'should be update a key attr to the folder synchronous', ->
        value = chance.string({pool: charset, length: 20})
        attr = chance.string({pool: charset, length: 10})
        testPutInFileSync(gKey, value, attr)

    it 'should be add a key to the folder asynchronous', ->
        value = chance.string({pool: charset, length: 16})
        testPutInFileAsync(gKey2, value)

    it 'should be update a key to the folder asynchronous', ->
        value = chance.string({pool: charset, length: 20})
        testPutInFileAsync(gKey2, value)

    it 'should be update a key attr to the folder asynchronous', ->
        value = chance.string({pool: charset, length: 20})
        attr = chance.string({pool: charset, length: 10})
        testPutInFileAsync(gKey2, value, attr)

