var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  console.log('getting weather..');
  var url = 'https://api.darksky.net/forecast/ae5dc1fae3c2457141ae555026f189e0/' +
      pos.coords.latitude + ',' + pos.coords.longitude;

  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);

      var temperature = Math.round(json.currently.temperature);
      var summary = json.daily.data[0].summary;      
      
      var result = {
        'TEMPERATURE': temperature,
        'SUMMARY': summary
      };
      console.log(result.TEMPERATURE);

      Pebble.sendAppMessage(result,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  console.log('getting location..');
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }                     
);

Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
  }
);