function degToText(deg) {
  var directions = ['N', 'NNE', 'NE', 'NEE',
                    'E', 'ESE', 'SE', 'SSE',
                    'S', 'SSW', 'SW', 'WSW',
                    'W', 'WNW', 'NW', 'NNW'];

  var index = parseInt(16 / 360 * deg + 0.5) % 16;

  return directions[index];
}

function fetchData() {
  var response;
  var req = new XMLHttpRequest();

  req.open('GET', "http://weather.wasabitlabs.com/palermo");

  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {

        response = JSON.parse(req.responseText);

        var temp_and_level, wind_speed, wind_direction, splited_wind, timestamp,
            last_update;

        if (response) {
          temp_and_level = response.temp + " " + response.level + 'm';
          wind_speed = response.wind_speed;
          splited_wind = wind_speed.split('|');
          wind_speed = splited_wind[1] + 'kt ' + splited_wind[0] + 'Km/h';
          wind_direction = response.wind_direction;
          wind_direction = wind_direction + " " + degToText(wind_direction);
          timestamp = formatedDate(response.timestamp * 1000);
          last_update = formatedDate(response.last_update * 1000);

          Pebble.sendAppMessage({
            "wind_direction":wind_direction,
            "wind_speed":wind_speed,
            "temp_and_level":temp_and_level,
            "last_update":last_update});
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
}

function formatedDate(timestamp) {
  var date = new Date(timestamp);
  var month = "" + (date.getMonth() + 1);

  if (month.length == 1) {
    month = "0" + month;
  }

  var day = "" + date.getDate();
  if (day.length == 1) {
    day = "0" + day;
  }

  var hour = "" + date.getHours();
  if (hour.length == 1) {
    hour = "0" + hour;
  }

  var minute = "" + date.getMinutes();
  if (minute.length == 1) {
    minute = "0" + minute;
  }

  return day + "/" + month + " " + hour + ":" + minute;
}

Pebble.addEventListener("ready", function() {
  fetchData();
});

Pebble.addEventListener("appmessage", function(e) {
  fetchData();
});
