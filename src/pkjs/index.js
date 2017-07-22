var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var darkSkyUrl = 'https://api.darksky.net/forecast/ae5dc1fae3c2457141ae555026f189e0/' +
      pos.coords.latitude + ',' + pos.coords.longitude;
  
  var googleUrl = 'https://maps.googleapis.com/maps/api/geocode/json?latlng=' +
      pos.coords.latitude + ',' + pos.coords.longitude + 
      '&key=AIzaSyA6UPzZMOH1zA3afmXHURgipL_lktn2wOw' + '&result_type=locality';
  
  xhrRequest(darkSkyUrl, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);

      var temperature = Math.round(json.currently.temperature);
      var summary = json.daily.data[0].summary;      
      
      xhrRequest(googleUrl, 'GET', 
        function(responseText) {
          var json = JSON.parse(responseText);
    
          var location = json.results[0].address_components[0].long_name;
          
          
           var result = {
            'TEMPERATURE': temperature,
            'SUMMARY': summary,
            'LOCATION': location
          };
          
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