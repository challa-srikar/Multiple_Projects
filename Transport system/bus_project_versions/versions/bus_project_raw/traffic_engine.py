speed_history=[]

def update_speed(data):

    if "speed" in data and data["speed"]!=None:
        speed_history.append(data["speed"])

    if len(speed_history)>10:
        speed_history.pop(0)

def congestion_detected():

    if len(speed_history)<5:
        return False

    avg=sum(speed_history)/len(speed_history)

    if avg < 15:
        return True

    return False