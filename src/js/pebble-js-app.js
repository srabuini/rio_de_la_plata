function degToText(deg) {
  if (deg < 45) {
    return "NNE";
  } else if (deg == 45) {
    return "NE";
  } else if (deg > 45 && deg < 90) {
    return "ENE";
  } else if (deg == 90) {
    return "E";
  } else if (deg > 90 && deg < 135) {
    return "ESE";
  } else if (deg == 135) {
    return "SE";
  } else if (deg > 135 && deg < 180) {
    return "SSE";
  } else if (deg == 180) {
    return "S";
  } else if (deg > 180 && deg < 225) {
    return "SSW";
  } else if (deg == 225) {
    return "SW";
  } else if (deg > 225 && deg < 270) {
    return "WSW";
  } else if (deg == 270) {
    return "W";
  } else if (deg > 270 && deg < 315) {
    return "WNW";
  } else if (deg == 315) {
    return "NW";
  } else if (deg > 315 && deg < 360) {
    return "NNW";
  } else if (deg == 360) {
    return "N";
  }
}

function fetchData() {
  var response;
  var req = new XMLHttpRequest();
  
  req.open('GET', "http://weather.wasabitlabs.com/palermo");
  
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        response = JSON.parse(req.responseText);
        var temp_and_level, wind_speed, wind_direction, splited_wind, timestamp, last_update;
        if (response) {
          temp_and_level = response.temp + " " + response.level + 'm';
          wind_speed = response.wind_speed;
          splited_wind = wind_speed.split('|');
          wind_speed = splited_wind[1] + 'kt ' + splited_wind[0] + 'Km/h';
          wind_direction = response.wind_direction;
          wind_direction = wind_direction + " " + degToText(wind_direction);
          timestamp = formatedDate(response.timestamp * 1000);
          last_update = formatedDate(response.last_update * 1000);
          console.log(temp_and_level);
          console.log(wind_speed);
          console.log(wind_direction);
          console.log(timestamp);
          console.log(last_update);
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
  var month = "" + (date.getMonth() + 1); if (month.length == 1) { month = "0" + month; }
  var day = "" + date.getDate(); if (day.length == 1) { day = "0" + day; }
  var hour = "" + date.getHours(); if (hour.length == 1) { hour = "0" + hour; }
  var minute = "" + date.getMinutes(); if (minute.length == 1) { minute = "0" + minute; }
  return day + "/" + month + " " + hour + ":" + minute;
}

Pebble.addEventListener("ready", function() {
  console.log("My app has started - Doing stuff...");
  fetchData();
});

Pebble.addEventListener("appmessage", function(e) {
  fetchData();
  console.log(e.type);
  console.log(e.payload);
  console.log("message!");
});