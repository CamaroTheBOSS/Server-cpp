#include <mutex>

#include "processor.h"
#pragma push_macro("ERROR")
#undef ERROR

Processor::Processor(Document& doc, std::mutex& docLock, logs::Logger& logger) :
	doc(doc),
	docLock(docLock),
	logger(logger) {}


ResponseType Processor::processMessage(msg::Buffer& buffer) {
	if (buffer.size == 1) {
		return processNewConnectionMsg(buffer);
	}

	auto header = msg::Header::parse(buffer, 0);
	switch (header.type) {
	case msg::MessageType::write:
		return processWriteMsg(buffer);
	case msg::MessageType::erase:
		return processEraseMsg(buffer);
	case msg::MessageType::sync:
		return processSyncMsg(buffer);
	}
	logger.log(logs::Level::ERROR, "Unknown header type in incoming message");
}

ResponseType Processor::processNewConnectionMsg(msg::Buffer& buffer) {
	logger.log(logs::Level::DEBUG, "Thread ", std::this_thread::get_id(), " got new connection");
	return ResponseType::none;
}

ResponseType Processor::processWriteMsg(msg::Buffer& buffer) {
	auto msg = msg::Write::parse(buffer, 0);
	std::scoped_lock loc{docLock};
	if (doc.setCursorPos(msg.cursorPos)) {
		doc.write(msg.msg);
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.msg, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
	}
	return ResponseType::broadcast;
}

ResponseType Processor::processEraseMsg(msg::Buffer& buffer) {
	auto msg = msg::Erase::parse(buffer, 0);
	std::scoped_lock loc{docLock};
	if (doc.setCursorPos(msg.cursorPos)) {
		doc.erase();
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] erased '", msg.eraseSize, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on erase msg!");
	}
	return ResponseType::broadcast;
}

ResponseType Processor::processSyncMsg(msg::Buffer& buffer) {
	return ResponseType::unicast;
}

#pragma pop_macro("ERROR")
