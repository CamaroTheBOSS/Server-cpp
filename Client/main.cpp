#include <iostream>

#include "tcp_client.h"
#include "document.h"
#include "terminal.h"
#include "command_executor.h"
   
constexpr int clientVer = 1;
constexpr int errCode = 0;
constexpr int eraseSize = 1;

TerminalManager::Mode writingMode(Document& doc, TerminalManager& terminal, Client& tcpClient) {
    terminal.setMode(TerminalManager::Mode::document);
    terminal.render(doc);
    while (true) {
        CONSOLE_SCREEN_BUFFER_INFO terminalCursorInfo;
        if (!GetConsoleScreenBufferInfo(terminal.getConsoleHandle(), &terminalCursorInfo)) {
            continue;
        }
        int keyCode = terminal.readChar();
        COORD docCursorPos = doc.getCursorPos();
        if (keyCode >= 32 && keyCode <= 127) {
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, tcpClient.getUserId(), docCursorPos, std::string{static_cast<char>(keyCode)});
            continue;
        }
        switch (keyCode) {
        case ENTER:
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, tcpClient.getUserId(), docCursorPos, "\n");
            break;
        case TABULAR:
            tcpClient.sendMsg<msg::Write>(clientVer, errCode, tcpClient.getUserId(), docCursorPos, "    ");
            break;
        case BACKSPACE:
            tcpClient.sendMsg<msg::Erase>(clientVer, errCode, tcpClient.getUserId(), docCursorPos, eraseSize);
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
        case CTRL_Q:
            return TerminalManager::Mode::command;
        }
    }
    return TerminalManager::Mode::none;
}

TerminalManager::Mode commandMode(CommandExecutor& commandExec, TerminalManager& terminal, Client& tcpClient) {
    terminal.setMode(TerminalManager::Mode::command);
    Document commandLine;
    while (true) {
    int keyCode = terminal.readChar();
        if (keyCode >= 32 && keyCode <= 127) {
            commandLine.write(static_cast<char>(keyCode));
            terminal.render(commandLine);
        }
        std::pair<std::string, int> responseAndCode {"", 0};
        switch (keyCode) {
        case ENTER:
            responseAndCode = commandExec.processCommand(commandLine.submit());
            std::cout << "\n" + std::to_string(responseAndCode.second) + ": " + responseAndCode.first;
            terminal.setCursorPos(COORD{ 0, 0 });
            break;
        case TABULAR:
            commandLine.write("    ");
            terminal.render(commandLine);
            break;
        case BACKSPACE:
            commandLine.erase();
            terminal.render(commandLine);
            break;
        case ARROW_LEFT:
            commandLine.moveCursorLeft();
            terminal.render(commandLine);
            break;
        case ARROW_RIGHT:
            commandLine.moveCursorRight();
            terminal.render(commandLine);
            break;
        }
        if (responseAndCode.second == -1) {
            return TerminalManager::Mode::document;
        }
        
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
    CommandExecutor commandExec{ tcpClient };
    if (tcpClient.connectToServer()) {
        std::cout << "Error when connecting to the server\n";
        return -1;
    }
    auto mode = TerminalManager::Mode::command;
    while (mode != TerminalManager::Mode::none) {
        if (mode == TerminalManager::Mode::command) {
            mode = commandMode(commandExec, terminal, tcpClient);
        }
        else if (mode == TerminalManager::Mode::document) {
            mode = writingMode(doc, terminal, tcpClient);
        }
    }

    WSACleanup();
    return 0;
}
