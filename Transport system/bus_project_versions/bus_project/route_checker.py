from math import radians, cos, sin, asin, sqrt


def haversine_m(lat1, lon1, lat2, lon2):
    r = 6371000
    dlat = radians(lat2 - lat1)
    dlon = radians(lon2 - lon1)
    a = sin(dlat / 2) ** 2 + cos(radians(lat1)) * cos(radians(lat2)) * sin(dlon / 2) ** 2
    return 2 * r * asin(sqrt(a))


def off_route(bus_location, route):
    if len(route) == 0:
        return False

    min_dist = 999999

    for point in route:
        dist = haversine_m(bus_location['lat'], bus_location['lon'], point[0], point[1])
        if dist < min_dist:
            min_dist = dist

    return min_dist > 80
