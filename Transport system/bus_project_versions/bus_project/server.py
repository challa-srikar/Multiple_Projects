import eventlet
eventlet.monkey_patch()

from flask import Flask, jsonify, render_template, request
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import route_engine
import traffic_engine
import route_checker
import json
from pathlib import Path

app = Flask(__name__)

# 🔥 NO threading (eventlet handles it)
socketio = SocketIO(app, cors_allowed_origins="*")

STATE_FILE = Path("runtime_state.json")
BUS_CAPACITY = 40


# -------- STATE --------
def _default_state():
    return {
        "passenger_count": 0,
        "bus_location": {"lat": 17.385, "lon": 78.486},
        "destination": None,
        "route": [],
        "stops": [],
    }


def _load_state():
    if STATE_FILE.exists():
        try:
            data = json.loads(STATE_FILE.read_text())
            base = _default_state()
            base.update(data)
            return base
        except json.JSONDecodeError:
            pass
    return _default_state()


state = _load_state()


def persist_state():
    STATE_FILE.write_text(json.dumps(state, indent=2))


# -------- ETA --------
def build_stop_predictions():
    route = state["route"]
    stops = state["stops"]
    bus = state["bus_location"]
    speed = traffic_engine.estimated_speed_kmph()

    if not route or not stops:
        return []

    bus_point = (bus["lat"], bus["lon"])
    projections = route_engine.project_distances(route, stops)
    bus_dist = route_engine.distance_to_route_position(route, bus_point)

    predictions = []
    for stop, stop_dist in zip(stops, projections):
        remain_km = max((stop_dist - bus_dist), 0) / 1000.0
        eta_min = 0 if remain_km == 0 else round((remain_km / max(speed, 5)) * 60)

        predictions.append({
            "name": stop.get("name", "Stop"),
            "lat": stop["lat"],
            "lon": stop["lon"],
            "eta_min": eta_min,
            "waiting_min": eta_min,
        })

    return predictions


# -------- SOCKET BROADCAST --------
def broadcast_state_updates():
    passenger_count = state["passenger_count"]
    seats_left = max(BUS_CAPACITY - passenger_count, 0)
    occupancy_pct = round((passenger_count / BUS_CAPACITY) * 100, 1)

    print("📡 Broadcasting:", passenger_count)

    socketio.emit("passenger_update", passenger_count)
    socketio.emit("occupancy_update", {
        "capacity": BUS_CAPACITY,
        "passengers": passenger_count,
        "seats_left": seats_left,
        "occupancy_pct": occupancy_pct,
    })

    socketio.emit("bus_location", state["bus_location"])
    socketio.emit("route_update", state["route"])
    socketio.emit("stops_update", state["stops"])
    socketio.emit("eta_update", build_stop_predictions())
    socketio.emit("traffic_heatmap", traffic_engine.get_heatmap_points(state["route"]))


# -------- MQTT --------
def on_connect(client, userdata, flags, rc):
    print("✅ MQTT Connected:", rc)
    client.subscribe("bus/BUS001/passenger")


def on_message(client, userdata, msg):
    try:
        value = int(msg.payload.decode())
        print("📥 MQTT Received:", value)

        state["passenger_count"] = value
        persist_state()

        # 🔥 NOW SAFE (eventlet handles threading)
        broadcast_state_updates()

    except Exception as e:
        print("MQTT Error:", e)


mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

try:
    mqtt_client.connect("localhost", 1883, 60)
    mqtt_client.loop_start()
except Exception as e:
    print("MQTT Connection Failed:", e)


# -------- ROUTES --------
@app.route("/")
def passenger_page():
    return render_template("passenger.html")


@app.route("/driver")
def driver_page():
    return render_template("driver.html")


@app.route("/state")
def get_state():
    return jsonify({
        **state,
        "capacity": BUS_CAPACITY,
        "eta": build_stop_predictions(),
        "traffic": traffic_engine.get_heatmap_points(state["route"]),
    })


@app.route("/gps", methods=["POST"])
def gps():
    data = request.json
    state["bus_location"] = {"lat": data["lat"], "lon": data["lon"]}

    traffic_engine.update_speed(data)

    if state["destination"] and state["route"]:
        off = route_checker.off_route(state["bus_location"], state["route"])
        if off:
            state["route"] = route_engine.get_route(
                state["bus_location"]["lat"],
                state["bus_location"]["lon"],
                state["destination"]["lat"],
                state["destination"]["lon"],
                state["stops"],
            )

    persist_state()
    broadcast_state_updates()
    return "ok"


@app.route("/set_destination", methods=["POST"])
def set_destination():
    data = request.json

    state["destination"] = {
        "lat": data["dest_lat"],
        "lon": data["dest_lon"]
    }

    state["route"] = route_engine.get_route(
        data["start_lat"],
        data["start_lon"],
        data["dest_lat"],
        data["dest_lon"],
        state["stops"],
    )

    persist_state()
    broadcast_state_updates()
    return "ok"


@app.route("/stops", methods=["POST"])
def set_stops():
    data = request.json
    state["stops"] = data.get("stops", [])

    if state["destination"] and state["bus_location"]:
        state["route"] = route_engine.get_route(
            state["bus_location"]["lat"],
            state["bus_location"]["lon"],
            state["destination"]["lat"],
            state["destination"]["lon"],
            state["stops"],
        )

    persist_state()
    broadcast_state_updates()
    return "ok"


@app.route("/recalculate_route", methods=["POST"])
def recalculate_route():
    if not state["destination"]:
        return jsonify({"error": "Destination not set"}), 400

    state["route"] = route_engine.get_route(
        state["bus_location"]["lat"],
        state["bus_location"]["lon"],
        state["destination"]["lat"],
        state["destination"]["lon"],
        state["stops"],
    )

    persist_state()
    broadcast_state_updates()
    return "ok"


@socketio.on("connect")
def on_socket_connect():
    print("🔌 Client Connected")
    broadcast_state_updates()


# -------- RUN --------
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000)