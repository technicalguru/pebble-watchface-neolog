
Pebble.addEventListener('ready', function(e) {
	console.log('JavaScript app ready and running!');
});

Pebble.addEventListener('showConfiguration', function(e) {
	var watch = null;
	if (typeof Pebble.getActiveWatchInfo == 'function') {
		watch = Pebble.getActiveWatchInfo();
	} else {
		watch = {
			platform : 'aplite',
			language : 'en_US'
		};
	}

	// Show config page
	var file = 'index.html';
	var lang = '';
	if (watch != null) {
		if (watch.platform == 'aplite') {
			file = 'index_aplite.html';
		}
		if (navigator.language == 'de_DE') {
			lang = 'de/';
		}
	}

	var url = 'http://pebble.ralph-schuster.eu/neolog/2.0/config/'+lang+file;
	console.log('Opening config URL: '+url);
	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));

	console.log('Configuration page returned: ' + JSON.stringify(configData));

	if (configData.backgroundColor) {
		Pebble.sendAppMessage({
			backgroundColor: parseInt(configData.backgroundColor, 16),
			foregroundColor: parseInt(configData.foregroundColor, 16),
			displayStatusBar: configData.displayStatusBar
		}, function() {
			console.log('Send successful!');
		}, function() {
			console.log('Send failed!');
		});
	}
});