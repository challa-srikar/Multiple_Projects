var map = L.map('map').setView([17.385, 78.486], 13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  maxZoom: 19
}).addTo(map);

// 🔥 IMPROVED SOCKET CONNECTION
var socket = io.connect(window.location.origin, {
  transports: ['websocket'],
  reconnection: true,
  reconnectionAttempts: 10,
  reconnectionDelay: 1000
});

var passengerLocationMarker = null;

// -------- DEBUG --------
socket.on('connect', () => {
  console.log("✅ Socket Connected");
});

socket.on('disconnect', () => {
  console.log("❌ Socket Disconnected");
});

socket.on('connect_error', (err) => {
  console.log("⚠️ Socket Error:", err);
});

// -------- BUS ICON --------
var busIcon = L.icon({
  iconUrl: '/static/bus_icon.png',
  iconSize: [40, 40],
  iconAnchor: [20, 20]
});

var busMarker = L.marker([17.385, 78.486], { icon: busIcon }).addTo(map);
var routeLine = L.polyline([], { color: 'blue' }).addTo(map);

var stopMarkers = [];
var heatLayers = [];

// -------- INITIAL LOAD --------
fetch('/state')
  .then(r => r.json())
  .then(function (data) {

    console.log("Initial State:", data);

    if (data.route) routeLine.setLatLngs(data.route);
    if (data.stops) renderStops(data.stops);
    if (data.eta) renderEta(data.eta);
    if (data.traffic) renderHeatmap(data.traffic);

    if (data.bus_location) {
      busMarker.setLatLng([data.bus_location.lat, data.bus_location.lon]);
    }

    // 👤 Passenger
    document.getElementById('count').innerHTML = data.passenger_count || 0;

    let capacity = data.capacity || 40;
    let passengers = data.passenger_count || 0;

    document.getElementById('seats').innerHTML = capacity - passengers;
    document.getElementById('occupancy').innerHTML =
      Math.round((passengers / capacity) * 100) + '%';
  });

// -------- USER LOCATION --------
if (navigator.geolocation) {
  navigator.geolocation.getCurrentPosition(function (pos) {
    passengerLocationMarker = L.circleMarker(
      [pos.coords.latitude, pos.coords.longitude],
      { color: '#2e7d32', radius: 7 }
    ).addTo(map).bindPopup('Your location');
  });
}

// -------- SOCKET EVENTS --------

// 👤 Passenger count
socket.on('passenger_update', function (data) {
  console.log("👤 Passenger Update:", data);

  document.getElementById('count').innerHTML = data;
});

// 💺 Seats + occupancy
socket.on('occupancy_update', function (data) {
  console.log("💺 Occupancy Update:", data);

  document.getElementById('seats').innerHTML = data.seats_left;
  document.getElementById('occupancy').innerHTML = data.occupancy_pct + '%';

  // 🔥 OPTIONAL: UI alert
  if (data.seats_left === 0) {
    document.getElementById('seats').style.color = 'red';
  } else {
    document.getElementById('seats').style.color = '';
  }
});

// 🛣 Route
socket.on('route_update', function (data) {
  routeLine.setLatLngs(data || []);
});

// 📍 Stops
socket.on('stops_update', function (data) {
  renderStops(data || []);
});

// ⏱ ETA
socket.on('eta_update', function (data) {
  renderEta(data || []);
});

// 🚦 Traffic
socket.on('traffic_heatmap', function (data) {
  renderHeatmap(data || []);
});

// 🚌 Bus movement (smooth)
socket.on('bus_location', function (data) {
  animateBus(data.lat, data.lon);
});

// -------- FUNCTIONS --------

// 📍 Stops
function renderStops(stops) {
  stopMarkers.forEach(m => map.removeLayer(m));

  stopMarkers = stops.map(s =>
    L.marker([s.lat, s.lon])
      .addTo(map)
      .bindPopup(s.name || 'Stop')
  );
}

// ⏱ ETA
function renderEta(items) {
  var html = '';

  items.forEach(function (item) {
    html += `<li>
      ${item.name}: ETA ${item.eta_min} min, wait ${item.waiting_min} min
    </li>`;
  });

  document.getElementById('etaList').innerHTML = html;
}

// 🚦 Traffic
function renderHeatmap(points) {
  heatLayers.forEach(layer => map.removeLayer(layer));

  heatLayers = points.map(function (p) {
    var color = p.intensity > 0.6 ? '#e53935' : '#ffb300';

    return L.circle([p.lat, p.lon], {
      radius: 120,
      color: color,
      fillColor: color,
      fillOpacity: 0.2,
      weight: 1
    }).addTo(map);
  });
}

// 🚌 Smooth animation (FIXED overlap issue)
var animationRunning = false;

function animateBus(newLat, newLon) {
  if (animationRunning) return;

  animationRunning = true;

  var start = busMarker.getLatLng();
  var steps = 20;

  var latStep = (newLat - start.lat) / steps;
  var lonStep = (newLon - start.lng) / steps;

  var i = 0;

  var interval = setInterval(function () {
    var lat = start.lat + latStep * i;
    var lon = start.lng + lonStep * i;

    busMarker.setLatLng([lat, lon]);

    i++;

    if (i > steps) {
      clearInterval(interval);
      animationRunning = false;
    }
  }, 80);
}