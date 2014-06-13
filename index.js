var jss = require('./build/Release/jss');
var fs = require('fs');

exports.createJssByJsonFile = createJssByJsonFile;
exports.createJssByJsonStr = createJssByJsonStr;

function createJssByJsonStr(jstr, size) {
	console.info('[jss] jstr.length=', jstr.length);
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
		obj = require(file);
	}

	return obj;
}