#! /bin/bash

# tg_shutdown.sh - Automated shutdown and reboot on receipt of talkgroup
# Copyright (C) 2017  Tony Corbett, G0WFV and Stuart Scott, VK6LS
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USAp

# exit status ...
# 1 script not run as root
# 2 ini file doesn't exist

### CONFIG VARIABLES ###

sysopCallsign=XX0XXX
iniFile=/path/to/MMDVM.ini
shutdownTG=999999
rebootTG=888888
allowShutdown=1
allowReboot=1

### DON'T EDIT BELOW HERE ###

# exit if we're not root ...
[[ $EUID -ne 0 ]] && exit 1

# exit if we can't find the ini file
[[ -f $iniFile ]] || exit 2

# process the inifile and convert to variables ...
# 
# [Foo]
# Bar=Baz
#
# ... becomes the variable $FooBar with the value "Baz"
# (NOTE: spaces in section headers are replaced by underscores)
#
# a real life example ...
# 
# [General]
# Callsign=G0WFV
#
# ... becomes the variable $GeneralCallsign with the value "G0WFV"

foo=$(
	cat $iniFile | while read line
	do
		if echo $line | grep '^#.*$' >/dev/null # comment line
		then
			# Ignore!
			continue
		elif echo $line | grep '^$' >/dev/null # blank line
		then
			# Ignore!
			continue
		elif echo $line | grep '^\[.*\]$' >/dev/null # [Section Header]
		then
			iniSection=$(echo $line | sed 's/^\[\(.*\)\]$/\1/' | sed 's/[ -]//g')
		elif echo $line | grep '^.*=.*$' >/dev/null # Key=Value pair
		then
			iniKey=$(echo $line | sed 's/\(.*\)=.*$/\1/')
			iniValue=$(echo $line | sed 's/.*=\(.*\)$/\1/')
			echo $iniSection$iniKey=\"$iniValue\" # print the result in a specific format
		else # hopefully we'll never get here, but you never know!
			continue
		fi
	done
) 
eval $foo

# fix filepath if it doesn't end with a /
[[ "$(echo ${LogFilePath: -1})" != "/" ]] && LogFilePath="$LogFilePath/"

currentDate=foo # dummy current date variable to kick off the 1st tail!
shuttingDown=0

while true # main outer loop (run this forever!)
do
	checkDate=$(date -u +%Y-%m-%d)
	if [ "$checkDate" != "$currentDate" ]
	then
		kill $tailPID 2>/dev/null
		currentDate=$checkDate
		logFile=$LogFilePath$LogFileRoot-$currentDate.log

		tail -n 0 -F $logFile | while read line # inner loop to tail the logfile
		do
			# only react to sysop callsign ...
			foo=$(echo $line | grep "received RF voice header from $sysopCallsign to TG")
	
			if [ $? = 0 ]
			then
				TG=$(echo $line | sed "s/.*TG\(.*\)$/\1/g")
	
				if [ $TG -eq $shutdownTG ] && [ $shuttingDown -eq 0 ] && [ $allowShutdown -eq 1 ]
				then
					# shutdown in 1 minute ...
					shutdown -h +1 >/dev/null 2>&1 && shuttingDown=1
				elif [ $TG -eq $rebootTG ] && [ $shuttingDown -eq 0 ] && [ $allowReboot -eq 1 ]
				then
					# reboot in 1 minute ...
					shutdown -r +1 >/dev/null 2>&1 && shuttingDown=1
				elif [ $shuttingDown -eq 1 ]
				then
					# cancel shutdown or reboot if sysop tx any TG in 1 min grace period ...
					shutdown -c && shuttingDown=0
				fi
			fi	
		done & 2>/dev/null # inner loop is run in background so we can periodically check if the date's changed
		tailPID=$(($! - 1)) # save the PID of the inner loop so we can kill it when the date rolls over
	fi
	sleep 1 # check every second for date rollover (reduces cpu load)
done
