import serial
import time

ser = serial.Serial('/dev/tty.DSDTECHHC-05', 9600) 

try:
    for _ in range(10):  
        ser.write(bytearray([0,1,2,1,100,200,200,100])) 
        time.sleep(.5)  
finally:
    ser.close()  