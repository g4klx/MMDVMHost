/*
 *   Copyright (C) 2015-2023,2025 by Jonathan Naylor G4KLX
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
 */

#include "DStarDefines.h"
#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum class SECTION {
	NONE,
	GENERAL,
	INFO,
	LOG,
	CWID,
	DMRID_LOOKUP,
	NXDNID_LOOKUP,
	MODEM,
	TRANSPARENT,
	DSTAR,
	DMR,
	FUSION,
	P25,
	NXDN,
	M17,
	POCSAG,
	FM,
	AX25,
	DSTAR_NETWORK,
	DMR_NETWORK,
	FUSION_NETWORK,
	P25_NETWORK,
	NXDN_NETWORK,
	M17_NETWORK,
	POCSAG_NETWORK,
	FM_NETWORK,
	AX25_NETWORK,
	TFTSERIAL_DISPLAY,
	HD44780_DISPLAY,
	NEXTION_DISPLAY,
	OLED_DISPLAY,
	LCDPROC_DISPLAY,
	LOCK_FILE,
	REMOTE_CONTROL
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_id(0U),
m_timeout(120U),
m_duplex(true),
m_display(),
m_daemon(false),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_latitude(0.0F),
m_longitude(0.0F),
m_height(0),
m_location(),
m_description(),
m_url(),
m_logDisplayLevel(0U),
m_logFileLevel(0U),
m_logFilePath(),
m_logFileRoot(),
m_logFileRotate(true),
m_cwIdEnabled(false),
m_cwIdTime(10U),
m_cwIdCallsign(),
m_dmrIdLookupFile(),
m_dmrIdLookupTime(0U),
m_nxdnIdLookupFile(),
m_nxdnIdLookupTime(0U),
m_modemProtocol("uart"),
m_modemUARTPort(),
m_modemUARTSpeed(115200U),
m_modemI2CPort(),
m_modemI2CAddress(0x22U),
m_modemModemAddress(),
m_modemModemPort(0U),
m_modemLocalAddress(),
m_modemLocalPort(0U),
m_modemRXInvert(false),
m_modemTXInvert(false),
m_modemPTTInvert(false),
m_modemTXDelay(100U),
m_modemDMRDelay(0U),
m_modemTXOffset(0),
m_modemRXOffset(0),
m_modemRXDCOffset(0),
m_modemTXDCOffset(0),
m_modemRFLevel(100.0F),
m_modemRXLevel(50.0F),
m_modemCWIdTXLevel(50.0F),
m_modemDStarTXLevel(50.0F),
m_modemDMRTXLevel(50.0F),
m_modemYSFTXLevel(50.0F),
m_modemP25TXLevel(50.0F),
m_modemNXDNTXLevel(50.0F),
m_modemM17TXLevel(50.0F),
m_modemPOCSAGTXLevel(50.0F),
m_modemFMTXLevel(50.0F),
m_modemAX25TXLevel(50.0F),
m_modemRSSIMappingFile(),
m_modemUseCOSAsLockout(false),
m_modemTrace(false),
m_modemDebug(false),
m_transparentEnabled(false),
m_transparentRemoteAddress(),
m_transparentRemotePort(0U),
m_transparentLocalPort(0U),
m_transparentSendFrameType(0U),
m_dstarEnabled(false),
m_dstarModule("C"),
m_dstarSelfOnly(false),
m_dstarBlackList(),
m_dstarWhiteList(),
m_dstarAckReply(true),
m_dstarAckTime(750U),
m_dstarAckMessage(DSTAR_ACK::BER),
m_dstarErrorReply(true),
m_dstarRemoteGateway(false),
m_dstarModeHang(10U),
m_dmrEnabled(false),
m_dmrBeacons(DMR_BEACONS::OFF),
m_dmrBeaconInterval(60U),
m_dmrBeaconDuration(3U),
m_dmrId(0U),
m_dmrColorCode(2U),
m_dmrSelfOnly(false),
m_dmrEmbeddedLCOnly(false),
m_dmrDumpTAData(true),
m_dmrPrefixes(),
m_dmrBlackList(),
m_dmrWhiteList(),
m_dmrSlot1TGWhiteList(),
m_dmrSlot2TGWhiteList(),
m_dmrCallHang(10U),
m_dmrTXHang(4U),
m_dmrModeHang(10U),
m_dmrOVCM(DMR_OVCM::OFF),
m_dmrProtect(false),
m_fusionEnabled(false),
m_fusionLowDeviation(false),
m_fusionRemoteGateway(false),
m_fusionSelfOnly(false),
m_fusionTXHang(4U),
m_fusionModeHang(10U),
m_p25Enabled(false),
m_p25Id(0U),
m_p25NAC(0x293U),
m_p25SelfOnly(false),
m_p25OverrideUID(false),
m_p25RemoteGateway(false),
m_p25TXHang(5U),
m_p25ModeHang(10U),
m_nxdnEnabled(false),
m_nxdnId(0U),
m_nxdnRAN(1U),
m_nxdnSelfOnly(false),
m_nxdnRemoteGateway(false),
m_nxdnTXHang(5U),
m_nxdnModeHang(10U),
m_m17Enabled(false),
m_m17CAN(0U),
m_m17SelfOnly(false),
m_m17AllowEncryption(false),
m_m17TXHang(5U),
m_m17ModeHang(10U),
m_pocsagEnabled(false),
m_pocsagFrequency(0U),
m_fmEnabled(false),
m_fmCallsign(),
m_fmCallsignSpeed(20U),
m_fmCallsignFrequency(1000U),
m_fmCallsignTime(10U),
m_fmCallsignHoldoff(1U),
m_fmCallsignHighLevel(35.0F),
m_fmCallsignLowLevel(15.0F),
m_fmCallsignAtStart(true),
m_fmCallsignAtEnd(true),
m_fmCallsignAtLatch(true),
m_fmRFAck("K"),
m_fmExtAck("N"),
m_fmAckSpeed(20U),
m_fmAckFrequency(1750U),
m_fmAckMinTime(5U),
m_fmAckDelay(1000U),
m_fmAckLevel(80.0F),
m_fmTimeout(180U),
m_fmTimeoutLevel(80.0F),
m_fmCTCSSFrequency(88.6F),
m_fmCTCSSHighThreshold(30U),
m_fmCTCSSLowThreshold(20U),
m_fmCTCSSLevel(2.0F),
m_fmKerchunkTime(0U),
m_fmHangTime(7U),
m_fmAccessMode(1U),
m_fmLinkMode(false),
m_fmCOSInvert(false),
m_fmNoiseSquelch(false),
m_fmSquelchHighThreshold(30U),
m_fmSquelchLowThreshold(20U),
m_fmRFAudioBoost(1U),
m_fmMaxDevLevel(90.0F),
m_fmExtAudioBoost(1U),
m_fmModeHang(10U),
m_ax25Enabled(false),
m_ax25TXDelay(300U),
m_ax25RXTwist(6),
m_ax25SlotTime(30U),
m_ax25PPersist(128U),
m_ax25Trace(false),
m_dstarNetworkEnabled(false),
m_dstarGatewayAddress(),
m_dstarGatewayPort(0U),
m_dstarLocalAddress(),
m_dstarLocalPort(0U),
m_dstarNetworkModeHang(3U),
m_dstarNetworkDebug(false),
m_dmrNetworkEnabled(false),
m_dmrNetworkType("Gateway"),
m_dmrNetworkRemoteAddress(),
m_dmrNetworkRemotePort(0U),
m_dmrNetworkLocalAddress(),
m_dmrNetworkLocalPort(0U),
m_dmrNetworkPassword(),
m_dmrNetworkOptions(),
m_dmrNetworkDebug(false),
m_dmrNetworkJitter(360U),
m_dmrNetworkSlot1(true),
m_dmrNetworkSlot2(true),
m_dmrNetworkModeHang(3U),
m_fusionNetworkEnabled(false),
m_fusionNetworkLocalAddress(),
m_fusionNetworkLocalPort(0U),
m_fusionNetworkGatewayAddress(),
m_fusionNetworkGatewayPort(0U),
m_fusionNetworkModeHang(3U),
m_fusionNetworkDebug(false),
m_p25NetworkEnabled(false),
m_p25GatewayAddress(),
m_p25GatewayPort(0U),
m_p25LocalAddress(),
m_p25LocalPort(0U),
m_p25NetworkModeHang(3U),
m_p25NetworkDebug(false),
m_nxdnNetworkEnabled(false),
m_nxdnNetworkProtocol("Icom"),
m_nxdnGatewayAddress(),
m_nxdnGatewayPort(0U),
m_nxdnLocalAddress(),
m_nxdnLocalPort(0U),
m_nxdnNetworkModeHang(3U),
m_nxdnNetworkDebug(false),
m_m17NetworkEnabled(false),
m_m17GatewayAddress(),
m_m17GatewayPort(0U),
m_m17LocalAddress(),
m_m17LocalPort(0U),
m_m17NetworkModeHang(3U),
m_m17NetworkDebug(false),
m_pocsagNetworkEnabled(false),
m_pocsagGatewayAddress(),
m_pocsagGatewayPort(0U),
m_pocsagLocalAddress(),
m_pocsagLocalPort(0U),
m_pocsagNetworkModeHang(3U),
m_pocsagNetworkDebug(false),
m_fmNetworkEnabled(false),
m_fmNetworkProtocol("USRP"),
m_fmNetworkSampleRate(48000U),
m_fmNetworkSquelchFile(),
m_fmGatewayAddress(),
m_fmGatewayPort(0U),
m_fmLocalAddress(),
m_fmLocalPort(0U),
m_fmPreEmphasis(true),
m_fmDeEmphasis(true),
m_fmTXAudioGain(1.0F),
m_fmRXAudioGain(1.0F),
m_fmNetworkModeHang(3U),
m_fmNetworkDebug(false),
m_ax25NetworkEnabled(false),
m_ax25NetworkPort(),
m_ax25NetworkSpeed(9600U),
m_ax25NetworkDebug(false),
m_tftSerialPort("/dev/ttyAMA0"),
m_tftSerialBrightness(50U),
m_tftSerialScreenLayout(0U),
m_hd44780Rows(2U),
m_hd44780Columns(16U),
m_hd44780Pins(),
m_hd44780i2cAddress(),
m_hd44780PWM(false),
m_hd44780PWMPin(),
m_hd44780PWMBright(),
m_hd44780PWMDim(),
m_hd44780DisplayClock(false),
m_hd44780UTC(false),
m_nextionPort("/dev/ttyAMA0"),
m_nextionBrightness(50U),
m_nextionDisplayClock(false),
m_nextionUTC(false),
m_nextionIdleBrightness(20U),
m_nextionScreenLayout(0U),
m_nextionTempInFahrenheit(false),
m_nextionOutput(false),
m_nextionUDPPort(6759),
m_oledType(3U),
m_oledBrightness(0U),
m_oledInvert(false),
m_oledScroll(false),
m_oledRotate(false),
m_oledLogoScreensaver(true),
m_lcdprocAddress(),
m_lcdprocPort(0U),
m_lcdprocLocalPort(0U),
m_lcdprocDisplayClock(false),
m_lcdprocUTC(false),
m_lcdprocDimOnIdle(false),
m_lockFileEnabled(false),
m_lockFileName(),
m_remoteControlEnabled(false),
m_remoteControlAddress("127.0.0.1"),
m_remoteControlPort(0U)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
	FILE* fp = ::fopen(m_file.c_str(), "rt");
	if (fp == nullptr) {
		::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
		return false;
	}

	SECTION section = SECTION::NONE;

	char buffer[BUFFER_SIZE];
	while (::fgets(buffer, BUFFER_SIZE, fp) != nullptr) {
		if (buffer[0U] == '#')
			continue;

		if (buffer[0U] == '[') {
			if (::strncmp(buffer, "[General]", 9U) == 0)
				section = SECTION::GENERAL;
			else if (::strncmp(buffer, "[Info]", 6U) == 0)
				section = SECTION::INFO;
			else if (::strncmp(buffer, "[Log]", 5U) == 0)
				section = SECTION::LOG;
			else if (::strncmp(buffer, "[CW Id]", 7U) == 0)
				section = SECTION::CWID;
			else if (::strncmp(buffer, "[DMR Id Lookup]", 15U) == 0)
				section = SECTION::DMRID_LOOKUP;
			else if (::strncmp(buffer, "[NXDN Id Lookup]", 16U) == 0)
				section = SECTION::NXDNID_LOOKUP;
			else if (::strncmp(buffer, "[Modem]", 7U) == 0)
				section = SECTION::MODEM;
			else if (::strncmp(buffer, "[Transparent Data]", 18U) == 0)
				section = SECTION::TRANSPARENT;
			else if (::strncmp(buffer, "[D-Star]", 8U) == 0)
				section = SECTION::DSTAR;
			else if (::strncmp(buffer, "[DMR]", 5U) == 0)
				section = SECTION::DMR;
			else if (::strncmp(buffer, "[System Fusion]", 15U) == 0)
				section = SECTION::FUSION;
			else if (::strncmp(buffer, "[P25]", 5U) == 0)
				section = SECTION::P25;
			else if (::strncmp(buffer, "[NXDN]", 6U) == 0)
				section = SECTION::NXDN;
			else if (::strncmp(buffer, "[M17]", 5U) == 0)
				section = SECTION::M17;
			else if (::strncmp(buffer, "[POCSAG]", 8U) == 0)
				section = SECTION::POCSAG;
			else if (::strncmp(buffer, "[FM]", 4U) == 0)
				section = SECTION::FM;
			else if (::strncmp(buffer, "[AX.25]", 7U) == 0)
				section = SECTION::AX25;
			else if (::strncmp(buffer, "[D-Star Network]", 16U) == 0)
				section = SECTION::DSTAR_NETWORK;
			else if (::strncmp(buffer, "[DMR Network]", 13U) == 0)
				section = SECTION::DMR_NETWORK;
			else if (::strncmp(buffer, "[System Fusion Network]", 23U) == 0)
				section = SECTION::FUSION_NETWORK;
			else if (::strncmp(buffer, "[P25 Network]", 13U) == 0)
				section = SECTION::P25_NETWORK;
			else if (::strncmp(buffer, "[NXDN Network]", 14U) == 0)
				section = SECTION::NXDN_NETWORK;
			else if (::strncmp(buffer, "[M17 Network]", 13U) == 0)
				section = SECTION::M17_NETWORK;
			else if (::strncmp(buffer, "[POCSAG Network]", 16U) == 0)
				section = SECTION::POCSAG_NETWORK;
			else if (::strncmp(buffer, "[FM Network]", 12U) == 0)
				section = SECTION::FM_NETWORK;
			else if (::strncmp(buffer, "[AX.25 Network]", 15U) == 0)
				section = SECTION::AX25_NETWORK;
			else if (::strncmp(buffer, "[TFT Serial]", 12U) == 0)
				section = SECTION::TFTSERIAL_DISPLAY;
			else if (::strncmp(buffer, "[HD44780]", 9U) == 0)
				section = SECTION::HD44780_DISPLAY;
			else if (::strncmp(buffer, "[Nextion]", 9U) == 0)
				section = SECTION::NEXTION_DISPLAY;
			else if (::strncmp(buffer, "[OLED]", 6U) == 0)
				section = SECTION::OLED_DISPLAY;
			else if (::strncmp(buffer, "[LCDproc]", 9U) == 0)
				section = SECTION::LCDPROC_DISPLAY;
			else if (::strncmp(buffer, "[Lock File]", 11U) == 0)
				section = SECTION::LOCK_FILE;
			else if (::strncmp(buffer, "[Remote Control]", 16U) == 0)
				section = SECTION::REMOTE_CONTROL;
			else
				section = SECTION::NONE;

			continue;
		}

		char* key = ::strtok(buffer, " \t=\r\n");
		if (key == nullptr)
			continue;

		char* value = ::strtok(nullptr, "\r\n");
		if (value == nullptr)
			continue;

		// Remove quotes from the value
		size_t len = ::strlen(value);
		if (len > 1U && *value == '"' && value[len - 1U] == '"') {
			value[len - 1U] = '\0';
			value++;
		} else {
			char *p;

			// if value is not quoted, remove after # (to make comment)
			if ((p = strchr(value, '#')) != nullptr)
				*p = '\0';

			// remove trailing tab/space
			for (p = value + strlen(value) - 1U; p >= value && (*p == '\t' || *p == ' '); p--)
				*p = '\0';
		}

		if (section == SECTION::GENERAL) {
			if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_fmCallsign = m_cwIdCallsign = m_callsign = value;
			} else if (::strcmp(key, "Id") == 0)
				m_id = m_p25Id = m_dmrId = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Timeout") == 0)
				m_fmTimeout = m_timeout = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Duplex") == 0)
				m_duplex = ::atoi(value) == 1;
			else if (::strcmp(key, "ModeHang") == 0)
				m_dstarNetworkModeHang = m_dmrNetworkModeHang = m_fusionNetworkModeHang = m_p25NetworkModeHang = m_nxdnNetworkModeHang = m_m17NetworkModeHang = m_fmNetworkModeHang = 
				m_dstarModeHang        = m_dmrModeHang        = m_fusionModeHang        = m_p25ModeHang        = m_nxdnModeHang        = m_m17ModeHang        = m_fmModeHang        = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RFModeHang") == 0)
				m_dstarModeHang = m_dmrModeHang = m_fusionModeHang = m_p25ModeHang = m_nxdnModeHang = m_m17ModeHang = m_fmModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "NetModeHang") == 0)
				m_dstarNetworkModeHang = m_dmrNetworkModeHang = m_fusionNetworkModeHang = m_p25NetworkModeHang = m_nxdnNetworkModeHang = m_m17NetworkModeHang = m_fmNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Display") == 0)
				m_display = value;
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
		} else if (section == SECTION::INFO) {
			if (::strcmp(key, "TXFrequency") == 0)
				m_pocsagFrequency = m_txFrequency = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RXFrequency") == 0)
				m_rxFrequency = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Power") == 0)
				m_power = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Latitude") == 0)
				m_latitude = float(::atof(value));
			else if (::strcmp(key, "Longitude") == 0)
				m_longitude = float(::atof(value));
			else if (::strcmp(key, "Height") == 0)
				m_height = ::atoi(value);
			else if (::strcmp(key, "Location") == 0)
				m_location = value;
			else if (::strcmp(key, "Description") == 0)
				m_description = value;
			else if (::strcmp(key, "URL") == 0)
				m_url = value;
		} else if (section == SECTION::LOG) {
			if (::strcmp(key, "FilePath") == 0)
				m_logFilePath = value;
			else if (::strcmp(key, "FileRoot") == 0)
				m_logFileRoot = value;
			else if (::strcmp(key, "FileLevel") == 0)
				m_logFileLevel = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayLevel") == 0)
				m_logDisplayLevel = (unsigned int)::atoi(value);
			else if (::strcmp(key, "FileRotate") == 0)
				m_logFileRotate = ::atoi(value) == 1;
		} else if (section == SECTION::CWID) {
			if (::strcmp(key, "Enable") == 0)
				m_cwIdEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Time") == 0)
				m_cwIdTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_cwIdCallsign = value;
			}
		} else if (section == SECTION::DMRID_LOOKUP) {
			if (::strcmp(key, "File") == 0)
				m_dmrIdLookupFile = value;
			else if (::strcmp(key, "Time") == 0)
				m_dmrIdLookupTime = (unsigned int)::atoi(value);
		} else if (section == SECTION::NXDNID_LOOKUP) {
			if (::strcmp(key, "File") == 0)
				m_nxdnIdLookupFile = value;
			else if (::strcmp(key, "Time") == 0)
				m_nxdnIdLookupTime = (unsigned int)::atoi(value);
		} else if (section == SECTION::MODEM) {
			if (::strcmp(key, "Protocol") == 0)
				m_modemProtocol = value;
			else if (::strcmp(key, "UARTPort") == 0)
				m_modemUARTPort = value;
			else if (::strcmp(key, "UARTSpeed") == 0)
				m_modemUARTSpeed = (unsigned int)::atoi(value);
			else if (::strcmp(key, "I2CPort") == 0)
				m_modemI2CPort = value;
			else if (::strcmp(key, "I2CAddress") == 0)
				m_modemI2CAddress = (unsigned int)::strtoul(value, nullptr, 16);
			else if (::strcmp(key, "ModemAddress") == 0)
				m_modemModemAddress = value;
			else if (::strcmp(key, "ModemPort") == 0)
				m_modemModemPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_modemLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_modemLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "RXInvert") == 0)
				m_modemRXInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "TXInvert") == 0)
				m_modemTXInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "PTTInvert") == 0)
				m_modemPTTInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "TXDelay") == 0)
				m_ax25TXDelay = m_modemTXDelay = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DMRDelay") == 0)
				m_modemDMRDelay = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RXOffset") == 0)
				m_modemRXOffset = ::atoi(value);
			else if (::strcmp(key, "TXOffset") == 0)
				m_modemTXOffset = ::atoi(value);
			else if (::strcmp(key, "RXDCOffset") == 0)
				m_modemRXDCOffset = ::atoi(value);
			else if (::strcmp(key, "TXDCOffset") == 0)
				m_modemTXDCOffset = ::atoi(value);
			else if (::strcmp(key, "RFLevel") == 0)
				m_modemRFLevel = float(::atof(value));
			else if (::strcmp(key, "RXLevel") == 0)
				m_modemRXLevel = float(::atof(value));
			else if (::strcmp(key, "TXLevel") == 0)
				m_modemAX25TXLevel = m_modemFMTXLevel = m_modemCWIdTXLevel = m_modemDStarTXLevel = m_modemDMRTXLevel = m_modemYSFTXLevel = m_modemP25TXLevel = m_modemNXDNTXLevel = m_modemM17TXLevel = m_modemPOCSAGTXLevel = float(::atof(value));
			else if (::strcmp(key, "CWIdTXLevel") == 0)
				m_modemCWIdTXLevel = float(::atof(value));
			else if (::strcmp(key, "D-StarTXLevel") == 0)
				m_modemDStarTXLevel = float(::atof(value));
			else if (::strcmp(key, "DMRTXLevel") == 0)
				m_modemDMRTXLevel = float(::atof(value));
			else if (::strcmp(key, "YSFTXLevel") == 0)
				m_modemYSFTXLevel = float(::atof(value));
			else if (::strcmp(key, "P25TXLevel") == 0)
				m_modemP25TXLevel = float(::atof(value));
			else if (::strcmp(key, "NXDNTXLevel") == 0)
				m_modemNXDNTXLevel = float(::atof(value));
			else if (::strcmp(key, "M17TXLevel") == 0)
				m_modemM17TXLevel = float(::atof(value));
			else if (::strcmp(key, "POCSAGTXLevel") == 0)
				m_modemPOCSAGTXLevel = float(::atof(value));
			else if (::strcmp(key, "FMTXLevel") == 0)
				m_modemFMTXLevel = float(::atof(value));
			else if (::strcmp(key, "AX25TXLevel") == 0)
				m_modemAX25TXLevel = float(::atof(value));
			else if (::strcmp(key, "RSSIMappingFile") == 0)
				m_modemRSSIMappingFile = value;
			else if (::strcmp(key, "UseCOSAsLockout") == 0)
				m_modemUseCOSAsLockout = ::atoi(value) == 1;
			else if (::strcmp(key, "Trace") == 0)
				m_modemTrace = ::atoi(value) == 1;
			else if (::strcmp(key, "Debug") == 0)
				m_modemDebug = ::atoi(value) == 1;
		} else if (section == SECTION::TRANSPARENT) {
			if (::strcmp(key, "Enable") == 0)
				m_transparentEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "RemoteAddress") == 0)
				m_transparentRemoteAddress = value;
			else if (::strcmp(key, "RemotePort") == 0)
				m_transparentRemotePort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalPort") == 0)
				m_transparentLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "SendFrameType") == 0)
				m_transparentSendFrameType = (unsigned int)::atoi(value);
		} else if (section == SECTION::DSTAR) {
			if (::strcmp(key, "Enable") == 0)
				m_dstarEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Module") == 0) {
				// Convert the module to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_dstarModule = value;
			} else if (::strcmp(key, "SelfOnly") == 0)
				m_dstarSelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "BlackList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					if (::strlen(p) > 0U) {
						for (unsigned int i = 0U; p[i] != 0; i++)
							p[i] = ::toupper(p[i]);
						std::string callsign = std::string(p);
						callsign.resize(DSTAR_LONG_CALLSIGN_LENGTH, ' ');
						m_dstarBlackList.push_back(callsign);
					}
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "WhiteList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					if (::strlen(p) > 0U) {
						for (unsigned int i = 0U; p[i] != 0; i++)
							p[i] = ::toupper(p[i]);
						std::string callsign = std::string(p);
						callsign.resize(DSTAR_LONG_CALLSIGN_LENGTH, ' ');
						m_dstarWhiteList.push_back(callsign);
					}
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "AckReply") == 0)
				m_dstarAckReply = ::atoi(value) == 1;
			else if (::strcmp(key, "AckTime") == 0)
				m_dstarAckTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AckMessage") == 0) {
				m_dstarAckMessage = DSTAR_ACK(::atoi(value));
				if ((m_dstarAckMessage != DSTAR_ACK::BER) && (m_dstarAckMessage != DSTAR_ACK::RSSI) && (m_dstarAckMessage != DSTAR_ACK::SMETER))
					m_dstarAckMessage = DSTAR_ACK::BER;
			} else if (::strcmp(key, "ErrorReply") == 0)
				m_dstarErrorReply = ::atoi(value) == 1;
			else if (::strcmp(key, "RemoteGateway") == 0)
				m_dstarRemoteGateway = ::atoi(value) == 1;
			else if (::strcmp(key, "ModeHang") == 0)
				m_dstarModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::DMR) {
			if (::strcmp(key, "Enable") == 0)
				m_dmrEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Beacons") == 0)
				m_dmrBeacons = ::atoi(value) == 1 ? DMR_BEACONS::NETWORK : DMR_BEACONS::OFF;
			else if (::strcmp(key, "BeaconInterval") == 0) {
				m_dmrBeacons = m_dmrBeacons != DMR_BEACONS::OFF ? DMR_BEACONS::TIMED : DMR_BEACONS::OFF;
				m_dmrBeaconInterval = (unsigned int)::atoi(value);
			} else if (::strcmp(key, "BeaconDuration") == 0)
				m_dmrBeaconDuration = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Id") == 0)
				m_dmrId = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ColorCode") == 0)
				m_dmrColorCode = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SelfOnly") == 0)
				m_dmrSelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "EmbeddedLCOnly") == 0)
				m_dmrEmbeddedLCOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "DumpTAData") == 0)
				m_dmrDumpTAData = ::atoi(value) == 1;
			else if (::strcmp(key, "Prefixes") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int prefix = (unsigned int)::atoi(p);
					if (prefix > 0U && prefix <= 999U)
						m_dmrPrefixes.push_back(prefix);
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "BlackList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int id = (unsigned int)::atoi(p);
					if (id > 0U)
						m_dmrBlackList.push_back(id);
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "WhiteList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int id = (unsigned int)::atoi(p);
					if (id > 0U)
						m_dmrWhiteList.push_back(id);
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "Slot1TGWhiteList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int id = (unsigned int)::atoi(p);
					if (id > 0U)
						m_dmrSlot1TGWhiteList.push_back(id);
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "Slot2TGWhiteList") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int id = (unsigned int)::atoi(p);
					if (id > 0U)
						m_dmrSlot2TGWhiteList.push_back(id);
					p = ::strtok(nullptr, ",\r\n");
				}
			} else if (::strcmp(key, "TXHang") == 0)
				m_dmrTXHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CallHang") == 0)
				m_dmrCallHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_dmrModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "OVCM") == 0) {
				switch (::atoi(value)) {
				case 1:
					m_dmrOVCM = DMR_OVCM::RX_ON;
					break;
				case 2:
					m_dmrOVCM = DMR_OVCM::TX_ON;
					break;
				case 3:
					m_dmrOVCM = DMR_OVCM::ON;
					break;
				case 4:
					m_dmrOVCM = DMR_OVCM::FORCE_OFF;
					break;
				default:
					m_dmrOVCM = DMR_OVCM::OFF;
					break;
				}
			} else if (::strcmp(key, "Protect") == 0)
				m_dmrProtect = ::atoi(value) == 1;
		} else if (section == SECTION::FUSION) {
			if (::strcmp(key, "Enable") == 0)
				m_fusionEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "LowDeviation") == 0)
				m_fusionLowDeviation = ::atoi(value) == 1;
			else if (::strcmp(key, "RemoteGateway") == 0)
				m_fusionRemoteGateway = ::atoi(value) == 1;
			else if (::strcmp(key, "SelfOnly") == 0)
				m_fusionSelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "TXHang") == 0)
				m_fusionTXHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_fusionModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::P25) {
			if (::strcmp(key, "Enable") == 0)
				m_p25Enabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Id") == 0)
				m_p25Id = (unsigned int)::atoi(value);
			else if (::strcmp(key, "NAC") == 0)
				m_p25NAC = (unsigned int)::strtoul(value, nullptr, 16);
			else if (::strcmp(key, "OverrideUIDCheck") == 0)
				m_p25OverrideUID = ::atoi(value) == 1;
			else if (::strcmp(key, "SelfOnly") == 0)
				m_p25SelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "RemoteGateway") == 0)
				m_p25RemoteGateway = ::atoi(value) == 1;
			else if (::strcmp(key, "TXHang") == 0)
				m_p25TXHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_p25ModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::NXDN) {
			if (::strcmp(key, "Enable") == 0)
				m_nxdnEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Id") == 0)
				m_nxdnId = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RAN") == 0)
				m_nxdnRAN = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SelfOnly") == 0)
				m_nxdnSelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "RemoteGateway") == 0)
				m_nxdnRemoteGateway = ::atoi(value) == 1;
			else if (::strcmp(key, "TXHang") == 0)
				m_nxdnTXHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_nxdnModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::M17) {
			if (::strcmp(key, "Enable") == 0)
				m_m17Enabled = ::atoi(value) == 1;
			else if (::strcmp(key, "CAN") == 0)
				m_m17CAN = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SelfOnly") == 0)
				m_m17SelfOnly = ::atoi(value) == 1;
			else if (::strcmp(key, "AllowEncryption") == 0)
				m_m17AllowEncryption = ::atoi(value) == 1;
			else if (::strcmp(key, "TXHang") == 0)
				m_m17TXHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_m17ModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::POCSAG) {
			if (::strcmp(key, "Enable") == 0)
				m_pocsagEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Frequency") == 0)
				m_pocsagFrequency = (unsigned int)::atoi(value);
		} else if (section == SECTION::FM) {
			if (::strcmp(key, "Enable") == 0)
				m_fmEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_fmCallsign = value;
			} else if (::strcmp(key, "CallsignSpeed") == 0)
				m_fmCallsignSpeed = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CallsignFrequency") == 0)
				m_fmCallsignFrequency = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CallsignTime") == 0)
				m_fmCallsignTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CallsignHoldoff") == 0)
				m_fmCallsignHoldoff = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CallsignHighLevel") == 0)
				m_fmCallsignHighLevel = float(::atof(value));
			else if (::strcmp(key, "CallsignLowLevel") == 0)
				m_fmCallsignLowLevel = float(::atof(value));
			else if (::strcmp(key, "CallsignAtStart") == 0)
				m_fmCallsignAtStart = ::atoi(value) == 1;
			else if (::strcmp(key, "CallsignAtEnd") == 0)
				m_fmCallsignAtEnd = ::atoi(value) == 1;
			else if (::strcmp(key, "CallsignAtLatch") == 0)
				m_fmCallsignAtLatch = ::atoi(value) == 1;
			else if (::strcmp(key, "RFAck") == 0) {
				// Convert the ack to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_fmRFAck = value;
			} else if (::strcmp(key, "ExtAck") == 0) {
				// Convert the ack to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_fmExtAck = value;
			} else if (::strcmp(key, "AckSpeed") == 0)
				m_fmAckSpeed = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AckFrequency") == 0)
				m_fmAckFrequency = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AckMinTime") == 0)
				m_fmAckMinTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AckDelay") == 0)
				m_fmAckDelay = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AckLevel") == 0)
				m_fmAckLevel = float(::atof(value));
			else if (::strcmp(key, "Timeout") == 0)
				m_fmTimeout = (unsigned int)::atoi(value);
			else if (::strcmp(key, "TimeoutLevel") == 0)
				m_fmTimeoutLevel = float(::atof(value));
			else if (::strcmp(key, "CTCSSFrequency") == 0)
				m_fmCTCSSFrequency = float(::atof(value));
			else if (::strcmp(key, "CTCSSThreshold") == 0)
				m_fmCTCSSHighThreshold = m_fmCTCSSLowThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CTCSSHighThreshold") == 0)
				m_fmCTCSSHighThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CTCSSLowThreshold") == 0)
				m_fmCTCSSLowThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "CTCSSLevel") == 0)
				m_fmCTCSSLevel = float(::atof(value));
			else if (::strcmp(key, "KerchunkTime") == 0)
				m_fmKerchunkTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "HangTime") == 0)
				m_fmHangTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "AccessMode") == 0)
				m_fmAccessMode = ::atoi(value);
			else if (::strcmp(key, "LinkMode") == 0)
				m_fmLinkMode = ::atoi(value) == 1;
			else if (::strcmp(key, "COSInvert") == 0)
				m_fmCOSInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "NoiseSquelch") == 0)
				m_fmNoiseSquelch = ::atoi(value) == 1;
			else if (::strcmp(key, "SquelchThreshold") == 0)
				m_fmSquelchHighThreshold = m_fmSquelchLowThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SquelchHighThreshold") == 0)
				m_fmSquelchHighThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SquelchLowThreshold") == 0)
				m_fmSquelchLowThreshold = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RFAudioBoost") == 0)
				m_fmRFAudioBoost = (unsigned int)::atoi(value);
			else if (::strcmp(key, "MaxDevLevel") == 0)
				m_fmMaxDevLevel = float(::atof(value));
			else if (::strcmp(key, "ExtAudioBoost") == 0)
				m_fmExtAudioBoost = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_fmModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::AX25) {
			if (::strcmp(key, "Enable") == 0)
				m_ax25Enabled = ::atoi(value) == 1;
			else if (::strcmp(key, "TXDelay") == 0)
				m_ax25TXDelay = (unsigned int)::atoi(value);
			else if (::strcmp(key, "RXTwist") == 0)
				m_ax25RXTwist = ::atoi(value);
			else if (::strcmp(key, "SlotTime") == 0)
				m_ax25SlotTime = (unsigned int)::atoi(value);
			else if (::strcmp(key, "PPersist") == 0)
				m_ax25PPersist = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Trace") == 0)
				m_ax25Trace = ::atoi(value) == 1;
		} else if (section == SECTION::DSTAR_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_dstarNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_dstarGatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_dstarGatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_dstarLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_dstarLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_dstarNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_dstarNetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::DMR_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_dmrNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Type") == 0)
				m_dmrNetworkType = value;
			else if (::strcmp(key, "RemoteAddress") == 0)
				m_dmrNetworkRemoteAddress = value;
			else if (::strcmp(key, "RemotePort") == 0)
				m_dmrNetworkRemotePort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_dmrNetworkLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_dmrNetworkLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "Password") == 0)
				m_dmrNetworkPassword = value;
			else if (::strcmp(key, "Options") == 0)
				m_dmrNetworkOptions = value;
			else if (::strcmp(key, "Debug") == 0)
				m_dmrNetworkDebug = ::atoi(value) == 1;
			else if (::strcmp(key, "Jitter") == 0)
				m_dmrNetworkJitter = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Slot1") == 0)
				m_dmrNetworkSlot1 = ::atoi(value) == 1;
			else if (::strcmp(key, "Slot2") == 0)
				m_dmrNetworkSlot2 = ::atoi(value) == 1;
			else if (::strcmp(key, "ModeHang") == 0)
				m_dmrNetworkModeHang = (unsigned int)::atoi(value);
		} else if (section == SECTION::FUSION_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_fusionNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "LocalAddress") == 0)
				m_fusionNetworkLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_fusionNetworkLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_fusionNetworkGatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_fusionNetworkGatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_fusionNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_fusionNetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::P25_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_p25NetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_p25GatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_p25GatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_p25LocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_p25LocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_p25NetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_p25NetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::NXDN_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_nxdnNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Protocol") == 0)
				m_nxdnNetworkProtocol = value;
			else if (::strcmp(key, "LocalAddress") == 0)
				m_nxdnLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_nxdnLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_nxdnGatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_nxdnGatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_nxdnNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_nxdnNetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::M17_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_m17NetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "LocalAddress") == 0)
				m_m17LocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_m17LocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_m17GatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_m17GatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_m17NetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_m17NetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::POCSAG_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_pocsagNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "LocalAddress") == 0)
				m_pocsagLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_pocsagLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_pocsagGatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_pocsagGatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "ModeHang") == 0)
				m_pocsagNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_pocsagNetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::FM_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_fmNetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Protocol") == 0)
				m_fmNetworkProtocol = value;
			else if (::strcmp(key, "SampleRate") == 0)
				m_fmNetworkSampleRate = (unsigned int)::atoi(value);
			else if (::strcmp(key, "SquelchFile") == 0)
				m_fmNetworkSquelchFile = value;
			else if (::strcmp(key, "LocalAddress") == 0)
				m_fmLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_fmLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "GatewayAddress") == 0)
				m_fmGatewayAddress = value;
			else if (::strcmp(key, "GatewayPort") == 0)
				m_fmGatewayPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "PreEmphasis") == 0)
				m_fmPreEmphasis = ::atoi(value) == 1;
			else if (::strcmp(key, "DeEmphasis") == 0)
				m_fmDeEmphasis = ::atoi(value) == 1;
			else if (::strcmp(key, "TXAudioGain") == 0)
				m_fmTXAudioGain = float(::atof(value));
			else if (::strcmp(key, "RXAudioGain") == 0)
				m_fmRXAudioGain = float(::atof(value));
			else if (::strcmp(key, "ModeHang") == 0)
				m_fmNetworkModeHang = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_fmNetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::AX25_NETWORK) {
			if (::strcmp(key, "Enable") == 0)
				m_ax25NetworkEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Port") == 0)
				m_ax25NetworkPort = value;
			else if (::strcmp(key, "Speed") == 0)
				m_ax25NetworkSpeed = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Debug") == 0)
				m_ax25NetworkDebug = ::atoi(value) == 1;
		} else if (section == SECTION::TFTSERIAL_DISPLAY) {
			if (::strcmp(key, "Port") == 0)
				m_tftSerialPort = value;
			else if (::strcmp(key, "Brightness") == 0)
				m_tftSerialBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ScreenLayout") == 0)
				m_tftSerialScreenLayout = (unsigned int)::atoi(value);
		} else if (section == SECTION::HD44780_DISPLAY) {
			if (::strcmp(key, "Rows") == 0)
				m_hd44780Rows = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Columns") == 0)
				m_hd44780Columns = (unsigned int)::atoi(value);
			else if (::strcmp(key, "I2CAddress") == 0)
				m_hd44780i2cAddress = (unsigned int)::strtoul(value, nullptr, 16);
			else if (::strcmp(key, "PWM") == 0)
				m_hd44780PWM = ::atoi(value) == 1;
			else if (::strcmp(key, "PWMPin") == 0)
				m_hd44780PWMPin = (unsigned int)::atoi(value);
			else if (::strcmp(key, "PWMBright") == 0)
				m_hd44780PWMBright = (unsigned int)::atoi(value);
			else if (::strcmp(key, "PWMDim") == 0)
				m_hd44780PWMDim = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_hd44780DisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_hd44780UTC = ::atoi(value) == 1;
			else if (::strcmp(key, "Pins") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int pin = (unsigned int)::atoi(p);
					m_hd44780Pins.push_back(pin);
					p = ::strtok(nullptr, ",\r\n");
				}
			}
		} else if (section == SECTION::NEXTION_DISPLAY) {
			if (::strcmp(key, "Port") == 0)
				m_nextionPort = value;
			else if (::strcmp(key, "Brightness") == 0)
				m_nextionIdleBrightness = m_nextionBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_nextionDisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_nextionUTC = ::atoi(value) == 1;
			else if (::strcmp(key, "IdleBrightness") == 0)
				m_nextionIdleBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ScreenLayout") == 0)
				m_nextionScreenLayout = (unsigned int)::strtoul(value, nullptr, 0);
			else if (::strcmp(key, "DisplayTempInFahrenheit") == 0)
				m_nextionTempInFahrenheit = ::atoi(value) == 1;
			else if (::strcmp(key, "NextionOutput") == 0)
				m_nextionOutput = ::atoi(value) == 1;
			else if (::strcmp(key, "NextionUDPPort") == 0)
				m_nextionUDPPort = (unsigned short)::atoi(value);
		} else if (section == SECTION::OLED_DISPLAY) {
			if (::strcmp(key, "Type") == 0)
				m_oledType = (unsigned char)::atoi(value);
			else if (::strcmp(key, "Brightness") == 0)
				m_oledBrightness = (unsigned char)::atoi(value);
			else if (::strcmp(key, "Invert") == 0)
				m_oledInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "Scroll") == 0)
				m_oledScroll = ::atoi(value) == 1;
			else if (::strcmp(key, "Rotate") == 0)
				m_oledRotate = ::atoi(value) == 1;
			else if (::strcmp(key, "LogoScreensaver") == 0)
				m_oledLogoScreensaver = ::atoi(value) == 1;
		} else if (section == SECTION::LCDPROC_DISPLAY) {
			if (::strcmp(key, "Address") == 0)
				m_lcdprocAddress = value;
			else if (::strcmp(key, "Port") == 0)
				m_lcdprocPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalPort") == 0)
				m_lcdprocLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_lcdprocDisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_lcdprocUTC = ::atoi(value) == 1;
			else if (::strcmp(key, "DimOnIdle") == 0)
				m_lcdprocDimOnIdle = ::atoi(value) == 1;
		} else if (section == SECTION::LOCK_FILE) {
			if (::strcmp(key, "Enable") == 0)
				m_lockFileEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "File") == 0)
				m_lockFileName = value;
		} else if (section == SECTION::REMOTE_CONTROL) {
			if (::strcmp(key, "Enable") == 0)
				m_remoteControlEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Port") == 0)
				m_remoteControlPort = (unsigned short)::atoi(value);
		}
	}

	::fclose(fp);

	return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

