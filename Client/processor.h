#pragma once
#include "messages.h"
#include "terminal.h"
#include "document.h"
#include "logger.h"

class Processor {
public:
	Processor(Document& doc, TerminalManager& terminal, logs::Logger& logger);
	void processMessage(msg::Buffer& buffer);

private:
	void processWriteMsg(msg::Buffer& buffer);
	void processEraseMsg(msg::Buffer& buffer);
	void processLoadMsg(msg::Buffer& buffer);

	Document& doc;
	TerminalManager& terminal;
	logs::Logger& logger;
};