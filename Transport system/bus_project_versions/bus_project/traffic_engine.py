speed_history = []


def update_speed(data):
    if "speed" in data and data["speed"] is not None:
        # incoming GPS speed is usually m/s; convert to km/h
        kmph = float(data["speed"]) * 3.6
        speed_history.append(max(kmph, 0))

    if len(speed_history) > 20:
        speed_history.pop(0)


def estimated_speed_kmph():
    if len(speed_history) < 3:
        return 28.0
    avg = sum(speed_history) / len(speed_history)
    return max(avg, 5.0)


def congestion_detected():
    return estimated_speed_kmph() < 15


def get_heatmap_points(route):
    if not route:
        return []

    congestion = congestion_detected()
    points = []
    stride = max(len(route) // 12, 1)
    for i, pt in enumerate(route[::stride]):
        intensity = 0.8 if congestion or i % 3 == 0 else 0.35
        points.append({"lat": pt[0], "lon": pt[1], "intensity": intensity})
    return points
