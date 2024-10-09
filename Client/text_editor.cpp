#include "windows.h"
#include "text_editor.h"

TextEditor::TextEditor() :
    document(),
    terminal(document) {}

void TextEditor::run() {
    terminal.setMode(TerminalManager::Mode::document);
    while (true) {
        CONSOLE_SCREEN_BUFFER_INFO terminalCursorInfo;
        if (!GetConsoleScreenBufferInfo(terminal.getConsoleHandle(), &terminalCursorInfo)) {
            return;
        }
        COORD documentCursorPos = document.getCursorPos();
        int keyCode = terminal.readChar();
        if (keyCode >= 32 && keyCode <= 127) {
            char letter = static_cast<char>(keyCode);
            document.write(letter);
            terminal.render();
        }
        else if (keyCode == ENTER) {
            document.write('\n');
            terminal.render();
        }
        else if (keyCode == BACKSPACE) {
            document.erase();
            terminal.render();
        }
        else if (keyCode == TABULAR) {
            for (int i = 0; i < 4; i++) {
                document.write(' ');
            }
            terminal.render();
        }
        else if (keyCode == ARROW_LEFT) {
            document.moveCursorLeft();
        }
        else if (keyCode == ARROW_RIGHT) {
            document.moveCursorRight();
        }
        else if (keyCode == ARROW_UP) {
            document.moveCursorUp(terminalCursorInfo.dwSize);
        }
        else if (keyCode == ARROW_DOWN) {
            document.moveCursorDown(terminalCursorInfo.dwSize);
        }
        terminal.syncCursors();
    }
}