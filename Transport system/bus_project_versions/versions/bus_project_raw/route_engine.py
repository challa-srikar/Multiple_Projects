import requests

def get_route(start_lat,start_lon,end_lat,end_lon):

    url=f"https://router.project-osrm.org/route/v1/driving/{start_lon},{start_lat};{end_lon},{end_lat}?overview=full&geometries=geojson"

    r=requests.get(url)

    data=r.json()

    coords=data["routes"][0]["geometry"]["coordinates"]

    route=[]

    for c in coords:
        route.append([c[1],c[0]])

    return route