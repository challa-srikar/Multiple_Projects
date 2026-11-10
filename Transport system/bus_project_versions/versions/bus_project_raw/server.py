from flask import Flask, render_template, request
from flask_socketio import SocketIO
import json
import os
import route_engine
import route_checker
import stop_predictor

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

STATE_FILE = "route_state.json"

bus_location = {"lat":17.385,"lon":78.486}
route = []
destination = None
bus_stops = []
passenger_count = 0

# -------- SAVE & LOAD STATE -------- #

def save_state():
    data = {
        "route": route,
        "destination": destination,
        "stops": bus_stops
    }
    with open(STATE_FILE,"w") as f:
        json.dump(data,f)

def load_state():
    global route,destination,bus_stops

    if os.path.exists(STATE_FILE):
        with open(STATE_FILE,"r") as f:
            data = json.load(f)
            route = data.get("route",[])
            destination = data.get("destination",None)
            bus_stops = data.get("stops",[])

load_state()

# -------- ROUTES -------- #

@app.route("/")
def passenger():
    return render_template("passenger.html")

@app.route("/driver")
def driver():
    return render_template("driver.html")

# -------- GPS -------- #

@app.route("/gps",methods=["POST"])
def gps():

    global bus_location,route,destination

    data = request.json

    bus_location["lat"] = data["lat"]
    bus_location["lon"] = data["lon"]

    socketio.emit("bus_location",bus_location)

    # ETA update
    if bus_stops:
        arrivals = stop_predictor.predict_arrival(bus_location,bus_stops)
        socketio.emit("stop_eta_update",arrivals)

    # Traffic heatmap
    speed = data.get("speed",0)

    intensity = 1 if speed < 10 else 0.5 if speed < 20 else 0.1

    socketio.emit("traffic_update",{
        "lat":bus_location["lat"],
        "lon":bus_location["lon"],
        "intensity":intensity
    })

    # Auto reroute
    if destination and route:

        if route_checker.off_route(bus_location,route):

            route = route_engine.get_route(
                bus_location["lat"],
                bus_location["lon"],
                destination["lat"],
                destination["lon"]
            )

            save_state()
            socketio.emit("route_update",route)

    return "ok"

# -------- START ROUTE -------- #

@app.route("/set_destination",methods=["POST"])
def set_destination():

    global route,destination,bus_stops

    data = request.json

    destination = {
        "lat":data["dest_lat"],
        "lon":data["dest_lon"]
    }

    bus_stops = data.get("stops",[])

    route = route_engine.get_route(
        data["start_lat"],
        data["start_lon"],
        data["dest_lat"],
        data["dest_lon"]
    )

    save_state()

    socketio.emit("route_update",route)
    socketio.emit("stops_update",bus_stops)

    return "ok"

# -------- ON CONNECT -------- #

@socketio.on("connect")
def send_existing_data():

    if route:
        socketio.emit("route_update",route)

    if bus_stops:
        socketio.emit("stops_update",bus_stops)

# -------- RUN -------- #

if __name__=="__main__":
    socketio.run(app,host="0.0.0.0",port=5000)