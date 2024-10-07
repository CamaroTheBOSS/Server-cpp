#pragma once
#include <vector>
#include <string>
#include "windows.h"

class Document {
public:
	Document();
	COORD write(const char letter);
	COORD erase();
	COORD getCursorPos() const;
	std::string getLine(const int lineIndex) const;

private:
	std::vector<std::string> data{""};
	COORD cursorPos{0, 0};
};
