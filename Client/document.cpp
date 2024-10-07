#include "document.h"

Document::Document() {
	data.reserve(1024);
}

COORD Document::getCursorPos() const {
	return cursorPos;
}

std::string Document::getLine(const int lineIndex) const {
	if (lineIndex >= data.size()) {
		return "";
	}
	return data[lineIndex];
}

COORD Document::write(const char letter) {
	if (cursorPos.Y > data.size()) {
		return cursorPos;
	}

	if (cursorPos.X >= data[cursorPos.Y].size()) {
		data[cursorPos.Y] += letter;
		cursorPos.X += 1;
		if (letter == '\n') {
			data.insert(data.begin() + cursorPos.Y + 1, "");
			cursorPos.Y += 1;
			cursorPos.X = 0;
		}
	}
	else {
		data[cursorPos.Y].insert(data[cursorPos.Y].begin() + cursorPos.X, letter);
		cursorPos.X += 1;
		if (letter == '\n') {
			std::string toMoveBelow = data[cursorPos.Y].substr(cursorPos.X, data[cursorPos.Y].size() - cursorPos.X);
			data[cursorPos.Y].erase(cursorPos.X, data[cursorPos.Y].size() - cursorPos.X);
			data.insert(data.begin() + cursorPos.Y + 1, toMoveBelow);
			cursorPos.Y += 1;
			cursorPos.X = 0;
		}
	}
	
	return cursorPos;
}

COORD Document::erase() {
	if (cursorPos.Y > data.size()) {
		return cursorPos;
	}

	if (cursorPos.X > 0) {
		data[cursorPos.Y].erase(data[cursorPos.Y].begin() + cursorPos.X - 1, data[cursorPos.Y].begin() + cursorPos.X);
		cursorPos.X -= 1;
	}
	else {
		if (cursorPos.Y <= 0) {
			return cursorPos;
		}
		std::string toMoveUpper = data[cursorPos.Y];
		data[cursorPos.Y - 1].erase(data[cursorPos.Y - 1].end() - 1, data[cursorPos.Y - 1].end() - 0);
		data.erase(data.begin() + cursorPos.Y, data.begin() + cursorPos.Y + 1);
		cursorPos.Y -= 1;
		cursorPos.X = data[cursorPos.Y].size();
		data[cursorPos.Y] += toMoveUpper;
	}
	return cursorPos;
}
