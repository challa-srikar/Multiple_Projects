from geopy.distance import geodesic

BUS_STOPS = [
{"name":"Stop 1","lat":17.385,"lon":78.486},
{"name":"Stop 2","lat":17.392,"lon":78.490},
{"name":"Stop 3","lat":17.400,"lon":78.500}
]

def predict_arrival(bus_location):

    avg_speed_kmh = 30

    results = []

    for stop in BUS_STOPS:

        dist = geodesic(
            (bus_location["lat"],bus_location["lon"]),
            (stop["lat"],stop["lon"])
        ).km

        time_minutes = int((dist/avg_speed_kmh)*60)

        results.append({
            "name":stop["name"],
            "eta":time_minutes
        })

    return results