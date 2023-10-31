#
# Quick upload method to development cartridge
#

import numpy as np
import serial
import serial.tools.list_ports

def main():
    ser = connect()
    test_board_id(ser)
    upload_rom(ser, 'main.rom')
    ser.close()

def connect():
    # autofind any available boards
    ports = serial.tools.list_ports.comports()
    portfound = None
    for port in ports:
        print(port.pid, port.vid)
        if port.pid == 54 and port.vid == 0x2341:
            portfound = port.device
            break

    # specify the COM port below
    if portfound:
        ser = serial.Serial(portfound, 
                            19200, 
                            bytesize=serial.EIGHTBITS,
                            parity=serial.PARITY_NONE,
                            stopbits=serial.STOPBITS_ONE,
                            timeout=None)  # open serial port
                   
        if not ser.isOpen():
            ser.open()
    
    return ser

def test_board_id(ser):
    ser.write(b'READINFO')
    res = ser.read(8)
    print(res)
    res = ser.read(16)
    print(res)
    
    if res == b'Ph2k-32u4-v1.0.3':
        print('Connection established. All ok!')
    else:
        raise Exception('Cannot connect. Invalid response.')

def upload_rom(ser, filename):
    ser.write(b'DEVIDSST')
    rsp = ser.read(8)
    rsp = ser.read(2)
    if rsp == bytearray([0xBF,0xB7]):
        print('Chip ID verified: %s' % rsp)
    else:
        raise Exception("Incorrect chip id.")
        
    f = open(filename, 'rb')
    data = bytearray(f.read())
    f.close()
    
    # wipe second bank
    for i in range(0,4):
        ser.write(b'ESST00%02X' % ((i+4) * 0x10))
        res = ser.read(8)
        print(res)
        res = ser.read(2)
        print(res)
    
    # expand data to 256-size
    sz = len(data)
    exp = (sz // 256 + 1) * 256
    print('Expanding %i to %i' % (sz, exp))
    data.extend(np.zeros(exp - sz))
    
    offset = 16*1024//256
    for i in range(0, exp // 256):
        ser.write(b'WRBK%04X' % (i + offset))
        res = ser.read(8)
        print(res)
        parcel = data[i*256:(i+1)*256]
        ser.write(parcel)
        checksum = np.uint8(ser.read(1)[0])
        print(checksum)
        print(np.sum(parcel) & 0xFF)
        
        
if __name__ == '__main__':
    main()
