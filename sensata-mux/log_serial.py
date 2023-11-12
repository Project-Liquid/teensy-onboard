port = "/dev/cu.usbmodem146492601"

import serial
import time

ser = serial.Serial(port, 9600, timeout=1)

def log(line):
    with open("log.txt", "a") as f:
        f.write(line)

with open("log.txt", "w") as f:
    f.write("")

while True:
    while ser.in_waiting == 0:
        time.sleep(0.01)
    line = ser.readline().decode("utf-8")
    if line[1] == ",":
        log(line)
    
