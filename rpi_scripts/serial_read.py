import sqlite3
import serial
import struct
import time
DB_PATH = '/home/pi/rssi_ranging/rssi_1.db'

conn = sqlite3.connect(DB_PATH)
c = conn.cursor()
ser = serial.Serial("/dev/ttyACM0", 9600, timeout=.1)
ser.setDTR(1)
config_format = 'fffB'
a_loc_format = 'ff'
data_format = 'h4B'
config_size = struct.calcsize(config_format)
a_loc_size = struct.calcsize(a_loc_format)
data_size = struct.calcsize(data_format)
while 1:
    try:
        if ser.in_waiting > 0:
            a_loc = struct.unpack(a_loc_format, ser.read(a_loc_size))
            config = struct.unpack(config_format, ser.read(config_size))
            time.sleep(.1)
            current_millis = time.time()
            while time.time() - current_millis < 4.8:
                try:
                    if ser.in_waiting > 0:
                        data = struct.unpack(data_format, ser.read(data_size))
                        current_millis = time.time()
                        c.execute('INSERT INTO exp_final (RSSI,SNR,T_FLAT,T_FLONG,A_FLAT,A_FLONG,FREQ,SF) VALUES (?,?,?,?,?,?,?,?)',
                                                    (data[0], (1*data[1] + (2**8)*data[2] + (2**16)*data[3] + (2**32)*data[4]), round(config[1],6), round(config[2],6), round(a_loc[0],6), round(a_loc[1],6), round(config[0],1), config[3]))
                except:
                    print("error in data packets")
            conn.commit()
    except:
        print("error in config packets")