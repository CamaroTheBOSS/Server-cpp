#define NOMINMAX
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
    return cursorPos;
}

HANDLE TerminalManager::getConsoleHandle() const {
    return hConsole;
}

void TerminalManager::setCursorPos(const COORD& newPos) {
    SetConsoleCursorPosition(hConsole, newPos);
    cursorPos = newPos;
}

void TerminalManager::render(){
    CONSOLE_CURSOR_INFO cursorInfo;
    if (!GetConsoleCursorInfo(hConsole, &cursorInfo)) {
        return;
    }
    cursorInfo.bVisible = 0;
    if (!SetConsoleCursorInfo(hConsole, &cursorInfo)) {
        return;
    }
    CONSOLE_SCREEN_BUFFER_INFO screenInfo;
    if (!GetConsoleScreenBufferInfo(hConsole, &screenInfo)) {
        return;
    }
    setCursorPos(COORD{ 0, screenInfo.srWindow.Top});
    int tLineCounter = 0;
    for (const auto& line : document.get()) {
        int head = 0;
        int tail = std::min((int)line.size(), (int)screenInfo.dwSize.X);
        while (head < tail) {
            bool last = tLineCounter >= screenInfo.srWindow.Bottom;
            std::string tLine = line.substr(head, tail - head - last);
            bool endlPresent = !tLine.empty() && tLine[tLine.size() - 1] == '\n';
            int spaceCount = ceil((float)tLine.size() / (float)screenInfo.dwSize.X) * screenInfo.dwSize.X - tLine.size();
            tLine.insert(tLine.size() - endlPresent, std::string(spaceCount, ' '));
            if (tLineCounter >= screenInfo.srWindow.Top) {
                std::cout << tLine;
                if (tLineCounter > screenInfo.srWindow.Bottom) {
                    break;
                }
            }
            head += screenInfo.dwSize.X;
            tail = std::min((int)line.size(), tail + (int)screenInfo.dwSize.X);
            tLineCounter++;
        }
        if (tLineCounter > screenInfo.srWindow.Bottom) {
            break;
        }
    }
    if (tLineCounter <= screenInfo.srWindow.Bottom) {
        std::cout << std::string(screenInfo.dwSize.X - 1, ' ');
    }
    syncCursors();
    cursorInfo.bVisible = 1;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void TerminalManager::syncCursors() {
    CONSOLE_SCREEN_BUFFER_INFO cursorInfo;
    if (!GetConsoleScreenBufferInfo(hConsole, &cursorInfo)) {
        return;
    }
    const auto& data = document.get();
    COORD terminalCursorPos{ 0, 0 };
    COORD documentCursorPos = document.getCursorPos();
    for (int i = 0; i <= documentCursorPos.Y; i++) {
        if (data[i].empty()) {
            continue;
        }
        bool endlPresent = data[i][data[i].size() - 1] == '\n' && i != documentCursorPos.Y;
        int base = i != documentCursorPos.Y ? data[i].size() : documentCursorPos.X;
        terminalCursorPos.Y += base / cursorInfo.dwSize.X + endlPresent;
    }
    terminalCursorPos.X = documentCursorPos.X % cursorInfo.dwSize.X;
    setCursorPos(terminalCursorPos);
}
