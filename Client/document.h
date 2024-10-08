#pragma once
#include <vector>
#include <string>
#include "windows.h"

class Document {
public:
	Document();
	COORD write(const char letter);
	COORD erase();
	COORD moveCursorLeft();
	COORD moveCursorRight();
	COORD moveCursorUp(COORD& terminalSize);
	COORD moveCursorDown(COORD& terminalSize);

	COORD getCursorPos() const;
	std::string getLine(const int lineIndex) const;
	std::vector<std::string> get() const;

private:
	std::vector<std::string> data{""};
	COORD cursorPos{0, 0};
	int offset = 0;
};
