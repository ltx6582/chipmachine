#include "log.h"
#include "TextScreen.h"
#include "utils.h"

using namespace std;

/** PETSCII->ASCII 
0x20,0x0020
0x21,0x0021
0x22,0x0022
0x23,0x0023
0x24,0x0024
0x25,0x0025
0x26,0x0026
0x27,0x0027
0x28,0x0028
0x29,0x0029
0x2A,0x002A
0x2B,0x002B
0x2C,0x002C
0x2D,0x002D
0x2E,0x002E
0x2F,0x002F
0x30,0x0030
0x31,0x0031
0x32,0x0032
0x33,0x0033
0x34,0x0034
0x35,0x0035
0x36,0x0036
0x37,0x0037
0x38,0x0038
0x39,0x0039
0x3A,0x003A
0x3B,0x003B
0x3C,0x003C
0x3D,0x003D
0x3E,0x003E
0x3F,0x003F
0x40,0x0040
0x41,0x0061
0x42,0x0062
0x43,0x0063
0x44,0x0064
0x45,0x0065
0x46,0x0066
0x47,0x0067
0x48,0x0068
0x49,0x0069
0x4A,0x006A
0x4B,0x006B
0x4C,0x006C
0x4D,0x006D
0x4E,0x006E
0x4F,0x006F
0x50,0x0070
0x51,0x0071
0x52,0x0072
0x53,0x0073
0x54,0x0074
0x55,0x0075
0x56,0x0076
0x57,0x0077
0x58,0x0078
0x59,0x0079
0x5A,0x007A
0x5B,0x005B
0x5C,0x00A3
0x5D,0x005D
0x5E,0x2191
0x5F,0x2190
0x60,0x2501
0x61,0x0041
0x62,0x0042
0x63,0x0043
0x64,0x0044
0x65,0x0045
0x66,0x0046
0x67,0x0047
0x68,0x0048
0x69,0x0049
0x6A,0x004A
0x6B,0x004B
0x6C,0x004C
0x6D,0x004D
0x6E,0x004E
0x6F,0x004F
0x70,0x0050
0x71,0x0051
0x72,0x0052
0x73,0x0053
0x74,0x0054
0x75,0x0055
0x76,0x0056
0x77,0x0057
0x78,0x0058
0x79,0x0059
0x7A,0x005A
0x7B,0x253C
0x7C,0xF12E
0x7D,0x2502
0x7E,0x2592
0x7F,0xF139

0x8D,0x000A
0xA0,0x00A0
0xA1,0x258C
0xA2,0x2584
0xA3,0x2594
0xA4,0x2581
0xA5,0x258F
0xA6,0x2592
0xA7,0x2595
0xA8,0xF12F
0xA9,0xF13A
0xAA,0xF130
0xAB,0x251C
0xAC,0xF134
0xAD,0x2514
0xAE,0x2510
0xAF,0x2582
0xB0,0x250C
0xB1,0x2534
0xB2,0x252C
0xB3,0x2524
0xB4,0x258E
0xB5,0x258D
0xB6,0xF131
0xB7,0xF132
0xB8,0xF133
0xB9,0x2583
0xBA,0x2713
0xBB,0xF135
0xBC,0xF136
0xBD,0x2518
0xBE,0xF137
0xBF,0xF138
0xC0,0x2501
0xC1,0x0041
0xC2,0x0042
0xC3,0x0043
0xC4,0x0044
0xC5,0x0045
0xC6,0x0046
0xC7,0x0047
0xC8,0x0048
0xC9,0x0049
0xCA,0x004A
0xCB,0x004B
0xCC,0x004C
0xCD,0x004D
0xCE,0x004E
0xCF,0x004F
0xD0,0x0050
0xD1,0x0051
0xD2,0x0052
0xD3,0x0053
0xD4,0x0054
0xD5,0x0055
0xD6,0x0056
0xD7,0x0057
0xD8,0x0058
0xD9,0x0059
0xDA,0x005A
0xDB,0x253C
0xDC,0xF12E
0xDD,0x2502
0xDE,0x2592
0xDF,0xF139
0xE0,0x00A0
0xE1,0x258C
0xE2,0x2584
0xE3,0x2594
0xE4,0x2581
0xE5,0x258F
0xE6,0x2592
0xE7,0x2595
0xE8,0xF12F
0xE9,0xF13A
0xEA,0xF130
0xEB,0x251C
0xEC,0xF134
0xED,0x2514
0xEE,0x2510
0xEF,0x2582
0xF0,0x250C
0xF1,0x2534
0xF2,0x252C
0xF3,0x2524
0xF4,0x258E
0xF5,0x258D
0xF6,0xF131
0xF7,0xF132
0xF8,0xF133
0xF9,0x2583
0xFA,0x2713
0xFB,0xF135
0xFC,0xF136
0xFD,0x2518
0xFE,0xF137
0xFF,0x2592
*/

DummyTerminal dummyTerminal;

void Screen::clear() {
	for(auto &t : grid) {
		t.c = 0x20;
		t.fg = t.bg = -1;
	}
}

void Screen::put(int x, int y, const string &text) {
	for(unsigned int i=0; i<text.length(); i++) {
		Tile &t = grid[(x+i) + y * width];
		t.c = text[i];
		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
	}
}

