#pragma once
#include <windows.h>
#include "document.h"

#define ARROW_UP 552
#define ARROW_DOWN 560
#define ARROW_LEFT 555 
#define ARROW_RIGHT 557

#define CTRL_A 1
#define CTRL_C 3
#define CTRL_V 22

#define BACKSPACE 8
#define TABULAR 9
#define ENTER 13


class TerminalManager {
public:
    enum class Mode { command, document };

    TerminalManager(Document& document);
    int readChar() const;

    void setMode(Mode mode) const;
    COORD getCursorPos() const;
    void setCursorPos(const COORD& newPos) const;
    void renderTextFromPos(const COORD documentCursorPos) const;
    void erase(const COORD documentCursorPos) const;
    COORD moveCursorLeft(const COORD documentCursorPos, COORD& coord) const;

private:
    Document& document;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int textLength = 0;
};