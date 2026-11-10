from geopy.distance import geodesic

def off_route(bus_location, route):

    if len(route)==0:
        return False

    bus=(bus_location["lat"],bus_location["lon"])

    min_dist=999999

    for point in route:

        dist=geodesic(bus,(point[0],point[1])).meters

        if dist < min_dist:
            min_dist = dist

    if min_dist > 80:
        return True

    return False