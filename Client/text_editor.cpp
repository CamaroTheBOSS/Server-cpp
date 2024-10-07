#include "windows.h"
#include "text_editor.h"

TextEditor::TextEditor() :
    document(),
    terminal(document) {}

void TextEditor::run() {
    terminal.setMode(TerminalManager::Mode::document);
    while (true) {
        COORD documentCursorPos = document.getCursorPos();
        int keyCode = terminal.readChar();
        if (keyCode >= 32 && keyCode <= 127) {
            char letter = static_cast<char>(keyCode);
            document.write(letter);
            terminal.renderTextFromPos(documentCursorPos);
        }
        else if (keyCode == ENTER) {
            document.write('\n');
            terminal.renderTextFromPos(documentCursorPos);
        }
        else if (keyCode == BACKSPACE) {
            document.erase();
            terminal.erase(documentCursorPos);
        }
        else if (keyCode == TABULAR) {
            for (int i = 0; i < 4; i++) {
                document.write(' ');
            }
            terminal.renderTextFromPos(documentCursorPos);
        }
    }
}