unsigned int CConf::getId() const
{
	return m_id;
}

unsigned int CConf::getTimeout() const
{
	return m_timeout;
}

bool CConf::getDuplex() const
{
	return m_duplex;
}

std::string CConf::getDisplay() const
{
	return m_display;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

unsigned int CConf::getRXFrequency() const
{
	return m_rxFrequency;
}

unsigned int CConf::getTXFrequency() const
{
	return m_txFrequency;
}

unsigned int CConf::getPower() const
{
	return m_power;
}

float CConf::getLatitude() const
{
	return m_latitude;
}

float CConf::getLongitude() const
{
	return m_longitude;
}

int CConf::getHeight() const
{
	return m_height;
}

std::string CConf::getLocation() const
{
	return m_location;
}

std::string CConf::getDescription() const
{
	return m_description;
}

std::string CConf::getURL() const
{
	return m_url;
}

unsigned int CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

unsigned int CConf::getLogFileLevel() const
{
	return m_logFileLevel;
}

std::string CConf::getLogFilePath() const
{
	return m_logFilePath;
}

std::string CConf::getLogFileRoot() const
{
	return m_logFileRoot;
}

bool CConf::getLogFileRotate() const
{
	return m_logFileRotate;
}

bool CConf::getCWIdEnabled() const
{
	return m_cwIdEnabled;
}

unsigned int CConf::getCWIdTime() const
{
	return m_cwIdTime;
}

std::string CConf::getCWIdCallsign() const
{
	return m_cwIdCallsign;
}

std::string CConf::getDMRIdLookupFile() const
{
	return m_dmrIdLookupFile;
}

unsigned int CConf::getDMRIdLookupTime() const
{
	return m_dmrIdLookupTime;
}

std::string CConf::getNXDNIdLookupFile() const
{
	return m_nxdnIdLookupFile;
}

unsigned int CConf::getNXDNIdLookupTime() const
{
	return m_nxdnIdLookupTime;
}

std::string CConf::getModemProtocol() const
{
	return m_modemProtocol;
}

std::string CConf::getModemUARTPort() const
{
	return m_modemUARTPort;
}

unsigned int CConf::getModemUARTSpeed() const
{
	return m_modemUARTSpeed;
}

std::string CConf::getModemI2CPort() const
{
	return m_modemI2CPort;
}

unsigned int CConf::getModemI2CAddress() const
{
	return m_modemI2CAddress;
}

std::string CConf::getModemModemAddress() const
{
	return m_modemModemAddress;
}

unsigned short CConf::getModemModemPort() const
{
	return m_modemModemPort;
}

std::string CConf::getModemLocalAddress() const
{
	return m_modemLocalAddress;
}

unsigned short CConf::getModemLocalPort() const
{
	return m_modemLocalPort;
}

bool CConf::getModemRXInvert() const
{
	return m_modemRXInvert;
}

bool CConf::getModemTXInvert() const
{
	return m_modemTXInvert;
}

bool CConf::getModemPTTInvert() const
{
	return m_modemPTTInvert;
}

unsigned int CConf::getModemTXDelay() const
{
	return m_modemTXDelay;
}

unsigned int CConf::getModemDMRDelay() const
{
	return m_modemDMRDelay;
}

int CConf::getModemRXOffset() const
{
	return m_modemRXOffset;
}

int CConf::getModemTXOffset() const
{
	return m_modemTXOffset;
}

int CConf::getModemRXDCOffset() const
{
	return m_modemRXDCOffset;
}

int CConf::getModemTXDCOffset() const
{
	return m_modemTXDCOffset;
}

float CConf::getModemRFLevel() const
{
	return m_modemRFLevel;
}

float CConf::getModemRXLevel() const
{
	return m_modemRXLevel;
}

float CConf::getModemCWIdTXLevel() const
{
	return m_modemCWIdTXLevel;
}

float CConf::getModemDStarTXLevel() const
{
	return m_modemDStarTXLevel;
}

float CConf::getModemDMRTXLevel() const
{
	return m_modemDMRTXLevel;
}

float CConf::getModemYSFTXLevel() const
{
	return m_modemYSFTXLevel;
}

float CConf::getModemP25TXLevel() const
{
	return m_modemP25TXLevel;
}

float CConf::getModemNXDNTXLevel() const
{
	return m_modemNXDNTXLevel;
}

float CConf::getModemM17TXLevel() const
{
	return m_modemM17TXLevel;
}

float CConf::getModemPOCSAGTXLevel() const
{
	return m_modemPOCSAGTXLevel;
}

float CConf::getModemFMTXLevel() const
{
	return m_modemFMTXLevel;
}

float CConf::getModemAX25TXLevel() const
{
	return m_modemAX25TXLevel;
}

std::string CConf::getModemRSSIMappingFile () const
{
	return m_modemRSSIMappingFile;
}

bool CConf::getModemUseCOSAsLockout() const
{
	return m_modemUseCOSAsLockout;
}

bool CConf::getModemTrace() const
{
	return m_modemTrace;
}

bool CConf::getModemDebug() const
{
	return m_modemDebug;
}

bool CConf::getTransparentEnabled() const
{
	return m_transparentEnabled;
}

std::string CConf::getTransparentRemoteAddress() const
{
	return m_transparentRemoteAddress;
}

unsigned short CConf::getTransparentRemotePort() const
{
	return m_transparentRemotePort;
}

unsigned short CConf::getTransparentLocalPort() const
{
	return m_transparentLocalPort;
}

unsigned int CConf::getTransparentSendFrameType() const
{
	return m_transparentSendFrameType;
}

bool CConf::getDStarEnabled() const
{
	return m_dstarEnabled;
}

std::string CConf::getDStarModule() const
{
	return m_dstarModule;
}

bool CConf::getDStarSelfOnly() const
{
	return m_dstarSelfOnly;
}

std::vector<std::string> CConf::getDStarBlackList() const
{
	return m_dstarBlackList;
}

std::vector<std::string> CConf::getDStarWhiteList() const
{
        return m_dstarWhiteList;
}

bool CConf::getDStarAckReply() const
{
	return m_dstarAckReply;
}

unsigned int CConf::getDStarAckTime() const
{
	return m_dstarAckTime;
}

DSTAR_ACK CConf::getDStarAckMessage() const
{
	return m_dstarAckMessage;
}

bool CConf::getDStarErrorReply() const
{
	return m_dstarErrorReply;
}

bool CConf::getDStarRemoteGateway() const
{
	return m_dstarRemoteGateway;
}

unsigned int CConf::getDStarModeHang() const
{
	return m_dstarModeHang;
}

bool CConf::getDMREnabled() const
{
	return m_dmrEnabled;
}

DMR_BEACONS CConf::getDMRBeacons() const
{
	return m_dmrBeacons;
}

unsigned int CConf::getDMRBeaconInterval() const
{
	return m_dmrBeaconInterval;
}

unsigned int CConf::getDMRBeaconDuration() const
{
	return m_dmrBeaconDuration;
}

unsigned int CConf::getDMRId() const
{
	return m_dmrId;
}

unsigned int CConf::getDMRColorCode() const
{
	return m_dmrColorCode;
}

bool CConf::getDMRSelfOnly() const
{
	return m_dmrSelfOnly;
}

bool CConf::getDMREmbeddedLCOnly() const
{
	return m_dmrEmbeddedLCOnly;
}

bool CConf::getDMRDumpTAData() const
{
	return m_dmrDumpTAData;
}

std::vector<unsigned int> CConf::getDMRPrefixes() const
{
	return m_dmrPrefixes;
}

std::vector<unsigned int> CConf::getDMRBlackList() const
{
	return m_dmrBlackList;
}

std::vector<unsigned int> CConf::getDMRWhiteList() const
{
	return m_dmrWhiteList;
}

std::vector<unsigned int> CConf::getDMRSlot1TGWhiteList() const
{
	return m_dmrSlot1TGWhiteList;
}

std::vector<unsigned int> CConf::getDMRSlot2TGWhiteList() const
{
	return m_dmrSlot2TGWhiteList;
}

unsigned int CConf::getDMRCallHang() const
{
	return m_dmrCallHang;
}

unsigned int CConf::getDMRTXHang() const
{
	return m_dmrTXHang;
}

unsigned int CConf::getDMRModeHang() const
{
	return m_dmrModeHang;
}

DMR_OVCM CConf::getDMROVCM() const
{
	return m_dmrOVCM;
}

bool CConf::getDMRProtect() const
{
	return m_dmrProtect;
}

bool CConf::getFusionEnabled() const
{
	return m_fusionEnabled;
}

bool CConf::getFusionLowDeviation() const
{
	return m_fusionLowDeviation;
}

bool CConf::getFusionRemoteGateway() const
{
	return m_fusionRemoteGateway;
}

unsigned int CConf::getFusionTXHang() const
{
	return m_fusionTXHang;
}

bool CConf::getFusionSelfOnly() const
{
	return m_fusionSelfOnly;
}

unsigned int CConf::getFusionModeHang() const
{
	return m_fusionModeHang;
}

bool CConf::getP25Enabled() const
{
	return m_p25Enabled;
}

unsigned int CConf::getP25Id() const
{
	return m_p25Id;
}

unsigned int CConf::getP25NAC() const
{
	return m_p25NAC;
}

bool CConf::getP25OverrideUID() const
{
	return m_p25OverrideUID;
}

bool CConf::getP25SelfOnly() const
{
	return m_p25SelfOnly;
}

bool CConf::getP25RemoteGateway() const
{
	return m_p25RemoteGateway;
}

unsigned int CConf::getP25TXHang() const
{
	return m_p25TXHang;
}

unsigned int CConf::getP25ModeHang() const
{
	return m_p25ModeHang;
}

bool CConf::getNXDNEnabled() const
{
	return m_nxdnEnabled;
}

unsigned int CConf::getNXDNId() const
{
	return m_nxdnId;
}

unsigned int CConf::getNXDNRAN() const
{
	return m_nxdnRAN;
}

bool CConf::getNXDNSelfOnly() const
{
	return m_nxdnSelfOnly;
}

bool CConf::getNXDNRemoteGateway() const
{
	return m_nxdnRemoteGateway;
}

unsigned int CConf::getNXDNTXHang() const
{
	return m_nxdnTXHang;
}

unsigned int CConf::getNXDNModeHang() const
{
	return m_nxdnModeHang;
}

bool CConf::getM17Enabled() const
{
	return m_m17Enabled;
}

unsigned int CConf::getM17CAN() const
{
	return m_m17CAN;
}

bool CConf::getM17SelfOnly() const
{
	return m_m17SelfOnly;
}

bool CConf::getM17AllowEncryption() const
{
	return m_m17AllowEncryption;
}

unsigned int CConf::getM17TXHang() const
{
	return m_m17TXHang;
}

unsigned int CConf::getM17ModeHang() const
{
	return m_m17ModeHang;
}

bool CConf::getPOCSAGEnabled() const
{
	return m_pocsagEnabled;
}

unsigned int CConf::getPOCSAGFrequency() const
{
	return m_pocsagFrequency;
}

bool CConf::getFMEnabled() const
{
	return m_fmEnabled;
}

std::string CConf::getFMCallsign() const
{
	return m_fmCallsign;
}

unsigned int CConf::getFMCallsignSpeed() const
{
	return m_fmCallsignSpeed;
}

unsigned int CConf::getFMCallsignFrequency() const
{
	return m_fmCallsignFrequency;
}

unsigned int CConf::getFMCallsignTime() const
{
	return m_fmCallsignTime;
}

unsigned int CConf::getFMCallsignHoldoff() const
{
	return m_fmCallsignHoldoff;
}

float CConf::getFMCallsignHighLevel() const
{
	return m_fmCallsignHighLevel;
}

float CConf::getFMCallsignLowLevel() const
{
	return m_fmCallsignLowLevel;
}

bool CConf::getFMCallsignAtStart() const
{
	return m_fmCallsignAtStart;
}

bool CConf::getFMCallsignAtEnd() const
{
	return m_fmCallsignAtEnd;
}

bool CConf::getFMCallsignAtLatch() const
{
	return m_fmCallsignAtLatch;
}

std::string CConf::getFMRFAck() const
{
	return m_fmRFAck;
}

std::string CConf::getFMExtAck() const
{
	return m_fmExtAck;
}

unsigned int CConf::getFMAckSpeed() const
{
	return m_fmAckSpeed;
}

unsigned int CConf::getFMAckFrequency() const
{
	return m_fmAckFrequency;
}

unsigned int CConf::getFMAckMinTime() const
{
	return m_fmAckMinTime;
}

unsigned int CConf::getFMAckDelay() const
{
	return m_fmAckDelay;
}

float CConf::getFMAckLevel() const
{
	return m_fmAckLevel;
}

unsigned int CConf::getFMTimeout() const
{
	return m_fmTimeout;
}

float CConf::getFMTimeoutLevel() const
{
	return m_fmTimeoutLevel;
}

float CConf::getFMCTCSSFrequency() const
{
	return m_fmCTCSSFrequency;
}

unsigned int CConf::getFMCTCSSHighThreshold() const
{
	return m_fmCTCSSHighThreshold;
}

unsigned int CConf::getFMCTCSSLowThreshold() const
{
	return m_fmCTCSSLowThreshold;
}

float CConf::getFMCTCSSLevel() const
{
	return m_fmCTCSSLevel;
}

unsigned int CConf::getFMKerchunkTime() const
{
	return m_fmKerchunkTime;
}

unsigned int CConf::getFMHangTime() const
{
	return m_fmHangTime;
}

unsigned int CConf::getFMAccessMode() const
{
	return m_fmAccessMode;
}

bool CConf::getFMLinkMode() const
{
	return m_fmLinkMode;
}

bool CConf::getFMCOSInvert() const
{
	return m_fmCOSInvert;
}

bool CConf::getFMNoiseSquelch() const
{
	return m_fmNoiseSquelch;
}

unsigned int CConf::getFMSquelchHighThreshold() const
{
	return m_fmSquelchHighThreshold;
}

unsigned int CConf::getFMSquelchLowThreshold() const
{
	return m_fmSquelchLowThreshold;
}

unsigned int CConf::getFMRFAudioBoost() const
{
	return m_fmRFAudioBoost;
}

float CConf::getFMMaxDevLevel() const
{
	return m_fmMaxDevLevel;
}

unsigned int CConf::getFMExtAudioBoost() const
{
	return m_fmExtAudioBoost;
}

unsigned int CConf::getFMModeHang() const
{
	return m_fmModeHang;
}

bool CConf::getAX25Enabled() const
{
	return m_ax25Enabled;
}

unsigned int CConf::getAX25TXDelay() const
{
	return m_ax25TXDelay;
}

int CConf::getAX25RXTwist() const
{
	return m_ax25RXTwist;
}

unsigned int CConf::getAX25SlotTime() const
{
	return m_ax25SlotTime;
}

unsigned int CConf::getAX25PPersist() const
{
	return m_ax25PPersist;
}

bool CConf::getAX25Trace() const
{
	return m_ax25Trace;
}

bool CConf::getDStarNetworkEnabled() const
{
	return m_dstarNetworkEnabled;
}

std::string CConf::getDStarGatewayAddress() const
{
	return m_dstarGatewayAddress;
}

unsigned short CConf::getDStarGatewayPort() const
{
	return m_dstarGatewayPort;
}

std::string CConf::getDStarLocalAddress() const
{
	return m_dstarLocalAddress;
}

unsigned short CConf::getDStarLocalPort() const
{
	return m_dstarLocalPort;
}

unsigned int CConf::getDStarNetworkModeHang() const
{
	return m_dstarNetworkModeHang;
}

bool CConf::getDStarNetworkDebug() const
{
	return m_dstarNetworkDebug;
}

bool CConf::getDMRNetworkEnabled() const
{
	return m_dmrNetworkEnabled;
}

std::string CConf::getDMRNetworkType() const
{
	return m_dmrNetworkType;
}

std::string CConf::getDMRNetworkRemoteAddress() const
{
	return m_dmrNetworkRemoteAddress;
}

unsigned short CConf::getDMRNetworkRemotePort() const
{
	return m_dmrNetworkRemotePort;
}

std::string CConf::getDMRNetworkLocalAddress() const
{
	return m_dmrNetworkLocalAddress;
}

unsigned short CConf::getDMRNetworkLocalPort() const
{
	return m_dmrNetworkLocalPort;
}

std::string CConf::getDMRNetworkPassword() const
{
	return m_dmrNetworkPassword;
}

std::string CConf::getDMRNetworkOptions() const
{
	return m_dmrNetworkOptions;
}

unsigned int CConf::getDMRNetworkModeHang() const
{
	return m_dmrNetworkModeHang;
}

bool CConf::getDMRNetworkDebug() const
{
	return m_dmrNetworkDebug;
}

unsigned int CConf::getDMRNetworkJitter() const
{
	return m_dmrNetworkJitter;
}

bool CConf::getDMRNetworkSlot1() const
{
	return m_dmrNetworkSlot1;
}

bool CConf::getDMRNetworkSlot2() const
{
	return m_dmrNetworkSlot2;
}

bool CConf::getFusionNetworkEnabled() const
{
	return m_fusionNetworkEnabled;
}

std::string CConf::getFusionNetworkLocalAddress() const
{
	return m_fusionNetworkLocalAddress;
}

unsigned short CConf::getFusionNetworkLocalPort() const
{
	return m_fusionNetworkLocalPort;
}

std::string CConf::getFusionNetworkGatewayAddress() const
{
	return m_fusionNetworkGatewayAddress;
}

unsigned short CConf::getFusionNetworkGatewayPort() const
{
	return m_fusionNetworkGatewayPort;
}

unsigned int CConf::getFusionNetworkModeHang() const
{
	return m_fusionNetworkModeHang;
}

bool CConf::getFusionNetworkDebug() const
{
	return m_fusionNetworkDebug;
}

bool CConf::getP25NetworkEnabled() const
{
	return m_p25NetworkEnabled;
}

std::string CConf::getP25GatewayAddress() const
{
	return m_p25GatewayAddress;
}

unsigned short CConf::getP25GatewayPort() const
{
	return m_p25GatewayPort;
}

std::string CConf::getP25LocalAddress() const
{
	return m_p25LocalAddress;
}

unsigned short CConf::getP25LocalPort() const
{
	return m_p25LocalPort;
}

unsigned int CConf::getP25NetworkModeHang() const
{
	return m_p25NetworkModeHang;
}

bool CConf::getP25NetworkDebug() const
{
	return m_p25NetworkDebug;
}

bool CConf::getNXDNNetworkEnabled() const
{
	return m_nxdnNetworkEnabled;
}

std::string CConf::getNXDNNetworkProtocol() const
{
	return m_nxdnNetworkProtocol;
}

std::string CConf::getNXDNGatewayAddress() const
{
	return m_nxdnGatewayAddress;
}

unsigned short CConf::getNXDNGatewayPort() const
{
	return m_nxdnGatewayPort;
}

std::string CConf::getNXDNLocalAddress() const
{
	return m_nxdnLocalAddress;
}

unsigned short CConf::getNXDNLocalPort() const
{
	return m_nxdnLocalPort;
}

unsigned int CConf::getNXDNNetworkModeHang() const
{
	return m_nxdnNetworkModeHang;
}

bool CConf::getNXDNNetworkDebug() const
{
	return m_nxdnNetworkDebug;
}

bool CConf::getM17NetworkEnabled() const
{
	return m_m17NetworkEnabled;
}

std::string CConf::getM17GatewayAddress() const
{
	return m_m17GatewayAddress;
}

unsigned short CConf::getM17GatewayPort() const
{
	return m_m17GatewayPort;
}

std::string CConf::getM17LocalAddress() const
{
	return m_m17LocalAddress;
}

unsigned short CConf::getM17LocalPort() const
{
	return m_m17LocalPort;
}

unsigned int CConf::getM17NetworkModeHang() const
{
	return m_m17NetworkModeHang;
}

bool CConf::getM17NetworkDebug() const
{
	return m_m17NetworkDebug;
}

bool CConf::getPOCSAGNetworkEnabled() const
{
	return m_pocsagNetworkEnabled;
}

std::string CConf::getPOCSAGGatewayAddress() const
{
	return m_pocsagGatewayAddress;
}

unsigned short CConf::getPOCSAGGatewayPort() const
{
	return m_pocsagGatewayPort;
}

std::string CConf::getPOCSAGLocalAddress() const
{
	return m_pocsagLocalAddress;
}

unsigned short CConf::getPOCSAGLocalPort() const
{
	return m_pocsagLocalPort;
}

unsigned int CConf::getPOCSAGNetworkModeHang() const
{
	return m_pocsagNetworkModeHang;
}

bool CConf::getPOCSAGNetworkDebug() const
{
	return m_pocsagNetworkDebug;
}

bool CConf::getFMNetworkEnabled() const
{
	return m_fmNetworkEnabled;
}

std::string CConf::getFMNetworkProtocol() const
{
	return m_fmNetworkProtocol;
}

unsigned int CConf::getFMNetworkSampleRate() const
{
	return m_fmNetworkSampleRate;
}

std::string CConf::getFMNetworkSquelchFile() const
{
	return m_fmNetworkSquelchFile;
}

std::string CConf::getFMGatewayAddress() const
{
	return m_fmGatewayAddress;
}

unsigned short CConf::getFMGatewayPort() const
{
	return m_fmGatewayPort;
}

std::string CConf::getFMLocalAddress() const
{
	return m_fmLocalAddress;
}

unsigned short CConf::getFMLocalPort() const
{
	return m_fmLocalPort;
}

bool CConf::getFMPreEmphasis() const
{
	return m_fmPreEmphasis;
}

bool CConf::getFMDeEmphasis() const
{
	return m_fmDeEmphasis;
}

float CConf::getFMTXAudioGain() const
{
	return m_fmTXAudioGain;
}

float CConf::getFMRXAudioGain() const
{
	return m_fmRXAudioGain;
}

unsigned int CConf::getFMNetworkModeHang() const
{
	return m_fmNetworkModeHang;
}

bool CConf::getFMNetworkDebug() const
{
	return m_fmNetworkDebug;
}

bool CConf::getAX25NetworkEnabled() const
{
	return m_ax25NetworkEnabled;
}

std::string CConf::getAX25NetworkPort() const
{
	return m_ax25NetworkPort;
}

unsigned int CConf::getAX25NetworkSpeed() const
{
	return m_ax25NetworkSpeed;
}

bool CConf::getAX25NetworkDebug() const
{
	return m_ax25NetworkDebug;
}

std::string CConf::getTFTSerialPort() const
{
	return m_tftSerialPort;
}

unsigned int CConf::getTFTSerialBrightness() const
{
	return m_tftSerialBrightness;
}

unsigned int CConf::getTFTSerialScreenLayout() const
{
	return m_tftSerialScreenLayout;
}

unsigned int CConf::getHD44780Rows() const
{
	return m_hd44780Rows;
}

unsigned int CConf::getHD44780Columns() const
{
	return m_hd44780Columns;
}

std::vector<unsigned int> CConf::getHD44780Pins() const
{
	return m_hd44780Pins;
}

unsigned int CConf::getHD44780i2cAddress() const
{
  return m_hd44780i2cAddress;
}

bool CConf::getHD44780PWM() const
{
	return m_hd44780PWM;
}

unsigned int CConf::getHD44780PWMPin() const
{
	return m_hd44780PWMPin;
}

unsigned int CConf::getHD44780PWMBright() const
{
	return m_hd44780PWMBright;
}

unsigned int CConf::getHD44780PWMDim() const
{
	return m_hd44780PWMDim;
}

bool CConf::getHD44780DisplayClock() const
{
	return m_hd44780DisplayClock;
}

bool CConf::getHD44780UTC() const
{
	return m_hd44780UTC;
}

std::string CConf::getNextionPort() const
{
	return m_nextionPort;
}

unsigned int CConf::getNextionBrightness() const
{
	return m_nextionBrightness;
}

bool CConf::getNextionDisplayClock() const
{
	return m_nextionDisplayClock;
}

bool CConf::getNextionUTC() const
{
	return m_nextionUTC;
}

unsigned int CConf::getNextionIdleBrightness() const
{
	return m_nextionIdleBrightness;
}

unsigned int CConf::getNextionScreenLayout() const
{
	return m_nextionScreenLayout;
}

bool CConf::getNextionTempInFahrenheit() const
{
	return m_nextionTempInFahrenheit;
}

bool CConf::getNextionOutput() const
{
	return m_nextionOutput;
}

unsigned short CConf::getNextionUDPPort() const
{
	return m_nextionUDPPort;
}

unsigned char CConf::getOLEDType() const
{
	return m_oledType;
}

unsigned char CConf::getOLEDBrightness() const
{
	return m_oledBrightness;
}

bool CConf::getOLEDInvert() const
{
	return m_oledInvert;
}

bool CConf::getOLEDScroll() const
{
	return m_oledScroll;
}

bool CConf::getOLEDRotate() const
{
	return m_oledRotate;
}

bool CConf::getOLEDLogoScreensaver() const
{
	return m_oledLogoScreensaver;
}

std::string CConf::getLCDprocAddress() const
{
	return m_lcdprocAddress;
}

unsigned short CConf::getLCDprocPort() const
{
	return m_lcdprocPort;
}

unsigned short CConf::getLCDprocLocalPort() const
{
	return m_lcdprocLocalPort;
}

bool CConf::getLCDprocDisplayClock() const
{
	return m_lcdprocDisplayClock;
}

bool CConf::getLCDprocUTC() const
{
	return m_lcdprocUTC;
}

bool CConf::getLCDprocDimOnIdle() const
{
	return m_lcdprocDimOnIdle;
}

bool CConf::getLockFileEnabled() const
{
	return m_lockFileEnabled;
}

std::string CConf::getLockFileName() const
{
	return m_lockFileName;
}

bool CConf::getRemoteControlEnabled() const
{
	return m_remoteControlEnabled;
}

std::string CConf::getRemoteControlAddress() const
{
	return m_remoteControlAddress;
}

unsigned short CConf::getRemoteControlPort() const
{
	return m_remoteControlPort;
}
