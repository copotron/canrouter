import sys
import serial
import zmq

port = sys.argv[1]
ser = serial.Serial(port, 115200)

context = zmq.Context()
socket = context.socket(zmq.DEALER)
socket.connect("tcp://localhost:5555")

while True:
    data = ser.readline()
    socket.send(data)

# Connect arduino CAN shield with computer using an USB cable and get the port assigned and run this like:
# python ./serialmon.py COM3
