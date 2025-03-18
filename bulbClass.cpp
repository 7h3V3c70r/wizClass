#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iostream>
#include "bulbClass.h"
#include "json.hpp"

using json = nlohmann::json;

//Start socket and update local pilot
bulb::bulb(const char* ipAddr) {
	ip = ipAddr;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int timeout = 500;
	setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout, sizeof(timeout));

	updatePilot();
}

//Closing and cleaning stuff
bulb::~bulb() {
	closesocket(sock);
	WSACleanup();
}

//Update local pilot (light values in the app) to sync the app with the light
void bulb::updatePilot() {
	std::string data = sendInstruction((char*)"getPilot", (char*)"");
	if (data.empty()) {
		return;
	}
	json jsonData = json::parse(data);

	if (jsonData["result"]["state"] == true) {
		state = true;
	}
	else if (jsonData["result"]["state"] == false) {
		state = false;
	}

	//checks if it is not white to keep the colors null
	if (jsonData["result"]["temp"] < 0) {
		color[0] = jsonData["result"]["r"].get<int>();
		color[1] = jsonData["result"]["g"].get<int>();
		color[2] = jsonData["result"]["b"].get<int>();
		color[3] = jsonData["result"]["dimming"].get<int>();
	}
}

//Send a udp instruction to the light with the method and params as input. If response is valid, return it. If not it will return nothing
std::string bulb::sendInstruction(char* method, char* params) {

	char data[256];
	sprintf_s(data, sizeof(data), "{\"method\": \"%s\",\"params\": {%s}}", method, params);


	sockaddr_in target;

	target.sin_family = AF_INET;
	target.sin_port = htons(38899);
	InetPton(AF_INET, (PCSTR)ip, &target.sin_addr.s_addr);

	char receivedData[256];

	sendto(sock, data, strlen(data), 0, (sockaddr*)&target, sizeof(target));

	int receivedBytes = recvfrom(sock,receivedData,sizeof(receivedData),0,0,0);
	if(receivedBytes != SOCKET_ERROR){
		receivedData[receivedBytes] = '\0';
		return std::string(receivedData);
	}
	else {
		return "";
	}
}

//Turn on or off the light, depending on its current state
void bulb::toggleState() {
	char setState[] = "getPilot";
	char params[] = "";

	std::string data = sendInstruction(setState, params);

	json jsonData = json::parse(data);

	if (jsonData["result"]["state"] == true) {
		sendInstruction((char*)"setState", (char*)"\"state\": false");
	}
	else if(jsonData["result"]["state"] == false) {
		sendInstruction((char*)"setState", (char*)"\"state\": true");
	}
}

//Change the light color by passing red, green and blue as parameters
void bulb::changeColor(int r, int g, int b) {
	if (r == 0 && g == 0 && b == 0) {
		sendInstruction((char*)"setState", (char*)"\"state\": false");
	}
	else {
		char dataToSend[50];
		sprintf_s(dataToSend, sizeof(dataToSend), "\"r\":%u,\"g\":%u,\"b\":%u,\"dimming\":100", r, g, b);
		std::string recivedData = sendInstruction((char*)"setState", dataToSend);
		if (recivedData != "") {
			json jsonData = json::parse(recivedData);
			if (jsonData["result"]["success"] == true) {
				color[0] = r;
				color[1] = g;
				color[2] = b;
			}
		}
	}
}

//make the light pulse for 100ms
void bulb::pulse() {
	sendInstruction((char*)"setState", (char*)"\"state\":true,\"dimming\":100,\"r\":255,\"g\":255,\"b\":255");

	Sleep(1);

	char params[50];
	sprintf_s(params, sizeof(params), "\"state\":true,\"dimming\":50,\"r\":%u,\"g\":%u,\"b\":%u", color[0], color[1], color[2]);
	sendInstruction((char*)"setState", params);
}