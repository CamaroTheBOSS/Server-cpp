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
#define CTRL_Q 17

#define BACKSPACE 8
#define TABULAR 9
#define ENTER 13


class TerminalManager {
public:
    enum class Mode { none, command, document };
    int readChar() const;

    void setMode(Mode mode) const;
    COORD getCursorPos() const;
    void setCursorPos(const COORD& newPos);
    void render(Document& doc);
    COORD syncCursors(Document& doc);
    HANDLE getConsoleHandle() const;

private:
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = COORD{0, 0};
};