// color_pulse.cpp : Defines the entry point for the console application.
//

#ifdef __APPLE__
#include <CUESDK/CUESDK.h>
#else
#include <CUESDK.h>
#endif

#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <cmath>
#include <random>
#include <time.h>

const char* toString(CorsairError error) 
{
	switch (error) {
	case CE_Success :
		return "CE_Success";
	case CE_ServerNotFound:
		return "CE_ServerNotFound";
	case CE_NoControl:
		return "CE_NoControl";
	case CE_ProtocolHandshakeMissing:
		return "CE_ProtocolHandshakeMissing";
	case CE_IncompatibleProtocol:
		return "CE_IncompatibleProtocol";
	case CE_InvalidArguments:
		return "CE_InvalidArguments";
	default:
		return "unknown error";
	}
}

struct Pos {
	int x;
	int y;

	operator std::pair<int, int>() {
		return std::make_pair(x, y);
	}
};

struct Ball {

	int radius;
	Pos pos;
	int velX, velY;
	int boundX, boundY;
	CorsairLedColor ballColor;
	const CorsairLedColor possibleColors[7] = {	{ CLI_Invalid,255,0,0 },
												{ CLI_Invalid,0,255,0 },
												{ CLI_Invalid,0,0,255 },
												{ CLI_Invalid,255,255,0 },
												{ CLI_Invalid,255,0,255 },
												{ CLI_Invalid,0,255,255 },
												{ CLI_Invalid,255,255,255 } };

	std::default_random_engine engine;
	std::uniform_int_distribution<int> dist;

	std::uniform_int_distribution<int> colorPick;

	Ball() {
		engine = std::default_random_engine(time(NULL));
		dist = std::uniform_int_distribution<int>(1,3);
		colorPick = std::uniform_int_distribution<int>(0, 6);
		ballColor = possibleColors[colorPick(engine)];
	}

	//return new coordinates of Ball
	std::pair<int, int> update() {
		pos.x += velX;
		pos.y += velY;

		if (pos.x < 0) {
			pos.x *= -1;
			velX *= -1;
			velY = dist(engine) * abs(velY) / velY;
			ballColor = possibleColors[colorPick(engine)];
		}

		if (pos.x > boundX) {
			pos.x = 2 * boundX - pos.x;
			velX *= -1;
			velY = dist(engine) * abs(velY) / velY;
			ballColor = possibleColors[colorPick(engine)];
		}

		if (pos.y < 0) {
			pos.y *= -1;
			velY *= -1;
			velX = dist(engine) * abs(velX) / velX;
			ballColor = possibleColors[colorPick(engine)];
		}

		if (pos.y > boundY) {
			pos.y = 2 * boundY - pos.y;
			velY *= -1;
			velX = dist(engine) * abs(velX) / velX;
			ballColor = possibleColors[colorPick(engine)];
		}

		return std::make_pair(pos.x, pos.y);
	}

	operator std::pair<int, int>() {
		return pos;
	}
};



//Return vector of keyboards
std::vector<int> findKeyboards() {
	//container to hold keyboards
	std::vector<int> keyboards;

	//iterate through number of devices, looking for keyboard
	for (int i = 0; i < CorsairGetDeviceCount(); i++) {
		CorsairDeviceInfo device = *CorsairGetDeviceInfo(i);

		if (device.type == CDT_Keyboard) {
			keyboards.push_back(i);
		}
	}

	return keyboards;
}



struct CorsairKey {
	CorsairLedColor color;
	Pos pos;

	CorsairKey(CorsairLedColor inColor, CorsairLedPosition inPos, CorsairLedId inID) {
		color = inColor;
		pos.x = inPos.left + inPos.width / 2;
		pos.y = inPos.top + inPos.height / 2;
		color.ledId = inID;
	}

	CorsairKey(const CorsairKey & rhs) {
		color = rhs.color;
		pos = rhs.pos;
	}

	CorsairKey & operator=(const CorsairLedId & rhs) {
		color.ledId = rhs;

		return *this;
	}

	CorsairKey & operator=(const CorsairLedColor & rhs) {
		color.r = rhs.r;
		color.b = rhs.b;
		color.g = rhs.g;

		return *this;
	}

