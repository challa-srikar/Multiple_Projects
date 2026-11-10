var map = L.map('map').setView([17.385, 78.486], 13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  maxZoom: 19
}).addTo(map);

var socket = io();

// -------- STATE --------
var firstLocationFix = true;
var watchId = null;

// -------- MARKERS --------
var busMarker = L.marker([17.385, 78.486], { draggable: true }).addTo(map);
var destMarker = null;
var routeLine = L.polyline([], { color: 'red' }).addTo(map);

var stopMarkers = [];
var stops = [];

// -------- INIT --------
fetch('/state').then(r => r.json()).then(initFromState);

function initFromState(data) {
  if (data.bus_location) {
    busMarker.setLatLng([data.bus_location.lat, data.bus_location.lon]);
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

// -------- UI --------
function setGpsStatus(text) {
  document.getElementById('gpsStatus').innerText = 'GPS: ' + text;
}

function setAccuracy(acc) {
  document.getElementById('gpsAccuracy').innerText = 'Accuracy: ' + acc + ' m';
}

// -------- GPS ERROR --------
function handleGpsError(error) {
  let msg = '';
  switch (error.code) {
    case error.PERMISSION_DENIED: msg = 'Permission Denied ❌'; break;
    case error.POSITION_UNAVAILABLE: msg = 'Unavailable ⚠️'; break;
    case error.TIMEOUT: msg = 'Timeout ⏳'; break;
    default: msg = 'Unknown Error';
  }
  setGpsStatus(msg);
}

// -------- SMOOTH ANIMATION --------
function moveBusSmooth(newLatLng) {
  let start = busMarker.getLatLng();
  let end = L.latLng(newLatLng);

  let steps = 20, i = 0;
  let latStep = (end.lat - start.lat) / steps;
  let lngStep = (end.lng - start.lng) / steps;

  let interval = setInterval(() => {
    if (i >= steps) return clearInterval(interval);
    start = L.latLng(start.lat + latStep, start.lng + lngStep);
    busMarker.setLatLng(start);
    i++;
  }, 50);
}

// -------- SEND GPS --------
function sendLocation(pos, isManual) {
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  moveBusSmooth([lat, lon]);

  // ✅ Only center ONCE (no continuous zoom)
  if (firstLocationFix) {
    map.setView([lat, lon], 15);
    firstLocationFix = false;
  }

  fetch('/gps', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ lat, lon })
  });

  setGpsStatus(isManual ? 'Manual Sync ✅' : 'Live 📡');
  setAccuracy(Math.round(pos.coords.accuracy));
}

// -------- LIVE TRACKING --------
function startLiveTracking() {
  if (watchId !== null) return;

  watchId = navigator.geolocation.watchPosition(
    (pos) => sendLocation(pos, false),
    handleGpsError,
    { enableHighAccuracy: true, timeout: 10000 }
  );
}

// -------- MANUAL SYNC --------
function syncLocation() {
  navigator.geolocation.getCurrentPosition(
    (pos) => sendLocation(pos, true),
    handleGpsError,
    { enableHighAccuracy: true, timeout: 10000 }
  );
}

// -------- CENTER BUTTON --------
function centerMap() {
  let pos = busMarker.getLatLng();
  map.setView(pos, 15);
}

// -------- DESTINATION --------
function setDestinationMarker(lat, lon) {
  if (destMarker) map.removeLayer(destMarker);

  destMarker = L.marker([lat, lon], { draggable: true }).addTo(map);

  destMarker.on('dragend', () => {
    startRoute();
  });
}

// -------- SEARCH DEST --------
function searchDestination() {
  var q = document.getElementById('destSearch').value;

  fetch('https://nominatim.openstreetmap.org/search?format=json&q=' + encodeURIComponent(q))
    .then(r => r.json())
    .then(d => {
      if (!d.length) return;
      setDestinationMarker(parseFloat(d[0].lat), parseFloat(d[0].lon));
      map.setView([d[0].lat, d[0].lon], 15);
    });
}

// -------- ROUTE --------
function startRoute() {
  if (!destMarker) return;

  let start = busMarker.getLatLng();
  let dest = destMarker.getLatLng();

  fetch('/set_destination', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({
      start_lat: start.lat,
      start_lon: start.lng,
      dest_lat: dest.lat,
      dest_lon: dest.lng
    })
  });
}

// -------- RESET --------
function resetRoute() {
  // 🧹 Clear route line
  routeLine.setLatLngs([]);

  // 🧹 Remove destination marker
  if (destMarker) {
    map.removeLayer(destMarker);
    destMarker = null;
  }

  // 🧹 Remove all stop markers from map
  stopMarkers.forEach(m => map.removeLayer(m));
  stopMarkers = [];

  // 🧹 Clear stops array
  stops = [];

  // ✅ IMPORTANT: Sync empty stops to backend
  fetch('/stops', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ stops: [] })
  });

  // ✅ Reset route on backend
  fetch('/reset', { method: 'POST' });

  // Optional UI feedback
  setGpsStatus("Route & Stops Reset ✅");
}
// -------- STOPS --------
function renderStops(newStops) {
  stops = newStops;

  stopMarkers.forEach(m => map.removeLayer(m));

  stopMarkers = stops.map((s, idx) => {
    let marker = L.marker([s.lat, s.lon]).addTo(map)
      .bindPopup((s.name || 'Stop') + ' #' + (idx + 1));

    // ❌ remove stop on click
    marker.on('click', () => {
      stops.splice(idx, 1);
      renderStops(stops);
      pushStops();
    });

    return marker;
  });
}

function pushStops() {
  fetch('/stops', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ stops })
  });
}

// ➕ add stop on map click
map.on('click', function (e) {
  let stop = {
    lat: e.latlng.lat,
    lon: e.latlng.lng,
    name: 'Stop ' + (stops.length + 1)
  };

  stops.push(stop);
  renderStops(stops);
  pushStops();
});

// -------- DRAG START --------
busMarker.on('dragend', () => {
  startRoute();
});

// -------- SOCKET --------
socket.on('route_update', (data) => {
  routeLine.setLatLngs(data || []);
});

socket.on('stops_update', (data) => {
  renderStops(data || []);
});

// -------- START --------
startLiveTracking();