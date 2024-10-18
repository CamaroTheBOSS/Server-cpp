#define NOMINMAX
#include <conio.h>
#include <iostream>
#include <windows.h>

#include "terminal.h"

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
    system("cls");
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

void TerminalManager::render(Document& doc){
    COORD correctCursorPos = syncCursors(doc);
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
    std::string toPrint;
    for (const auto& line : doc.get()) {
        int head = 0;
        int tail = std::min((int)line.size(), (int)screenInfo.dwSize.X);
        while (head < tail && tLineCounter <= screenInfo.srWindow.Bottom) {
            if (tLineCounter < screenInfo.srWindow.Top) {
                head += screenInfo.dwSize.X;
                tail = std::min((int)line.size(), tail + (int)screenInfo.dwSize.X);
                tLineCounter++;
                continue;
            }
            std::string tLine = line.substr(head, tail - head);
            bool endlPresent = !tLine.empty() && tLine[tLine.size() - 1] == '\n';
            int spaceCount = ceil((float)tLine.size() / (float)screenInfo.dwSize.X) * screenInfo.dwSize.X - tLine.size();
            tLine.insert(tLine.size() - endlPresent, std::string(spaceCount, ' '));
            toPrint += tLine;
            head += screenInfo.dwSize.X;
            tail = std::min((int)line.size(), tail + (int)screenInfo.dwSize.X);
            tLineCounter++;
        }
    }
    if (!toPrint.empty() && toPrint[toPrint.size() - 1] == '\n') {
        toPrint[toPrint.size() - 1] = ' ';
    }
    if (tLineCounter <= screenInfo.srWindow.Bottom) {
        toPrint += std::string(screenInfo.dwSize.X - 1, ' ');
    }
    std::cout << toPrint;
    setCursorPos(correctCursorPos);
    cursorInfo.bVisible = 1;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

COORD TerminalManager::syncCursors(Document& doc) {
    CONSOLE_SCREEN_BUFFER_INFO cursorInfo;
    if (!GetConsoleScreenBufferInfo(hConsole, &cursorInfo)) {
        return COORD{0, 0};
    }
    const auto& data = doc.get();
    COORD terminalCursorPos{ 0, 0 };
    COORD documentCursorPos = doc.getCursorPos();
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
    return terminalCursorPos;
}
