from socket import *
from datetime import datetime
import os.path as osp

localIP = "192.168.0.29"
localPort = 20001

bufferSize = 1024
UDPServerSocket: socket
stopThread = False

MAX_LOG_SIZE = 32_000

MESSAGES = {
    0: "{}: STANDBY, ALL INPUTS NORMAL. SCALE INDICATING: {} KG",
    1: "{}: !!WARNING!!: DOOR {} OPEN!",
    2: "{}: !!WARNING!!: SCALE ERROR: {} KG",
    8: "{}: {} CYCLE ACTIVE: SCALE CORRECTION {} KG",
    9: "{}: CYCLE ENDED: {}",
    10: "{}: SCALE VALUE: {}KG",
    11: "{}: WEIGHT CLASS MISMATCH: {} KG NOT IN CLASS {}",
    12: "{}: TIME LIMIT EXPIRED USER NOT PRESENT/NOT REGISTERED"
  }

TAG = {
    "0": "normal",
    "1": "warning",
    "2": "warning",
    "8": "normal",
    "9": "normal",
    "10": "normal",
    "11": "special",
    "12": "special"
}

STATUS = {
    1: "ACCESS GRANTED",
    2: "ACCESS DENIED",
    3: "NO ACCESS",
    4: "TIME LIMIT EXPIRED"
  }

CYCLE_TYPE = {
    0: "ENTRY",
    1: "EXIT"
}


def decode_message(msg):
    timestamp = float(msg[-1])
    msg_type = int(msg[0])
    if msg_type == 0:
        weight = int(msg[1])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), weight)
    elif msg_type == 1:
        door_no = int(msg[1])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), door_no)
    elif msg_type == 2:
        weight = int(msg[1])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), weight)
    elif msg_type == 8:
        cycle = CYCLE_TYPE[int(msg[1])]
        weight = int(msg[2])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), cycle, weight)
    elif msg_type == 9:
        end_status = STATUS[int(msg[1])]
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), end_status)
    elif msg_type == 10:
        weight = int(msg[1])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), weight)
    elif msg_type == 11:
        w_class = int(msg[1])
        weight = int(msg[2])
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp), weight, w_class)
    elif msg_type == 12:
        return MESSAGES[msg_type].format(datetime.fromtimestamp(timestamp))

    return f"{datetime.now()}: Unknown message from Client: {msg}"


def init_server():
    global localIP, localPort
    with open(osp.join("config", "IP_signatures")) as cin:
        data = cin.read().split("\n")
        localIP = data[0]
        localPort = int(data[1])

    global UDPServerSocket
    UDPServerSocket = socket(family=AF_INET, type=SOCK_DGRAM)
    UDPServerSocket.bind((localIP, localPort))
    print("UDP server up and listening")


def run_server():
    global stopThread
    init_server()

    i = 0
    while True:
        if stopThread:
            break
        bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)

        data = [str(int(x)) for x in bytesAddressPair[0]]
        data.append(str(datetime.now().timestamp()))
        message = " ".join(data)

        address = bytesAddressPair[1]
        file = osp.join("data", f"obo_{address[0]}.txt")


        size = osp.getsize(file)

        if size > MAX_LOG_SIZE:
            flag = "w"
        else:
            flag = "a"

        with open(file, flag) as fout:
            fout.write(f"{message}\n")

        print(f"{address}:{decode_message(data)}")
        i += 1