void Screen::resize(int w, int h) {
	width = w;
	height = h;

	grid.resize(w*h);
	oldGrid.resize(w*h);

	clear();
}

void Screen::flush() {
	vector<int8_t> temp;
	int rc = update(temp);
	if(rc > 0)
		terminal.write(temp, rc);
}


// ANSISCREEN

int AnsiScreen::update(std::vector<int8_t> &dest) {

	int w = terminal.getWidth();
	int h = terminal.getHeight();
	//LOGD("Size %dx%d", w, h);
	if(w > 0 && h > 0 && (w != width || h != height)) {
		resize(w, h);
		outBuffer = { '\x1b', '[', '2', 'J' };
		LOGD("Size now %dx%d", w, h);
	}

	int orgSize = dest.size();

	if(outBuffer.size() > 0) {
		dest.insert(dest.end(), outBuffer.begin(), outBuffer.end());
		outBuffer.resize(0);
	}

	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			Tile &t1 = grid[x+y*width];
			if(t0 != t1) {					
				if(curY != y or curX != x)
					smartGoto(dest, x, y);
				if(t0.fg != t1.fg || t0.bg != t1.bg)
					setColor(dest, t1.fg, t1.bg);
				putChar(dest, t1.c);
				t0 = t1;
			}
		}
	}

	return dest.size() - orgSize;

}

void AnsiScreen::setColor(vector<int8_t> &dest, int fg, int bg) {
	const string &s = utils::format("\x1b[%dm", fg + 30);
	dest.insert(dest.end(), s.begin(), s.end());			
};

void AnsiScreen::putChar(vector<int8_t> &dest, char c) {
	dest.push_back(c);
	curX++;
	if(curX > width) {
		curX -= width;
		curY++;
	}
}

void AnsiScreen::smartGoto(vector<int8_t> &dest, int x, int y) {
	// Not so smart for now
	const string &s = utils::format("\x1b[%d;%dH", y+1, x+1);
	dest.insert(dest.end(), s.begin(), s.end());			
	curX = x;
	curY = y;
}

// PETSCII SCREEN

int PetsciiScreen::update(std::vector<int8_t> &dest) {
	int orgSize = dest.size();

	if(outBuffer.size() > 0) {
		dest.insert(dest.end(), outBuffer.begin(), outBuffer.end());
		outBuffer.resize(0);
	}

	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			Tile &t1 = grid[x+y*width];
			if(t0 != t1) {					
				if(curY != y or curX != x)
					smartGoto(dest, x, y);
				//if(t0.fg != t1.fg || t0.bg != t1.bg)
				//	setColor(dest, t1.fg, t1.bg);
				putChar(dest, t1.c);
				t0 = t1;
			}
		}
	}

	return dest.size() - orgSize;
}

void PetsciiScreen::putChar(vector<int8_t> &dest, char c) {
	dest.push_back(c);
	curX++;
	if(curX > width) {
		curX -= width;
		curY++;
	}
}

void PetsciiScreen::smartGoto(vector<int8_t> &dest, int x, int y) {
	// Not so smart for now
	//const string &s = utils::format("\x1b[%d;%dH", y+1, x+1);
	//dest.insert(dest.end(), s.begin(), s.end());
	if(x < curX || y < curY) {
		dest.push_back(19);
		curX = curY = 0;
	}
	while(y > curY) {
		dest.push_back(17);
		curY++;
	}
	while(x > curX) {
		dest.push_back(29);
		curX++;
	}

	curX = x;
	curY = y;
}

void PetsciiScreen::put(int x, int y, const std::string &text) {
	for(unsigned int i=0; i<text.length(); i++) {
		Tile &t = grid[(x+i) + y * width];
		t.c = text[i];
		if(t.c >= 'A' && t.c <= 'Z')
			t.c = t.c - 'A' + 33;
		else
		if(t.c >= 'a' && t.c <= 'z')
			t.c = t.c - 'a' + 1;
		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
	}
}

// ANSI INPUT

int AnsiInput::getKey(int timeout) {

	std::chrono::milliseconds ms { 100 };

	while(true) {
		int rc = terminal.read(temp, temp.size());
		if(rc > 0) {
			LOGD("Got %d bytes", rc);
			for(int i=0; i<rc; i++)
				buffer.push(temp[i]);
			temp.resize(0);
		}
		if(buffer.size() > 0) {
			int8_t &c = buffer.front();
			if(c != 0x1b) {
				buffer.pop();
				return c;
			} else if(c == 0x1b && buffer.size() > 2) {
				buffer.pop();
				int8_t c2 = buffer.front();
				buffer.pop();
				int8_t c3 = buffer.front();
				buffer.pop();

				if(c2 == 0x5b) {
					switch(c3) {
					case 0x44:
						return KEY_LEFT;
					case 0x43:
						return KEY_RIGHT;
					case 0x41:
						return KEY_UP;
					case 0x42:
						return KEY_DOWN;
					}
				}

			}
		}
		std::this_thread::sleep_for(ms);
		if(timeout >= 0) {
			timeout -= 100;
			if(timeout < 0)
				return KEY_TIMEOUT;
		}
	}
}