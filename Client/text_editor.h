#pragma once
#include "document.h"
#include "terminal.h"

class TextEditor {
public:
	TextEditor();
	void run();

private:
	Document document;
	TerminalManager terminal;
};
