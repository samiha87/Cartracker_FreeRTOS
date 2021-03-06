/*
 * GNSSHandler.cpp
 *
 *  Created on: Aug 8, 2019
 *      Author: samih
 */

#include <Handlers/GNSSHandler.h>
#include <Hardware/USART.h>
#include <Parsers/ArrayManagement.h>

USART gnss(USART::eUSART3, 9600);
USART debug_gnss(USART::eUSART1, 9600);
// TODO minimize these to save space

// GNSS Message start with ASCII $ and ends to carriage return \r
const char START_BYTE = '$';	// Start of string
const char END_BYTE = '\r';		// End of string

GNSSHandler::GNSSHandler() {
	// TODO Auto-generated constructor stub
	localBufPointer = 0;
	lockedSatellites = 0;
}

GNSSHandler::~GNSSHandler() {
	// TODO Auto-generated destructor stub
}

void GNSSHandler::configure() {
	gnss.enable();
	gnss.enableIRQ();
}

void GNSSHandler::read() {

	if(gnss.available()) {
		int size = gnss.getCount();
		char buffer[size];
		gnss.getBuffer(buffer, size);
		//debug_gnss.send(buffer, size);
		// Store buffer to to local buffer
		for(int i = 0; i < size; i++) {
			localBuffer[localBufPointer++] = buffer[i];
			if(localBufPointer >= GNSS_BUFFER_SIZE) {
				parseMessage();
				localBufPointer = 0;
				// No need to clear
			}
		}
	}
}

void GNSSHandler::parseMessage(void) {
	parseGPGSV();
	// If satellite count is less than 3, no point parsing rest
	if(lockedSatellites < 3) return;
	parseGPGGA();
	parseGPRMC();
}

// $GPGSV,NoMsg,MsgNo,NoSv,{,sv,elv,az,cno}*cs<CR><LF>
void GNSSHandler::parseGPGSV()
{
	ArrayManagement arrayManagement;
	int startByte = 0;
	startByte = arrayManagement.containsCharAdv(localBuffer, "$GPGSV", GNSS_BUFFER_SIZE, 5);
	if(startByte != -1 ) {
		char buffer[50];
		int size = arrayManagement.copyFromUntilFind(localBuffer, buffer, startByte, GNSS_BUFFER_SIZE, 50,  '*');
		if(size == -1) return;
		// Find count of commas. If comma count is less than 5 then it's not valid data
		if(arrayManagement.countChars(buffer, ',', size) >= 6) {
			debug_gnss.send(buffer, size);
			debug_gnss.send("\n", 1);
			int outputSize = -1;
			for (int i = 0; i < 3; i++) satellites[i] = 0;
			outputSize = arrayManagement.split(buffer, satellites, ',', 3);
			if(outputSize > 2 || outputSize < 1) debug_gnss.send("No satellites found\n", 20);
			else {
				// Debug output
				debug_gnss.send("Satellites: ", 12);
				debug_gnss.send(satellites, outputSize);
				debug_gnss.send("\n",1);
				lockedSatellites = arrayManagement.toInteger(satellites, 3); // TODO fix crash
			}
		}
	}
}
// $GPGGA,hhmmss.ss,Latitude,N,Longitude,E,FS,NoSV,HDOP,msl,m,Altref,m,DiffAge,DiffStation*cs<CR><LF>
// $GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
// Compare current buffer to GPGGA if GPGGA is found and all 5 characters matches in row process the message
void GNSSHandler::parseGPGGA()
{
	ArrayManagement ar;
	int startByte = ar.containsCharAdv(localBuffer, "$GPGGA",GNSS_BUFFER_SIZE, 5);
	if(startByte != -1 ) {
		char buffer[50];
		int size = ar.copyFromUntilFind(localBuffer, buffer, startByte, GNSS_BUFFER_SIZE, 50,  '*');
		if(size == -1) return;
		// Find count of commas. If comma count is less than 5 then it's not valid data
		if(ar.countChars(buffer, ',', size) >= 6) {
			debug_gnss.send(buffer, size);
			debug_gnss.send("\n", 1);
		}
	}
}

void GNSSHandler::parseGPRMC() {
	ArrayManagement ar;
	int startByte = ar.containsCharAdv(localBuffer, "$GPRMC", GNSS_BUFFER_SIZE, 5);
	if(startByte != -1 ) {
		char buffer[50];
		int size = ar.copyFromUntilFind(localBuffer, buffer, startByte, GNSS_BUFFER_SIZE, 50,  '*');
		if(size == -1) return;
		// Find count of commas. If comma count is less than 5 then it's not valid data
		if(ar.countChars(buffer, ',', size) >= 6) {
			debug_gnss.send(buffer, size);
			debug_gnss.send("\n", 1);
		}
	}
}
