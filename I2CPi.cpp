/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#include "I2CPi.h"
#include "Thread.h"

#include <bcm2835.h>

const unsigned char OLED_ADAFRUIT_SPI_128x32 = 0U;
const unsigned char OLED_ADAFRUIT_SPI_128x64 = 1U;


CI2CPi::CI2CPi() :
m_spi(false)
{
}

CI2CPi::~CI2CPi()
{
}

bool CI2CPi::open(unsigned char displayType)
{
    if (displayType == OLED_ADAFRUIT_SPI_128x32 ||
	    displayType == OLED_ADAFRUIT_SPI_128x64) {
		m_spi = true;

		// Init & Configure Raspberry PI SPI
		bcm2835_spi_begin(OLED_SPI_CS);
		bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      
		bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                
  
		// 16 MHz SPI bus, but Worked at 62 MHz also  
		bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16); 

		// Set the pin that will control DC as output
		bcm2835_gpio_fsel(OLED_SPI_DC, BCM2835_GPIO_FSEL_OUTP);

		// Setup reset pin direction as output
		bcm2835_gpio_fsel(OLED_SPI_RESET, BCM2835_GPIO_FSEL_OUTP);

		// Setup reset pin direction (used by both SPI and I2C)
		bcm2835_gpio_fsel(OLED_SPI_RESET, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(OLED_SPI_RESET, HIGH);

		// VDD (3.3V) goes high at start, lets just chill for a ms
		CThread::sleep(1U);

		// bring reset low
		bcm2835_gpio_write(OLED_SPI_RESET, LOW);

		// wait 10ms
		CThread::sleep(10U);

		// bring out of reset
		bcm2835_gpio_write(OLED_SPI_RESET, HIGH);
    } else {
		m_spi = false;

		// Init Raspberry PI GPIO
		if (!bcm2835_init())
			return false;

		// Init & Configure Raspberry PI I2C
		if (!bcm2835_i2c_begin())
			return false;

		// Setup reset pin direction as output
		bcm2835_gpio_fsel(OLED_I2C_RESET, BCM2835_GPIO_FSEL_OUTP);

		// Setup reset pin direction (used by both SPI and I2C)
		bcm2835_gpio_fsel(OLED_I2C_RESET, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(OLED_I2C_RESET, HIGH);

		// VDD (3.3V) goes high at start, lets just chill for a ms
		CThread::sleep(1U);

		// bring reset low
		bcm2835_gpio_write(OLED_I2C_RESET, LOW);

		// wait 10ms
		CThread::sleep(10U);

		// bring out of reset
		bcm2835_gpio_write(OLED_I2C_RESET, HIGH);
  	}

	return true;
}

void CI2CPi::setAddress(unsigned char address)
{
	if (!m_spi)
		bcm2835_i2c_setSlaveAddress(address);
}

void CI2CPi::setDataMode()
{
	if (m_spi) {
		// Setup D/C line to high to switch to data mode
		bcm2835_gpio_write(OLED_SPI_DC, HIGH);
	}
}

void CI2CPi::sendCommand(uint8_t c0, uint8_t c1, uint8_t c2) 
{ 
	uint8_t buff[4U];
    
	buff[1U] = c0;
	buff[2U] = c1;
	buff[3U] = c2;

	// Is SPI
	if (m_spi) {
		// Setup D/C line to low to switch to command mode
		bcm2835_gpio_write(OLED_SPI_DC, LOW);

		// Write Data
		bcm2835_spi_writenb(buff + 1U, 3U);
	} else {
		// Clear D/C to switch to command mode
		buff[0U] = SSD_Command_Mode; 

		// Write Data on I2C
		bcm2835_i2c_write(buff, 4U);
	}
}

void CI2CPi::sendCommand(uint8_t c0, uint8_t c1)
{ 
	uint8_t buff[3U];
    
	buff[1U] = c0;
	buff[2U] = c1;

	// Is SPI
	if (m_spi) {
		// Setup D/C line to low to switch to command mode
		bcm2835_gpio_write(OLED_SPI_DC, LOW);

		// Write Data
		bcm2835_spi_writenb(buff + 1U, 2U);
	} else {
		// Clear D/C to switch to command mode
		buff[0U] = SSD_Command_Mode; 

		// Write Data on I2C
		bcm2835_i2c_write(buff, 3U);
	}
}

void CI2CPi::sendCommand(uint8_t c) 
{ 
	// Is SPI
	if (m_spi) {
		// Setup D/C line to low to switch to command mode
		bcm2835_gpio_write(OLED_SPI_DC, LOW);

		// Write Data on SPI
		bcm2835_spi_transfer(c);
	} else {
		uint8_t buff[2U];

		// Clear D/C to switch to command mode
		buff[0] = SSD_Command_Mode; 
		buff[1] = c;

		// Write Data on I2C
		bcm2835_i2c_write(buff, 2U);
	}
}

void CI2CPi::writeData(const uint8_t* data, uint16_t length)
{
	if (m_spi)
		bcm2835_spi_writenb(data, length);
	else
		bcm2835_i2c_write(data, length);
}

void CI2CPi::writeData(uint8_t c)
{
	if (m_spi)
		bcm2835_spi_transfer(c);
	else
		bcm2835_spi_transfer(c);	// ???
}

void CI2CPi::close()
{
	if (m_spi)
		bcm2835_spi_end();
	else
		bcm2835_i2c_end();

	// Release Raspberry I/O control
	bcm2835_close();
}
