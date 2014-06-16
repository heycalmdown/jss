var glob = require('glob');
var jss = require('./index.js');

var errorcount = 0;
function touchall(object1, object2, level) {
	if (!level) level = 1;
	var isArray = (object1.constructor == Array );
	if (typeof object1 === 'object') {
		//console.info(Object.keys(object1));
		for (key in object1) {
			//console.error('trace 1');
			if (isArray) key = Number(key);
			//console.error('trace 2');
			if (typeof object1[key] !== 'object' || !object1[key] ) {
				//console.error(object1);
				//console.error(object2);
				if (object1[key] !== object2[key]) {
					//console.error('touchall error');
					console.error('[ERROR](', key,')', object1[key], object2[key], level);
					console.info(object1);
					console.info(object2);

					return process.exit();
				} else {
					errorcount++;
					//console.error('touchall ok');
					//console.error('[OK](', key,')', object1[key], object2[key]);
				}
				//console.error('trace 4');
			} else {
				//console.error('trace 5');
				touchall(object1[key], object2[key], level+1);
			}
		}
	}
}

function touchall_view(object1) {
	var isArray = (object1.constructor == Array );
	if (typeof object1 === 'object') {
		for (key in object1) {
			if (isArray) key = Number(key);
			if (typeof object1[key] !== 'object' || !object1[key] ) {
				console.error('[OK](', key,')', object1[key]);
			} else {
				touchall_view(object1[key]);
			}
		}
	}
}



var staticdataPath = '../fo3-staticdata/data/';
glob.sync([staticdataPath, '*.json'].join('/')).forEach(function (file) {


	var staticdata = require(file);
	//var string = JSON.stringify(staticdata);

	console.info(file);

	var storage = jss.createJssByJsonFile(file);
	if (!storage) {
		console.info('jss error');
		process.exit();
	}

	errorcount=0;
	touchall(staticdata, storage);
	console.info("count:", errorcount);

	errorcount=0;
	touchall(storage, staticdata);
	console.info("count:", errorcount);


	delete staticdata;
	delete storage;
	global.gc();

	console.info('-------------------------------------------------')

});

