s=require('../index');
console.log('putInFileSync.result=',s.putInFileSync("hi", "world"));

s.putInFile('h1', 'va1', function(err){
    console.log("error=",err);
});
