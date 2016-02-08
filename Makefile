CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x
LIBS    =
LDFLAGS = -g

OBJECTS = \
		AMBEFEC.o BPTC19696.o Conf.o CRC.o CSBK.o Display.o DMRControl.o DMRData.o DMRDataHeader.o DMRSlot.o DMRSync.o DStarControl.o DStarHeader.o \
		DStarNetwork.o DStarSlowData.o EMB.o EmbeddedLC.o FullLC.o Golay2087.o Golay24128.o Hamming.o HomebrewDMRIPSC.o LC.o Log.o MMDVMHost.o Modem.o \
		NullDisplay.o QR1676.o RS129.o SerialController.o SHA256.o ShortLC.o SlotType.o StopWatch.o TFTSerial.o Timer.o UDPSocket.o Utils.o YSFConvolution.o \
		YSFEcho.o YSFFICH.o

all:		MMDVMHost

MMDVMHost:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o MMDVMHost

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) MMDVMHost *.o *.d *.bak *~
 