path=require('path')
fse=require('fs-extra')
fs=require('graceful-fs')
chance = new require('chance')()
idb = require('../index')
utils = require('./utils')

dataDir = path.join(__dirname, 'tmp')
gKey    = path.join(dataDir, 'u'+utils.getRandomStr(5))
gKey2   = path.join(dataDir, 'u'+utils.getRandomStr(5))
fse.remove(dataDir)

describe 'Get Key\'s Attrs from a directory', ->

    it 'try to get attrs from a non-exists key synchronous', ->
        key    = utils.getRandomStr(5)
        utils.testGetAttrCountInFileSync(key, 0)
        utils.testGetAttrsInFileSync(key)

    it 'get attrs from a key synchronous', ->
        attr1   = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(gKey, value, attr1)
        attr2   = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(gKey, value, attr2)
        attr3   = utils.getRandomStr(5)
        value   = utils.getRandomStr(5)
        utils.testPutInFileSync(gKey, value, attr3)

        utils.testGetAttrCountInFileSync(gKey, 3)
        utils.testGetAttrsInFileSync(gKey, [attr1,attr2,attr3])


