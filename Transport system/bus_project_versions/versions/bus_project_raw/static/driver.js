var map = L.map('map').setView([17.385, 78.486], 13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  maxZoom: 19
}).addTo(map);

var socket = io();

// -------- Bus Marker (Animated) --------
var busMarker = L.marker([17.385, 78.486]).addTo(map);

var destMarker = null;
var routeLine = L.polyline([], { color: 'red' }).addTo(map);

var stopMarkers = [];
var stops = [];

var watchId = null;

// -------- INIT --------
fetch('/state').then(r => r.json()).then(initFromState);

function initFromState(data) {
  if (data.bus_location) {
    moveBusSmooth([data.bus_location.lat, data.bus_location.lon]);
  }
  if (data.destination) {
    setDestinationMarker(data.destination.lat, data.destination.lon);
  }
  if (data.route) {
    routeLine.setLatLngs(data.route);
  }
  if (data.stops) {
    renderStops(data.stops);
  }
}

// -------- GPS STATUS --------
function setGpsStatus(text) {
  document.getElementById('gpsStatus').innerText = 'GPS: ' + text;
}

// -------- ACCURACY DISPLAY --------
function setAccuracy(acc) {
  if (document.getElementById('gpsAccuracy')) {
    document.getElementById('gpsAccuracy').innerText = 'Accuracy: ' + acc + ' m';
  }
}

// -------- GPS ERROR HANDLING --------
function handleGpsError(error) {
  let msg = '';

  switch (error.code) {
    case error.PERMISSION_DENIED:
      msg = 'Permission Denied ❌ (Enable location)';
      break;
    case error.POSITION_UNAVAILABLE:
      msg = 'Position Unavailable ⚠️';
      break;
    case error.TIMEOUT:
      msg = 'Timeout ⏳';
      break;
    default:
      msg = 'Unknown Error';
  }

  setGpsStatus(msg);
}

// -------- SMOOTH ANIMATION --------
function moveBusSmooth(newLatLng) {
  let start = busMarker.getLatLng();
  let end = L.latLng(newLatLng);

  let steps = 20;
  let i = 0;

  let latStep = (end.lat - start.lat) / steps;
  let lngStep = (end.lng - start.lng) / steps;

  let interval = setInterval(() => {
    if (i >= steps) {
      clearInterval(interval);
      return;
    }

    start = L.latLng(start.lat + latStep, start.lng + lngStep);
    busMarker.setLatLng(start);
    i++;
  }, 50);
}

// -------- SEND GPS TO SERVER --------
function sendLocation(pos, isManual) {
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  moveBusSmooth([lat, lon]);
  map.setView([lat, lon], 15);

  fetch('/gps', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      lat: lat,
      lon: lon,
      speed: pos.coords.speed || 0
    })
  });

  setGpsStatus(isManual ? 'Manual Sync ✅' : 'Live Tracking 📡');
  setAccuracy(Math.round(pos.coords.accuracy));
}

// -------- LIVE TRACKING --------
function startLiveTracking() {
  if (!navigator.geolocation) {
    setGpsStatus('Geolocation not supported');
    return;
  }

  if (watchId !== null) return;

  watchId = navigator.geolocation.watchPosition(
    (pos) => sendLocation(pos, false),
    handleGpsError,
    {
      enableHighAccuracy: true,
      timeout: 10000,
      maximumAge: 0
    }
  );

  setGpsStatus('Live tracking started...');
}

// -------- MANUAL SYNC --------
function syncLocation() {
  if (!navigator.geolocation) {
    setGpsStatus('Geolocation not supported');
    return;
  }

  setGpsStatus('Fetching location...');

  navigator.geolocation.getCurrentPosition(
    (pos) => sendLocation(pos, true),
    handleGpsError,
    {
      enableHighAccuracy: true,
      timeout: 10000
    }
  );
}

// -------- SEARCH START --------
function searchStart() {
  var query = document.getElementById('startSearch').value;

  fetch('https://nominatim.openstreetmap.org/search?format=json&q=' + encodeURIComponent(query))
    .then(res => res.json())
    .then(data => {
      if (!data.length) return;

      var lat = parseFloat(data[0].lat);
      var lon = parseFloat(data[0].lon);

      moveBusSmooth([lat, lon]);
      map.setView([lat, lon], 15);
    });
}

// -------- DESTINATION --------
function setDestinationMarker(lat, lon) {
  if (destMarker) map.removeLayer(destMarker);
  destMarker = L.circleMarker([lat, lon], { color: 'red', radius: 8 }).addTo(map);
}

function searchDestination() {
  var query = document.getElementById('destSearch').value;

  fetch('https://nominatim.openstreetmap.org/search?format=json&q=' + encodeURIComponent(query))
    .then(res => res.json())
    .then(data => {
      if (!data.length) return;

      var lat = parseFloat(data[0].lat);
      var lon = parseFloat(data[0].lon);

      setDestinationMarker(lat, lon);
      map.setView([lat, lon], 15);
    });
}

// -------- ROUTE --------
function startRoute() {
  if (!destMarker) return;

  var start = busMarker.getLatLng();
  var dest = destMarker.getLatLng();

  fetch('/set_destination', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      start_lat: start.lat,
      start_lon: start.lng,
      dest_lat: dest.lat,
      dest_lon: dest.lng
    })
  });
}

function recalculateRoute() {
  fetch('/recalculate_route', { method: 'POST' });
}

// -------- STOPS --------
function renderStops(newStops) {
  stops = newStops;

  stopMarkers.forEach(m => map.removeLayer(m));

  stopMarkers = stops.map((s, idx) => {
    return L.marker([s.lat, s.lon])
      .addTo(map)
      .bindPopup((s.name || 'Stop') + ' #' + (idx + 1));
  });
}

function pushStops() {
  fetch('/stops', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({ stops: stops })
  });
}

map.on('click', function (e) {
  var stop = {
    lat: e.latlng.lat,
    lon: e.latlng.lng,
    name: 'Stop ' + (stops.length + 1)
  };

  stops.push(stop);
  renderStops(stops);
  pushStops();
});

// -------- SOCKET EVENTS --------
socket.on('route_update', function (data) {
  routeLine.setLatLngs(data || []);
});

socket.on('stops_update', function (data) {
  renderStops(data || []);
});

// -------- START LIVE TRACKING --------
startLiveTracking();