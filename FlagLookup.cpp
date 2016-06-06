/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "FlagLookup.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <cctype>

CFLAGLookup::CFLAGLookup(const std::string& filename) :
m_flagUsed()
{
}

CFLAGLookup::~CFLAGLookup()
{
}

std::string CFLAGLookup::flagFind(const char* source)
{
	char call_1[3]="";
	char call_2[3]="";
	char flagNumber[4]="";
        strncpy(call_1,source,1); 
        strncpy(call_2,source,2);
        	if      ((strcmp(call_2,"C3")==0) ){
			::sprintf(flagNumber,"%s","004"); //Andorra 			
			}
		else if ((strcmp(call_2,"A6")==0)){
			::sprintf(flagNumber,"%s","005"); // 'United Arab Emirates' 	
			}
		else if ((strcmp(call_2,"T6")==0) || (strcmp(call_2,"YA")==0) ){
			::sprintf(flagNumber,"%s","006"); // 'Afghanistan' 		
			}
		else if ((strcmp(call_2,"V2")==0)){
			::sprintf(flagNumber,"%s","007"); // 'Antigua and Barbuda' 	
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","008"); // 'Anguilla'			
			}
		else if ((strcmp(call_2,"ZA")==0)){
			::sprintf(flagNumber,"%s","009"); // 'Albania'			
			}
		else if ((strcmp(call_2,"EK")==0)){
			::sprintf(flagNumber,"%s","010"); // 'Armenia'			
			}
		else if ((strcmp(call_2,"PJ")==0)){
			::sprintf(flagNumber,"%s","011"); // 'Netherlands Antilles'	
			}
		else if ((strcmp(call_2,"D2")==0) || (strcmp(call_2,"D3")==0) ){
			::sprintf(flagNumber,"%s","012"); // 'Angola'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","013"); // 'Antarctica'		
			}
		else if ((strcmp(call_2,"AY")==0) || (strcmp(call_2,"AZ")==0) ||
			 (strcmp(call_2,"L2")==0) || (strcmp(call_2,"L3")==0) ||
			 (strcmp(call_2,"L4")==0) || (strcmp(call_2,"L5")==0) ||
			 (strcmp(call_2,"L6")==0) || (strcmp(call_2,"L7")==0) ||
			 (strcmp(call_2,"L9")==0) || (strcmp(call_2,"L9")==0) ||
			 (strcmp(call_2,"LO")==0) || (strcmp(call_2,"LP")==0) ||
			 (strcmp(call_2,"LQ")==0) || (strcmp(call_2,"LR")==0) ||
			 (strcmp(call_2,"LS")==0) || (strcmp(call_2,"LT")==0) ||
			 (strcmp(call_2,"LU")==0) || (strcmp(call_2,"LV")==0) ||
			 (strcmp(call_2,"LW")==0) ){
			::sprintf(flagNumber,"%s","014"); // 'Argentina'		
			}
		else if ((strcmp(call_2,"5W")==0)){
			::sprintf(flagNumber,"%s","015"); // 'American Samoa'		
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","203"); // 'Saint Helena		
			}
		else if ((strcmp(call_2,"OE")==0)){
			::sprintf(flagNumber,"%s","016"); // 'Austria'			
			}
		else if ((strcmp(call_2,"AX")==0) || (strcmp(call_2,"VH")==0) ||
			 (strcmp(call_2,"VI")==0) || (strcmp(call_2,"VJ")==0) ||
			 (strcmp(call_2,"VK")==0) || (strcmp(call_2,"VL")==0) ||
			 (strcmp(call_2,"VM")==0) || (strcmp(call_2,"VN")==0) ||
			 (strcmp(call_2,"VZ")==0) ){
			::sprintf(flagNumber,"%s","017"); // 'Australia'		
			}
		else if ((strcmp(call_2,"P4")==0)){
			::sprintf(flagNumber,"%s","018"); // 'Aruba'			
			}
		else if ((strcmp(call_2,"OG")==0)){
			::sprintf(flagNumber,"%s","019"); // 'Aland Islands'		
			}
		else if ((strcmp(call_2,"4J")==0) || (strcmp(call_2,"4K")==0)){
			::sprintf(flagNumber,"%s","020"); // 'Azerbaijan'		
			}
		else if ((strcmp(call_2,"E7")==0)){
			::sprintf(flagNumber,"%s","021"); // 'Bosnia and Herzegovina'	

			}
		else if ((strcmp(call_2,"8P")==0)){
			::sprintf(flagNumber,"%s","022"); // 'Barbados'			
			}
		else if ((strcmp(call_2,"S2")==0) || (strcmp(call_2,"S3")==0) ){
			::sprintf(flagNumber,"%s","023"); // 'Bangladesh'		
			}
		else if ((strcmp(call_2,"ON")==0) || (strcmp(call_2,"OO")==0) ||
			 (strcmp(call_2,"OP")==0) || (strcmp(call_2,"OQ")==0) ||
			 (strcmp(call_2,"OR")==0) || (strcmp(call_2,"OS")==0) ||
			 (strcmp(call_2,"OT")==0) ){ 
			::sprintf(flagNumber,"%s","024"); // 'Belgium'			
			}
		else if ((strcmp(call_2,"XT")==0)){
			::sprintf(flagNumber,"%s","025"); // 'Burkina Faso'		
			}
		else if ((strcmp(call_2,"LZ")==0)){
			::sprintf(flagNumber,"%s","026"); // 'Bulgaria'
			}
		else if ((strcmp(call_2,"A9")==0)){
			::sprintf(flagNumber,"%s","027"); // 'Bahrain'			
			}
		else if ((strcmp(call_2,"9U")==0)){
			::sprintf(flagNumber,"%s","028"); // 'Burundi'			
			}
		else if ((strcmp(call_2,"TY")==0)){
			::sprintf(flagNumber,"%s","029"); // 'Benin'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","030"); // 'Saint Barthelemy'		
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","031"); // 'Bermuda'			
			}
		else if ((strcmp(call_2,"V8")==0)){
			::sprintf(flagNumber,"%s","032"); // 'Brunei Darussalam'	
			}
		else if ((strcmp(call_2,"4M")==0) || (strcmp(call_2,"YV")==0) ||
			 (strcmp(call_2,"YW")==0) || (strcmp(call_2,"YX")==0) ||
			 (strcmp(call_2,"YY")==0) ){
			::sprintf(flagNumber,"%s","033"); // 'Venezuela			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","034"); // 'Ikariam'			
			}
		else if ((strcmp(call_2,"PP")==0) || (strcmp(call_2,"PQ")==0) ||
			 (strcmp(call_2,"PR")==0) || (strcmp(call_2,"PS")==0) ||
			 (strcmp(call_2,"PT")==0) || (strcmp(call_2,"PU")==0) ||
			 (strcmp(call_2,"PV")==0) || (strcmp(call_2,"PW")==0) ||
			 (strcmp(call_2,"PX")==0) || (strcmp(call_2,"PY")==0) ||
			 (strcmp(call_2,"ZV")==0) || (strcmp(call_2,"ZW")==0) ||
			 (strcmp(call_2,"ZX")==0) || (strcmp(call_2,"ZY")==0) ||
			 (strcmp(call_2,"ZZ")==0) ){
			::sprintf(flagNumber,"%s","035"); // 'Brazil'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","244"); // 'Virgin Islands		
			}
		else if ((strcmp(call_2,"C6")==0)){
			::sprintf(flagNumber,"%s","036"); // 'Bahamas'			
			}
		else if ((strcmp(call_2,"A5")==0)){
			::sprintf(flagNumber,"%s","037"); // 'Bhutan'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","038"); // 'Bouvet Island'		
			}
		else if ((strcmp(call_2,"8O")==0) || (strcmp(call_2,"A2")==0) ){
			::sprintf(flagNumber,"%s","039"); // 'Botswana'			
			}
		else if ((strcmp(call_2,"EU")==0) || (strcmp(call_2,"EV")==0) ||
			 (strcmp(call_2,"EW")==0) ){
			::sprintf(flagNumber,"%s","040"); // 'Belarus'			
			}
		else if ((strcmp(call_2,"V3")==0)){
			::sprintf(flagNumber,"%s","041"); // 'Belize'			
			}
		else if ((strcmp(call_2,"CF")==0) || (strcmp(call_2,"CG")==0) ||
			 (strcmp(call_2,"CH")==0) || (strcmp(call_2,"CI")==0) ||
			 (strcmp(call_2,"CJ")==0) || (strcmp(call_2,"CK")==0) ||
			 (strcmp(call_2,"CY")==0) || (strcmp(call_2,"CZ")==0) ||
			 (strcmp(call_2,"VA")==0) || (strcmp(call_2,"VB")==0) ||
			 (strcmp(call_2,"VC")==0) || (strcmp(call_2,"VD")==0) ||
			 (strcmp(call_2,"VE")==0) || (strcmp(call_2,"VF")==0) ||
			 (strcmp(call_2,"VG")==0) || (strcmp(call_2,"VO")==0) ||
			 (strcmp(call_2,"VX")==0) || (strcmp(call_2,"VY")==0) ||
			 (strcmp(call_2,"XJ")==0) || (strcmp(call_2,"XK")==0) ||
			 (strcmp(call_2,"XL")==0) || (strcmp(call_2,"XM")==0) ||
			 (strcmp(call_2,"XN")==0) || (strcmp(call_2,"XO")==0) ||
			 (strcmp(call_2,"CZ")==0) ){
			::sprintf(flagNumber,"%s","042"); // 'Canada'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","043"); // 'Cocos (Keeling Islands'
			}
		else if ((strcmp(call_2,"TL")==0)){
			::sprintf(flagNumber,"%s","045"); // 'Central African Republic'	
			}
		else if ((strcmp(call_2,"TN")==0)){
			::sprintf(flagNumber,"%s","044"); // 'Congo (Republic of the)'	
			}
		else if ((strcmp(call_2,"HB")==0) || (strcmp(call_2,"HE")==0) ){
			::sprintf(flagNumber,"%s","047"); // 'Switzerland'		
			}
		else if ((strcmp(call_2,"TU")==0)){
			::sprintf(flagNumber,"%s","048"); // 'Cote d'Ivoire'		
			}
		else if ((strcmp(call_2,"E5")==0)){
			::sprintf(flagNumber,"%s","049"); // 'Cook Islands'		
			}
		else if ((strcmp(call_2,"3G")==0) || (strcmp(call_2,"CA")==0) ||
			 (strcmp(call_2,"CB")==0) || (strcmp(call_2,"CC")==0) ||
			 (strcmp(call_2,"CD")==0) || (strcmp(call_2,"CE")==0) ||
			 (strcmp(call_2,"XQ")==0) || (strcmp(call_2,"XR")==0) ){
			::sprintf(flagNumber,"%s","050"); // 'Chile'			
			}
		else if ((strcmp(call_2,"TJ")==0)){
			::sprintf(flagNumber,"%s","051"); // 'Cameroon'			
			}
		else if ((strcmp(call_2,"3H")==0) || (strcmp(call_2,"3I")==0) ||
			 (strcmp(call_2,"3J")==0) || (strcmp(call_2,"3K")==0) ||
		  	 (strcmp(call_2,"3L")==0) || (strcmp(call_2,"3M")==0) ||
			 (strcmp(call_2,"3N")==0) || (strcmp(call_2,"3O")==0) ||
			 (strcmp(call_2,"3P")==0) || (strcmp(call_2,"3Q")==0) ||
			 (strcmp(call_2,"3R")==0) || (strcmp(call_2,"3S")==0) ||
			 (strcmp(call_2,"3T")==0) || (strcmp(call_2,"3U")==0) ||
			 (strcmp(call_1,"B")==0)  || (strcmp(call_2,"XS")==0) ){
			::sprintf(flagNumber,"%s","052"); // 'China'			
			}
		else if ((strcmp(call_2,"5J")==0) || (strcmp(call_2,"5K")==0) ||
			 (strcmp(call_2,"HJ")==0) || (strcmp(call_2,"HK")==0) ){
			::sprintf(flagNumber,"%s","053"); // 'Colombia'			
			}
		else if ((strcmp(call_2,"TE")==0) || (strcmp(call_2,"TI")==0) ){
			::sprintf(flagNumber,"%s","054"); // 'Costa Rica'		
			}
		else if ((strcmp(call_2,"CL")==0) || (strcmp(call_2,"CM")==0) ||
			 (strcmp(call_2,"CO")==0) || (strcmp(call_2,"T4")==0) ){
			::sprintf(flagNumber,"%s","055"); // 'Cuba'			
			}
		else if ((strcmp(call_2,"D4")==0)){
			::sprintf(flagNumber,"%s","056"); // 'Cape Verde'		
			}
		else if ((strcmp(call_2,"PJ")==0)){
			::sprintf(flagNumber,"%s","057"); // 'Curacao'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","058"); // 'Christmas Island'
			}
		else if ((strcmp(call_2,"5B")==0) || (strcmp(call_2,"C4")==0) ||
			 (strcmp(call_2,"H2")==0) || (strcmp(call_2,"P3")==0) ){
			::sprintf(flagNumber,"%s","059"); // 'Cyprus'			
			}
		else if ((strcmp(call_2,"OK")==0) || (strcmp(call_2,"OL")==0) ){
			::sprintf(flagNumber,"%s","060"); // 'Czech Republic'		
			}
		else if ((strcmp(call_2,"DA")==0) || (strcmp(call_2,"DB")==0) || 
			 (strcmp(call_2,"DC")==0) || (strcmp(call_2,"DD")==0) ||
			 (strcmp(call_2,"DE")==0) || (strcmp(call_2,"DF")==0) ||
			 (strcmp(call_2,"DG")==0) || (strcmp(call_2,"DH")==0) ||
			 (strcmp(call_2,"DI")==0) || (strcmp(call_2,"DJ")==0) ||
			 (strcmp(call_2,"DK")==0) || (strcmp(call_2,"DL")==0) ||
			 (strcmp(call_2,"DM")==0) || (strcmp(call_2,"DN")==0) ||
			 (strcmp(call_2,"DO")==0) || (strcmp(call_2,"DP")==0) ||
			 (strcmp(call_2,"Y2")==0) || (strcmp(call_2,"Y3")==0) ||
			 (strcmp(call_2,"Y4")==0) || (strcmp(call_2,"Y5")==0) ||
			 (strcmp(call_2,"Y6")==0) || (strcmp(call_2,"Y7")==0) ||
			 (strcmp(call_2,"Y8")==0) || (strcmp(call_2,"Y9")==0) ){
			::sprintf(flagNumber,"%s","061"); // 'Germany'			
			}
		else if ((strcmp(call_2,"HM")==0) || (strcmp(call_2,"P5")==0) ||
			 (strcmp(call_2,"P6")==0) || (strcmp(call_2,"P7")==0) ||
			 (strcmp(call_2,"P8")==0) || (strcmp(call_2,"P9")==0) ){
			::sprintf(flagNumber,"%s","125"); // 'Korea (Deomocratic Peoples Republic of)' 
			}
		else if ((strcmp(call_2,"J2")==0)){
			::sprintf(flagNumber,"%s","062"); // 'Djibouti'			
			}
		else if ((strcmp(call_2,"5P")==0) || (strcmp(call_2,"5Q")==0) ||
			 (strcmp(call_2,"OU")==0) || (strcmp(call_2,"OV")==0) ||
			 (strcmp(call_2,"OW")==0) || (strcmp(call_2,"OX")==0) ||
			 (strcmp(call_2,"OY")==0) || (strcmp(call_2,"OZ")==0) ||
			 (strcmp(call_2,"XP")==0) ){
			::sprintf(flagNumber,"%s","063"); // 'Denmark'			
			}
		else if ((strcmp(call_2,"J7")==0)){
			::sprintf(flagNumber,"%s","064"); // 'Dominica'			
			}
		else if ((strcmp(call_2,"HI")==0)){
			::sprintf(flagNumber,"%s","065"); // 'Dominican Republic'	
			}
		else if ((strcmp(call_2,"7R")==0) || (strcmp(call_2,"7T")==0) || 
			 (strcmp(call_2,"7U")==0) || (strcmp(call_2,"7V")==0) ||
                         (strcmp(call_2,"7W")==0) || (strcmp(call_2,"7X")==0) ||
                         (strcmp(call_2,"7Y")==0) ){
			::sprintf(flagNumber,"%s","066"); // 'Algeria'			
			}
		else if ((strcmp(call_2,"HC")==0) || (strcmp(call_2,"HD")==0) ){
			::sprintf(flagNumber,"%s","067"); // 'Ecuador'			
			}
		else if ((strcmp(call_2,"ES")==0)){
			::sprintf(flagNumber,"%s","068"); // 'Estonia'			
			}
		else if ((strcmp(call_2,"6A")==0) || (strcmp(call_2,"6B")==0) ||
			 (strcmp(call_2,"SS")==0) || (strcmp(call_2,"SU")==0) ){
			::sprintf(flagNumber,"%s","069"); // 'Egypt'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","070"); // 'Western Sahara'		
			}
		else if ((strcmp(call_2,"E3")==0)){
			::sprintf(flagNumber,"%s","071"); // 'Eritrea'			
			}
		else if ((strcmp(call_2,"AM")==0) || (strcmp(call_2,"AN")==0) ||
			 (strcmp(call_2,"AO")==0) || (strcmp(call_2,"EA")==0) ||
			 (strcmp(call_2,"EB")==0) || (strcmp(call_2,"EC")==0) ||
			 (strcmp(call_2,"ED")==0) || (strcmp(call_2,"EF")==0) ||
			 (strcmp(call_2,"EG")==0) || (strcmp(call_2,"EH")==0) ||
			 (strcmp(call_2,"EE")==0) ){
			 ::sprintf(flagNumber,"%s","072"); // 'Spain'			
			}
		else if ((strcmp(call_2,"9E")==0) || (strcmp(call_2,"9F")==0) ||
			 (strcmp(call_2,"ET")==0) ){
			::sprintf(flagNumber,"%s","073"); // 'Ethiopia'			
			}
		else if ((strcmp(call_2,"V6")==0)){
			::sprintf(flagNumber,"%s","077"); // 'Micronesia
			}
		else if ((strcmp(call_2,"OF")==0) || (strcmp(call_2,"OG")==0) ||
			 (strcmp(call_2,"OH")==0) || (strcmp(call_2,"OI")==0) ||
			 (strcmp(call_2,"OJ")==0) ){
			::sprintf(flagNumber,"%s","074"); // 'Finland			
			}
		else if ((strcmp(call_2,"3D")==0)){
			::sprintf(flagNumber,"%s","075"); // 'Fiji'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","076"); // 'Falkland Islands (Malvinas '
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","078"); // 'Faroe Islands'
			}
		else if ((strcmp(call_1,"F")==0)  || (strcmp(call_2,"HW")==0) ||
			 (strcmp(call_2,"HX")==0) || (strcmp(call_2,"HY")==0) ||
			 (strcmp(call_2,"TH")==0) || (strcmp(call_2,"TK")==0) ||
			 (strcmp(call_2,"TM")==0) || (strcmp(call_2,"TO")==0) ||
			 (strcmp(call_2,"TP")==0) || (strcmp(call_2,"TQ")==0) ||
			 (strcmp(call_2,"TV")==0) || (strcmp(call_2,"TW")==0) ||
			 (strcmp(call_2,"TX")==0) ){
			::sprintf(flagNumber,"%s","079"); // 'France'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","080"); // 'Gabon'
			}
		else if ((strcmp(call_1,"G")==0)  || (strcmp(call_1,"M")==0) || 
			 (strcmp(call_2,"2E")==0) || (strcmp(call_2,"VP")==0) ||
			 (strcmp(call_2,"VQ")==0) || (strcmp(call_2,"VS")==0) ||
			 (strcmp(call_2,"ZB")==0) || (strcmp(call_2,"ZC")==0) ||
			 (strcmp(call_2,"ZD")==0) || (strcmp(call_2,"ZE")==0) ||
			 (strcmp(call_2,"ZF")==0) || (strcmp(call_2,"ZG")==0) ||
			 (strcmp(call_2,"ZH")==0) || (strcmp(call_2,"ZI")==0) ||
			 (strcmp(call_2,"ZJ")==0) || (strcmp(call_2,"ZN")==0) ||
			 (strcmp(call_2,"ZO")==0) || (strcmp(call_2,"ZQ")==0) ){
			::sprintf(flagNumber,"%s","081"); // 'United Kingdom'	
			}
		else if ((strcmp(call_2,"J3")==0)){
			::sprintf(flagNumber,"%s","082"); // 'Grenada'			
			}
		else if ((strcmp(call_2,"4L")==0)){
			::sprintf(flagNumber,"%s","083"); // 'Georgia'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","084"); // 'French Guiana'		
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","085"); // 'Guernsey'			
			}
		else if ((strcmp(call_2,"9G")==0)){
			::sprintf(flagNumber,"%s","086"); // 'Ghana'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","087"); // 'Gibraltar'		
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","088"); // 'Greenland'		
			}
		else if ((strcmp(call_2,"C5")==0)){
			::sprintf(flagNumber,"%s","089"); // 'Gambia'			
			}
		else if ((strcmp(call_2,"3X")==0)){
			::sprintf(flagNumber,"%s","090"); // 'Guinea'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","091"); // 'Guadeloupe'
			}
		else if ((strcmp(call_2,"3C")==0)){
			::sprintf(flagNumber,"%s","092"); // 'Equatorial Guinea'
			}
		else if ((strcmp(call_2,"J4")==0) || (strcmp(call_2,"SV")==0) ||
			 (strcmp(call_2,"SW")==0) || (strcmp(call_2,"SX")==0) ||
			 (strcmp(call_2,"SY")==0) || (strcmp(call_2,"SZ")==0) ){
			::sprintf(flagNumber,"%s","093"); // 'Greece'
			}
		else if ((strcmp(call_2,"4L")==0)){
			::sprintf(flagNumber,"%s","094"); // 'South Georgia and The South Sandwich Islands'
			}
		else if ((strcmp(call_2,"TD")==0) || (strcmp(call_2,"TG")==0) ){
			::sprintf(flagNumber,"%s","095"); // 'Guatemala'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","096"); // 'Guam'
			}
		else if ((strcmp(call_2,"J5")==0)){
			::sprintf(flagNumber,"%s","097"); // 'Guinea-Bissau'
			}
		else if ((strcmp(call_2,"8R")==0)){
			::sprintf(flagNumber,"%s","098"); // 'Guyana'
			}
		else if ((strcmp(call_2,"VR")==0)){
			::sprintf(flagNumber,"%s","099"); // 'Hong Kong'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","100"); // 'Heard Island and McDonald Islands'
			}
		else if ((strcmp(call_2,"HQ")==0) || (strcmp(call_2,"HR")==0) ){
			::sprintf(flagNumber,"%s","101"); // 'Honduras'
			}
		else if ((strcmp(call_2,"9A")==0)){
			::sprintf(flagNumber,"%s","102"); // 'Croatia'
			}
		else if ((strcmp(call_2,"4V")==0) || (strcmp(call_2,"HH")==0) ){
			::sprintf(flagNumber,"%s","103"); // 'Haiti'
			}
		else if ((strcmp(call_2,"HA")==0) || (strcmp(call_2,"HG")==0) ){
			::sprintf(flagNumber,"%s","104"); // 'Hungary'
			}
		else if ((strcmp(call_2,"7A")==0) || (strcmp(call_2,"7B")==0) || 
			 (strcmp(call_2,"7C")==0) || (strcmp(call_2,"7D")==0) ||
			 (strcmp(call_2,"7E")==0) || (strcmp(call_2,"7F")==0) ||
			 (strcmp(call_2,"7G")==0) || (strcmp(call_2,"7H")==0) ||
			 (strcmp(call_2,"7I")==0) || (strcmp(call_2,"8A")==0) ||
                         (strcmp(call_2,"8B")==0) || (strcmp(call_2,"8C")==0) ||
                         (strcmp(call_2,"8D")==0) || (strcmp(call_2,"8E")==0) ||
                         (strcmp(call_2,"8F")==0) || (strcmp(call_2,"8G")==0) ||
                         (strcmp(call_2,"8H")==0) || (strcmp(call_2,"8I")==0) ){
			::sprintf(flagNumber,"%s","105"); // 'Indonesia'
			}
		else if ((strcmp(call_2,"EI")==0) || (strcmp(call_2,"EJ")==0)){
			::sprintf(flagNumber,"%s","106"); // 'Ireland'
			}
		else if ((strcmp(call_2,"4X")==0) || (strcmp(call_2,"4Z")==0) ){
			::sprintf(flagNumber,"%s","107"); // 'Israel'
			}
		else if ((strcmp(call_2,"GD")==0)){
			::sprintf(flagNumber,"%s","108"); // 'Isle of Man'
			}
		else if ((strcmp(call_2,"8T")==0) || (strcmp(call_2,"8U")==0) || 
			 (strcmp(call_2,"8V")==0) || (strcmp(call_2,"8W")==0) ||
			 (strcmp(call_2,"8X")==0) || (strcmp(call_2,"8Y")==0) ||
			 (strcmp(call_2,"AT")==0) || (strcmp(call_2,"AU")==0) ||
			 (strcmp(call_2,"AV")==0) || (strcmp(call_2,"AW")==0) ){
			::sprintf(flagNumber,"%s","109"); // 'India'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","110"); // 'British Indian Ocean Territory'
			}
		else if ((strcmp(call_2,"HN")==0) || (strcmp(call_2,"YI")==0) ){
			::sprintf(flagNumber,"%s","111"); // 'Iraq'
			}
		else if ((strcmp(call_2,"TF")==0)){
			::sprintf(flagNumber,"%s","113"); // 'Iceland'
			}
		else if ((strcmp(call_2,"9B")==0) || (strcmp(call_2,"9C")==0) || 
			 (strcmp(call_2,"9D")==0) || (strcmp(call_2,"EP")==0) ||
			 (strcmp(call_2,"EQ")==0) ){
			::sprintf(flagNumber,"%s","112"); // 'Iran (Islamic Republic of)'
			}
		else if ((strcmp(call_1,"I")==0)){
			::sprintf(flagNumber,"%s","114"); // 'Italy'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","115"); // 'Jersey'
			}
		else if ((strcmp(call_2,"6Y")==0)){
			::sprintf(flagNumber,"%s","116"); // 'Jamaica'
			}
		else if ((strcmp(call_2,"JY")==0)){
			::sprintf(flagNumber,"%s","117"); // 'Jordan'
			}
		else if ((strcmp(call_2,"7J")==0) || (strcmp(call_2,"7K")==0) || 
			 (strcmp(call_2,"7L")==0) || (strcmp(call_2,"7M")==0) ||
			 (strcmp(call_2,"7N")==0) || (strcmp(call_2,"8J")==0) ||
			 (strcmp(call_2,"8K")==0) || (strcmp(call_2,"8L")==0) ||
			 (strcmp(call_2,"8M")==0) || (strcmp(call_2,"8N")==0) ){
			::sprintf(flagNumber,"%s","118"); // 'Japan'
			}
		else if ((strcmp(call_2,"5Y")==0) || (strcmp(call_2,"5Z")==0)){
			::sprintf(flagNumber,"%s","119"); // 'Kenya'
			}
		else if ((strcmp(call_2,"EX")==0)){
			::sprintf(flagNumber,"%s","120"); // 'Kyrgyzstan'
			}
		else if ((strcmp(call_2,"XU")==0)){
			::sprintf(flagNumber,"%s","121"); // 'Cambodia'
			}
		else if ((strcmp(call_2,"T3")==0)){
			::sprintf(flagNumber,"%s","122"); // 'Kiribati'
			}
		else if ((strcmp(call_2,"D6")==0)){
			::sprintf(flagNumber,"%s","123"); // 'Comoros'
			}
		else if ((strcmp(call_2,"V4")==0)){
			::sprintf(flagNumber,"%s","124"); // 'Saint Kitts and Nevis'
			}
		else if ((strcmp(call_2,"9K")==0)){
			::sprintf(flagNumber,"%s","127"); // 'Kuwait'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","128"); // 'Cayman Islands'
			}
		else if ((strcmp(call_2,"UN")==0) || (strcmp(call_2,"UO")==0) || 
			 (strcmp(call_2,"UP")==0) || (strcmp(call_2,"UQ")==0) ){
			::sprintf(flagNumber,"%s","129"); // 'Kazakhstan'
			}
		else if ((strcmp(call_2,"XW")==0)){
			::sprintf(flagNumber,"%s","130"); // 'Lao People's Democratic Republic'
			}
		else if ((strcmp(call_2,"OD")==0)){
			::sprintf(flagNumber,"%s","131"); // 'Lebanon'
			}
		else if ((strcmp(call_2,"J6")==0)){
			::sprintf(flagNumber,"%s","132"); // 'Saint Lucia'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","133"); // 'Liechtenstein'
			}
		else if ((strcmp(call_2,"4P")==0) || (strcmp(call_2,"4Q")==0) || 
			 (strcmp(call_2,"4R")==0) || (strcmp(call_2,"4S")==0) ){
			::sprintf(flagNumber,"%s","134"); // 'Sri Lanka'
			}
		else if ((strcmp(call_2,"5L")==0) || (strcmp(call_2,"5M")==0) ||
			 (strcmp(call_2,"6Z")==0) || (strcmp(call_2,"A8")==0) ||
			 (strcmp(call_2,"D5")==0) || (strcmp(call_2,"EL")==0) ){
			::sprintf(flagNumber,"%s","135"); // 'Liberia'
			}
		else if ((strcmp(call_2,"7P")==0)){
			::sprintf(flagNumber,"%s","136"); // 'Lesotho'
			}
		else if ((strcmp(call_2,"LY")==0)){
			::sprintf(flagNumber,"%s","137"); // 'Lithuania'
			}
		else if ((strcmp(call_2,"LX")==0)){
			::sprintf(flagNumber,"%s","138"); // 'Luxembourg'
			}
		else if ((strcmp(call_2,"YL")==0)){
			::sprintf(flagNumber,"%s","139"); // 'Latvia'
			}
		else if ((strcmp(call_2,"5A")==0)){
			::sprintf(flagNumber,"%s","140"); // 'Libyan Arab Jamahiriya'
			}
		else if ((strcmp(call_2,"5C")==0) || (strcmp(call_2,"5D")==0) || 
			 (strcmp(call_2,"5E")==0) || (strcmp(call_2,"5F")==0) ||
			 (strcmp(call_2,"5G")==0) ){
			::sprintf(flagNumber,"%s","141"); // 'Morocco'
			}
		else if ((strcmp(call_2,"3A")==0)){
			::sprintf(flagNumber,"%s","142"); // 'Monaco'
			}
		else if ((strcmp(call_2,"4O")==0)){
			::sprintf(flagNumber,"%s","144"); // 'Montenegro'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","145"); // 'Saint Martin (French Part)'
			}
		else if ((strcmp(call_2,"5R")==0) || (strcmp(call_2,"5S")==0) ||
			 (strcmp(call_2,"6X")==0) ){
			::sprintf(flagNumber,"%s","146"); // 'Madagascar'
			}
		else if ((strcmp(call_2,"V7")==0)){
			::sprintf(flagNumber,"%s","147"); // 'Marshall Islands'
			}
		else if ((strcmp(call_2,"TZ")==0)){
			::sprintf(flagNumber,"%s","149"); // 'Mali'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","150"); // 'Myanmar'
			}
		else if ((strcmp(call_2,"JT")==0) || (strcmp(call_2,"JU")==0) ||
			 (strcmp(call_2,"JV")==0) ){
			::sprintf(flagNumber,"%s","151"); // 'Mongolia'
			}
		else if ((strcmp(call_2,"XX")==0)){
			::sprintf(flagNumber,"%s","152"); // 'Macao'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","153"); // 'Northern Mariana Islands'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","154"); // 'Martinique'
			}
		else if ((strcmp(call_2,"5T")==0)){
			::sprintf(flagNumber,"%s","155"); // 'Mauritania'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","156"); // 'Montserrat'
			}
		else if ((strcmp(call_2,"9H")==0)){
			::sprintf(flagNumber,"%s","157"); // 'Malta'
			}
		else if ((strcmp(call_2,"3B")==0)){
			::sprintf(flagNumber,"%s","158"); // 'Mauritius'
			}
		else if ((strcmp(call_2,"8Q")==0)){
			::sprintf(flagNumber,"%s","159"); // 'Maldives'
			}
		else if ((strcmp(call_2,"7Q")==0)){
			::sprintf(flagNumber,"%s","160"); // 'Malawi'
			}
		else if ((strcmp(call_2,"4A")==0) || (strcmp(call_2,"4B")==0) ||
			 (strcmp(call_2,"4C")==0) || (strcmp(call_2,"6D")==0) || 
			 (strcmp(call_2,"6E")==0) || (strcmp(call_2,"6F")==0) || 
			 (strcmp(call_2,"6G")==0) || (strcmp(call_2,"6H")==0) || 
			 (strcmp(call_2,"6I")==0) || (strcmp(call_2,"6J")==0) ){
			::sprintf(flagNumber,"%s","161"); // 'Mexico'
			}
		else if ((strcmp(call_2,"9M")==0) || (strcmp(call_2,"9W")==0) ){
			::sprintf(flagNumber,"%s","162"); // 'Malaysia'
			}
		else if ((strcmp(call_2,"C8")==0) || (strcmp(call_2,"C9")==0) ){
			::sprintf(flagNumber,"%s","163"); // 'Mozambique'
			}
		else if ((strcmp(call_2,"V5")==0)){
			::sprintf(flagNumber,"%s","164"); // 'Namibia'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","165"); // 'New Caledonia'
			}
		else if ((strcmp(call_2,"5U")==0)){
			::sprintf(flagNumber,"%s","166"); // 'Niger'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","167"); // 'Norfolk Island'
			}
		else if ((strcmp(call_2,"5N")==0) || (strcmp(call_2,"5M")==0) ){
			::sprintf(flagNumber,"%s","168"); // 'Nigeria'
			}
		else if ((strcmp(call_2,"H6")==0) || (strcmp(call_2,"H7")==0) ||
			 (strcmp(call_2,"H7")==0) ){
			::sprintf(flagNumber,"%s","169"); // 'Nicaragua'
			}
		else if ((strcmp(call_2,"PA")==0) || (strcmp(call_2,"PB")==0) ||
			 (strcmp(call_2,"PC")==0) || (strcmp(call_2,"PD")==0) ||
			 (strcmp(call_2,"PE")==0) || (strcmp(call_2,"PF")==0) ||
			 (strcmp(call_2,"PG")==0) || (strcmp(call_2,"PH")==0) ||
			 (strcmp(call_2,"PI")==0) ){
			::sprintf(flagNumber,"%s","170"); // 'Netherlands'
			}
		else if ((strcmp(call_2,"3Y")==0)){
			::sprintf(flagNumber,"%s","171"); // 'Norway'
			}
		else if ((strcmp(call_2,"9N")==0)){
			::sprintf(flagNumber,"%s","172"); // 'Nepal'
			}
		else if ((strcmp(call_2,"C2")==0)){
			::sprintf(flagNumber,"%s","173"); // 'Nauru'
			}
		else if ((strcmp(call_2,"E6")==0)){
			::sprintf(flagNumber,"%s","174"); // 'Niue'
			}
		else if ((strcmp(call_2,"E5")==0) || (strcmp(call_2,"E6")==0)){
			::sprintf(flagNumber,"%s","175"); // 'New Zealand'
			}
		else if ((strcmp(call_2,"E4")==0)){
			::sprintf(flagNumber,"%s","187"); // 'Palestinian Territory
			}
		else if ((strcmp(call_2,"A4")==0)){
			::sprintf(flagNumber,"%s","176"); // 'Oman'
			}
		else if ((strcmp(call_2,"3E")==0) || (strcmp(call_2,"3F")==0) ||
			 (strcmp(call_2,"H3")==0) || (strcmp(call_2,"H8")==0) ||
			 (strcmp(call_2,"H9")==0) ){
			::sprintf(flagNumber,"%s","177"); // 'Panama'
			}
		else if ((strcmp(call_2,"4T")==0) || (strcmp(call_2,"OA")==0) ||
			 (strcmp(call_2,"OB")==0) || (strcmp(call_2,"OC")==0) ){
			::sprintf(flagNumber,"%s","178"); // 'Peru'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","179"); // 'French Polynesia'
			}
		else if ((strcmp(call_2,"P2")==0)){
			::sprintf(flagNumber,"%s","180"); // 'Papua New Guinea'
			}
		else if ((strcmp(call_2,"4D")==0) || (strcmp(call_2,"4E")==0) || 
			 (strcmp(call_2,"4F")==0) || (strcmp(call_2,"4G")==0) || 
			 (strcmp(call_2,"4H")==0) || (strcmp(call_2,"4I")==0) ||
			 (strcmp(call_2,"DU")==0) || (strcmp(call_2,"DV")==0) || 
			 (strcmp(call_2,"DW")==0) || (strcmp(call_2,"DX")==0) || 
			 (strcmp(call_2,"DY")==0) || (strcmp(call_2,"DZ")==0) ){
			::sprintf(flagNumber,"%s","181"); // 'Philippines'
			}
		else if ((strcmp(call_2,"6P")==0) || (strcmp(call_2,"6Q")==0) ||
			 (strcmp(call_2,"6R")==0) || (strcmp(call_2,"6S")==0) ||
			 (strcmp(call_2,"AP")==0) || (strcmp(call_2,"AQ")==0) ||
			 (strcmp(call_2,"AR")==0) || (strcmp(call_2,"AS")==0) ){
			::sprintf(flagNumber,"%s","182"); // 'Pakistan'
			}
		else if ((strcmp(call_2,"3Z")==0) || (strcmp(call_2,"HF")==0) ){
			::sprintf(flagNumber,"%s","183"); // 'Poland'
			}
		else if ((strcmp(call_2,"CP")==0)){
			::sprintf(flagNumber,"%s","033"); // 'Bolivia
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","184"); // 'Saint Pierre and Miquelon'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","185"); // 'Pitcairn'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","232"); // 'Taiwan
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","186"); // 'Puerto Rico'
			}
		else if ((strcmp(call_2,"CQ")==0) || (strcmp(call_2,"CR")==0) ||
			 (strcmp(call_2,"CS")==0) || (strcmp(call_2,"CT")==0) ||
			 (strcmp(call_2,"CU")==0) ){  
			::sprintf(flagNumber,"%s","188"); // 'Portugal'
			}
		else if ((strcmp(call_2,"T8")==0)){
			::sprintf(flagNumber,"%s","189"); // 'Palau'
			}
		else if ((strcmp(call_2,"ZP")==0)){
			::sprintf(flagNumber,"%s","190"); // 'Paraguay'
			}
		else if ((strcmp(call_2,"A7")==0)){
			::sprintf(flagNumber,"%s","191"); // 'Qatar'
			}
		else if ((strcmp(call_2,"6K")==0) || (strcmp(call_2,"6L")==0) || 
		  	 (strcmp(call_2,"6M")==0) || (strcmp(call_2,"6N")==0) ||
			 (strcmp(call_2,"D7")==0) || (strcmp(call_2,"D8")==0) ||
			 (strcmp(call_2,"D9")==0) ){
			::sprintf(flagNumber,"%s","126"); // 'Korea (Republic of)'
			}
		else if ((strcmp(call_2,"ER")==0)){
			::sprintf(flagNumber,"%s","143"); // 'Moldova
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","192"); // 'Reunion'
			}
		else if ((strcmp(call_2,"YO")==0) || (strcmp(call_2,"YP")==0) ||
			 (strcmp(call_2,"YQ")==0) || (strcmp(call_2,"YR")==0) ){
			::sprintf(flagNumber,"%s","193"); // 'Romania'
			}
		else if ((strcmp(call_2,"YT")==0) || (strcmp(call_2,"YU")==0) ){
			::sprintf(flagNumber,"%s","194"); // 'Serbia'
			}
		else if ((strcmp(call_1,"R")==0)  || (strcmp(call_2,"UA")==0) ||
			 (strcmp(call_2,"UA")==0) || (strcmp(call_2,"UB")==0) ||
			 (strcmp(call_2,"UC")==0) || (strcmp(call_2,"UD")==0) ||
			 (strcmp(call_2,"UE")==0) || (strcmp(call_2,"UF")==0) ||
			 (strcmp(call_2,"UG")==0) || (strcmp(call_2,"UH")==0) ||
			 (strcmp(call_2,"UI")==0) ){
			::sprintf(flagNumber,"%s","195"); // 'Russian Federation' 
			}
		else if ((strcmp(call_2,"9X")==0)){
			::sprintf(flagNumber,"%s","196"); // 'Rwanda'
			}
		else if ((strcmp(call_2,"7Z")==0)|| (strcmp(call_2,"8Z")==0) ){
			::sprintf(flagNumber,"%s","197"); // 'Saudi Arabia'
			}
		else if ((strcmp(call_2,"H4")==0)){
			::sprintf(flagNumber,"%s","198"); // 'Solomon Islands'
			}
		else if ((strcmp(call_2,"S7")==0)){
			::sprintf(flagNumber,"%s","199"); // 'Seychelles'
			}
		else if ((strcmp(call_2,"6T")==0) || (strcmp(call_2,"6U")==0)){
			::sprintf(flagNumber,"%s","200"); // 'Sudan'
			}
		else if ((strcmp(call_2,"SM")==0) || (strcmp(call_2,"7S")==0) ||
			 (strcmp(call_2,"8S")==0) ){
			::sprintf(flagNumber,"%s","201"); // 'Sweden'
			}
		else if ((strcmp(call_2,"9V")==0)){
			::sprintf(flagNumber,"%s","202"); // 'Singapore'
			}
		else if ((strcmp(call_2,"PJ")==0)){
			::sprintf(flagNumber,"%s","??"); // 'Bonaire
			}
		else if ((strcmp(call_2,"S5")==0)){
			::sprintf(flagNumber,"%s","205"); // 'Slovenia'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","205"); // 'Svalbard and Jan Mayen'
			}
		else if ((strcmp(call_2,"OM")==0)){
			::sprintf(flagNumber,"%s","206"); // 'Slovakia'
			}
		else if ((strcmp(call_2,"9L")==0)){
			::sprintf(flagNumber,"%s","207"); // 'Sierra Leone'
			}
		else if ((strcmp(call_2,"T7")==0)){
			::sprintf(flagNumber,"%s","208"); // 'San Marino'
			}
		else if ((strcmp(call_2,"6V")==0) || (strcmp(call_2,"6W")==0)){
			::sprintf(flagNumber,"%s","209"); // 'Senegal'
			}
		else if ((strcmp(call_2,"6O")==0) || (strcmp(call_2,"T5")==0) ){
			::sprintf(flagNumber,"%s","210"); // 'Somalia'
			}
		else if ((strcmp(call_2,"PZ")==0)){
			::sprintf(flagNumber,"%s","211"); // 'Suriname'
			}
		else if ((strcmp(call_2,"Z8")==0)){
			::sprintf(flagNumber,"%s","212"); // 'South Sudan'
			}
		else if ((strcmp(call_2,"S9")==0)){
			::sprintf(flagNumber,"%s","213"); // 'Sao Tome and Principe'
			}
		else if ((strcmp(call_2,"HU")==0) || (strcmp(call_2,"YS")==0) ){
			::sprintf(flagNumber,"%s","214"); // 'El Salvador'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","215"); // 'Sint Maarten (Dutch Part '
			}
		else if ((strcmp(call_2,"6C")==0)){
			::sprintf(flagNumber,"%s","216"); // 'Syrian Arab Republic'
			}
		else if ((strcmp(call_2,"3D")==0)){
			::sprintf(flagNumber,"%s","217"); // 'Swaziland'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","218"); // 'Turks and Caicos Islands'
			}
		else if ((strcmp(call_2,"TT")==0)){
			::sprintf(flagNumber,"%s","219"); // 'Chad'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","220"); // 'French Southern Territories'
			}
		else if ((strcmp(call_2,"5V")==0)){
			::sprintf(flagNumber,"%s","221"); // 'Togo'
			}
		else if ((strcmp(call_2,"9O")==0) || (strcmp(call_2,"9P")==0) ||
			 (strcmp(call_2,"9Q")==0) || (strcmp(call_2,"9R")==0) ||
			 (strcmp(call_2,"9S")==0) || (strcmp(call_2,"9T")==0) ){
			::sprintf(flagNumber,"%s","046"); // 'Congo (Democratic Republic of the)'	
			}
		else if ((strcmp(call_2,"Z3")==0)){
			::sprintf(flagNumber,"%s","148"); // 'Macedonia
			}
		else if ((strcmp(call_2,"E2")==0)){
			::sprintf(flagNumber,"%s","222"); // 'Thailand'
			}
		else if ((strcmp(call_2,"EY")==0)){
			::sprintf(flagNumber,"%s","223"); // 'Tajikistan'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","224"); // 'Tokelau'
			}
		else if ((strcmp(call_2,"4W")==0)){
			::sprintf(flagNumber,"%s","225"); // 'Timor-Leste'
			}
		else if ((strcmp(call_2,"EZ")==0)){
			::sprintf(flagNumber,"%s","226"); // 'Turkmenistan'
			}
		else if ((strcmp(call_2,"3V")==0)){
			::sprintf(flagNumber,"%s","227"); // 'Tunisia'
			}
		else if ((strcmp(call_2,"A3")==0)){
			::sprintf(flagNumber,"%s","228"); // 'Tonga'
			}
		else if ((strcmp(call_2,"TA")==0) || (strcmp(call_2,"TB")==0) || 
			 (strcmp(call_2,"TC")==0) || (strcmp(call_2,"YM")==0) ){
			::sprintf(flagNumber,"%s","229"); // 'Turkey'
			}
		else if ((strcmp(call_2,"9Y")==0) || (strcmp(call_2,"9Z")==0) ){
			::sprintf(flagNumber,"%s","230"); // 'Trinidad and Tobago'
			}
		else if ((strcmp(call_2,"T2")==0)){
			::sprintf(flagNumber,"%s","231"); // 'Tuvalu'
			}
		else if ((strcmp(call_2,"EM")==0) || (strcmp(call_2,"EN")==0) ||
			 (strcmp(call_2,"EO")==0) ){
			::sprintf(flagNumber,"%s","234"); // 'Ukraine'
			}
		else if ((strcmp(call_2,"5X")==0)){
			::sprintf(flagNumber,"%s","235"); // 'Uganda'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","236"); // 'United States Minor Outlying Islands'
			}
		else if ((strcmp(call_2,"5H")==0) || (strcmp(call_2,"5I")==0)){
			::sprintf(flagNumber,"%s","233"); // 'Tanzania
			}
		else if ((strcmp(call_1,"N")==0)  || (strcmp(call_1,"W")==0)  ||
			 (strcmp(call_2,"AA")==0) || (strcmp(call_2,"AB")==0) ||
			 (strcmp(call_2,"AC")==0) || (strcmp(call_2,"AD")==0) ||
			 (strcmp(call_2,"AF")==0) || (strcmp(call_2,"AG")==0) ||
			 (strcmp(call_2,"AH")==0) || (strcmp(call_2,"AI")==0) ||
			 (strcmp(call_2,"AJ")==0) || (strcmp(call_2,"AK")==0) ||
			 (strcmp(call_2,"AE")==0) || (strcmp(call_2,"AL")==0) ||
			 (strcmp(call_1,"K")==0) ){
			::sprintf(flagNumber,"%s","237"); // 'United States'
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","244"); // 'Virgin Islands
			}
		else if ((strcmp(call_2,"CV")==0) || (strcmp(call_2,"CW")==0) ||
			 (strcmp(call_2,"CX")==0) ){
			::sprintf(flagNumber,"%s","238"); // 'Uruguay'
			}
		else if ((strcmp(call_2,"UJ")==0) || (strcmp(call_2,"UK")==0) || 
			 (strcmp(call_2,"UL")==0) || (strcmp(call_2,"UM")==0) ){
			::sprintf(flagNumber,"%s","239"); // 'Uzbekistan'		
			}
		else if ((strcmp(call_2,"HV")==0)){
			::sprintf(flagNumber,"%s","240"); // 'Holy See (Vatican City State '
			}
		else if ((strcmp(call_2,"J8")==0)){
			::sprintf(flagNumber,"%s","241"); // 'Saint Vincent and The Grenadines'
			}
		else if ((strcmp(call_2,"3W")==0) || (strcmp(call_2,"XV")==0) ){
			::sprintf(flagNumber,"%s","245"); // 'Viet Nam'			
			}
		else if ((strcmp(call_2,"YJ")==0)){
			::sprintf(flagNumber,"%s","246"); // 'Vanuatu'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","247"); // 'Wallis and Futuna'	
			}
		else if ((strcmp(call_2,"5W")==0)){
			::sprintf(flagNumber,"%s","248"); // 'Samoa'			
			}
		else if ((strcmp(call_2,"7O")==0)){
			::sprintf(flagNumber,"%s","249"); // 'Yemen'			
			}
		else if ((strcmp(call_2,"--")==0)){
			::sprintf(flagNumber,"%s","250"); // 'Mayotte'
			}
		else if ((strcmp(call_2,"S8")==0) || (strcmp(call_2,"ZR")==0) ||
			 (strcmp(call_2,"ZS")==0) || (strcmp(call_2,"ZT")==0) ||
			 (strcmp(call_2,"ZU")==0) ){
			::sprintf(flagNumber,"%s","251"); // 'South Africa'		
			}
		else if ((strcmp(call_2,"9I")==0) || (strcmp(call_2,"9J")==0) ){
			::sprintf(flagNumber,"%s","252"); // 'Zambia'			
			}
		else if ((strcmp(call_2,"Z2")==0)){
			::sprintf(flagNumber,"%s","253"); // 'Zimbabwe'			
			}
	else
			{
			::sprintf(flagNumber,"%s","254");
                        }
        return(flagNumber);
}



