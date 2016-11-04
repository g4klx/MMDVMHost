'''
 *   Copyright (C) 2016 Alex Koren
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
'''

import serial
import time
import sys
import os

e = "\xff\xff\xff"
deviceName = '/dev/ttyUSB0' #MAKE SURE THIS IS THE CORRECT DEVICE
CHECK_MODEL = 'NX3224T024'

def getBaudrate(ser, fSize=None):
    for baudrate in (2400, 4800, 9600, 19200, 38400, 57600, 115200):
        ser.baudrate = baudrate
        ser.timeout = 3000 / baudrate + .2
        print 'Trying with baudrate: ' + str(baudrate) + '...'
        ser.write("\xff\xff\xff")
        ser.write('connect')
        ser.write("\xff\xff\xff")
        r = ser.read(128)
        if 'comok' in r:
            print 'Connected with baudrate: ' + str(baudrate) + '...'
            noConnect = False
            status, unknown1, model, unknown2, version, serial, flashSize = r.strip("\xff\x00").split(',')
            print 'Status: ' + status
            print 'Model: ' + model
            print 'Version: ' + version
            print 'Serial: ' + serial
            print 'Flash size: ' + flashSize
            if fSize and fSize > flashSize:
                print 'File too big!'
                return False
            if not CHECK_MODEL in model:
                print 'Wrong Display!'
                return False
            return True
    return False

def setDownloadBaudrate(ser, fSize, baudrate):
    ser.write("")
    ser.write("whmi-wri " + str(fSize) + "," + str(baudrate) + ",0" + e)
    time.sleep(.05)
    ser.baudrate = baudrate
    ser.timeout = .5
    r = ser.read(1)
    if "\x05" in r:
        return True
    return False

def transferFile(ser, filename, fSize):
    with open(filename, 'rb') as hmif:
        dcount = 0
        while True:
            data = hmif.read(4096)
            if len(data) == 0:
                break
            dcount += len(data)
            ser.timeout = 5
            ser.write(data)
            sys.stdout.write('\rDownloading, %3.1f%%...'% (dcount / float(fSize) * 100.0))
            sys.stdout.flush()
            ser.timeout = .5
            time.sleep(.5)
            r = ser.read(1)
            if "\x05" in r:
                continue
            else:
                break
        print
    return True

def upload(ser, filename):
    if not getBaudrate(ser, os.path.getsize(filename)):
        print 'could not find baudrate'
        exit(1)

    if not setDownloadBaudrate(ser, os.path.getsize(filename), 115200):
        print 'could not set download baudrate'
        exit(1)

    if not transferFile(ser, filename, os.path.getsize(filename)):
        print 'could not transfer file'
        exit(1)

    exit(0)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print 'usage:\npython nextion.py file_to_upload.tft'
        exit(1)

    ser = serial.Serial(deviceName, 9600, timeout=5)
    if not ser:
        print 'could not open device'
        exit(1)
    if not ser.is_open:
        ser.open()

    upload(ser, sys.argv[1])
