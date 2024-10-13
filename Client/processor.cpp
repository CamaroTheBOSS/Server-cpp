#include "processor.h"
#pragma push_macro("ERROR")
#undef ERROR

Processor::Processor(Document& doc, TerminalManager& terminal, logs::Logger& logger):
	doc(doc),
	terminal(terminal),
	logger(logger) {}


void Processor::processMessage(msg::Buffer& buffer) {
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

void Processor::processWriteMsg(msg::Buffer& buffer) {
	// TODO when its response for my msg set cursor to original cursor pos;
	auto msg = msg::Write::parse(buffer, 0);
	COORD docCursorPos = doc.getCursorPos();
	if (doc.setCursorPos(msg.cursorPos)) {
		doc.write(msg.msg);
		/*if (!msg.isResponse) {
			document.setCursorPos(docCursorPos);
		}*/
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.msg, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
	}
	terminal.render();
}

void Processor::processEraseMsg(msg::Buffer& buffer) {
	// TODO when its response for my msg set cursor to original cursor pos;
	auto msg = msg::Erase::parse(buffer, 0);
	COORD docCursorPos = doc.getCursorPos();
	if (doc.setCursorPos(msg.cursorPos)) {
		for (int i = 0; i < msg.eraseSize; i++) {
			doc.erase();
		}
		/*if (!msg.isResponse) {
			document.setCursorPos(docCursorPos);
		}*/
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] erased ", msg.eraseSize, " letters");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on erase msg!");
	}
	terminal.render();
}

void Processor::processSyncMsg(msg::Buffer& buffer) {

}

#pragma pop_macro("ERROR")
