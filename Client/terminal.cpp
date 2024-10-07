#include <conio.h>
#include <iostream>

#include "terminal.h"


TerminalManager::TerminalManager(Document& document):
    document(document) {}

void TerminalManager::setMode(Mode mode) const {
    if (mode == Mode::command) {

    }
    else if (mode == Mode::document) {
        DWORD currMode;
        auto stdInput = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(stdInput, &currMode);
        SetConsoleMode(
            stdInput,
            currMode & ~(
                ENABLE_LINE_INPUT |
                ENABLE_ECHO_INPUT | 
                ENABLE_MOUSE_INPUT |
                ENABLE_PROCESSED_INPUT | 
                ENABLE_QUICK_EDIT_MODE | 
                ENABLE_WINDOW_INPUT |
                ENABLE_PROCESSED_OUTPUT
                )
        );
    }
}

int TerminalManager::readChar() const {
    int keyCode = _getch();
    if (keyCode == 0 || keyCode == 224) {
        keyCode += _getch() + 256;
    }
    bool shiftPressed = ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000);
    return keyCode;
}

COORD TerminalManager::getCursorPos() const {
    CONSOLE_SCREEN_BUFFER_INFO cursorInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &cursorInfo)) {
        return cursorInfo.dwCursorPosition;
    }
    return COORD{ 0, 0 };
}

void TerminalManager::setCursorPos(const COORD& newPos) const {
    SetConsoleCursorPosition(hConsole, newPos);
}

void TerminalManager::renderTextFromPos(const COORD documentCursorPos) const {
    int lineIndexOffset = 0;
    std::string textToRender = document.getLine(documentCursorPos.Y).substr(documentCursorPos.X);
    while (!textToRender.empty()) {
        std::cout << textToRender;
        textToRender = document.getLine(documentCursorPos.Y + ++lineIndexOffset);
    }
}

void TerminalManager::erase(const COORD documentCursorPos) const {
    COORD cursorPos = getCursorPos();
    COORD textEndPos = moveCursorLeft(documentCursorPos, cursorPos);
    setCursorPos(textEndPos);
    std::cout << ' ';
    setCursorPos(textEndPos);
}

COORD TerminalManager::moveCursorLeft(const COORD documentCursorPos, COORD& coord) const {
    if (coord.X == 0 && coord.Y == 0) {
        return coord;
    }
    if (coord.X > 0) {
        coord.X--;
        return coord;
    }
    CONSOLE_SCREEN_BUFFER_INFO cursorInfo;
    if (!GetConsoleScreenBufferInfo(hConsole, &cursorInfo)) {
        return coord;
    }
    std::string currentTextLine = document.getLine(documentCursorPos.Y);
    if (!currentTextLine.empty()) {
        coord.X = currentTextLine.size() % cursorInfo.dwSize.X;
    }
    else {
        coord.X = document.getLine(documentCursorPos.Y - 1).size() % cursorInfo.dwSize.X;
    }
    coord.Y--;
    return coord;
}
