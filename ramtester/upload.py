#
# Quick upload method to development cartridge
#
# Requirements: pyserial and tqdm modules
#

import numpy as np
import serial
import serial.tools.list_ports
from tqdm import tqdm

def main():
    ser = connect()
    upload_rom(ser, 'main.rom', 3)
    ser.close()

def connect():
    # autofind any available boards
    ports = serial.tools.list_ports.comports()
    portfound = None
    for port in ports:
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
    """
    Check that a connection to the board can be established
    """
    ser.write(b'READINFO')
    res = ser.read(8)
    print(res)
    res = ser.read(16)
    print(res)
    
    if res == b'Ph2k-32u4-v1.0.3':
        print('Connection established. All ok!')
    else:
        raise Exception('Cannot connect. Invalid response.')

def upload_rom(ser, filename, bank=0):
    """
    Upload the ROM file
    """
    # check that a connection to the board can be established
    ser.write(b'DEVIDSST')
    rsp = ser.read(8)
    rsp = ser.read(2)
    if rsp != bytearray([0xBF,0xB7]):
        raise Exception("Incorrect chip id.")
        
    f = open(filename, 'rb')
    data = bytearray(f.read())
    f.close()
    
    # wipe second bank
    print('Wiping bank %i' % bank)
    for i in tqdm(range(0,4)):
        ser.write(b'ESST00%02X' % ((i+bank*4) * 0x10))
        res = ser.read(8)
        res = ser.read(2)

    # expand data to first 256 byte increment
    sz = len(data)
    exp = (sz // 256 + 1) * 256
    data.extend(np.zeros(exp - sz))
    
    offset = bank * 16 * 1024 // 256

    print('Writing data to bank %i' % bank)
    for i in tqdm(range(0, exp // 256)):
        ser.write(b'WRBK%04X' % (i + offset))
        res = ser.read(8)
        parcel = data[i*256:(i+1)*256]
        ser.write(parcel)
        checksum = np.uint8(ser.read(1)[0])
        
        
if __name__ == '__main__':
    main()
