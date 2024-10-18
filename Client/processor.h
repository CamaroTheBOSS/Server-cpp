#pragma once
#include <string>

#include "messages.h"
#include "terminal.h"
#include "document.h"
#include "logger.h"

class Processor {
public:
	Processor(Document& doc, TerminalManager& terminal, logs::Logger& logger, std::string& userId);
	void process(msg::Buffer& buffer);
	std::pair<std::string, int> waitForResponse();

private:
	std::pair<std::string, int> processWriteMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processEraseMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processRegisterMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processLoginMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processCreateMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processLoadMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processJoinMsg(msg::Buffer& buffer);
	std::pair<std::string, int> processErrorMsg(msg::Buffer& buffer);

	std::string response;
	int errCode;
	bool responseReady = false;

	std::string& userId;
	Document& doc;
	TerminalManager& terminal;
	logs::Logger& logger;
};