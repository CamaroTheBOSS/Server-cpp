#pragma once
#include "messages.h"
#include "document.h"
#include "logger.h"

enum class ResponseType {none, unicast, broadcast};

class Processor {
public:
	Processor(Document& doc, std::mutex& docLock, logs::Logger& logger);
	ResponseType processMessage(msg::Buffer& buffer);

private:
	ResponseType processNewConnectionMsg(msg::Buffer& buffer);
	ResponseType processWriteMsg(msg::Buffer& buffer);
	ResponseType processEraseMsg(msg::Buffer& buffer);
	ResponseType processSyncMsg(msg::Buffer& buffer);

	Document& doc;
	std::mutex& docLock;
	logs::Logger& logger;
};
