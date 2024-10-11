#define NOMINMAX

#include "pch.h"
#include "document.h"
#include <algorithm>

Document::Document() {
	data.reserve(1024);
}

bool Document::setCursorPos(COORD newPos) {
	if (data.size() <= newPos.Y || data[newPos.Y].size() < newPos.X) {
		return false;
	}
	cursorPos = newPos;
	offset = newPos.X;
	return true;
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

const std::vector<std::string>& Document::get() {
	return data;
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
	offset = cursorPos.X;
	return cursorPos;
}

COORD Document::write(const std::string& text) {
	for (const auto letter : text) {
		write(letter);
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
	offset = cursorPos.X;
	return cursorPos;
}

COORD Document::moveCursorLeft() {
	if (cursorPos.X == 0 && cursorPos.Y == 0) {
		return cursorPos;
	}
	if (cursorPos.X > 0) {
		cursorPos.X--;
		offset = cursorPos.X;
		return cursorPos;
	}
	cursorPos.Y--;
	cursorPos.X = data[cursorPos.Y].size() - 1;
	offset = cursorPos.X;
	return cursorPos;
}

COORD Document::moveCursorRight() {
	if (cursorPos.Y == data.size() - 1 && cursorPos.X == data[cursorPos.Y].size()) {
		return cursorPos;
	}
	bool endlPresent = !data[cursorPos.Y].empty() && data[cursorPos.Y][data[cursorPos.Y].size() - 1] == '\n';
	if (cursorPos.X < data[cursorPos.Y].size() - endlPresent) {
		cursorPos.X++;
		offset = cursorPos.X;
		return cursorPos;
	}
	cursorPos.Y++;
	cursorPos.X = 0;
	offset = cursorPos.X;
	return cursorPos;
}

COORD Document::moveCursorUp(COORD& terminalSize) {
	offset = offset % terminalSize.X;
	if (cursorPos.X >= terminalSize.X) {
		cursorPos.X = (cursorPos.X / terminalSize.X - 1) * terminalSize.X + offset;
		return cursorPos;
	}
	if (cursorPos.Y == 0) {
		return cursorPos;
	}
	if (cursorPos.X < terminalSize.X) {
		cursorPos.Y--;
	}
	int perfectCursorPos = data[cursorPos.Y].size() / terminalSize.X * terminalSize.X + offset;
	cursorPos.X = (std::min)(perfectCursorPos, (int)data[cursorPos.Y].size() - 1);
	return cursorPos;
}

COORD Document::moveCursorDown(COORD& terminalSize) {
	offset = offset % terminalSize.X;
	if (data[cursorPos.Y].size() > (cursorPos.X / terminalSize.X + 1) * terminalSize.X) {
		bool endlPresent = !data[cursorPos.Y].empty() && data[cursorPos.Y][data[cursorPos.Y].size() - 1] == '\n';
		cursorPos.X = (std::min)(cursorPos.X + terminalSize.X, (int)data[cursorPos.Y].size() - endlPresent);
		return cursorPos;
	}
	if (cursorPos.Y == data.size() - 1) {
		return cursorPos;
	}
	cursorPos.Y++;
	bool endlPresent = !data[cursorPos.Y].empty() && data[cursorPos.Y][data[cursorPos.Y].size() - 1] == '\n';
	int perfectCursorPos = (data[cursorPos.Y].size() % terminalSize.X) / terminalSize.X * terminalSize.X + offset;
	cursorPos.X = (std::min)(perfectCursorPos, (int)data[cursorPos.Y].size() - endlPresent);
	return cursorPos;
}
