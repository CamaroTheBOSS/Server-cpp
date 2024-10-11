#include "text_editor.h"
#include "windows.h"

TextEditor::TextEditor() :
    document(),
    terminal(document),
    tcpClient("CAMAROTHEBOSS", "192.168.1.10", 8081, document, terminal) {}

void TextEditor::run() {
    terminal.setMode(TerminalManager::Mode::document);
    tcpClient.connectToServer();
    while (true) {
        CONSOLE_SCREEN_BUFFER_INFO terminalCursorInfo;
        if (!GetConsoleScreenBufferInfo(terminal.getConsoleHandle(), &terminalCursorInfo)) {
            return;
        }
        int keyCode = terminal.readChar();
        COORD documentCursorPos = document.getCursorPos();
        if (keyCode >= 32 && keyCode <= 127) {
            char letter = static_cast<char>(keyCode);
            tcpClient.sendMsg(documentCursorPos, std::string{letter});
        }
        else if (keyCode == ENTER) {
            tcpClient.sendMsg(documentCursorPos, "\n");
        }
        else if (keyCode == BACKSPACE) {
            document.erase();
        }
        else if (keyCode == TABULAR) {
            tcpClient.sendMsg(documentCursorPos, "    ");
        }
        else if (keyCode == ARROW_LEFT) {
            document.moveCursorLeft();
            terminal.render();
        }
        else if (keyCode == ARROW_RIGHT) {
            document.moveCursorRight();
            terminal.render();
        }
        else if (keyCode == ARROW_UP) {
            document.moveCursorUp(terminalCursorInfo.dwSize);
            terminal.render();
        }
        else if (keyCode == ARROW_DOWN) {
            document.moveCursorDown(terminalCursorInfo.dwSize);
            terminal.render();
        }
    }
}