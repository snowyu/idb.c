s=require('./index');
console.log('putInFileSync.result=',s.putInFileSync("hi", "world"));

s.putInFile('你好', 'va1', function(err, result){console.log("putInFile.result=", result);});
