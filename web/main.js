var http = require('http');
var url = require('url');
var static = require('node-static');
var file = new static.Server('.');
var fs = require("fs");


const path = require('path');
const fifoPath = path.resolve("/dev", 'fb0');

function accept(req, res) {

  if (req.url == '/mpu6050') {   

    req.on('data', function(chunk) {
      console.log(JSON.parse(chunk).length);
          var ws = fs.createWriteStream(fifoPath);
          ws.write( new Buffer(JSON.parse(chunk)));
          ws.close();
      });
/*
    var txt ={
        "temp": "36",
        "ax": "11",
		"ay": "12",
		"az": "13",
		"gx": "14",
		"gy": "15",
		"gz": "44"
			};
    res.end(JSON.stringify(txt));*/
    
	var fileContent = fs.readFileSync("/dev/dyn_1", "utf8");
	res.end(fileContent);
    
  } else {
    file.serve(req, res);
  }
}

if (!module.parent) {
    http.createServer(accept).listen(666);
} else {
    exports.accept = accept;
}









