/*
 *   Copyright (C) 2009,2010,2011,2014 by Jonathan Naylor G4KLX
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

#ifndef	Timer_H
#define	Timer_H

class CTimer {
public:
	CTimer(unsigned int ticksPerSec, unsigned int secs = 0U, unsigned int msecs = 0U);
	~CTimer();

	void setTimeout(unsigned int secs, unsigned int msecs = 0U);

	unsigned int getTimeout() const;
	unsigned int getTimer() const;

	unsigned int getRemaining()
	{
		if (m_timeout == 0U || m_timer == 0U)
			return 0U;

		if (m_timer >= m_timeout)
			return 0U;

		return (m_timeout - m_timer) / m_ticksPerSec;
	}

	bool isRunning()
	{
		return m_timer > 0U;
	}

	void start(unsigned int secs, unsigned int msecs = 0U)
	{
		setTimeout(secs, msecs);

		start();
	}

	void start()
	{
		if (m_timeout > 0U)
			m_timer = 1U;
	}

	void stop()
	{
		m_timer = 0U;
	}

	bool hasExpired()
	{
		if (m_timeout == 0U || m_timer == 0U)
			return false;

		if (m_timer >= m_timeout)
			return true;

		return false;
	}

	void clock(unsigned int ticks = 1U)
	{
		if (m_timer > 0U && m_timeout > 0U)
			m_timer += ticks;
	}

private:
	unsigned int m_ticksPerSec;
	unsigned int m_timeout;
	unsigned int m_timer;
};

#endif
