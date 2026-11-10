import requests
from math import radians, cos, sin, asin, sqrt


def haversine_m(lat1, lon1, lat2, lon2):
    r = 6371000
    dlat = radians(lat2 - lat1)
    dlon = radians(lon2 - lon1)
    a = sin(dlat / 2) ** 2 + cos(radians(lat1)) * cos(radians(lat2)) * sin(dlon / 2) ** 2
    return 2 * r * asin(sqrt(a))


def get_route(start_lat, start_lon, end_lat, end_lon, stops=None):
    stops = stops or []
    coords = [f"{start_lon},{start_lat}"]
    for stop in stops:
        coords.append(f"{stop['lon']},{stop['lat']}")
    coords.append(f"{end_lon},{end_lat}")

    url = "https://router.project-osrm.org/route/v1/driving/" + ";".join(coords) + "?overview=full&geometries=geojson"

    r = requests.get(url, timeout=10)
    data = r.json()
    if "routes" not in data or not data["routes"]:
        return []

    points = data["routes"][0]["geometry"]["coordinates"]
    return [[c[1], c[0]] for c in points]


def cumulative_distances(route):
    if not route:
        return []
    dists = [0.0]
    total = 0.0
    for i in range(1, len(route)):
        total += haversine_m(route[i - 1][0], route[i - 1][1], route[i][0], route[i][1])
        dists.append(total)
    return dists


def nearest_index(route, point):
    min_idx = 0
    min_dist = float('inf')
    for i, route_pt in enumerate(route):
        dist = haversine_m(route_pt[0], route_pt[1], point[0], point[1])
        if dist < min_dist:
            min_dist = dist
            min_idx = i
    return min_idx


def distance_to_route_position(route, point):
    if not route:
        return 0.0
    dists = cumulative_distances(route)
    return dists[nearest_index(route, point)]


def project_distances(route, stops):
    if not route:
        return [0.0 for _ in stops]
    dists = cumulative_distances(route)
    projected = []
    for stop in stops:
        idx = nearest_index(route, (stop['lat'], stop['lon']))
        projected.append(dists[idx])
    return projected
