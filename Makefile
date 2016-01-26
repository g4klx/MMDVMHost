CC      = g++
CFLAGS  = -g -O2 -Wall -std=c++11
LIBS    =
LDFLAGS = -g

all:		MMDVMHost

MMDVMHost:	AMBEFEC.o BPTC19696.o Conf.o CRC.o CSBK.o Display.o DMRControl.o DMRData.o DMRDataHeader.o DMRSlot.o DMRSync.o DStarEcho.o DStarHeader.o DStarNetwork.o \
						EMB.o EmbeddedLC.o FullLC.o Golay2087.o Golay24128.o Hamming.o HomebrewDMRIPSC.o LC.o Log.o MMDVMHost.o Modem.o NullDisplay.o QR1676.o RS129.o \
						SerialController.o SHA256.o ShortLC.o SlotType.o StopWatch.o TFTSerial.o Timer.o UDPSocket.o Utils.o YSFEcho.o
		$(CC) $(LDFLAGS) -o MMDVMHost AMBEFEC.o BPTC19696.o Conf.o CRC.o CSBK.o Display.o DMRControl.o DMRData.o DMRDataHeader.o DMRSlot.o DMRSync.o DStarEcho.o \
						DStarHeader.o DStarNetwork.o EMB.o EmbeddedLC.o FullLC.o Golay2087.o Golay24128.o  Hamming.o HomebrewDMRIPSC.o LC.o Log.o MMDVMHost.o Modem.o \
						NullDisplay.o QR1676.o RS129.o SerialController.o SHA256.o ShortLC.o SlotType.o StopWatch.o TFTSerial.o Timer.o UDPSocket.o Utils.o YSFEcho.o $(LIBS)

AMBEFEC.o:	AMBEFEC.cpp AMBEFEC.h Golay24128.h
		$(CC) $(CFLAGS) -c AMBEFEC.cpp

BPTC19696.o:	BPTC19696.cpp BPTC19696.h Utils.h Hamming.h
		$(CC) $(CFLAGS) -c BPTC19696.cpp

Conf.o:	Conf.cpp Conf.h Log.h
		$(CC) $(CFLAGS) -c Conf.cpp

CRC.o:	CRC.cpp CRC.h Utils.h
		$(CC) $(CFLAGS) -c CRC.cpp

CSBK.o:	CSBK.cpp CSBK.h Utils.h DMRDefines.h BPTC19696.h CRC.h
		$(CC) $(CFLAGS) -c CSBK.cpp

Display.o:	Display.cpp Display.h
		$(CC) $(CFLAGS) -c Display.cpp

DMRControl.o:	DMRControl.cpp DMRControl.h DMRSlot.h DMRData.h Modem.h HomebrewDMRIPSC.h Defines.h CSBK.h Log.h Display.h
		$(CC) $(CFLAGS) -c DMRControl.cpp

DMRData.o:	DMRData.cpp DMRData.h DMRDefines.h Utils.h Log.h
		$(CC) $(CFLAGS) -c DMRData.cpp

DMRDataHeader.o:	DMRDataHeader.cpp DMRDataHeader.h DMRDefines.h Utils.h Log.h CRC.h BPTC19696.h
		$(CC) $(CFLAGS) -c DMRDataHeader.cpp

DMRSlot.o:	DMRSlot.cpp DMRSlot.h DMRData.h Modem.h HomebrewDMRIPSC.h Defines.h Log.h EmbeddedLC.h RingBuffer.h Timer.h LC.h SlotType.h DMRSync.h FullLC.h \
						EMB.h CRC.h CSBK.h ShortLC.h Utils.h Display.h StopWatch.h AMBEFEC.h DMRDataHeader.h
		$(CC) $(CFLAGS) -c DMRSlot.cpp

DMRSync.o:	DMRSync.cpp DMRSync.h DMRDefines.h
		$(CC) $(CFLAGS) -c DMRSync.cpp

DStarEcho.o:	DStarEcho.cpp DStarEcho.h RingBuffer.h Timer.h
		$(CC) $(CFLAGS) -c DStarEcho.cpp

DStarHeader.o:	DStarHeader.cpp DStarHeader.h DStarDefines.h CRC.h
		$(CC) $(CFLAGS) -c DStarHeader.cpp

