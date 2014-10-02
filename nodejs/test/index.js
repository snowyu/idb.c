s=require('../index');
console.log('putInFileSync.result=',s.putInFileSync("hi你好足够长的话", "world", "myass", 2));
console.log('putInFileSync.result=',s.putInFileSync("hi", "world2"));

s.putInFile('h1你好足够长的话', 'va1', 'atrass', 3, function(err){
    console.log("error=",err);
});

