#include <iostream>

#include "tcp_client.h"
#include "document.h"
#include "terminal.h"


int main()
{
    WSADATA wsaData;
    int wsaError;
    WORD mVersionRequired = MAKEWORD(2, 2);
    wsaError = WSAStartup(mVersionRequired, &wsaData);
    if (wsaError) {
        std::cout << wsaError << " Error on WSA startup\n";
        WSACleanup();
        return -1;
    }

    Document doc;
    TerminalManager terminal{ doc };
    Client tcpClient{"CamaroTheBOSS", "192.168.1.10", 8081, "client.log", doc, terminal};

    terminal.setMode(TerminalManager::Mode::document);
    if (tcpClient.connectToServer()) {
        std::cout << "Error when connecting to the server\n";
        return -1;
    }
    while (true) {
        CONSOLE_SCREEN_BUFFER_INFO terminalCursorInfo;
        if (!GetConsoleScreenBufferInfo(terminal.getConsoleHandle(), &terminalCursorInfo)) {
            continue;
        }
        int keyCode = terminal.readChar();
        COORD docCursorPos = doc.getCursorPos();
        if (keyCode >= 32 && keyCode <= 127) {
            char letter = static_cast<char>(keyCode);
            tcpClient.sendMsg(docCursorPos, std::string{letter});
            continue;
        }
        std::string msg;
        switch (keyCode) {
        case ENTER:
            tcpClient.sendMsg(docCursorPos, "\n");
            break;
        case TABULAR:
            tcpClient.sendMsg(docCursorPos, "    ");
            break;
        case BACKSPACE:
            doc.erase();
            break;
        case ARROW_LEFT:
            doc.moveCursorLeft();
            terminal.render();
            break;
        case ARROW_RIGHT:
            doc.moveCursorRight();
            terminal.render();
            break;
        case ARROW_UP:
            doc.moveCursorUp(terminalCursorInfo.dwSize);
            terminal.render();
            break;
        case ARROW_DOWN:
            doc.moveCursorDown(terminalCursorInfo.dwSize);
            terminal.render();
            break;
        }
    }

    WSACleanup();
    return 0;
}