DStarNetwork.o:	DStarNetwork.cpp DStarNetwork.h Log.h UDPSocket.h RingBuffer.h Utils.h StopWatch.h DStarDefines.h Defines.h Timer.h
		$(CC) $(CFLAGS) -c DStarNetwork.cpp

EMB.o:		EMB.cpp EMB.h
		$(CC) $(CFLAGS) -c EMB.cpp

EmbeddedLC.o:	EmbeddedLC.cpp EmbeddedLC.h CRC.h Utils.h LC.h Hamming.h Log.h
		$(CC) $(CFLAGS) -c EmbeddedLC.cpp

FullLC.o:	FullLC.cpp FullLC.h BPTC19696.h LC.h SlotType.h Log.h DMRDefines.h RS129.h
		$(CC) $(CFLAGS) -c FullLC.cpp

Golay2087.o:	Golay2087.cpp Golay2087.h
		$(CC) $(CFLAGS) -c Golay2087.cpp

Golay24128.o:	Golay24128.cpp Golay24128.h
		$(CC) $(CFLAGS) -c Golay24128.cpp

Hamming.o:	Hamming.cpp Hamming.h
		$(CC) $(CFLAGS) -c Hamming.cpp

HomebrewDMRIPSC.o:	HomebrewDMRIPSC.cpp HomebrewDMRIPSC.h Log.h UDPSocket.h Timer.h DMRData.h RingBuffer.h Utils.h SHA256.h StopWatch.h
		$(CC) $(CFLAGS) -c HomebrewDMRIPSC.cpp

LC.o:	LC.cpp LC.h Utils.h DMRDefines.h
		$(CC) $(CFLAGS) -c LC.cpp
	
Log.o:	Log.cpp Log.h
		$(CC) $(CFLAGS) -c Log.cpp

MMDVMHost.o:	MMDVMHost.cpp MMDVMHost.h Conf.h Log.h Version.h Modem.h StopWatch.h Defines.h DMRSync.h DStarEcho.h YSFEcho.h DMRControl.h HomebrewDMRIPSC.h \
							Display.h TFTSerial.h NullDisplay.h DStarNetwork.h
		$(CC) $(CFLAGS) -c MMDVMHost.cpp

Modem.o:	Modem.cpp Modem.h Log.h SerialController.h Timer.h RingBuffer.h Utils.o DMRDefines.h DStarDefines.h YSFDefines.h Defines.h
		$(CC) $(CFLAGS) -c Modem.cpp

NullDisplay.o:	NullDisplay.cpp NullDisplay.h Display.h
		$(CC) $(CFLAGS) -c NullDisplay.cpp

QR1676.o:	QR1676.cpp QR1676.h Log.h
		$(CC) $(CFLAGS) -c QR1676.cpp

RS129.o:	RS129.cpp RS129.h
		$(CC) $(CFLAGS) -c RS129.cpp

SerialController.o:	SerialController.cpp SerialController.h Log.h
		$(CC) $(CFLAGS) -c SerialController.cpp

SHA256.o:	SHA256.cpp SHA256.h
		$(CC) $(CFLAGS) -c SHA256.cpp

ShortLC.o:	ShortLC.cpp ShortLC.h Utils.h Hamming.h
		$(CC) $(CFLAGS) -c ShortLC.cpp

SlotType.o:	SlotType.cpp SlotType.h Golay2087.h
		$(CC) $(CFLAGS) -c SlotType.cpp

StopWatch.o:	StopWatch.cpp StopWatch.h
		$(CC) $(CFLAGS) -c StopWatch.cpp

TFTSerial.o:	TFTSerial.cpp TFTSerial.h Display.h SerialController.h Log.h
		$(CC) $(CFLAGS) -c TFTSerial.cpp

Timer.o:	Timer.cpp Timer.h
		$(CC) $(CFLAGS) -c Timer.cpp

UDPSocket.o:	UDPSocket.cpp UDPSocket.h Log.h
		$(CC) $(CFLAGS) -c UDPSocket.cpp

Utils.o:	Utils.cpp Utils.h Log.h
		$(CC) $(CFLAGS) -c Utils.cpp

YSFEcho.o:	YSFEcho.cpp YSFEcho.h YSFDefines.h RingBuffer.h Timer.h
		$(CC) $(CFLAGS) -c YSFEcho.cpp

clean:
		$(RM) MMDVMHost *.o *.bak *~