	CorsairKey & operator=(const CorsairLedPosition & rhs) {
		pos.x = rhs.left + rhs.width / 2;
		pos.y = rhs.top + rhs.height / 2;

		return *this;
	}
};


//get keyboard bounds
std::pair<int, int> getBounds(CorsairLedPositions * keypositions) {

	int greatestX, greatestY;

	greatestX = greatestY = 0;

	for (int i = 0; i < keypositions->numberOfLed; i++) {

		if (keypositions->pLedPosition[i].top > greatestY) {
			greatestY = keypositions->pLedPosition[i].top;
		}

		if (keypositions->pLedPosition[i].left > greatestX) {
			greatestX = keypositions->pLedPosition[i].left;
		}
	}


	return std::make_pair(greatestX, greatestY);
}

//draw box on keyboard at specified location with specified color
bool drawBox(std::vector<CorsairKey> * keys, int top, int left, int width, int height, CorsairLedColor color) {

	for (int i = 0; i < keys->size(); i++) {
		if ((*keys)[i].pos.y > top && (*keys)[i].pos.x > left && (*keys)[i].pos.y < top + height && (*keys)[i].pos.x < left + width) {
			(*keys)[i] = color;
		}
	}

	return true;
}

double distance(Pos a, Pos b) {
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

bool drawBall(std::vector<CorsairKey> * keys, const Ball * b) {
	Pos pos;
	double distan;
	for (int i = 0; i < keys->size(); i++) {
		pos = keys->data()[i].pos;
		if (distance(b->pos, pos) <= b->radius) {
			distan = 1.0 - 0.5 * (distance(b->pos, pos) / double(b->radius));
			
			keys->data()[i].color = { keys->data()[i].color.ledId, int(double(b->ballColor.r) * distan), int(double(b->ballColor.g) * distan) ,int(double(b->ballColor.b) * distan) };
		}
		
	}

	return true;
}

bool clearKeys(std::vector<CorsairKey> * keys) {
	CorsairLedColor tmp{ CLI_Invalid,0,0,0 };

	for (int i = 0; i < keys->size(); i++) {
		(*keys)[i] = tmp;
	}

	return true;
}

bool renderKeys(std::vector<CorsairKey> * keys) {
	for (int i = 0; i < keys->size(); i++) {
		CorsairSetLedsColors(1, &((*keys)[i].color));
	}

	return true;
}

int ballThread(std::atomic_bool * loo)
{
	std::default_random_engine engine(time(NULL));
	std::uniform_int_distribution<int> dist(-3, 3);

	std::uniform_int_distribution<int> colorPick(0, 6);

	//sizes in mm
	const int paddleLen = 15;
	const int paddleWid = 5;
	const int paddlePad = 5;

	const int ballRad = 40;

	//create vector of keyboards
	std::vector<int> keyboards = findKeyboards();

	//get available keys
	CorsairLedPositions * keys = CorsairGetLedPositionsByDeviceIndex(keyboards[0]);

	//create key colors array
	std::vector<CorsairKey> * keyColors = new std::vector<CorsairKey>();
	
	CorsairLedColor tempColor{ CLI_Invalid,0,0,0 };

	for (int i = 0; i < keys->numberOfLed; i++) {
		keyColors->push_back({ tempColor,keys->pLedPosition[i],keys->pLedPosition[i].ledId });
	}

	//get bounding box bounds
	std::pair<int, int> bounds = getBounds(keys);
	
	//set up ball
	Ball b;
	b.pos.x = bounds.first / 2;
	b.pos.y = bounds.second / 2;
	b.radius = ballRad;
	b.velX = dist(engine);
	b.velY = dist(engine);
	b.boundX = bounds.first;
	b.boundY = bounds.second;

	


	//draw ball
	while (loo->load()) {
		drawBall(keyColors, &b);
		b.update();

		renderKeys(keyColors);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		clearKeys(keyColors);
	}

	delete keyColors;

	return 0;
}

void main() {
	//Interface with API
	CorsairPerformProtocolHandshake();

	//request Exclusive Control of Keyboard
	CorsairRequestControl(CAM_ExclusiveLightingControl);

	std::atomic_bool loo(true);

	std::thread thr(ballThread, &loo);

	system("PAuse");

	loo.store(false);

	thr.join();

	CorsairReleaseControl(CAM_ExclusiveLightingControl);
}