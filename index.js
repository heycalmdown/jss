var jss = require('./build/Release/jss');
var fs = require('fs');

exports.createJssByJsonFile = createJssByJsonFile;
exports.createJssByJsonStr = createJssByJsonStr;

function createJssByJsonStr(jstr, size) {
	try {
		var obj = jss.createJssObject(jstr);
		return obj;
	} catch(e) {
		console.error('[fo3-jss] '+e);
		return;
	}
}

function createJssByJsonFile(file) {
	var stats = fs.statSync(file);

	var jstr = fs.readFileSync(file).toString();
	var obj = createJssByJsonStr(jstr);

	if (!obj) {
		console.error('[fo3-jss] '+file);
		obj = require(file);
	}
	return obj;
}