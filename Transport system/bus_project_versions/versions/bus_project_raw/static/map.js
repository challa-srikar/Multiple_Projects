var map = L.map('map').setView([17.385,78.486],13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);

var socket = io();

var heatLayer = L.heatLayer([], {radius:25}).addTo(map);

var busMarker = L.marker([17.385,78.486],{
icon:L.icon({
iconUrl:'/static/bus_icon.png',
iconSize:[40,40],
iconAnchor:[20,20]
})
}).addTo(map);

var routeLine = L.polyline([],{color:"blue"}).addTo(map);

var stopMarkers=[];

socket.on("route_update",data=>routeLine.setLatLngs(data));

socket.on("bus_location",data=>animateBus(data));

socket.on("traffic_update",data=>{
heatLayer.addLatLng([data.lat,data.lon,data.intensity]);
});

socket.on("stops_update",stops=>{

stopMarkers.forEach(m=>map.removeLayer(m));
stopMarkers=[];

stops.forEach(s=>{
var m=L.marker([s.lat,s.lon]).addTo(map);
stopMarkers.push(m);
});

});

socket.on("stop_eta_update",data=>{

let html="";

data.forEach(s=>{
html+=`<p>Stop ETA: ${s.eta} min</p>`;
});

document.getElementById("stops").innerHTML=html;

});

function animateBus(data){

var start=busMarker.getLatLng();

var steps=20;
var latStep=(data.lat-start.lat)/steps;
var lonStep=(data.lon-start.lng)/steps;

let i=0;

let interval=setInterval(()=>{

busMarker.setLatLng([
start.lat+latStep*i,
start.lng+lonStep*i
]);

i++;

if(i>steps) clearInterval(interval);

},100);
}