// util.js

// 次のような文字列と比較する
// String
// Number
// Boolean
// Date
// Error
// Array
// Function
// RegExp
// Object
function is(type, obj) {
	var c = Object.prototype.toString.call(obj).slice(8, -1);
	return obj !== undefined && obj !== null && c === type;
}

// 
function removeAllChildren(ele, callback) {
	while(0 < ele.childNodes.length) {
		var item = ele.childNodes.item(0);
		if(is("Function", callback)) {
			callback(item);
		}
		ele.removeChild(item);
	}
}

// 
function checkClass(ele, className) {
	if(ele.hasOwnProperty("classList")) {
		return ele.classList.contains(className);
	} else {
		function checkClassName(classes, classname) {
			var c = classes.split(" ");
			return (0 <= c.indexOf(classname));
		}

		if(is("String", ele.className)) {
			return checkClassName(ele.className, className);
		} else
		if(is("String", ele.className.baseVal)) {
			return checkClassName(ele.className.baseVal, className);
		}
	}

	return false;
}

// 
function addClass(ele, className) {
	if(ele.hasOwnProperty("classList")) {
		if(!ele.classList.contains(className)) {
			ele.classList.add(className);
		}
	} else {
		function newClassNames(classes, classname) {
			var c = classes.split(" ");
			if(0 > c.indexOf(classname)) {
				c.push(classname);
			}
			return c.join(" ");
		}

		if(is("String", ele.className)) {
			ele.className = newClassNames(ele.className, className);
		} else
		if(is("String", ele.className.baseVal)) {
			ele.className.baseVal = newClassNames(ele.className.baseVal, className);
		}
	}
}

// 
function removeClass(ele, className) {
	if(ele.hasOwnProperty("classList")) {
		if(ele.classList.contains(className)) {
			ele.classList.remove(className);
		}
	} else {
		function newClassNames(classes, classname) {
			var c = classes.split(" ");
			var idx = c.indexOf(classname)
			if(0 <= idx) {
				c.splice(idx, 1);
			}
			return c.join(" ");
		}

		if(is("String", ele.className)) {
			ele.className = newClassNames(ele.className, className);
		} else
		if(is("String", ele.className.baseVal)) {
			ele.className.baseVal = newClassNames(ele.className.baseVal, className);
		}
	}
}

// 
function addClasses(ele, classes) {
	for(var i = 0; i < classes.length; i ++) {
		addClass(ele, classes[i]);
	}
}

// 
function removeClasses(ele, classes) {
	for(var i = 0; i < classes.length; i ++) {
		removeClass(ele, classes[i]);
	}
}

// 
function isIOS() {
	var userAgent = window.navigator.userAgent.toLowerCase();
	return ((userAgent.indexOf('ipad') !== -1)
		  || (userAgent.indexOf('ipod') !== -1)
		  || (userAgent.indexOf('iphone') !== -1));
}

// 
var dumpObject = function(obj, indent) {
	var result = '';
	var tab    = '\t';
	var br     = '\n';

	indent = indent || 0;

	if(obj) {
		if(indent) {
			tab += indent;
		}

		if(typeof obj === 'object' && !obj.tagName) {
			result += '[Object]' + br;
			for(var key in obj) {
				result += tab + key + ': ';
				if(typeof obj[key] === 'object') {
					result += dumpObject(obj[key], tab);
				} else
				if(typeof obj[key] === 'function') {
					result += "{...}";
				} else
				if(typeof obj[key] === 'number') {
					result += obj[key];
/*
					var wk = obj[key].toString(16);
					result += "0x"+("0"+obj[key].toString(16)).substr(parseInt((wk.length+1)/2, 10) * -2);
*/
				} else
				if((typeof obj[key] === 'string')
				|| (typeof obj[key] === 'boolean')) {
					result += obj[key];
				} else {
					result += obj[key];
				}
				result += br;
			}
		} else {
			result = obj;
		}
	}

	result = String(result);

	return result;
};

// 
function getFileName(pathName) {
	return pathName.substring(pathName.lastIndexOf("/") + 1, pathName.length);
}

// 
function getFName(pathName) {
	var filename = getFileName(pathName);
	return filename.substring(filename, filename.lastIndexOf("."));
}

// 
function buildUrl(location, params) {
	var url = location;

	if(null !== params) {
		var param = "";
		for(var key in params) {
			if("" !== param) {
				param += "&";
			}
			param += encodeURIComponent(key);
			param += "=";
			param += encodeURIComponent(params[key]);
		}

		if("" !== param) {
			url += "?";
			url += param;
		}
	}

	return url;
}

// 
function getUrlParams(location) {

	var params = {};
	if(1 < location.search.length) {	
		var str = window.location.search.substring(1);
		var items = str.split("&");
		for(var i = 0; i < items.length; i ++) {
			var pair = items[i].split("=");
			if("" !== pair[0]) {
				var name = decodeURIComponent(pair[0]);
				var value = (1 >= pair.length) ? null : decodeURIComponent(pair[1]);
				params[name] = value;
			}
		}
	}

	return params;
}

// 
function msecToTimeText(msec, needHour, needMsec) {
	var text = "";

	var sec = parseInt(msec / 1000, 10);
	msec %= 1000;
	var min = parseInt(sec / 60, 10);
	sec %= 60;
	var hour = parseInt(min / 60, 10);

	if(needHour) {
		min %= 60;
		text = hour + ":" + ("0" + min).slice(-2) + ":" + ("0" + sec).slice(-2);
	} else {
		text = min + ":" + ("0" + sec).slice(-2);
	}

	if(needMsec) {
		text += ":" + ("00" + msec).slice(-3);
	}

	return text;
}

// UTF8の文字列を取得する
function string2utf8(str) {
	return unescape(encodeURIComponent(str));
}

// 
function arrayBuffer2string(buff) {

// このコードだと、大きなバッファを与えたときに "maximum call stack size exceeded" 
// になるので、１文字ずつ処理する。
//	return String.fromCharCode.apply(null, new Uint8Array(buff));

	var a = new Uint8Array(buff);
	var str = "";
	for(var i = 0; i < a.length; i ++) {
		var c = a[i];
		if(0x00 == c)
		{
			break;
		}
		str += String.fromCharCode(c);
	}
	return str;
}

