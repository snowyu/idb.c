idb  = require('../index')
fs   = require('graceful-fs')
path = require('path')
chance = new require('chance')()

IDB_VALUE_NAME = idb.IDB_VALUE_NAME
charset = '中文你好abcdefghijklmnopqrstuvwxyz关键爱英明真光明浮现美丽宝贝'

utils =
    getRandomStr: (aLen, aCharset)->
        if not aCharset?
            aCharset = charset
        chance.string({pool: aCharset, length: aLen})

    testIncrByInFileSync : (key, value, attr, partitionKeyWay, expectedResult)->
        erred = false
        if not expectedResult?
            n = idb.getInFileSync(key, attr)
            n = parseInt(n)
            if isNaN(n)
                n = 0
            incr = value
            if not incr?
                incr = 1
            expectedResult = n + incr
            #console.log("incr=",incr," n=",n)
        else if typeof expectedResult == "object"
            erred = true
        if not erred
            expect(idb.incrByInFileSync(key, value, attr, partitionKeyWay)).toBe expectedResult
        else
            bar = () ->
                idb.incrByInFileSync(key, value, attr, partitionKeyWay)
            expect(bar).toThrow(expectedResult)

    testIncrByInFileAsync : (key, value, attr, partitionKeyWay, expectedResult)->
        erred = false
        if not expectedResult?
            n = idb.getInFileSync(key, attr)
            n = parseInt(n)
            if isNaN(n)
                n = 0
            incr = value
            if not incr?
                incr = 1
            expectedResult = n + incr
            #console.log("incr.async=",incr," n=",n)
        else if typeof expectedResult == "object"
            erred = true
        idb.incrByInFile key, value, attr, partitionKeyWay, (err, result)->
            if erred
                expect(err).toBe expectedResult
            else
                expect(err).toBe null
                expect(result).toBe expectedResult
                utils.testGetInFileSync(key, attr, expectedResult.toString())
            asyncSpecDone()
        asyncSpecWait()
        
    testIsExistsInFileSync : (key, attr, expectedResult)->
        if not expectedResult?
            expectedResult = attr
            attr = null
        expect(idb.isExistsInFileSync(key, attr)).toBe expectedResult

    testIsExistsInFileAsync : (key, attr, expectedResult)->
        if not expectedResult?
            expectedResult = attr
            attr = null
        idb.isExistsInFile key, attr, (err, result)->
            expect(err).toBe null
            expect(result).toBe expectedResult
            asyncSpecDone()
        asyncSpecWait()

    testGetInFileSync : (key, attr, expectedResult)->
        if not expectedResult?
            expectedResult = attr
            attr = null
        expect(idb.getInFileSync(key, attr)).toBe expectedResult

    testGetInFileAsync : (key, attr, expectedResult)->
        if not expectedResult?
            expectedResult = attr
            attr = null
        idb.getInFile key, attr, (err, result)->
            expect(err).toBe null
            expect(result).toBe expectedResult
            asyncSpecDone()
        asyncSpecWait()

    testPutInFileSync : (key, value, attr, partitionKeyWay, expectedResult)->
        if not expectedResult?
            expectedResult = idb.IDB_OK
        expect(idb.putInFileSync(key, value, attr, partitionKeyWay)).toBe expectedResult
        attr = IDB_VALUE_NAME unless attr
        if expectedResult == idb.IDB_OK and value?
            result = fs.readFileSync(path.join(key,attr), 'utf8')
            expect(result).toBe value
            utils.testIsExistsInFileSync(key, attr, true)

    testPutInFileAsync : (key, value, attr, partitionKeyWay, expectedResult)->
        if not expectedResult?
            expectedResult = idb.IDB_OK
        idb.putInFile key, value, attr, partitionKeyWay, (err)->
            if expectedResult == idb.IDB_OK
                expect(err).toBe null
            else
                expect(err).not.toBe null
                expect(err.code).toBe expectedResult
            attr = IDB_VALUE_NAME unless attr
            if expectedResult == idb.IDB_OK and value?
                result = fs.readFileSync(path.join(key,attr), 'utf8')
                expect(result).toBe value
                utils.testIsExistsInFileSync(key, attr, true)
            asyncSpecDone()
        asyncSpecWait()

module.exports = utils

