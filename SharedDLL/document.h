#pragma once
#define NOMINMAX

#include <vector>
#include <string>
#include <memory>
#include "windows.h"

#ifdef SHAREDDLL_EXPORTS
#define DOCUMENT_API __declspec(dllexport)
#else
#define DOCUMENT_API __declspec(dllimport)
#endif

class DOCUMENT_API Document {
public:
	Document();
	Document(const std::string& text);
	COORD write(const char letter);
	COORD write(const std::string& text);
	COORD erase();
	COORD erase(const int eraseSize);
	std::string submit();
	COORD moveCursorLeft();
	COORD moveCursorRight();
	COORD moveCursorUp(COORD& terminalSize);
	COORD moveCursorDown(COORD& terminalSize);

	bool setCursorPos(COORD newPos);
	COORD getCursorPos() const;
	std::string getLine(const int lineIndex) const;
	std::string getText() const;
	const std::vector<std::string>& get();

private:
	std::vector<std::string> data{ "" };
	COORD cursorPos{ 0, 0 };
	int offset = 0;
};