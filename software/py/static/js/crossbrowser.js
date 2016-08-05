navigator.mediaDevices = navigator.mediaDevices || {};
navigator.mediaDevices.getUserMedia = navigator.mediaDevices.getUserMedia || ((navigator.mozGetUserMedia || navigator.webkitGetUserMedia) ? 
	function(constraints) {
		return new Promise(function(resolve, reject) {
			(navigator.mozGetUserMedia || navigator.webkitGetUserMedia).call(navigator, constraints, resolve, reject);
		});
	} : null);

// innerTextクロスブラウザ対応 (FF では textContent なので innerText に統一)
(function() {
	var temp = document.createElement("div");
	if (temp.innerText == undefined) {
		Object.defineProperty(HTMLElement.prototype, "innerText", {
			get: function()  { return this.textContent },
			set: function(v) { this.textContent = v; }
		});
	}
})();

// AudioContextクロスブラウザ対応（Safariなどへの対応）
window.AudioContext = window.AudioContext || window.webkitAudioContext;

// Date.nowクロスブラウザ対応（IE8などへの対応）
Date.now = (Date.now || function() {
	return new Date().getTime();
});

// window.performance.nowクロスブラウザ対応（Safariなどへの対応）
(function() {
	if("performance" in window == false) {
		window.performance = {};
	}

	if("now" in window.performance == false) {
		var nowOffset = Date.now();
		if(performance.timing && performance.timing.navigationStart) {
			nowOffset = performance.timing.navigationStart;
		}

		window.performance.now = function now() {
			return Date.now() - nowOffset;
		};
	}
})();
