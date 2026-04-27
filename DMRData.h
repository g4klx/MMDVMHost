/*
 *	Copyright (C) 2015,2016,2017,2023 by Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#ifndef DMRData_H
#define	DMRData_H

#include "DMRDefines.h"
#include "Defines.h"

#if defined(USE_DMR)

struct TrunkingCommandParameters {
	unsigned int commandType = 0U;
	bool trunkingParams      = false;
	bool channelEnable       = false;
	unsigned int slot        = 1U;
	bool ceaseTransmission   = false;
};

namespace DMRCommand {
	const unsigned int RCNoCommand                = 0U;
	const unsigned int ChannelEnableDisable       = 1U;
	const unsigned int RCCeaseTransmission        = 2U;
	const unsigned int RCRequestCeaseTransmission = 3U;
	const unsigned int RCPowerIncreaseOneStep     = 4U;
	const unsigned int RCPowerDecreaseOneStep     = 5U;
	const unsigned int RCMaximumPower             = 6U;
	const unsigned int RCMinimumPower             = 7U;
};

class CDMRData {
public:
	CDMRData(const CDMRData& data);
	CDMRData();
	~CDMRData();

	CDMRData& operator=(const CDMRData& data);

	unsigned int getSlotNo() const;
	void setSlotNo(unsigned int slotNo);

	unsigned int getSrcId() const;
	void setSrcId(unsigned int id);

	unsigned int getDstId() const;
	void setDstId(unsigned int id);

	FLCO getFLCO() const;
	void setFLCO(FLCO flco);

	unsigned char getN() const;
	void setN(unsigned char n);

	unsigned char getSeqNo() const;
	void setSeqNo(unsigned char seqNo);

	unsigned char getDataType() const;
	void setDataType(unsigned char dataType);

	unsigned char getBER() const;
	void setBER(unsigned char ber);

	unsigned char getRSSI() const;
	void setRSSI(unsigned char rssi);

	void setData(const unsigned char* buffer);
	unsigned int getData(unsigned char* buffer) const;

private:
	unsigned int   m_slotNo;
	unsigned char* m_data;
	unsigned int   m_srcId;
	unsigned int   m_dstId;
	FLCO           m_flco;
	unsigned char  m_dataType;
	unsigned char  m_seqNo;
	unsigned char  m_n;
	unsigned char  m_ber;
	unsigned char  m_rssi;
};

#endif

#endif
