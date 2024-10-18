#include <iostream>

#include "tcp_client.h"
#include "document.h"
#include "terminal.h"
   
constexpr int clientVer = 1;
constexpr int errCode = 0;
constexpr int eraseSize = 1;

TerminalManager::Mode writingMode(Document& doc, TerminalManager& terminal, Client& tcpClient) {
    terminal.setMode(TerminalManager::Mode::document);
    while (true) {
        CONSOLE_SCREEN_BUFFER_INFO terminalCursorInfo;
        if (!GetConsoleScreenBufferInfo(terminal.getConsoleHandle(), &terminalCursorInfo)) {
            continue;
        }
        int keyCode = terminal.readChar();
        COORD docCursorPos = doc.getCursorPos();
        if (keyCode >= 32 && keyCode <= 127) {
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, "", docCursorPos, std::string{static_cast<char>(keyCode)});
            continue;
        }
        switch (keyCode) {
        case ENTER:
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, "", docCursorPos, "\n");
            break;
        case TABULAR:
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, "", docCursorPos, "    ");
            break;
        case BACKSPACE:
            tcpClient.sendMsg<msg::Erase>(clientVer, errCode, "", docCursorPos, eraseSize);
            break;
        case ARROW_LEFT:
            doc.moveCursorLeft();
            terminal.render(doc);
            break;
        case ARROW_RIGHT:
            doc.moveCursorRight();
            terminal.render(doc);
            break;
        case ARROW_UP:
            doc.moveCursorUp(terminalCursorInfo.dwSize);
            terminal.render(doc);
            break;
        case ARROW_DOWN:
            doc.moveCursorDown(terminalCursorInfo.dwSize);
            terminal.render(doc);
            break;
        }
    }
    return TerminalManager::Mode::none;
}

TerminalManager::Mode commandMode(TerminalManager& terminal, Client& tcpClient) {
    terminal.setMode(TerminalManager::Mode::command);
    Document commandLine;
    while (true) {
    int keyCode = terminal.readChar();
        if (keyCode >= 32 && keyCode <= 127) {
            commandLine.write(static_cast<char>(keyCode));
        }
        switch (keyCode) {
        case ENTER:
            commandLine.submit();
            break;
        case TABULAR:
            commandLine.write("    ");
            break;
        case BACKSPACE:
            commandLine.erase();
            break;
        case ARROW_LEFT:
            commandLine.moveCursorLeft();
            break;
        case ARROW_RIGHT:
            commandLine.moveCursorRight();
            break;
        }
        terminal.render(commandLine);
    }
    return TerminalManager::Mode::none;
}

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
    TerminalManager terminal;
    Client tcpClient{"192.168.1.10", 8081, "client.log", doc, terminal};
    if (tcpClient.connectToServer()) {
        std::cout << "Error when connecting to the server\n";
        return -1;
    }
    auto mode = TerminalManager::Mode::command;
    while (mode != TerminalManager::Mode::none) {
        if (mode == TerminalManager::Mode::command) {
            mode = commandMode(terminal, tcpClient);
        }
        else if (mode == TerminalManager::Mode::document) {
            mode = writingMode(doc, terminal, tcpClient);
        }
    }

    WSACleanup();
    return 0;
}
