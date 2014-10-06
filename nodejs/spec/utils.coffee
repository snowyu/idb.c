idb  = require('../index')
fs   = require('graceful-fs')
fse  = require('fs-extra')
path = require('path')
_    = require('lodash')
chance = new require('chance')()

IDB_VALUE_NAME = idb.IDB_VALUE_NAME
charset = '中文你好abcdefghijklmnopqrstuvwxyz关键爱英明真光明浮现美丽宝贝'

utils =
    dataDir: path.join(__dirname, 'tmp')
    clearDataDir: ->
        fse.remove(utils.dataDir)
    getRandomKey: (aLen, aCharset)->
        if not aCharset?
            aCharset = charset
        aLen = 5 unless _.isNumber aLen
        path.join utils.dataDir, "key"+chance.string({pool: aCharset, length: aLen})
    getRandomStr: (aLen, aCharset)->
        if not aCharset?
            aCharset = charset
        chance.string({pool: aCharset, length: aLen})

    testGetSubkeyCountSync: (key, pattern, expectedResult)->
        if not expectedResult?
            expectedResult = pattern
            pattern = undefined
        expect(idb.getSubkeyCountSync(key, pattern)).toBe expectedResult

    testGetSubkeysSync : (key, pattern, skipCount, count, duplicationKeyProcess, expectedResult)->
        if not expectedResult?
            if _.isArray duplicationKeyProcess
                expectedResult = duplicationKeyProcess
                duplicationKeyProcess = undefined
            else if _.isArray count
                expectedResult = count
                count = undefined
            else if _.isArray skipCount
                expectedResult = skipCount
                skipCount = undefined
            else if _.isArray pattern
                expectedResult = pattern
                pattern = undefined
            expectedResult = _.sortBy expectedResult if _.isArray expectedResult
            result = idb.getSubkeysSync(key, pattern, skipCount, count, duplicationKeyProcess)
            result = _.sortBy result if _.isArray result
        expect(result).toEqual expectedResult

    testGetAttrsInFileSync : (key, pattern, skipCount, count, expectedResult)->
        if not expectedResult?
            if _.isArray count
                expectedResult = count
                count = undefined
            else if _.isArray skipCount
                expectedResult = skipCount
                skipCount = undefined
            else if _.isArray pattern
                expectedResult = pattern
                pattern = undefined
            expectedResult = _.sortBy expectedResult if _.isArray expectedResult
            result = idb.getAttrsInFileSync(key, pattern, skipCount, count)
            result = _.sortBy result if _.isArray result
        expect(result).toEqual expectedResult

    testGetAttrCountInFileSync : (key, pattern, expectedResult)->
        if not expectedResult?
            expectedResult = pattern
            pattern = undefined
        expect(idb.getAttrCountInFileSync(key, pattern)).toBe expectedResult

    testGetAttrsInFileSync : (key, pattern, skipCount, count, expectedResult)->
        if not expectedResult?
            if _.isArray count
                expectedResult = count
                count = undefined
            else if _.isArray skipCount
                expectedResult = skipCount
                skipCount = undefined
            else if _.isArray pattern
                expectedResult = pattern
                pattern = undefined
            expectedResult = _.sortBy expectedResult if _.isArray expectedResult
            result = idb.getAttrsInFileSync(key, pattern, skipCount, count)
            result = _.sortBy result if _.isArray result
        expect(result).toEqual expectedResult

    testCreateKeyAliasSync : (dir, key, alias, partitionKeyWay, expectedResult)->
        if not expectedResult?
            expectedResult = partitionKeyWay
            partitionKeyWay = undefined
        if not expectedResult?
            expectedResult = idb.IDB_OK
        expect(idb.createKeyAliasSync(dir, key, alias, partitionKeyWay)).toBe expectedResult
        if expectedResult == idb.IDB_OK
            vkey = path.join(dir,key)
            value = idb.getInFileSync(vkey)
            utils.testGetInFileSync(path.join(dir, alias), value)

    testDeleteInFileSync : (key, attr, expectedResult)->
        erred = false
        err = null
        if not expectedResult?
            expectedResult = attr
            attr = null
        if _.isObject expectedResult
            erred = true
        try
            result = idb.deleteInFileSync(key, attr)
        catch e
            err = e
            result = false
        if erred
            expect(err).toEqual expectedResult
        else
            if expectedResult
                expect(err).toBe null
            expect(result).toBe expectedResult
        ###
        if err
            console.log("err=", err)
            expect(err).toEqual {code:2} #no such file or dir.
        ###
            

        if expectedResult
            p = key
            if attr
                p = path.join(p, attr)
            expect(fs.existsSync(p)).toBe false

    testDeleteInFileAsync : (key, attr, expectedResult)->
        erred = false
        if not expectedResult?
            expectedResult = attr
            attr = null
        if _.isObject expectedResult
            erred = true
        idb.deleteInFile key, attr, (err, result)->
            if erred
                expect(err).toEqual(expectedResult)
            else
                if expectedResult
                    expect(err).toBe null
                    expect(result).toBe true
                else
                    expect(err).not.toBe null
            asyncSpecDone()
        asyncSpecWait()

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
        else if _.isObject expectedResult
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
        else if _.isObject expectedResult
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

