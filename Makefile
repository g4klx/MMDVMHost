# This makefile is for all platforms, but doesn't include support for the HD44780 display on the Raspberry Pi.

CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x
LIBS    =
LDFLAGS = -g

OBJECTS = \
		AMBEFEC.o BPTC19696.o Conf.o CRC.o Display.o DMRControl.o DMRCSBK.o DMRData.o DMRDataHeader.o DMREMB.o DMREmbeddedLC.o DMRFullLC.o DMRIPSC.o DMRLC.o DMRShortLC.o \
		DMRSlot.o DMRSlotType.o DStarControl.o DStarHeader.o DStarNetwork.o DStarSlowData.o Golay2087.o Golay24128.o Hamming.o Log.o MMDVMHost.o Modem.o NullDisplay.o \
		QR1676.o RS129.o SerialController.o SHA256.o StopWatch.o Sync.o TFTSerial.o Timer.o Trellis.o UDPSocket.o Utils.o YSFControl.o YSFConvolution.o YSFFICH.o \
		YSFParrot.o YSFPayload.o

all:		MMDVMHost

MMDVMHost:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o MMDVMHost

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) MMDVMHost *.o *.d *.bak *~
 