# This makefile is for all platforms, but doesn't include support for the HD44780, OLED, or PCF8574 displays on the Raspberry Pi.

# If you have the resampler library installed, add -DHAS_SRC to the CFLAGS line, and -lsamplerate to the LIBS line.

CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHAVE_LOG_H -I/usr/local/include
LIBS    = -lpthread -lutil
LDFLAGS = -g -L/usr/local/lib

OBJECTS = \
		AMBEFEC.o BCH.o AX25Control.o AX25Network.o BPTC19696.o CASTInfo.o Conf.o CRC.o Display.o DMRControl.o DMRCSBK.o DMRData.o DMRDataHeader.o \
		DMRDirectNetwork.o DMREMB.o DMREmbeddedData.o DMRFullLC.o DMRGatewayNetwork.o DMRLookup.o DMRLC.o DMRNetwork.o DMRShortLC.o DMRSlot.o DMRSlotType.o \
		DMRAccessControl.o DMRTA.o DMRTrellis.o DStarControl.o DStarHeader.o DStarNetwork.o DStarSlowData.o FMControl.o FMNetwork.o Golay2087.o Golay24128.o \
		Hamming.o I2CController.o IIRDirectForm1Filter.o LCDproc.o Log.o M17Control.o M17Convolution.o M17CRC.o M17LSF.o M17Network.o M17Utils.o MMDVMHost.o \
		Modem.o ModemPort.o ModemSerialPort.o Mutex.o NetworkInfo.o Nextion.o NullController.o NullDisplay.o NXDNAudio.o NXDNControl.o \
		NXDNConvolution.o NXDNCRC.o NXDNFACCH1.o NXDNIcomNetwork.o NXDNKenwoodNetwork.o NXDNLayer3.o NXDNLICH.o NXDNLookup.o NXDNNetwork.o NXDNSACCH.o \
		NXDNUDCH.o P25Audio.o P25Control.o P25Data.o P25LowSpeedData.o P25Network.o P25NID.o P25Trellis.o P25Utils.o PseudoTTYController.o POCSAGControl.o \
		POCSAGNetwork.o QR1676.o RemoteControl.o RS129.o RS634717.o RSSIInterpolator.o SerialPort.o SMeter.o StopWatch.o Sync.o SHA256.o TFTSurenoo.o Thread.o \
		Timer.o UARTController.o UDPController.o UDPSocket.o UserDB.o UserDBentry.o Utils.o YSFControl.o YSFConvolution.o YSFFICH.o YSFNetwork.o YSFPayload.o

all:		MMDVMHost RemoteCommand

MMDVMHost:	GitVersion.h $(OBJECTS) 
		$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o MMDVMHost

RemoteCommand:	Log.o RemoteCommand.o UDPSocket.o
		$(CXX) Log.o RemoteCommand.o UDPSocket.o $(LDFLAGS) $(LIBS) -o RemoteCommand

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

.PHONY install:
install: all
		install -m 755 MMDVMHost /usr/local/bin/
		install -m 755 RemoteCommand /usr/local/bin/

.PHONY install-service:
install-service: install /etc/MMDVM.ini
		@useradd --user-group -M --system mmdvm --shell /bin/false || true
		@usermod --groups dialout --append mmdvm || true
		@mkdir /var/log/mmdvm || true
		@chown mmdvm:mmdvm /var/log/mmdvm
		@cp ./linux/systemd/mmdvmhost.service /lib/systemd/system/
		@systemctl enable mmdvmhost.service

/etc/MMDVM.ini:
		@cp -n MMDVM.ini /etc/MMDVM.ini
		@sed -i 's/FilePath=./FilePath=\/var\/log\/mmdvm\//' /etc/MMDVM.ini
		@sed -i 's/Daemon=0/Daemon=1/' /etc/MMDVM.ini
		@chown mmdvm:mmdvm /etc/MMDVM.ini

.PHONY uninstall-service:
uninstall-service:
		@systemctl stop mmdvmhost.service || true
		@systemctl disable mmdvmhost.service || true
		@rm -f /usr/local/bin/MMDVMHost || true
		@rm -f /lib/systemd/system/mmdvmhost.service || true

clean:
		$(RM) MMDVMHost RemoteCommand *.o *.d *.bak *~ GitVersion.h

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif
