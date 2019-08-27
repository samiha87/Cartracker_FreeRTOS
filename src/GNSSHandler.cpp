/*
 * GNSSHandler.cpp
 *
 *  Created on: Aug 8, 2019
 *      Author: samih
 */

#include "GNSSHandler.h"
#include "USART.h"
#include "ArrayManagement.h"

USART gnss(USART::eUSART3, 9600);
USART debug_gnss(USART::eUSART1, 9600);
// TODO minimize these to save space

// GNSS Message start with ASCII $ and ends to carriage return \r
const char START_BYTE = '$';	// Start of string
const char END_BYTE = '\r';		// End of string

GNSSHandler::GNSSHandler() {
	// TODO Auto-generated constructor stub
	localBufPointer = 0;
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
	// When localbuffer is full parse messages
}

void GNSSHandler::parseMessage(void) {

	ArrayManagement ar;
	// $GPGSV,NoMsg,MsgNo,NoSv,{,sv,elv,az,cno}*cs<CR><LF>
	int startByte = 0;
	startByte = ar.containsCharAdv(localBuffer, "$GPGSV", GNSS_BUFFER_SIZE, 5);
	if(startByte != -1 ) {
		debug_gnss.send("Found GPGSV\n", 12);
		//char convert[10];
		//ar.integerToArray(startByte, convert, 10);
		//debug_gnss.send(convert, 2);
		// Find end byte
		char buffer[50];
		int size = ar.copyFromUntilFind(localBuffer, buffer, startByte, GNSS_BUFFER_SIZE, 50,  '*');
		if(size == -1) return;

		// Find count of commas. If comma count is less than 5 then it's not valid data
		if(ar.countChars(buffer, ',', size) >= 6) {
			debug_gnss.send(buffer, size);
			debug_gnss.send("\n", 1);
			int outputSize = -1;
			// Get current time in UTC
			for (int i = 0; i < 3; i++) {
				satellites[i] = 0;	// Clear time
			}
			// Buffer to search from, output buffer, sperator character, which string to output
			// Returns size of found string
			outputSize = ar.split(buffer, satellites, ',', 3);
			if(outputSize > 2 || outputSize < 1) {
				// No satellites found
				debug_gnss.send("No satellites found\n", 20);
			} else {
				// Debug output
				debug_gnss.send("Satellites: ", 12);
				debug_gnss.send(satellites, outputSize);
				debug_gnss.send("\n",1);
			}
		}
	}
	// Detect which type of message is incoming
	// $GPGGA,hhmmss.ss,Latitude,N,Longitude,E,FS,NoSV,HDOP,msl,m,Altref,m,DiffAge,DiffStation*cs<CR><LF>
	// $GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
	// Compare current buffer to GPGGA if GPGGA is found and all 5 characters matches in row process the message
	startByte = ar.containsCharAdv(localBuffer, "$GPGGA",GNSS_BUFFER_SIZE, 5);
	if(startByte) {
		int outputSize = -1;
		debug_gnss.send("Found GPGGA\n", 12);
		// Get current time in UTC
		for (int i = 0; i < 15; i++) {
			currentTime[i] = 0;	// Clear time
		}
		// Buffer to search from, output buffer, sperator character, which string to output
		// Returns size of found string
		outputSize = ar.split(localBuffer, currentTime, ',', 1);
		// Debug output
		debug_gnss.send(currentTime, outputSize);
	}
	startByte = ar.containsCharAdv(localBuffer, "$GPRMC", GNSS_BUFFER_SIZE, 5);
	if(startByte != -1) {
		int outputSize = -1;
		debug_gnss.send("Found GPRMC\n", 12);
		// Get current time in UTC
		for (int i = 0; i < 15; i++) {
			currentTime[i] = 0;	// Clear time
		}
		// Buffer to search from, output buffer, sperator character, which string to output
		// Returns size of found string
		outputSize = ar.split(localBuffer, currentTime, ',', 1);
		// Debug output
		debug_gnss.send(currentTime, outputSize);
		}
}

