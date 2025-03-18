#pragma once

class bulb {
private:
	
public:
	const char* ip;
	SOCKET sock;
	bool state;
	int color[3];


	bulb(const char* ipAddr);
	~bulb();
	void updatePilot();
	std::string sendInstruction(char* method, char* params);
	void toggleState();
	void changeColor(int r, int g, int b);
	void pulse();
};